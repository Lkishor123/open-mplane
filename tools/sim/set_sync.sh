#!/usr/bin/env bash
set -euo pipefail

STATE=${1:-LOCKED}
printf 'sync state=%s\n' "$STATE" | socat - UNIX-CONNECT:/tmp/haltest.sock

