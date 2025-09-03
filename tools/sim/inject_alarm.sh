#!/usr/bin/env bash
set -euo pipefail

SEV=${2:-Major}
CLEAR=${3:-false}
TEXT=${4:-sim}
printf 'alarm id=%s severity=%s clear=%s text=%s\n' "$1" "$SEV" "$CLEAR" "$TEXT" | socat - UNIX-CONNECT:/tmp/haltest.sock

