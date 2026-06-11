#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
. "${SCRIPT_DIR}/env.sh"

GRPC_PORT="${GRPC_PORT:-50051}"
NETCONF_PORT="${NETCONF_PORT:-1830}"
NETCONF_USER="${NETCONF_USER:-vagrant}"
EDIT_DESCRIPTION="${EDIT_DESCRIPTION:-mock-du-poc-interface-edit}"
RUN_DIR="${OPEN_MPLANE_ROOT}/build/dev/mock-du-flow"
RUNTIME_DIR="${TMPDIR:-/tmp}/open-mplane-mock-du-flow-${USER}"
SERVER_LOG="${RUN_DIR}/mplane-server.log"
MPC_LOG="${RUN_DIR}/mpc-client.log"
DEMO_LOG="${RUN_DIR}/mpclient-demo.log"
MOCK_RU_LOG="${RUN_DIR}/mock-ru.log"
COMMANDS_JSON="${RUN_DIR}/mock-du-commands.json"
USERLIST="${RUN_DIR}/userlist"
NETCONF_SERVER_CONFIG="${RUN_DIR}/netconf-server-port.xml"
SSH_KEY="${RUN_DIR}/netconf-client-key"
NETOPEER_PID_FILE="${RUNTIME_DIR}/netopeer2-server.pid"
NETOPEER_SOCKET="${RUNTIME_DIR}/netopeer2-server.sock"
SYSREPO_REPO=""
NATIVE_SYSREPO_REPO="${RUNTIME_DIR}/sysrepo-repository"
SYSREPO_REPO_MOUNTED=0
AUTHORIZED_KEYS="${HOME}/.ssh/authorized_keys"
SERVER_PID=""
MPC_PID=""

require_file() {
  if [[ ! -e "$1" ]]; then
    echo "Missing required file: $1" >&2
    echo "Run ./dev/scripts/build_all.sh first." >&2
    exit 1
  fi
}

wait_for_port() {
  local host="$1"
  local port="$2"
  local label="$3"
  local timeout_sec="${4:-30}"
  local deadline=$((SECONDS + timeout_sec))

  while (( SECONDS < deadline )); do
    if (echo >"/dev/tcp/${host}/${port}") >/dev/null 2>&1; then
      return 0
    fi
    sleep 1
  done

  echo "Timed out waiting for ${label} on ${host}:${port}" >&2
  return 1
}

pid_is_running() {
  local pid="$1"
  local state

  state="$(ps -p "${pid}" -o stat= 2>/dev/null || true)"
  [[ -n "${state}" && "${state}" != Z* ]]
}

cleanup() {
  set +e
  if [[ -n "${MPC_PID}" ]] && kill -0 "${MPC_PID}" >/dev/null 2>&1; then
    kill "${MPC_PID}" >/dev/null 2>&1
    wait "${MPC_PID}" >/dev/null 2>&1
  fi
  if [[ -n "${SERVER_PID}" ]] && kill -0 "${SERVER_PID}" >/dev/null 2>&1; then
    sudo kill -INT "${SERVER_PID}" >/dev/null 2>&1
    sleep 2
    sudo kill "${SERVER_PID}" >/dev/null 2>&1
    wait "${SERVER_PID}" >/dev/null 2>&1
  fi
  if [[ -f "${NETOPEER_PID_FILE}" ]]; then
    sudo kill "$(cat "${NETOPEER_PID_FILE}")" >/dev/null 2>&1
  fi
  if [[ "${SYSREPO_REPO_MOUNTED}" == "1" && -n "${SYSREPO_REPO}" ]]; then
    sudo umount "${SYSREPO_REPO}" >/dev/null 2>&1
  fi
  if [[ -f "${SSH_KEY}.pub" && -f "${AUTHORIZED_KEYS}" ]]; then
    grep -v -F -f "${SSH_KEY}.pub" "${AUTHORIZED_KEYS}" >"${RUN_DIR}/authorized_keys.tmp" || true
    cp "${RUN_DIR}/authorized_keys.tmp" "${AUTHORIZED_KEYS}"
  fi
}

trap cleanup EXIT

require_file "${OPEN_MPLANE_ROOT}/mplane_client/build/mpc_client"
require_file "${OPEN_MPLANE_ROOT}/mplane_client/build/mpclient-demo"
require_file "${OPEN_MPLANE_DEV_PREFIX}/sbin/mplane-server-app"
require_file "${OPEN_MPLANE_DEV_PREFIX}/lib/libhalmplane.so"
require_file "${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server/YangConfig.xml"

mkdir -p "${RUN_DIR}" "${RUNTIME_DIR}" "${HOME}/.ssh"
chmod 700 "${HOME}/.ssh"
rm -f "${SERVER_LOG}" "${MPC_LOG}" "${DEMO_LOG}" "${MOCK_RU_LOG}" \
  "${COMMANDS_JSON}" "${USERLIST}" "${SSH_KEY}" "${SSH_KEY}.pub" \
  "${NETCONF_SERVER_CONFIG}" "${NETOPEER_PID_FILE}" "${NETOPEER_SOCKET}"

if ! sudo -n true >/dev/null 2>&1; then
  echo "Passwordless sudo is required to clean stale sysrepo shared-memory files." >&2
  exit 1
fi
sudo rm -f /dev/shm/sr_* /dev/shm/srsub_* /var/run/netopeer2-server.pid

SYSREPOCTL_OUTPUT="$("${SYSREPOCTL_EXECUTABLE}" -l 2>/dev/null)"
SYSREPO_REPO="$(awk -F': ' '/^Sysrepo repository:/ {print $2; exit}' <<<"${SYSREPOCTL_OUTPUT}")"
if [[ -z "${SYSREPO_REPO}" || ! -d "${SYSREPO_REPO}" ]]; then
  echo "Unable to locate the sysrepo repository from sysrepoctl." >&2
  exit 1
fi
if mountpoint -q "${SYSREPO_REPO}"; then
  sudo umount "${SYSREPO_REPO}"
fi
rm -rf "${NATIVE_SYSREPO_REPO}"
mkdir -p "${NATIVE_SYSREPO_REPO}"
cp -a "${SYSREPO_REPO}/." "${NATIVE_SYSREPO_REPO}/"
sudo mount --bind "${NATIVE_SYSREPO_REPO}" "${SYSREPO_REPO}"
SYSREPO_REPO_MOUNTED=1

ssh-keygen -q -t ed25519 -N "" -f "${SSH_KEY}"
touch "${AUTHORIZED_KEYS}"
chmod 600 "${AUTHORIZED_KEYS}"
cat "${SSH_KEY}.pub" >>"${AUTHORIZED_KEYS}"

cat >"${USERLIST}" <<EOF
${NETCONF_USER}:root:unused
EOF

"${OPEN_MPLANE_ROOT}/mplane_server/scripts/o-ran-user-config.sh" \
  --modules "${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server/modules" \
  --sysrepo-path "${OPEN_MPLANE_CLIENT_DEPS}/bin" \
  --userlist "${USERLIST}" >"${RUN_DIR}/o-ran-user-config.log" 2>&1

cat >"${NETCONF_SERVER_CONFIG}" <<EOF
<netconf-server xmlns="urn:ietf:params:xml:ns:yang:ietf-netconf-server">
  <listen>
    <endpoint>
      <name>default-ssh</name>
      <ssh>
        <tcp-server-parameters>
          <local-address>0.0.0.0</local-address>
          <local-port>${NETCONF_PORT}</local-port>
        </tcp-server-parameters>
      </ssh>
    </endpoint>
  </listen>
</netconf-server>
EOF
"${SYSREPOCFG_EXECUTABLE}" --edit="${NETCONF_SERVER_CONFIG}" -d startup -f xml \
  -m ietf-netconf-server -v2 >>"${RUN_DIR}/o-ran-user-config.log" 2>&1
"${SYSREPOCFG_EXECUTABLE}" -C startup -m ietf-netconf-server -v2 \
  >>"${RUN_DIR}/o-ran-user-config.log" 2>&1

cat >"${COMMANDS_JSON}" <<EOF
{
  "commands": [
    {
      "connect": {}
    },
    {
      "netconfRpc": {
        "serializedYang": "<get xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><filter type=\"subtree\"><netconf-state xmlns=\"urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring\"><capabilities/></netconf-state></filter></get>",
        "timeoutSec": 10
      }
    },
    {
      "netconfRpc": {
        "serializedYang": "<edit-config xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"><target><running/></target><default-operation>merge</default-operation><config><interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\"><interface><name>eth0</name><description>${EDIT_DESCRIPTION}</description></interface></interfaces></config></edit-config>",
        "timeoutSec": 10
      }
    },
    {
      "disconnect": {}
    }
  ]
}
EOF

export MPLANE_MOCK_RU_LOG="${MOCK_RU_LOG}"
export NETOPEER2_SERVER_ARGS="-p ${NETOPEER_PID_FILE} -U${NETOPEER_SOCKET}"

mplane-server-app \
  --cfg-data-path "${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server" \
  --netopeer-path "${OPEN_MPLANE_CLIENT_DEPS}/bin" \
  --yang-mods-path "${OPEN_MPLANE_DEV_PREFIX}/share/mplane-server/modules" \
  --netopeerdbg 2 >"${SERVER_LOG}" 2>&1 &
SERVER_PID=$!

sleep 2
if ! pid_is_running "${SERVER_PID}"; then
  echo "mplane-server-app exited during startup. See ${SERVER_LOG}" >&2
  tail -n 80 "${SERVER_LOG}" >&2 || true
  exit 1
fi
if ! wait_for_port 127.0.0.1 "${NETCONF_PORT}" "netopeer2 NETCONF" 45; then
  if ! pid_is_running "${SERVER_PID}"; then
    echo "mplane-server-app exited before netopeer2 opened NETCONF. See ${SERVER_LOG}" >&2
  fi
  tail -n 80 "${SERVER_LOG}" >&2 || true
  exit 1
fi

"${OPEN_MPLANE_ROOT}/mplane_client/build/mpc_client" \
  --host 127.0.0.1 \
  --port "${GRPC_PORT}" \
  --insecure >"${MPC_LOG}" 2>&1 &
MPC_PID=$!

if ! wait_for_port 127.0.0.1 "${GRPC_PORT}" "mplane_client gRPC" 30; then
  tail -n 80 "${MPC_LOG}" >&2 || true
  exit 1
fi

"${OPEN_MPLANE_ROOT}/mplane_client/build/mpclient-demo" \
  --commands "${COMMANDS_JSON}" \
  --serverHost 127.0.0.1 \
  --serverPort "${GRPC_PORT}" \
  --netconfHost 127.0.0.1 \
  --netconfPort "${NETCONF_PORT}" \
  --netconfUser "${NETCONF_USER}" \
  --netconfPublicKeyPath "${SSH_KEY}.pub" \
  --netconfPrivateKeyPath "${SSH_KEY}" \
  --insecure >"${DEMO_LOG}" 2>&1

if grep -E "connect failed|error before response|timeout communicating|disconnect failed|the mpclient RPC response was not received" "${DEMO_LOG}" >/dev/null; then
  echo "mpclient-demo reported a command failure. See ${DEMO_LOG}" >&2
  exit 1
fi

if ! grep -F "Command 3: " "${DEMO_LOG}" | grep -F "SUCCESS" >/dev/null; then
  echo "mpclient-demo did not complete all mock DU commands. See ${DEMO_LOG}" >&2
  exit 1
fi

if ! grep -F "interface_update_description name=eth0 description=${EDIT_DESCRIPTION}" "${MOCK_RU_LOG}" >/dev/null; then
  echo "Mock RU HAL log did not record the expected interface edit. See ${MOCK_RU_LOG}" >&2
  exit 1
fi

echo "Mock DU M-Plane flow passed."
echo "  NETCONF get capabilities: sent through mplane_client"
echo "  NETCONF edit-config: updated ietf-interfaces/eth0 description"
echo "  Mock RU HAL log: ${MOCK_RU_LOG}"
