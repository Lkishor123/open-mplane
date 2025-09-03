import os
import socket
import subprocess
import threading
import time
from collections import deque
from pathlib import Path

import grpc
import pytest

# Add mpclient gRPC python bindings to path
_STUB_DIR = Path(__file__).resolve().parents[2] / "mplane_client" / "example" / "demo"
import sys
sys.path.append(str(_STUB_DIR))

import mpclient_pb2
import mpclient_pb2_grpc

SOCK = "/tmp/haltest.sock"
GRPC_ADDR = f"{os.getenv('MP_GRPC_HOST', 'localhost')}:{os.getenv('MP_GRPC_PORT', '50051')}"
NETCONF_HOST = os.getenv("MP_NETCONF_HOST", "localhost")
NETCONF_PORT = int(os.getenv("MP_NETCONF_PORT", "830"))
NETCONF_USER = os.getenv("MP_NETCONF_USER", "oran")
NETCONF_PASS = os.getenv("MP_NETCONF_PASS", "oran")

SIM_SHIM_BIN = Path(os.getenv("SIM_SHIM_BIN", "mplane_server/utils/test_shim/build/server-test-shim"))
SIM_SERVER_BIN = Path(os.getenv("SIM_SERVER_BIN", "build/server-sim/mplane-server-app"))
SIM_CLIENT_BIN = Path(os.getenv("SIM_CLIENT_BIN", "build/client-sim/mpc_client"))


def _spawn_with_log(cmd):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    log = deque()

    def reader():
        for line in proc.stderr:
            log.append(line.strip())

    threading.Thread(target=reader, daemon=True).start()
    return proc, log


@pytest.fixture
def simulator():
    if not SIM_SHIM_BIN.exists() or not SIM_SERVER_BIN.exists():
        pytest.skip("simulator binaries not built; run tools/sim/build_sim.sh")
    shim_proc = subprocess.Popen([str(SIM_SHIM_BIN)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    server_proc, server_log = _spawn_with_log([str(SIM_SERVER_BIN)])
    time.sleep(1)
    yield server_proc, shim_proc, server_log
    for p in (server_proc, shim_proc):
        if p.poll() is None:
            p.terminate()
            try:
                p.wait(timeout=5)
            except Exception:
                p.kill()
    if os.path.exists(SOCK):
        os.remove(SOCK)


@pytest.fixture
def mpclient():
    if not SIM_CLIENT_BIN.exists():
        pytest.skip("mplane_client binary not built; run mplane_client utils/build_mpclient.sh")
    proc = subprocess.Popen([str(SIM_CLIENT_BIN), "--insecure"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(1)
    yield proc
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except Exception:
            proc.kill()


def _shim(cmd: str) -> str:
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(SOCK)
    s.sendall((cmd + "\n").encode())
    data = s.recv(4096)
    s.close()
    return data.decode().strip()


def _open_session():
    channel = grpc.insecure_channel(GRPC_ADDR)
    stub = mpclient_pb2_grpc.MpclientStub(channel)
    try:
        conn = stub.connect(
            mpclient_pb2.ConnectRequest(
                host=NETCONF_HOST,
                port=NETCONF_PORT,
                auth=mpclient_pb2.Authentication(user=NETCONF_USER, password=NETCONF_PASS),
            ),
            timeout=5,
        )
    except Exception:
        return None, None, None, None
    if not conn.success:
        return None, None, None, None
    session = conn.sessionId

    sub_xml = (
        '<create-subscription xmlns="urn:ietf:params:xml:ns:netconf:notification:1.0"/>'
    )
    resp = stub.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session, serializedYang=sub_xml, timeoutSec=1
        ),
        timeout=5,
    )
    if resp.status != mpclient_pb2.NetconfRpcResponse.Status.SUCCESS:
        return None, None, None, None

    notifications = []

    def listener():
        try:
            for note in stub.streamNotifications(
                mpclient_pb2.StreamNotificationsRequest(sessionId=session)
            ):
                notifications.append(note.serializedYang)
        except grpc.RpcError:
            pass

    t = threading.Thread(target=listener, daemon=True)
    t.start()
    time.sleep(0.1)
    return stub, session, notifications, t


def _open_session_or_skip():
    stub, session, notifications, t = _open_session()
    if stub is None:
        pytest.skip("mplane_client gRPC not running")
    return stub, session, notifications, t


_EDIT_CARRIER = (
    '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
    '  <target><running/></target>'
    '  <config>'
    '    <user-plane-configuration xmlns="urn:o-ran:uplane-conf:1.0">'
    '      <tx-array-carriers>'
    '        <name>{name}</name>'
    '        <center-of-channel-bandwidth>100000000</center-of-channel-bandwidth>'
    '        <channel-bandwidth>1000000</channel-bandwidth>'
    '        <gain>0</gain>'
    '        <downlink-radio-frame-offset>0</downlink-radio-frame-offset>'
    '        <downlink-sfn-offset>0</downlink-sfn-offset>'
    '        <active>ACTIVE</active>'
    '      </tx-array-carriers>'
    '    </user-plane-configuration>'
    '  </config>'
    '</edit-config>'
)

_GET_STATE = (
    '<get xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
    '  <filter type="subtree">'
    '    <user-plane-configuration xmlns="urn:o-ran:uplane-conf:1.0">'
    '      <tx-array-carriers>'
    '        <name>{name}</name>'
    '        <state/>'
    '      </tx-array-carriers>'
    '    </user-plane-configuration>'
    '  </filter>'
    '</get>'
)


def test_tx_carrier_ready(simulator, mpclient):
    if not os.path.exists(SOCK):
        pytest.skip("shim not running; start with tools/sim/sim_start.sh")
    stub, session, notifications, t = _open_session_or_skip()

    assert _shim("sync state=LOCKED").startswith("OK")

    edit_xml = _EDIT_CARRIER.format(name="c1")
    resp = stub.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session, serializedYang=edit_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.OK

    time.sleep(0.1)

    get_xml = _GET_STATE.format(name="c1")
    resp = stub.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session, serializedYang=get_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.DATA
    assert "<state>READY</state>" in resp.message

    stub.disconnect(mpclient_pb2.DisconnectRequest(sessionId=session))
    t.join(timeout=1)

    assert any(
        "tx-array-carriers-state-change" in n and "<name>c1</name>" in n and "<state>READY</state>" in n
        for n in notifications
    )

    server_proc, shim_proc, server_log = simulator
    lines = list(server_log)
    assert any("tx_carrier_state_change" in l and "do_apply=1" in l for l in lines)


def test_tx_carrier_sync_dependency(simulator, mpclient):
    if not os.path.exists(SOCK):
        pytest.skip("shim not running; start with tools/sim/sim_start.sh")
    stub, session, notifications, t = _open_session_or_skip()

    assert _shim("sync state=FREERUN").startswith("OK")

    edit_xml = _EDIT_CARRIER.format(name="c2")
    resp = stub.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session, serializedYang=edit_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.RPC_ERROR
    assert "operational" in resp.message.lower() or "dependency" in resp.message.lower()

    stub.disconnect(mpclient_pb2.DisconnectRequest(sessionId=session))
    t.join(timeout=1)
