#!/bin/bash
# Boot ISO without TPM for initial testing
set -e

REPO=/home/scott/Repo/lux9-kernel
ISO=${REPO}/lux9.iso
QEMU=qemu-system-x86_64

echo "[boot_iso_notpm] Booting lux9.iso (NO TPM)..."

# Check if ISO exists
if [ ! -f "$ISO" ]; then
    echo "[boot_iso_notpm] ERROR: ISO not found at $ISO"
    echo "[boot_iso_notpm] Run 'make iso' first"
    exit 1
fi

# Boot WITHOUT TPM support for testing
cd "$REPO"
$QEMU \
    -drive file="$ISO",format=raw,media=disk \
    -m 2G -smp 4 \
    -nographic \
    -serial mon:stdio
