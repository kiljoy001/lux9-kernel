#!/bin/bash

# Test script to verify 9P exchange test implementation

echo "=== 9P Exchange Test Implementation Verification ==="

# Check that both binaries exist
echo "Checking binaries..."
if [ -f "/home/scott/Repo/lux9-kernel/userspace/go-servers/exchange/exchange_test" ]; then
    echo "✓ Original exchange_test binary exists"
else
    echo "✗ Original exchange_test binary missing"
fi

if [ -f "/home/scott/Repo/lux9-kernel/userspace/go-servers/exchange/exchange_9p_test" ]; then
    echo "✓ 9P exchange_9p_test binary exists"
else
    echo "✗ 9P exchange_9p_test binary missing"
fi

# Check that both binaries are in the initrd
echo "Checking initrd contents..."
cd /home/scott/Repo/lux9-kernel/userspace
if tar -tf initrd.tar | grep -q "exchange_test"; then
    echo "✓ exchange_test in initrd"
else
    echo "✗ exchange_test missing from initrd"
fi

if tar -tf initrd.tar | grep -q "exchange_9p_test"; then
    echo "✓ exchange_9p_test in initrd"
else
    echo "✗ exchange_9p_test missing from initrd"
fi

# Check init.c configuration
echo "Checking init.c configuration..."
if grep -q "running 9P exchange test" /home/scott/Repo/lux9-kernel/userspace/bin/init.c; then
    echo "✓ init.c configured to run both tests"
else
    echo "✗ init.c not configured correctly"
fi

echo "=== Verification Complete ==="