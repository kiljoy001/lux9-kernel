#!/bin/bash
# Debug ISO boot with GDB
set -e

REPO=/home/scott/Repo/lux9-kernel
ISO=${REPO}/lux9.iso

echo "[debug_iso] Starting QEMU with GDB server..."

cd "$REPO"
qemu-system-x86_64 \
    -drive file="$ISO",format=raw,media=disk \
    -m 2G \
    -no-reboot \
    -display none \
    -serial stdio \
    -s -S &

QEMU_PID=$!
echo "[debug_iso] QEMU PID: $QEMU_PID (waiting on :1234)"
echo "[debug_iso] Connect with: gdb lux9.elf -ex 'target remote :1234'"
echo "[debug_iso] Press Ctrl+C to stop"

wait $QEMU_PID
