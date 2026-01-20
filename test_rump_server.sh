#!/bin/bash
# Test script for rump_server basic functionality

set -e

echo "=== Rump Server Test Suite ==="
echo

# Test 1: Binary exists and is executable
echo "[TEST 1] Checking rump_server binary..."
if [ -f "userspace/rump/rump_server" ]; then
    echo "✓ Binary exists"
    ls -lh userspace/rump/rump_server
else
    echo "✗ Binary not found"
    exit 1
fi

# Test 2: Verify ELF format
echo
echo "[TEST 2] Verifying ELF format..."
file userspace/rump/rump_server
if file userspace/rump/rump_server | grep -q "ELF 64-bit"; then
    echo "✓ Valid ELF64 binary"
else
    echo "✗ Not a valid ELF64 binary"
    exit 1
fi

# Test 3: Check for rump symbols
echo
echo "[TEST 3] Checking for rump kernel symbols..."
if nm userspace/rump/rump_server | grep -q "rump_"; then
    echo "✓ Rump symbols found:"
    nm userspace/rump/rump_server | grep "T rump_" | head -5
else
    echo "✗ No rump symbols found"
    exit 1
fi

# Test 4: Check for file I/O symbols
echo
echo "[TEST 4] Checking for file I/O symbols..."
if nm userspace/rump/rump_server | grep -q "rumpuser_open"; then
    echo "✓ rumpuser_open found"
fi
if nm userspace/rump/rump_server | grep -q "rumpuser_bio"; then
    echo "✓ rumpuser_bio found"
fi
if nm userspace/rump/rump_server | grep -q "rumpuser_iovread"; then
    echo "✓ rumpuser_iovread found"
fi

# Test 5: Check for liblux integration
echo
echo "[TEST 5] Checking for liblux symbols..."
if nm userspace/rump/rump_server | grep -q "strlen"; then
    echo "✓ strlen found (from liblux)"
fi
if nm userspace/rump/rump_server | grep -q "malloc"; then
    echo "✓ malloc found (from liblux)"
fi

# Test 6: Verify initrd deployment
echo
echo "[TEST 6] Checking initrd deployment..."
if [ -f "initrd/boot/rump_server" ]; then
    echo "✓ Deployed to initrd"
    ls -lh initrd/boot/rump_server
else
    echo "⚠ Not deployed to initrd yet"
fi

# Test 7: Check dependencies
echo
echo "[TEST 7] Checking library dependencies..."
readelf -d userspace/rump/rump_server 2>/dev/null | grep -i "needed" || echo "✓ Statically linked (no dynamic dependencies)"

echo
echo "=== Test Summary ==="
echo "✓ All basic tests passed"
echo "✓ Binary: $(ls -lh userspace/rump/rump_server | awk '{print $5}')"
echo "✓ Entry point: $(readelf -h userspace/rump/rump_server | grep Entry | awk '{print $4}')"
echo
echo "Next: Boot kernel and test rump_server functionality"
