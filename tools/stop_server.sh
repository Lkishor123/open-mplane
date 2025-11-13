#!/usr/bin/env bash

# Stop M-Plane Server and HAL shim

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}[Server] Stopping M-Plane Server...${NC}"

# Stop server
if [[ -f /tmp/mplane-server-app.pid ]]; then
    SERVER_PID=$(cat /tmp/mplane-server-app.pid)
    if ps -p $SERVER_PID > /dev/null 2>&1; then
        echo -e "${YELLOW}[Server] Stopping server (PID: $SERVER_PID)${NC}"
        sudo kill $SERVER_PID 2>/dev/null || true
        sleep 1
        # Force kill if still running
        if ps -p $SERVER_PID > /dev/null 2>&1; then
            echo -e "${RED}[Server] Force killing server${NC}"
            sudo kill -9 $SERVER_PID 2>/dev/null || true
        fi
    else
        echo -e "${YELLOW}[Server] Server process not running${NC}"
    fi
    rm -f /tmp/mplane-server-app.pid
else
    echo -e "${YELLOW}[Server] No PID file found, searching for process...${NC}"
    sudo pkill -f mplane-server-app || true
fi

# Stop shim
if [[ -f /tmp/server-test-shim.pid ]]; then
    SHIM_PID=$(cat /tmp/server-test-shim.pid)
    if ps -p $SHIM_PID > /dev/null 2>&1; then
        echo -e "${YELLOW}[Server] Stopping test shim (PID: $SHIM_PID)${NC}"
        kill $SHIM_PID 2>/dev/null || true
        sleep 1
        # Force kill if still running
        if ps -p $SHIM_PID > /dev/null 2>&1; then
            echo -e "${RED}[Server] Force killing test shim${NC}"
            kill -9 $SHIM_PID 2>/dev/null || true
        fi
    else
        echo -e "${YELLOW}[Server] Test shim process not running${NC}"
    fi
    rm -f /tmp/server-test-shim.pid
else
    echo -e "${YELLOW}[Server] No shim PID file found, searching for process...${NC}"
    pkill -f server-test-shim || true
fi

# Clean up socket
if [[ -S /tmp/haltest.sock ]]; then
    echo -e "${YELLOW}[Server] Removing socket: /tmp/haltest.sock${NC}"
    rm -f /tmp/haltest.sock
fi

# Clean up sysrepo shared memory
echo -e "${YELLOW}[Server] Cleaning sysrepo shared memory${NC}"
sudo rm -rf /dev/shm/sr_* /dev/shm/srsub_* 2>/dev/null || true

echo -e "${GREEN}[Server] Server stopped${NC}"
