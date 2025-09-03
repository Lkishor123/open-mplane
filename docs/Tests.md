# Tests and Simulator Harness

Scope:
- End-to-end tests will drive the stack via `mplane_client` gRPC and NETCONF RPCs. This repo now includes the simulator shim and mock HAL to make x86 testing feasible without hardware.

Quickstart:
- Build HAL, shim, server, and client with adapters: `./tools/sim/build_sim.sh`
- Start shim: `./tools/sim/sim_start.sh`
- Start server (separate terminal): `build/server-sim/mplane-server-app --cfg-data-path <...> --netopeer-path <...> --yang-mods-path <...>`
- Inject scenarios using helper scripts:
  - `./tools/sim/set_sync.sh LOCKED`
  - `./tools/sim/uplane_tx_ready.sh <carrier>`
  - `./tools/sim/inject_alarm.sh 1001 Major false "RF path warning"`
  - `./tools/sim/trigger_pm.sh RX_POWER /tmp`

pytest (skeleton):
- Place tests under `tests/sim/` and use a fixture that ensures shim is running.
- Use `mplane_client` to connect to the NETCONF server and perform:
  - MP_STARTUP_001/002: listen/call-home versus direct connect
  - MP_SW_*: drive software mgmt outcomes by setting `sw` phase via shim and asserting notifications
  - MP_FM_*: assert alarms stream contains injected faults
  - MP_UPLANE_*: edit-config to set `active=ACTIVE`, verify readiness with sync preconditions enforced (toggle with `set_sync.sh`)
  - MP_PM_*: trigger PM and fetch CSV artifacts

Status & Artifacts:
- `server-test-shim` logs to stderr; capture with your CI runner
- PM CSV files are written to the directory provided in `pm out=...`
- Use `status` command to dump current mock state

CI:
- The `sim-x86` workflow compiles HAL, Shim, and mplane_server with adapters and runs a basic shim sanity ping. Extend it to launch server and `pytest` once your environment provides sysrepo/netopeer2.
