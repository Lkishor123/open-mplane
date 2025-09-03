#!/usr/bin/env bash
set -euo pipefail

stop_proc() {
  local pid_file="$1"
  local label="$2"
  if [[ -f "$pid_file" ]]; then
    local pid=$(cat "$pid_file")
    echo "Stopping $label PID $pid"
    kill "$pid" || true
    rm -f "$pid_file"
  fi
}

stop_proc "${SIM_SERVER_PID_FILE:-/tmp/mplane-server-app.pid}" "server"
stop_proc "${SIM_CLIENT_PID_FILE:-/tmp/mplane-client.pid}" "client"
stop_proc "${SIM_SHIM_PID_FILE:-/tmp/server-test-shim.pid}" "shim"

rm -f /tmp/haltest.sock || true

