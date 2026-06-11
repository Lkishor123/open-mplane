#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPEN_MPLANE_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

if [[ -r /etc/os-release ]]; then
  # shellcheck disable=SC1091
  . /etc/os-release
else
  echo "Cannot detect OS; expected Ubuntu 22.04." >&2
  exit 1
fi

if [[ "${ID}" != "ubuntu" || "${VERSION_ID}" != "22.04" ]]; then
  echo "Unsupported OS ${PRETTY_NAME:-unknown}; Open M-Plane dev build expects Ubuntu 22.04." >&2
  exit 1
fi

if [[ "${1:-}" == "--apt-only" || "${1:-}" == "--with-apt" ]]; then
  sudo apt-get update
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
    autoconf automake bison build-essential ca-certificates ccache chrpath \
    cmake curl diffstat doxygen flex gdb git graphviz iproute2 \
    libboost-all-dev libcurl4-openssl-dev libfftw3-dev libgflags-dev \
    libgoogle-glog-dev libgtest-dev libpcre3-dev libssl-dev libtinyxml2-dev \
    libtool libxml2-dev net-tools ninja-build pkg-config protobuf-compiler \
    protobuf-compiler-grpc python3 python3-pip python3-venv rsync sudo swig unzip \
    vim wget zlib1g-dev
  if [[ "${1:-}" == "--apt-only" ]]; then
    exit 0
  fi
fi

cd "${OPEN_MPLANE_ROOT}/mplane_client/utils"

if [[ ! -d ../deps/libnetconf2 || ! -d ../deps/grpc ]]; then
  ./get_deps.sh --no-fwdproxy --dir ../
fi

./build_deps.sh --dir ../

echo
echo "Dependency bootstrap complete."
echo "Next: ./dev/scripts/build_all.sh"
