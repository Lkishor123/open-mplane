#!/usr/bin/env bash
set -euo pipefail

CARRIER=${1:?carrier}
printf 'uplane dir=tx name=%s state=ACTIVE\n' "$CARRIER" | socat - UNIX-CONNECT:/tmp/haltest.sock
