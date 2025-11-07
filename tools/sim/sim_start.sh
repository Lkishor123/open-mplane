#!/usr/bin/env bash
set -euo pipefail

# Simple launcher for simulator shim and optional server/client

# Get the root directory
ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." &>/dev/null && pwd)

# Set library path for server, client, and HAL dependencies
export LD_LIBRARY_PATH="$ROOT_DIR/mplane_server/deps/install/lib:$ROOT_DIR/mplane_client/deps/install/lib:$ROOT_DIR/build/hal-x86:${LD_LIBRARY_PATH:-}"

# Set libyang module search path
export YANG_MODPATH="$ROOT_DIR/mplane_server/deps/install/share/yang/modules/libyang"

SOCK=/tmp/haltest.sock
if [[ -S "$SOCK" ]]; then
  echo "Removing stale $SOCK"
  rm -f "$SOCK"
fi

echo "Starting server-test-shim..."
"${SIM_SHIM_BIN:-mplane_server/utils/test_shim/build/server-test-shim}" &
echo $! > "${SIM_SHIM_PID_FILE:-/tmp/server-test-shim.pid}"
echo "Shim PID $(cat ${SIM_SHIM_PID_FILE:-/tmp/server-test-shim.pid})"

echo "Starting mplane-server-app..."
"${SIM_SERVER_BIN:-build/server-sim/mplane-server-app}" &
echo $! > "${SIM_SERVER_PID_FILE:-/tmp/mplane-server-app.pid}"
echo "Server PID $(cat ${SIM_SERVER_PID_FILE:-/tmp/mplane-server-app.pid})"

echo "Starting mplane_client gRPC listener..."
eval "${SIM_CLIENT_BIN:-mplane_client/build/mpc_client} --insecure &"
echo $! > "${SIM_CLIENT_PID_FILE:-/tmp/mplane-client.pid}"
echo "Client PID $(cat ${SIM_CLIENT_PID_FILE:-/tmp/mplane-client.pid})"

