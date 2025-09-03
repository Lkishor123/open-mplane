#!/usr/bin/env bash
set -euo pipefail

PHASE=${1:?phase}
RESULT=${2:?result}
printf 'sw phase=%s result=%s\n' "$PHASE" "$RESULT" | socat - UNIX-CONNECT:/tmp/haltest.sock

