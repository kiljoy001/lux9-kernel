#!/bin/bash
# run_debug.sh - Run QEMU with GDB debugging

echo "Starting QEMU with GDB debugging..."
echo "Make sure to run: gdb -x debug_mmu.gdb lux9.elf"
echo ""

# Clean up previous logs
rm -f mmu_debug.log qemu_debug.log

# Start QEMU with comprehensive debugging
qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -cdrom lux9.iso \
    -boot d \
    -s -S \
    -d int,cpu_reset,mmu \
    -D qemu_debug.log \
    -no-shutdown \
    -no-reboot &

QEMU_PID=$!

echo "QEMU started with PID $QEMU_PID"
echo "QEMU debug output in qemu_debug.log"
echo ""
echo "Now run: gdb -x debug_mmu.gdb lux9.elf"
echo "Press Enter when ready to kill QEMU..."
read

# Kill QEMU when done
kill $QEMU_PID 2>/dev/null
echo "QEMU killed"

# Show summary
echo ""
echo "=== DEBUG SUMMARY ==="
echo "GDB log: mmu_debug.log"
echo "QEMU log: qemu_debug.log"