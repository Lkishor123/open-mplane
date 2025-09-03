#!/usr/bin/env bash
set -euo pipefail

# Simple launcher for simulator shim and optional server/client

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
eval "${SIM_CLIENT_BIN:-mplane_client/build/mpc_client} &"
echo $! > "${SIM_CLIENT_PID_FILE:-/tmp/mplane-client.pid}"
echo "Client PID $(cat ${SIM_CLIENT_PID_FILE:-/tmp/mplane-client.pid})"

