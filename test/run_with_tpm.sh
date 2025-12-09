#!/bin/bash
# Run Lux9 kernel with TPM 2.0 emulation

set -e

TPM_DIR="/tmp/lux9-tpm"
ISO="../lux9.iso"

# Check if ISO exists, build if not
if [ ! -f "$ISO" ]; then
    echo "ISO not found, building..."
    cd .. && make iso
    if [ ! -f "$ISO" ]; then
        echo "ERROR: Failed to build ISO"
        exit 1
    fi
    cd test
fi

# Check if TPM is initialized
if [ ! -d "$TPM_DIR" ]; then
    echo "TPM not initialized. Running setup..."
    ./setup_swtpm.sh
fi

# Kill any existing swtpm instances
pkill -f "swtpm socket" || true
sleep 0.5

# Start swtpm in background
echo "Starting swtpm TPM 2.0 emulator..."
swtpm socket \
    --tpm2 \
    --tpmstate dir=$TPM_DIR \
    --ctrl type=unixio,path=$TPM_DIR/swtpm-sock \
    --log file=$TPM_DIR/swtpm.log,level=20 \
    --flags not-need-init \
    &

SWTPM_PID=$!
echo "✓ swtpm running (PID: $SWTPM_PID)"
sleep 0.5

# Cleanup on exit
trap "kill $SWTPM_PID 2>/dev/null || true" EXIT

# Run QEMU with TPM
echo ""
echo "=== Starting Lux9 with TPM 2.0 ==="
echo "TPM device: tpm-tis at default I/O base (0xFED40000)"
echo ""

qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -cdrom "$ISO" \
    -boot d \
    -chardev socket,id=chrtpm,path=$TPM_DIR/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis,tpmdev=tpm0 \
    -serial stdio \
    -no-reboot \
    -display none

echo ""
echo "=== QEMU exited ==="
echo "TPM log: $TPM_DIR/swtpm.log"
