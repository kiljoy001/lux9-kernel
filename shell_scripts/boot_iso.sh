#!/bin/bash
# Boot ISO with TPM support for testing lux-c init
set -e

REPO=/home/scott/Repo/lux9-kernel
ISO=${REPO}/lux9.iso
QEMU=/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64

echo "[boot_iso] Booting lux9.iso with TPM support..."

# Check if ISO exists
if [ ! -f "$ISO" ]; then
    echo "[boot_iso] ERROR: ISO not found at $ISO"
    echo "[boot_iso] Run 'make iso' first"
    exit 1
fi

# Check if swtpm socket exists
if [ ! -S "$REPO/swtpm/swtpm-sock" ]; then
    echo "[boot_iso] WARNING: swtpm socket not found at $REPO/swtpm/swtpm-sock"
    echo "[boot_iso] TPM emulation may not work"
fi

# Boot with TPM support (matching os/Makefile configuration)
cd "$REPO"
$QEMU \
    -cdrom "$ISO" -boot d \
    -m 2G -smp 4 \
    -chardev socket,id=chrtpm,path=./swtpm/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis,tpmdev=tpm0 \
    -nographic -serial stdio \
    -display none
