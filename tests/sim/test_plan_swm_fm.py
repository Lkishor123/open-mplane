import os
import socket
import time

SOCK = "/tmp/haltest.sock"

def _send(cmd: str) -> str:
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(SOCK)
    s.sendall((cmd + "\n").encode())
    data = s.recv(4096)
    s.close()
    return data.decode().strip()

def test_sw_flow_commands_only():
    if not os.path.exists(SOCK):
        import pytest
        pytest.skip("shim not running; start with tools/sim/sim_start.sh")
    # Drive a nominal sw update flow on shim; server adapters emit notifications when enabled
    assert _send("sw phase=download result=COMPLETED file=image.bin").startswith("OK")
    assert _send("sw phase=install result=COMPLETED slot=slot1").startswith("OK")
    assert _send("sw phase=activate result=COMPLETED slot=slot1").startswith("OK")
    # No assertion here without NETCONF client; this is a smoke path

def test_fm_alarm_commands_only():
    if not os.path.exists(SOCK):
        import pytest
        pytest.skip("shim not running; start with tools/sim/sim_start.sh")
    assert _send("alarm id=1 severity=Major clear=false text=demo").startswith("OK")
    time.sleep(0.1)
    assert _send("alarm id=1 severity=Major clear=true text=demo").startswith("OK")

