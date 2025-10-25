#!/bin/bash
# Test memfs server functionality

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

SERVER="./memfs/memfs"
CLIENT="./test-client"

echo "=== Testing memfs Server ==="

if [ ! -f "$SERVER" ]; then
    echo -e "${RED}Error: memfs server not found at $SERVER${NC}"
    echo "Please build it with: go build -o memfs/memfs memfs.go"
    exit 1
fi

if [ ! -f "$CLIENT" ]; then
    echo -e "${RED}Error: test client not found at $CLIENT${NC}"
    exit 1
fi

# Start server in background
echo "Starting memfs server..."
timeout 5 "$SERVER" </dev/null >/tmp/memfs_output.log 2>&1 &
SERVER_PID=$!
sleep 0.2

if kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "Server started successfully (PID: $SERVER_PID)"
    
    # Run the test client
    echo "Running test client..."
    if timeout 2 "$CLIENT" 2>/tmp/client_output.log; then
        echo -e "${GREEN}Test PASSED${NC}"
        echo "Client output:"
        cat /tmp/client_output.log
    else
        echo -e "${RED}Test FAILED${NC}"
        echo "Client output:"
        cat /tmp/client_output.log
    fi
    
    # Kill server
    kill $SERVER_PID 2>/dev/null || true
else
    echo -e "${RED}Server failed to start${NC}"
    echo "Server output:"
    cat /tmp/memfs_output.log
fi