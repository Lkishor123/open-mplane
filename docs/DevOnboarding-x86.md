# Dev Onboarding (x86 Simulator)

Prereqs (Ubuntu 22.04):
- build-essential, cmake, socat
- Optional: sysrepo/netopeer2, libyang, gRPC stack (for full server/client builds)

Build HAL, shim, server, and client (with adapters):
- `./tools/sim/build_sim.sh`

Run simulator:
- `./tools/sim/sim_start.sh`
- `./tools/sim/set_sync.sh LOCKED`
- `./tools/sim/inject_alarm.sh 1001 Major false "demo"`
- `./tools/sim/trigger_pm.sh RX_POWER /tmp`

Stop:
- `./tools/sim/sim_stop.sh`

Notes:
- `build/server-sim/mplane-server-app` is compiled with HAL_TEST adapters enabled to emit FM and SWM notifications and update software-inventory when driven by the shim.
- For end-to-end NETCONF tests, run `mplane-server-app` with your usual arguments (cfg-data, yang-mods-path, netopeer path), start the shim, then use `mplane_client` tests to validate the MP_* cases.
