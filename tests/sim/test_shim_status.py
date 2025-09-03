import os
import socket

SOCK = "/tmp/haltest.sock"

def send(cmd: str) -> str:
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(SOCK)
    s.sendall((cmd + "\n").encode())
    data = s.recv(4096)
    s.close()
    return data.decode().strip()

def test_status_roundtrip():
    if not os.path.exists(SOCK):
        import pytest
        pytest.skip("shim not running")
    out = send("status")
    assert "sync_locked" in out
    assert out.endswith("}")

def test_sync_toggle():
    if not os.path.exists(SOCK):
        import pytest
        pytest.skip("shim not running")
    assert send("sync state=LOCKED").startswith("OK")
    js = send("status")
    assert '"sync_locked":true' in js
    assert send("sync state=FREERUN").startswith("OK")
    js = send("status")
    assert '"sync_locked":false' in js

