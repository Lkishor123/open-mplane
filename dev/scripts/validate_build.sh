#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
. "${SCRIPT_DIR}/env.sh"

required_files=(
  "${OPEN_MPLANE_ROOT}/mplane_client/build/mpc_client"
  "${OPEN_MPLANE_ROOT}/mplane_client/build/mpclient-demo"
  "${OPEN_MPLANE_ROOT}/mplane_client/build/mpc_tester"
  "${OPEN_MPLANE_DEV_PREFIX}/lib/libhalmplane.so"
  "${OPEN_MPLANE_DEV_PREFIX}/sbin/mplane-server-app"
  "${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server/YangConfig.xml"
)

for path in "${required_files[@]}"; do
  if [[ ! -e "${path}" ]]; then
    echo "Missing expected build artifact: ${path}" >&2
    exit 1
  fi
done

"${OPEN_MPLANE_ROOT}/mplane_client/build/mpc_client" --help >/dev/null || true
"${OPEN_MPLANE_ROOT}/mplane_client/build/mpclient-demo" --help >/dev/null || true
"${OPEN_MPLANE_DEV_PREFIX}/sbin/mplane-server-app" --help >/dev/null || true

echo "Open M-Plane dev artifacts are present and executable."
echo
echo "Server smoke command:"
echo "  mplane-server-app --cfg-data-path ${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server --netopeer-path ${OPEN_MPLANE_CLIENT_DEPS}/bin --yang-mods-path ${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server/modules --netopeerdbg 2"
