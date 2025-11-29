#!/bin/bash
# Test memfs server functionality

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

SERVER="memfs/memfs"
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

# Run the test client (which starts the server itself)
echo "Running test client..."
if timeout 5 "$CLIENT" >/tmp/client_output.log 2>&1; then
    echo -e "${GREEN}Test PASSED${NC}"
    echo "Client output:"
    cat /tmp/client_output.log
else
    echo -e "${RED}Test FAILED${NC}"
    echo "Client output:"
    cat /tmp/client_output.log
    exit 1
fi