#!/bin/bash

# Test script for userspace drivers

set -e

echo "Testing userspace drivers..."

# Build drivers first
echo "Building drivers..."
./build.sh

# Test AHCI driver
echo "Testing AHCI driver..."
if [ -f "bin/ahci_driver" ]; then
    echo "AHCI driver binary exists"
    file bin/ahci_driver
else
    echo "ERROR: AHCI driver binary not found"
    exit 1
fi

# Test IDE driver
echo "Testing IDE driver..."
if [ -f "bin/ide_driver" ]; then
    echo "IDE driver binary exists"
    file bin/ide_driver
else
    echo "ERROR: IDE driver binary not found"
    exit 1
fi

echo "Driver tests completed successfully!"
