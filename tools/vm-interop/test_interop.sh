#!/bin/bash
# Test VM interoperability by creating timestamped files

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
HOSTNAME=$(hostname)

echo "Testing VM Interop at $TIMESTAMP from $HOSTNAME" > "test_from_${HOSTNAME}_${TIMESTAMP}.txt"
echo "Created test file: test_from_${HOSTNAME}_${TIMESTAMP}.txt"

echo ""
echo "Current files in VM-Interop:"
ls -la /home/scott/Repo/VM-Interop/

echo ""
echo "To test from other VMs:"
echo "  9front:  echo 'Hello from 9front' > /n/interop/from_9front.txt"
echo "  Linux:   echo 'Hello from Linux' > /mnt/interop/from_linux.txt"