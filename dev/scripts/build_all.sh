#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPEN_MPLANE_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
JOBS="${JOBS:-$(nproc)}"

# shellcheck disable=SC1091
. "${SCRIPT_DIR}/env.sh"

if [[ ! -x "${OPEN_MPLANE_CLIENT_DEPS}/bin/protoc" ]]; then
  echo "Missing client dependency prefix. Run ./dev/scripts/bootstrap_ubuntu22.sh first." >&2
  exit 1
fi

mkdir -p "${OPEN_MPLANE_DEV_PREFIX}"

echo "==> Building mplane_client"
(
  cd "${OPEN_MPLANE_ROOT}/mplane_client/utils"
  ./build_mpclient.sh --parallel "${JOBS}"
)

echo "==> Building libhalmplane modular HAL"
cmake -S "${OPEN_MPLANE_ROOT}/libhalmplane" \
  -B "${OPEN_MPLANE_ROOT}/build/dev/libhalmplane" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX="${OPEN_MPLANE_DEV_PREFIX}" \
  -DCONTEXT=YOCTO \
  -DBUILD_BOARD=modular \
  -DBUILD_BOARD_STYLE=STATIC \
  -DCMAKE_PREFIX_PATH="${OPEN_MPLANE_DEV_PREFIX};${OPEN_MPLANE_CLIENT_DEPS}" \
  -DCMAKE_CXX_FLAGS="-I${OPEN_MPLANE_CLIENT_DEPS}/include" \
  -DCMAKE_EXE_LINKER_FLAGS="-L${OPEN_MPLANE_CLIENT_DEPS}/lib -L${OPEN_MPLANE_CLIENT_DEPS}/lib64 -Wl,-rpath,${OPEN_MPLANE_CLIENT_DEPS}/lib -Wl,-rpath,${OPEN_MPLANE_CLIENT_DEPS}/lib64"
cmake --build "${OPEN_MPLANE_ROOT}/build/dev/libhalmplane" --parallel "${JOBS}"
cmake --install "${OPEN_MPLANE_ROOT}/build/dev/libhalmplane"

echo "==> Building mplane_server"
cmake -S "${OPEN_MPLANE_ROOT}/mplane_server" \
  -B "${OPEN_MPLANE_ROOT}/build/dev/mplane_server" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX="${OPEN_MPLANE_DEV_PREFIX}" \
  -DCMAKE_PREFIX_PATH="${OPEN_MPLANE_DEV_PREFIX};${OPEN_MPLANE_CLIENT_DEPS}" \
  -DCMAKE_CXX_FLAGS="-I${OPEN_MPLANE_DEV_PREFIX}/include -I${OPEN_MPLANE_CLIENT_DEPS}/include" \
  -DCMAKE_EXE_LINKER_FLAGS="-L${OPEN_MPLANE_DEV_PREFIX}/lib -L${OPEN_MPLANE_CLIENT_DEPS}/lib -L${OPEN_MPLANE_CLIENT_DEPS}/lib64 -Wl,-rpath,${OPEN_MPLANE_DEV_PREFIX}/lib -Wl,-rpath,${OPEN_MPLANE_CLIENT_DEPS}/lib -Wl,-rpath,${OPEN_MPLANE_CLIENT_DEPS}/lib64"
cmake --build "${OPEN_MPLANE_ROOT}/build/dev/mplane_server" --parallel "${JOBS}"
cmake --install "${OPEN_MPLANE_ROOT}/build/dev/mplane_server"

"${SCRIPT_DIR}/install_dev_assets.sh"

echo
echo "Open M-Plane dev build complete."
echo "Load environment with: . ./dev/scripts/env.sh"
