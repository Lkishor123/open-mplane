#!/bin/bash
# M-Plane Server Setup Script
# Runs all required setup steps in the correct order

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Configuration
DEPS_INSTALL_DIR="${PROJECT_ROOT}/mplane_server/deps/install"
YANG_MODULES_DIR="/usr/share/mplane-server/modules"
SYSREPO_REPO_DIR="/etc/sysrepo"

# Command line options
FORCE=false
SETUP_ONLY=false
DRY_RUN=false

# Parse command line arguments
for arg in "$@"; do
    case $arg in
        --force)
            FORCE=true
            ;;
        --setup-only)
            SETUP_ONLY=true
            ;;
        --dry-run)
            DRY_RUN=true
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --force        Force reinstall/reconfigure everything"
            echo "  --setup-only   Only run setup, don't start server"
            echo "  --dry-run      Show what would be done without executing"
            echo "  --help         Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $arg${NC}"
            echo "Use --help to see available options"
            exit 1
            ;;
    esac
done

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

run_step() {
    local description="$1"
    local command="$2"

    if [ "$DRY_RUN" = true ]; then
        echo -e "${BLUE}[DRY-RUN]${NC} $description"
        echo "  Command: $command"
        return 0
    fi

    log_info "$description"
    if eval "$command"; then
        log_success "$description - Done"
        return 0
    else
        log_error "$description - Failed"
        return 1
    fi
}

check_exists() {
    local description="$1"
    local path="$2"

    if [ -e "$path" ]; then
        log_success "$description: $path"
        return 0
    else
        log_error "$description not found: $path"
        return 1
    fi
}

# Start
echo ""
echo "=== M-Plane Server Setup ==="
echo ""

# Phase 0: Pre-flight checks
log_info "Phase 0: Pre-flight Checks"

# Find sysrepo tools
SYSREPOCTL=""
SYSREPOCFG=""

if [ -x "${DEPS_INSTALL_DIR}/bin/sysrepoctl" ]; then
    SYSREPOCTL="${DEPS_INSTALL_DIR}/bin/sysrepoctl"
    SYSREPOCFG="${DEPS_INSTALL_DIR}/bin/sysrepocfg"
elif command -v sysrepoctl &> /dev/null; then
    SYSREPOCTL="sysrepoctl"
    SYSREPOCFG="sysrepocfg"
else
    log_error "sysrepoctl not found"
    exit 1
fi

log_success "Found sysrepoctl: $SYSREPOCTL"

# Check openssl
if ! command -v openssl &> /dev/null; then
    log_error "openssl not found"
    exit 1
fi
log_success "Found openssl"

# Check YANG modules directory
if ! check_exists "YANG modules directory" "$YANG_MODULES_DIR"; then
    exit 1
fi

# Check sysrepo repository
if ! check_exists "Sysrepo repository" "$SYSREPO_REPO_DIR"; then
    exit 1
fi

# Phase 1: Script discovery
log_info "Phase 1: Script Discovery"

# Find netopeer2 scripts (check build directory first, it has generated scripts)
NETOPEER2_SCRIPTS=""
if [ -f "${PROJECT_ROOT}/mplane_server/deps/netopeer2/build/setup.sh" ]; then
    NETOPEER2_SCRIPTS="${PROJECT_ROOT}/mplane_server/deps/netopeer2/build"
elif [ -f "${PROJECT_ROOT}/mplane_server/deps/netopeer2/scripts/setup.sh" ]; then
    NETOPEER2_SCRIPTS="${PROJECT_ROOT}/mplane_server/deps/netopeer2/scripts"
else
    log_error "Netopeer2 scripts not found (looking for setup.sh)"
    log_error "Build directory may not exist: ${PROJECT_ROOT}/mplane_server/deps/netopeer2/build"
    exit 1
fi
log_success "Found netopeer2 scripts: $NETOPEER2_SCRIPTS"

# Check all required scripts exist
check_exists "setup.sh" "${NETOPEER2_SCRIPTS}/setup.sh" || exit 1
check_exists "merge_hostkey.sh" "${NETOPEER2_SCRIPTS}/merge_hostkey.sh" || exit 1
check_exists "merge_config.sh" "${NETOPEER2_SCRIPTS}/merge_config.sh" || exit 1

# Find O-RAN user config script
ORAN_USER_SCRIPT="${PROJECT_ROOT}/mplane_server/scripts/o-ran-user-config.sh"
check_exists "o-ran-user-config.sh" "$ORAN_USER_SCRIPT" || exit 1

# Phase 3: Environment setup
log_info "Phase 3: Environment Setup"

export PATH="${DEPS_INSTALL_DIR}/bin:${PATH}"
export LD_LIBRARY_PATH="${DEPS_INSTALL_DIR}/lib64:${DEPS_INSTALL_DIR}/lib:${LD_LIBRARY_PATH}"
export NP2_MODULE_DIR="${YANG_MODULES_DIR}"
export NP2_MODULE_PERMS="600"

log_success "Environment configured"

# Phase 4: YANG Module Installation
log_info "Phase 4: YANG Module Installation"

# Check if modules already installed
if [ "$FORCE" = false ]; then
    if $SYSREPOCTL -l 2>/dev/null | grep -q "ietf-netconf-server"; then
        log_warning "NETCONF modules already installed (use --force to reinstall)"
    else
        run_step "Installing YANG modules" "cd '${NETOPEER2_SCRIPTS}' && ./setup.sh"
    fi
else
    run_step "Installing YANG modules" "cd '${NETOPEER2_SCRIPTS}' && ./setup.sh"
fi

# Phase 5: SSH Key Generation
log_info "Phase 5: SSH Key Generation"

# Check if key already exists
if [ "$FORCE" = false ]; then
    if $SYSREPOCFG -X -d startup -f json -x "/ietf-keystore:keystore" 2>/dev/null | grep -q "genkey"; then
        log_warning "SSH key 'genkey' already exists (use --force to regenerate)"
    else
        run_step "Generating SSH host key" "cd '${NETOPEER2_SCRIPTS}' && ./merge_hostkey.sh"
    fi
else
    run_step "Generating SSH host key" "cd '${NETOPEER2_SCRIPTS}' && ./merge_hostkey.sh"
fi

# Phase 6: NETCONF Server Configuration
log_info "Phase 6: NETCONF Server Configuration"

# Check if endpoint already configured
if [ "$FORCE" = false ]; then
    if $SYSREPOCFG -X -d startup -f json -x "/ietf-netconf-server:netconf-server" 2>/dev/null | grep -q "ssh"; then
        log_warning "NETCONF endpoint already configured (use --force to reconfigure)"
    else
        run_step "Configuring NETCONF endpoint" "cd '${NETOPEER2_SCRIPTS}' && ./merge_config.sh"
    fi
else
    run_step "Configuring NETCONF endpoint" "cd '${NETOPEER2_SCRIPTS}' && ./merge_config.sh"
fi

# Phase 7: O-RAN User Management
log_info "Phase 7: O-RAN User Management"

# Check if already configured
if [ "$FORCE" = false ]; then
    if $SYSREPOCTL -l 2>/dev/null | grep -q "o-ran-usermgmt"; then
        log_warning "O-RAN users already configured (use --force to reconfigure)"
    else
        run_step "Configuring O-RAN users" "cd '$(dirname ${ORAN_USER_SCRIPT})' && ./$(basename ${ORAN_USER_SCRIPT}) --sysrepo-path '${DEPS_INSTALL_DIR}/bin' --modules '${YANG_MODULES_DIR}'"
    fi
else
    run_step "Configuring O-RAN users" "cd '$(dirname ${ORAN_USER_SCRIPT})' && ./$(basename ${ORAN_USER_SCRIPT}) --sysrepo-path '${DEPS_INSTALL_DIR}/bin' --modules '${YANG_MODULES_DIR}'"
fi

# Phase 8: Cleanup
log_info "Phase 8: Pre-start Cleanup"

# Clean sysrepo shared memory
if [ "$DRY_RUN" = false ]; then
    sudo rm -rf /dev/shm/sr_* /dev/shm/srsub_* 2>/dev/null || true
    rm -rf /dev/shm/sr_* /dev/shm/srsub_* 2>/dev/null || true
    log_success "Cleaned sysrepo shared memory"
fi

# Show summary
echo ""
log_success "Setup completed successfully!"
echo ""

# Show installed module count
if [ "$DRY_RUN" = false ]; then
    MODULE_COUNT=$(LD_LIBRARY_PATH="${DEPS_INSTALL_DIR}/lib64:${DEPS_INSTALL_DIR}/lib:${LD_LIBRARY_PATH}" $SYSREPOCTL -l 2>/dev/null | grep -c "^[a-zA-Z0-9]" || echo "0")
    log_info "Installed sysrepo modules: $MODULE_COUNT"
fi

# Phase 9: Start server (optional)
if [ "$SETUP_ONLY" = false ] && [ "$DRY_RUN" = false ]; then
    echo ""
    read -p "Start M-Plane server now? [Y/n]: " -n 1 -r
    echo ""

    if [[ $REPLY =~ ^[Yy]$ ]] || [[ -z $REPLY ]]; then
        # Find server binary
        SERVER_BINARY="${PROJECT_ROOT}/build/server-sim/mplane-server-app"

        if [ ! -x "$SERVER_BINARY" ]; then
            log_error "Server binary not found: $SERVER_BINARY"
            log_info "Build the server first: cd ${PROJECT_ROOT} && ./build.sh"
            exit 1
        fi

        # Server configuration
        CFG_DATA_PATH="${PROJECT_ROOT}/mplane_server/yang-manager-server/yang-config"
        HAL_LIB_PATH="${PROJECT_ROOT}/build/hal-x86"
        NETOPEER_BIN_DIR="${DEPS_INSTALL_DIR}/bin"

        log_info "Starting M-Plane server..."
        echo ""

        # Start server
        LD_LIBRARY_PATH="${DEPS_INSTALL_DIR}/lib64:${DEPS_INSTALL_DIR}/lib:${HAL_LIB_PATH}:${LD_LIBRARY_PATH}" \
        "${SERVER_BINARY}" \
            --cfg-data-path "${CFG_DATA_PATH}" \
            --yang-mods-path "${YANG_MODULES_DIR}" \
            --netopeer-path "${NETOPEER_BIN_DIR}" \
            --netopeerdbg 2
    else
        echo ""
        log_info "To start server manually, run:"
        echo ""
        echo "  cd ${PROJECT_ROOT}"
        echo "  ./tools/run_server_only.sh"
        echo ""
    fi
else
    if [ "$DRY_RUN" = false ]; then
        echo ""
        log_info "Setup complete. To start server, run:"
        echo ""
        echo "  cd ${PROJECT_ROOT}"
        echo "  ./tools/run_server_only.sh"
        echo ""
    fi
fi
