#!/usr/bin/env bash
set -euo pipefail

# Simple launcher for simulator shim only (server/client launch hooks TODO)

SOCK=/tmp/haltest.sock
if [[ -S "$SOCK" ]]; then
  echo "Removing stale $SOCK"
  rm -f "$SOCK"
fi

echo "Starting server-test-shim..."
"${SIM_SHIM_BIN:-mplane_server/utils/test_shim/build/server-test-shim}" &
echo $! > "${SIM_SHIM_PID_FILE:-/tmp/server-test-shim.pid}"
echo "Shim PID $(cat ${SIM_SHIM_PID_FILE:-/tmp/server-test-shim.pid})"

