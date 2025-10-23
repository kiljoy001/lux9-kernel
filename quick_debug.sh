#!/bin/bash
# quick_debug.sh - Simple debugging that runs QEMU and GDB together

# Kill any existing QEMU instances
killall qemu-system-x86_64 2>/dev/null

# Start QEMU in background
echo "Starting QEMU..."
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d -s -S -serial stdio &
QEMU_PID=$!

# Give QEMU time to start
sleep 1

echo "Starting GDB..."
# Run GDB with automatic commands
gdb -batch \
    -ex "target remote :1234" \
    -ex "break proc0" \
    -ex "continue" \
    -ex "printf \"===== IN PROC0 - STARTING STEP TRACE =====\\n\"" \
    -ex "set \$counter = 0" \
    -ex "while (\$counter < 10000)" \
    -ex "  stepi" \
    -ex "  set \$counter = \$counter + 1" \
    -ex "  if (\$counter % 1000 == 0)" \
    -ex "    printf \"Step %d: RIP=0x%016lx RSP=0x%016lx\\n\", \$counter, \$rip, \$rsp" \
    -ex "  end" \
    -ex "end" \
    lux9.elf

# Kill QEMU when done
kill $QEMU_PID 2>/dev/null
echo "Debugging complete"