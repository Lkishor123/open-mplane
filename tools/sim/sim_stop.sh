#!/usr/bin/env bash
set -euo pipefail

PID_FILE="${SIM_SHIM_PID_FILE:-/tmp/server-test-shim.pid}"
if [[ -f "$PID_FILE" ]]; then
  PID=$(cat "$PID_FILE")
  echo "Stopping shim PID $PID"
  kill "$PID" || true
  rm -f "$PID_FILE"
fi
rm -f /tmp/haltest.sock || true

