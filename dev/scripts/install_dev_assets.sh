#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
. "${SCRIPT_DIR}/env.sh"

CFG_DIR="${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server"
MODULES_DIR="${CFG_DIR}/modules"

mkdir -p "${MODULES_DIR}"

cp "${OPEN_MPLANE_ROOT}/mplane_server/yang-manager-server/yang-config/YangConfig.xml" "${CFG_DIR}/YangConfig.xml"
find "${OPEN_MPLANE_ROOT}/mplane_server/yang-models" -type f -name '*.yang' -exec cp {} "${MODULES_DIR}/" \;

echo "Installed dev server assets:"
echo "  ${CFG_DIR}/YangConfig.xml"
echo "  ${MODULES_DIR}"
