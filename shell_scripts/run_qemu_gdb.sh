#!/bin/bash
cd /home/scott/Repo/lux9-kernel || exit

# QEMU binary configuration
# Default to system QEMU 10.1.2, with fallback to older 9.1.0 if needed
QEMU_BIN=${QEMU_BIN:-$(which qemu-system-x86_64)}

# Verify QEMU binary exists and is executable
if [ ! -x "$QEMU_BIN" ]; then
    echo "ERROR: QEMU binary not found or not executable: $QEMU_BIN"
    echo "Checking fallback to QEMU 9.1.0..."
    QEMU_BIN="/home/scott/Repo/gnumach/qemu-9.1.0-x64-install/bin/qemu-system-x86_64"
    if [ ! -x "$QEMU_BIN" ]; then
        echo "ERROR: Fallback QEMU 9.1.0 not found either: $QEMU_BIN"
        exit 1
    fi
fi

# Display QEMU version
echo "Using QEMU: $QEMU_BIN"
echo "QEMU Version: $($QEMU_BIN --version | head -1)"

# Log file path
LOGFILE="/home/scott/Repo/lux9-kernel/qemu.log"
echo "Log file: $LOGFILE"

# Clear previous log
> "$LOGFILE"

# Run QEMU with GTK window and serial output to logfile
echo "Starting QEMU with GDB remote debugging..."
"$QEMU_BIN" \
  -M q35 \
  -m 2G \
  -cdrom lux9.iso \
  -boot d \
  -display gtk \
  -serial file:"$LOGFILE" \
  -no-reboot \
  -s -S &

QEMU_PID=$!
echo "QEMU started with PID: $QEMU_PID"

# Wait two seconds for VM launch:
echo "Waiting for QEMU to initialize..."
sleep 2

# Verify QEMU is still running before starting GDB
if ! kill -0 $QEMU_PID 2>/dev/null; then
    echo "ERROR: QEMU process died before GDB connection"
    echo "Check log file for errors: $LOGFILE"
    exit 1
fi

# Start GDB and connect automatically
echo "Starting GDB..."
gdb -ex "target remote tcp:localhost:1234" lux9.elf

# Clean up: kill QEMU when GDB exits
echo "Cleaning up QEMU process..."
kill $QEMU_PID 2>/dev/null
wait $QEMU_PID 2>/dev/null
echo "Debugging session complete"