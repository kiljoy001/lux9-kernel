#!/bin/bash
# Setup and run Software TPM 2.0 for kernel testing
# Uses swtpm to provide a virtual TPM device for QEMU

set -e

TPM_DIR="/tmp/lux9-tpm"
SWTPM_LOG="$TPM_DIR/swtpm.log"

echo "=== Lux9 TPM 2.0 Testing Setup ==="

# Clean up old TPM state
if [ -d "$TPM_DIR" ]; then
    echo "Cleaning old TPM state..."
    rm -rf "$TPM_DIR"
fi

# Create TPM directory
mkdir -p "$TPM_DIR"

# Initialize TPM state
echo "Initializing TPM 2.0 state..."
swtpm_setup \
    --tpm2 \
    --tpmstate "$TPM_DIR" \
    --create-ek-cert \
    --create-platform-cert \
    --lock-nvram \
    --not-overwrite \
    --display

if [ $? -ne 0 ]; then
    echo "ERROR: swtpm_setup failed"
    exit 1
fi

echo ""
echo "✓ TPM 2.0 state initialized in $TPM_DIR"
echo ""
echo "To start swtpm socket, run:"
echo "  swtpm socket --tpm2 --tpmstate dir=$TPM_DIR --ctrl type=unixio,path=$TPM_DIR/swtpm-sock --log file=$SWTPM_LOG,level=20 &"
echo ""
echo "To run QEMU with TPM:"
echo "  qemu-system-x86_64 -M q35 -m 2G -kernel lux9.elf \\"
echo "    -chardev socket,id=chrtpm,path=$TPM_DIR/swtpm-sock \\"
echo "    -tpmdev emulator,id=tpm0,chardev=chrtpm \\"
echo "    -device tpm-tis,tpmdev=tpm0 \\"
echo "    -serial stdio -no-reboot"
echo ""
