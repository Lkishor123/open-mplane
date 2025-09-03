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


USER_NAME = "testuser_fmpm"
USER_PASS = "aabbCC11!!"


def _open_session(user: str = NETCONF_USER, password: str = NETCONF_PASS):
    """Open a NETCONF session via mpclient."""
    channel = grpc.insecure_channel(GRPC_ADDR)
    stub = mpclient_pb2_grpc.MpclientStub(channel)
    try:
        conn = stub.connect(
            mpclient_pb2.ConnectRequest(
                host=NETCONF_HOST,
                port=NETCONF_PORT,
                auth=mpclient_pb2.Authentication(user=user, password=password),
            ),
            timeout=5,
        )
    except Exception:
        return None, None
    if not conn.success:
        return None, None
    return stub, conn.sessionId


def _open_session_or_skip(user: str = NETCONF_USER, password: str = NETCONF_PASS):
    stub, session = _open_session(user, password)
    if stub is None:
        pytest.skip("mplane_client gRPC not running")
    return stub, session


def _disconnect(stub, session):
    try:
        stub.disconnect(mpclient_pb2.DisconnectRequest(sessionId=session))
    except Exception:
        pass


def test_fmpm_user_restricted_operation():
    stub_priv, session_priv = _open_session_or_skip()

    create_user_xml = (
        '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "  <config>"
        "    <users xmlns=\"urn:o-ran:user-mgmt:1.0\">"
        "      <user>"
        f"        <name>{USER_NAME}</name>"
        f"        <password>{USER_PASS}</password>"
        "        <enabled>true</enabled>"
        "      </user>"
        "    </users>"
        "  </config>"
        "</edit-config>"
    )
    resp = stub_priv.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_priv, serializedYang=create_user_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.OK

    assign_group_xml = (
        '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "  <config>"
        "    <nacm xmlns=\"urn:ietf:params:xml:ns:yang:ietf-netconf-acm\">"
        "      <groups>"
        "        <group>"
        "          <name>fm-pm</name>"
        f"          <user-name>{USER_NAME}</user-name>"
        "        </group>"
        "      </groups>"
        "    </nacm>"
        "  </config>"
        "</edit-config>"
    )
    resp = stub_priv.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_priv, serializedYang=assign_group_xml, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.OK

    stub_user, session_user = _open_session_or_skip(USER_NAME, USER_PASS)

    download_rpc = (
        '<software-download xmlns="urn:o-ran:software-management:1.0" '
        'xmlns:o-ran-fm="urn:o-ran:file-management:1.0">'
        '<remote-file-path>http://example.com/image.bin</remote-file-path>'
        '<o-ran-fm:password><o-ran-fm:password>pw</o-ran-fm:password>'
        '</o-ran-fm:password>'
        '</software-download>'
    )
    resp = stub_user.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_user, serializedYang=download_rpc, timeoutSec=1
        )
    )
    assert resp.status == mpclient_pb2.NetconfRpcResponse.Status.SUCCESS
    assert (
        resp.returnType == mpclient_pb2.NetconfRpcResponse.ReturnType.RPC_ERROR
    )
    assert "access-denied" in resp.message

    _disconnect(stub_user, session_user)

    delete_group_xml = (
        '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "  <config>"
        "    <nacm xmlns=\"urn:ietf:params:xml:ns:yang:ietf-netconf-acm\">"
        "      <groups>"
        "        <group operation=\"delete\">"
        "          <name>fm-pm</name>"
        "        </group>"
        "      </groups>"
        "    </nacm>"
        "  </config>"
        "</edit-config>"
    )
    stub_priv.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_priv, serializedYang=delete_group_xml, timeoutSec=1
        )
    )

    delete_user_xml = (
        '<edit-config xmlns="urn:ietf:params:xml:ns:netconf:base:1.0">'
        "  <target><running/></target>"
        "  <config>"
        "    <users xmlns=\"urn:o-ran:user-mgmt:1.0\">"
        "      <user operation=\"delete\">"
        f"        <name>{USER_NAME}</name>"
        "      </user>"
        "    </users>"
        "  </config>"
        "</edit-config>"
    )
    stub_priv.netconfRpc(
        mpclient_pb2.NetconfRpcRequest(
            sessionId=session_priv, serializedYang=delete_user_xml, timeoutSec=1
        )
    )

    _disconnect(stub_priv, session_priv)
