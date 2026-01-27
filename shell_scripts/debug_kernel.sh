#!/bin/bash
# debug_kernel.sh - Simple boot test for Lux9 Kernel

echo "=== Lux9 Kernel Boot Test ==="
echo "Booting lux9.elf directly in QEMU..."
echo ""

# Run QEMU directly with the kernel ELF
# -kernel lux9.elf: Boot the kernel directly
# -serial stdio: Output serial logs to console
# -display none: No GUI window
# -no-reboot: Exit instead of rebooting on panic
qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -kernel lux9.elf \
    -no-reboot \
    -display none \
    -serial stdio

echo ""
echo "=== Boot Test Complete ==="