import os
import socket
import threading
import time
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


def _shim(cmd: str) -> str:
    """Send a command to the simulator shim."""
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(SOCK)
    s.sendall((cmd + "\n").encode())
    data = s.recv(4096)
    s.close()
    return data.decode().strip()


def _open_session():
    """Connect to mpclient and subscribe to notifications."""
    channel = grpc.insecure_channel(GRPC_ADDR)
    stub = mpclient_pb2_grpc.MpclientStub(channel)
    try:
        conn = stub.connect(
            mpclient_pb2.ConnectRequest(
                host=NETCONF_HOST,
                port=NETCONF_PORT,
                auth=mpclient_pb2.Authentication(
                    user=NETCONF_USER,
                    password=NETCONF_PASS,
                ),
            ),
            timeout=5,
        )
    except Exception:
        return None, None, None, None
    if not conn.success:
        return None, None, None, None
    session = conn.sessionId

    # Subscribe to all notifications
    sub_xml = (
        '<create-subscription '
        'xmlns="urn:ietf:params:xml:ns:netconf:notification:1.0"/>'
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


def test_sw_mgmt_notifications():
    if not os.path.exists(SOCK):
        pytest.skip("shim not running; start with tools/sim/sim_start.sh")
    stub, session, notifications, t = _open_session_or_skip()

    download_rpc = (
        '<software-download xmlns="urn:o-ran:software-management:1.0" '
        'xmlns:o-ran-fm="urn:o-ran:file-management:1.0">'
        '<remote-file-path>http://example.com/image.bin</remote-file-path>'
        '<o-ran-fm:password><o-ran-fm:password>pw</o-ran-fm:password>'
        '</o-ran-fm:password>'
        '</software-download>'
    )
    install_rpc = (
        '<software-install xmlns="urn:o-ran:software-management:1.0">'
        '<slot-name>slot1</slot-name>'
        '<file-names>image.bin</file-names>'
        '</software-install>'
    )

    stub.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session, serializedYang=download_rpc, timeoutSec=1
        )
    )
    assert _shim("sw phase=download result=COMPLETED file=image.bin").startswith("OK")
    time.sleep(0.1)

    stub.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session, serializedYang=install_rpc, timeoutSec=1
        )
    )
    assert _shim("sw phase=install result=COMPLETED slot=slot1").startswith("OK")
    time.sleep(0.1)

    stub.disconnect(mpclient_pb2.DisconnectRequest(sessionId=session))
    t.join(timeout=1)

    assert any(
        "download-event" in n and "image.bin" in n and "COMPLETED" in n
        for n in notifications
    )
    assert any(
        "install-event" in n and "slot1" in n and "COMPLETED" in n
        for n in notifications
    )


def test_alarm_notifications():
    if not os.path.exists(SOCK):
        pytest.skip("shim not running; start with tools/sim/sim_start.sh")
    stub, session, notifications, t = _open_session_or_skip()

    assert _shim("alarm id=1 severity=Major clear=false text=demo").startswith("OK")
    time.sleep(0.1)
    assert _shim("alarm id=1 severity=Major clear=true text=demo").startswith("OK")
    time.sleep(0.1)

    stub.disconnect(mpclient_pb2.DisconnectRequest(sessionId=session))
    t.join(timeout=1)

    assert any(
        "alarm-notif" in n and "<fault-id>1</fault-id>" in n and "<is-cleared>false</is-cleared>" in n
        for n in notifications
    )
    assert any(
        "alarm-notif" in n and "<fault-id>1</fault-id>" in n and "<is-cleared>true</is-cleared>" in n
        for n in notifications
    )

