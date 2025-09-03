#!/usr/bin/env bash
set -euo pipefail

OBJ=${1:?object}
OUT=${2:-/tmp}
printf 'pm object=%s out=%s\n' "$OBJ" "$OUT" | socat - UNIX-CONNECT:/tmp/haltest.sock

