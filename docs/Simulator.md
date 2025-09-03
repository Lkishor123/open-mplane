# Simulator Mode (x86)

Design notes (short):
- Control plane: a lightweight UNIX socket daemon `server-test-shim` at `/tmp/haltest.sock` accepts line commands to manipulate the mock HAL at runtime. It maps to internal C state (sync, carriers, SW mgmt, PM) and emits state through existing HAL callbacks so the server sends standard notifications.
- HAL wiring: x86 `halmplane` library gains mock implementations for U-Plane, Sync, Alarms, and PM guarded by the `x86` board selection. Carrier state transitions honor a sync precondition (LOCKED required for `ACTIVE`).
- Observability: `status` command returns a compact JSON snapshot of sync + SW outcomes. PM stub writes CSV files to a local directory (simulated SFTP upload).
- Non-x86 boards: No behavior change; mocks only compile for `BUILD_BOARD=x86`.

Shim command protocol (one line per command):
- `status` → returns JSON: `{ "sync_locked": <bool>, "sw": { ... } }`
- `sync state=LOCKED|FREERUN` → toggles mock Sync status
- `alarm id=<u16> source=<str> severity=Critical|Major|Minor|Warning clear=true|false text=<str>` → emits FM alarm
- `sw phase=download|install|activate|reset result=OK|INTEGRITY_ERROR|FAIL [slot=<name>] [file=<name>]` → sets SW outcomes and context
- `pm object=<RX_POWER> out=/tmp` → generates a CSV file in `out`
- `uplane dir=tx|rx name=<carrier> state=ACTIVE|INACTIVE|SLEEP` → apply carrier state

Files:
- `libhalmplane/x86/inc/mock_hal_control.h`: control API used by the shim
- `libhalmplane/x86/src/*`: mock implementations and init hooks
- `mplane_server/utils/test_shim/ServerTestShim.cpp`: UNIX socket daemon

Scripts (tools/sim):
- `sim_start.sh`, `sim_stop.sh` – start/stop shim, `mplane-server-app`, and `mplane_client`
- `inject_alarm.sh`, `set_sync.sh`, `force_sw_result.sh`, `trigger_pm.sh` – helpers

Environment variables:
- `SIM_SHIM_BIN` – shim binary path
- `SIM_SERVER_BIN` – `mplane-server-app` binary (default `build/server-sim/mplane-server-app`)
- `SIM_CLIENT_BIN` – `mplane_client` command (default `mplane_client/build/mpc_client`)
- `SIM_SHIM_PID_FILE`, `SIM_SERVER_PID_FILE`, `SIM_CLIENT_PID_FILE` – PID file locations

Build (example):
- Build halmplane for x86 (Yocto-style vars shown for clarity):
  - `cmake -S libhalmplane -B build/hal-x86 -DCONTEXT=YOCTO -DBUILD_BOARD=x86 -DBUILD_BOARD_STYLE=MONO -DCMAKE_BUILD_TYPE=RelWithDebInfo`
  - `cmake --build build/hal-x86 -j`
- Build shim:
  - `cmake -S mplane_server/utils/test_shim -B mplane_server/utils/test_shim/build`
  - `cmake --build mplane_server/utils/test_shim/build -j`
- Start shim: `./tools/sim/sim_start.sh`

Notes:
- `sim_start.sh` launches `mplane-server-app` and the `mplane_client` gRPC listener automatically; override paths via variables above.
- When SW events are emitted with `slot=<name>`, the server updates `/o-ran-swm:software-inventory/software-slot[name='<name>']` status/active/running accordingly.
