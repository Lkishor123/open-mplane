#!/usr/bin/env bash
set -euo pipefail
DIR=${1:?tx|rx}
CARRIER=${2:?carrier}
STATE=${3:?state}
printf 'uplane dir=%s name=%s state=%s\n' "$DIR" "$CARRIER" "$STATE" | socat - UNIX-CONNECT:/tmp/haltest.sock
