import os
from pathlib import Path

import grpc
import pytest

# Add mpclient gRPC python bindings to path
_STUB_DIR = Path(__file__).resolve().parents[2] / "mplane_client" / "example" / "demo"
import sys
sys.path.append(str(_STUB_DIR))

import mpclient_pb2
import mpclient_pb2_grpc

GRPC_ADDR = f"{os.getenv('MP_GRPC_HOST', 'localhost')}:{os.getenv('MP_GRPC_PORT', '50051')}"
NETCONF_HOST = os.getenv("MP_NETCONF_HOST", "localhost")
NETCONF_PORT = int(os.getenv("MP_NETCONF_PORT", "830"))
NETCONF_USER = os.getenv("MP_NETCONF_USER", "oran")
NETCONF_PASS = os.getenv("MP_NETCONF_PASS", "oran")


def _open_session():
    """Open a NETCONF session via mpclient."""
    channel = grpc.insecure_channel(GRPC_ADDR)
    stub = mpclient_pb2_grpc.MpclientStub(channel)
    try:
        conn = stub.connect(
            mpclient_pb2.ConnectRequest(
                host=NETCONF_HOST,
                port=NETCONF_PORT,
                auth=mpclient_pb2.Authentication(
                    user=NETCONF_USER, password=NETCONF_PASS
                ),
            ),
            timeout=5,
        )
    except Exception:
        return None, None
    if not conn.success:
        return None, None
    return stub, conn.sessionId


def _open_session_or_skip():
    stub, session = _open_session()
    if stub is None:
        pytest.skip("mplane_client gRPC not running")
    return stub, session


def _disconnect(stub, session):
    try:
        stub.disconnect(mpclient_pb2.DisconnectRequest(sessionId=session))
    except Exception:
        pass


def test_config_lock_persists():
    stub_a, session_a = _open_session_or_skip()
    stub_b, session_b = _open_session_or_skip()

    lock_xml = (
        '<lock xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "</lock>"
    )
    resp = stub_a.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_a, serializedYang=lock_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.OK

    value = "42"
    edit_xml = (
        '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "  <config>"
        "    <re-call-home-no-ssh-timer xmlns=\"urn:o-ran:operations:1.0\">"
        f"{value}</re-call-home-no-ssh-timer>"
        "  </config>"
        "</edit-config>"
    )
    resp = stub_a.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_a, serializedYang=edit_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.OK

    unlock_xml = (
        '<unlock xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "</unlock>"
    )
    resp = stub_a.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_a, serializedYang=unlock_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.OK

    get_xml = (
        '<get-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <source><running/></source>"
        "</get-config>"
    )
    resp = stub_b.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_b, serializedYang=get_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.DATA
    assert (
        f"<re-call-home-no-ssh-timer>{value}</re-call-home-no-ssh-timer>" in resp.message
    )

    _disconnect(stub_a, session_a)
    _disconnect(stub_b, session_b)


def test_edit_config_lock_denied():
    stub_a, session_a = _open_session_or_skip()
    stub_b, session_b = _open_session_or_skip()

    lock_xml = (
        '<lock xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "</lock>"
    )
    resp = stub_a.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_a, serializedYang=lock_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.OK

    edit_xml = (
        '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "  <config>"
        "    <re-call-home-no-ssh-timer xmlns=\"urn:o-ran:operations:1.0\">0</re-call-home-no-ssh-timer>"
        "  </config>"
        "</edit-config>"
    )
    resp = stub_b.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_b, serializedYang=edit_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert (
        resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.RPC_ERROR
    )
    assert "lock-denied" in resp.message

    unlock_xml = (
        '<unlock xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "</unlock>"
    )
    stub_a.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_a, serializedYang=unlock_xml, timeoutSec=1
        )
    )

    _disconnect(stub_a, session_a)
    _disconnect(stub_b, session_b)
