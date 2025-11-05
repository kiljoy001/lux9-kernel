#!/bin/bash
# Debug CR3 switch with GDB

cd /home/scott/Repo/lux9-kernel

# Kill any existing QEMU
pkill -9 qemu-system-x86_64 2>/dev/null

# Start QEMU in background with GDB stub
echo "Starting QEMU with GDB stub..."
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d \
    -serial file:qemu.log -no-reboot -display none \
    -s -S &

QEMU_PID=$!
echo "QEMU started (PID: $QEMU_PID)"

# Give QEMU time to start
sleep 2

# Run GDB with our script
echo "Attaching GDB..."
gdb lux9.elf -x gdb_smart_step.gdb

# Kill QEMU when done
kill $QEMU_PID 2>/dev/null
wait $QEMU_PID 2>/dev/null

echo "Done. Check gdb_cr3_debug.log for results"
