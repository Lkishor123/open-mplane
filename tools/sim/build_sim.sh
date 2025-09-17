#!/usr/bin/env bash
set -euo pipefail

# Build halmplane (x86), mplane-server with HAL_TEST adapters, and mplane_client

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." &>/dev/null && pwd)

echo "[sim] Building halmplane (x86)"
cmake -S "$ROOT_DIR/libhalmplane" -B "$ROOT_DIR/build/hal-x86" \
  -DCONTEXT=YOCTO -DBUILD_BOARD=x86 -DBUILD_BOARD_STYLE=MONO -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/build/hal-x86" -j

echo "[sim] Building server-test-shim"

HALMPLANE_LIB="$ROOT_DIR/build/hal-x86/libhalmplane.so"
HALMPLANE_INCLUDE_DIR="$ROOT_DIR/libhalmplane/inc"
HALMPLANE_X86_INCLUDE_DIR="$ROOT_DIR/libhalmplane/x86/inc"
HALMPLANE_BUILD_INCLUDE_DIR="$ROOT_DIR/build/hal-x86"

cmake -S "$ROOT_DIR/mplane_server/utils/test_shim" \
      -B "$ROOT_DIR/mplane_server/utils/test_shim/build" \
      -DHALMPLANE_LIB="$HALMPLANE_LIB" \
      -DHALMPLANE_INCLUDE_DIR="$HALMPLANE_INCLUDE_DIR" \
      -DHALMPLANE_X86_INCLUDE_DIR="$HALMPLANE_X86_INCLUDE_DIR" \
      -DHALMPLANE_BUILD_INCLUDE_DIR="$HALMPLANE_BUILD_INCLUDE_DIR"

cmake --build "$ROOT_DIR/mplane_server/utils/test_shim/build" -j

echo "[sim] Building mplane_server with HAL_TEST"
cmake -S "$ROOT_DIR/mplane_server" -B "$ROOT_DIR/build/server-sim" -DHAL_TEST=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/build/server-sim" -j

echo "[sim] Building mplane_client"
cmake -S "$ROOT_DIR/mplane_client" -B "$ROOT_DIR/build/client-sim" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/build/client-sim" -j
export SIM_CLIENT_BIN="$ROOT_DIR/build/client-sim/mpc_client"

echo "[sim] Done. Binaries:"
echo " - Shim: $ROOT_DIR/mplane_server/utils/test_shim/build/server-test-shim"
echo " - Server: $ROOT_DIR/build/server-sim/mplane-server-app"
echo " - Client: $SIM_CLIENT_BIN"

