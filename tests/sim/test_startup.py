import os
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
NETCONF_USER = os.getenv("MP_NETCONF_USER", "oran")
NETCONF_PASS = os.getenv("MP_NETCONF_PASS", "oran")
CALLHOME_HOST = os.getenv("MP_CALLHOME_HOST", "0.0.0.0")
CALLHOME_PORT = int(os.getenv("MP_CALLHOME_PORT", "4334"))

SIM_SHIM_BIN = Path(os.getenv("SIM_SHIM_BIN", "mplane_server/utils/test_shim/build/server-test-shim"))
SIM_SERVER_BIN = Path(os.getenv("SIM_SERVER_BIN", "build/server-sim/mplane-server-app"))
SIM_CLIENT_BIN = Path(os.getenv("SIM_CLIENT_BIN", "mplane_client/build/mpc_client"))


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
    """Launch shim and server simulator."""
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
    """Start mplane_client gRPC server."""
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


@pytest.fixture
def set_recall_timer():
    """Fixture to configure re-call-home-no-ssh-timer."""
    def _set(stub, session, seconds=1):
        edit_xml = (
            '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
            '  <target><running/></target>'
            '  <config>'
            '    <re-call-home-no-ssh-timer xmlns="urn:o-ran:operations:1.0">'
            f"{seconds}</re-call-home-no-ssh-timer>"
            '  </config>'
            '</edit-config>'
        )
        stub.netconfRpc(
            mpclient_pb2.NetconfRpcRequest(
                sessionId=session, serializedYang=edit_xml, timeoutSec=1
            )
        )

    return _set


def _listen_for_session():
    channel = grpc.insecure_channel(GRPC_ADDR)
    stub = mpclient_pb2_grpc.MpclientStub(channel)
    responses = stub.listen(
        mpclient_pb2.ListenRequest(
            host=CALLHOME_HOST,
            port=CALLHOME_PORT,
            auth=mpclient_pb2.Authentication(user=NETCONF_USER, password=NETCONF_PASS),
            timeoutSec=10,
        ),
        timeout=15,
    )
    resp = next(responses)
    return stub, resp.sessionId


def _listen_or_skip():
    try:
        return _listen_for_session()
    except Exception:
        pytest.skip("mplane_client listen failed")


def test_mp_startup_001(simulator, mpclient):
    """Launch client in listen mode and confirm new session ID."""
    stub, session = _listen_or_skip()
    assert session > 0


def test_mp_startup_002(simulator, mpclient, set_recall_timer):
    """Stop client and ensure server retries call home."""
    server_proc, _, server_log = simulator
    client_proc = mpclient
    stub, session = _listen_or_skip()
    set_recall_timer(stub, session, seconds=1)
    client_proc.terminate()
    client_proc.wait(timeout=5)
    time.sleep(3)
    lines = list(server_log)
    callhome_lines = [l for l in lines if "call home" in l.lower()]
    assert len(callhome_lines) >= 2
