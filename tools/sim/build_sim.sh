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

export CMAKE_PREFIX_PATH="$ROOT_DIR/mplane_server/deps/install"
DEPS_INSTALL="$ROOT_DIR/mplane_server/deps/install"

echo "[sim] Building mplane_server with HAL_TEST"
cmake -S "$ROOT_DIR/mplane_server" \
      -B "$ROOT_DIR/build/server-sim" \
      -DHAL_TEST=ON \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DHALMPLANE="$HALMPLANE_LIB" \
      -DSYSREPO-CPP="$DEPS_INSTALL/lib/libsysrepo-cpp.so" \
      -DYANG-CPP="$DEPS_INSTALL/lib/libyang-cpp.so" \
      -DCMAKE_PREFIX_PATH="$DEPS_INSTALL" \
      -DCMAKE_INCLUDE_PATH="$DEPS_INSTALL/include" \
      -DCMAKE_LIBRARY_PATH="$DEPS_INSTALL/lib"

cmake --build "$ROOT_DIR/build/server-sim" -j

echo "[sim] Building mplane_client"

# Check if dependencies need to be fetched and built
MPLANE_CLIENT_DIR="$ROOT_DIR/mplane_client"
DEPS_DIR="$MPLANE_CLIENT_DIR/deps"

if [[ ! -d "$DEPS_DIR/install" ]]; then
    echo "[sim] Setting up mplane_client dependencies..."

    # Fetch dependencies
    cd "$MPLANE_CLIENT_DIR/utils"
    ./get_deps.sh --no-fwdproxy --dir ../

    # Build dependencies with patches
    ./build_deps.sh --no-netopeer2 --dir ../

    # Clean up sim-o1-interface directory if it exists
    if [[ -d "$MPLANE_CLIENT_DIR/test/docker/sim-o1-interface" ]]; then
        echo "[sim] Removing sim-o1-interface directory to avoid build conflicts..."
        rm -rf "$MPLANE_CLIENT_DIR/test/docker/sim-o1-interface"
    fi

    cd "$ROOT_DIR"
else
    echo "[sim] Dependencies already built, skipping dependency setup..."
fi

# Build mplane_client using the build script
cd "$MPLANE_CLIENT_DIR/utils"
./build_mpclient.sh --parallel 2

cd "$ROOT_DIR"
export SIM_CLIENT_BIN="$MPLANE_CLIENT_DIR/build/mpc_client"

echo "[sim] Done. Binaries:"
echo " - Shim: $ROOT_DIR/mplane_server/utils/test_shim/build/server-test-shim"
echo " - Server: $ROOT_DIR/build/server-sim/mplane-server-app"
echo " - Client: $SIM_CLIENT_BIN"

