#!/usr/bin/env bash
set -euo pipefail

# Build halmplane (x86) and mplane-server with HAL_TEST adapters

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." &>/dev/null && pwd)

echo "[sim] Building halmplane (x86)"
cmake -S "$ROOT_DIR/libhalmplane" -B "$ROOT_DIR/build/hal-x86" \
  -DCONTEXT=YOCTO -DBUILD_BOARD=x86 -DBUILD_BOARD_STYLE=MONO -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/build/hal-x86" -j

echo "[sim] Building server-test-shim"
cmake -S "$ROOT_DIR/mplane_server/utils/test_shim" -B "$ROOT_DIR/mplane_server/utils/test_shim/build"
cmake --build "$ROOT_DIR/mplane_server/utils/test_shim/build" -j

echo "[sim] Building mplane_server with HAL_TEST"
cmake -S "$ROOT_DIR/mplane_server" -B "$ROOT_DIR/build/server-sim" -DHAL_TEST=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/build/server-sim" -j

echo "[sim] Done. Binaries:"
echo " - Shim: $ROOT_DIR/mplane_server/utils/test_shim/build/server-test-shim"
echo " - Server: $ROOT_DIR/build/server-sim/mplane-server-app"

