#!/bin/bash
# Test script for running Lux9 with SWTPM enabled

cd /home/scott/Repo/lux9-kernel || exit 1

# Setup TPM directory
TPM_DIR="/tmp/lux9-tpm"
mkdir -p "$TPM_DIR"
rm -rf "$TPM_DIR/*"

# Start SWTPM
echo "Starting SWTPM..."
swtpm socket --tpmstate dir="$TPM_DIR" \
             --ctrl type=unixio,path="$TPM_DIR/swtpm-sock" \
             --tpm2 \
             --log level=20 &
SWTPM_PID=$!
echo "SWTPM PID: $SWTPM_PID"

# Wait for SWTPM to initialize
sleep 2

# Log file
LOGFILE="/home/scott/Repo/lux9-kernel/qemu_tpm.log"
> "$LOGFILE"

# QEMU binary
QEMU_BIN=$(which qemu-system-x86_64)

echo "Starting QEMU with TPM..."
timeout 15s "$QEMU_BIN" \
  -M q35 \
  -m 2G \
  -accel kvm \
  -cpu host \
  -cdrom lux9.iso \
  -boot d \
  -display none \
  -serial file:"$LOGFILE" \
  -no-reboot \
  -chardev socket,id=chrtpm,path="$TPM_DIR/swtpm-sock" \
  -tpmdev emulator,id=tpm0,chardev=chrtpm \
  -device tpm-tis,tpmdev=tpm0 &

QEMU_PID=$!
echo "QEMU PID: $QEMU_PID"

# Wait for QEMU to finish or timeout
wait $QEMU_PID

# Cleanup
echo "Cleaning up..."
kill $SWTPM_PID 2>/dev/null
rm -rf "$TPM_DIR"

# Analyze log
echo "=== Kernel Log Output ==="
grep -i "tpm" "$LOGFILE" || echo "No TPM messages found in log."
echo "=== End Log ==="
