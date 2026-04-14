#!/bin/bash
echo "Testing different QEMU configurations for APIC timer support..."
KERNEL="kernel/lux9.elf"

echo "=========================================="
echo "Test 1: Current configuration (Q35)"
echo "=========================================="
timeout 15 qemu-system-x86_64 -M q35 -m 2G -kernel $KERNEL -no-reboot -display none -serial stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 2: Classic PC (i440FX)"
echo "=========================================="
timeout 15 qemu-system-x86_64 -M pc-i440fx-noble -m 2G -kernel $KERNEL -no-reboot -display none -serial stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 3: MicroVM (minimal)"
echo "=========================================="
timeout 15 qemu-system-x86_64 -M microvm -cpu host -m 2G -kernel $KERNEL -no-reboot -display none -serial stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 4: Q35 with explicit APIC"
echo "=========================================="
timeout 15 qemu-system-x86_64 -M q35,apic=on -m 2G -kernel $KERNEL -no-reboot -display none -serial stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 5: Q35 with PIT timer"
echo "=========================================="
timeout 15 qemu-system-x86_64 -M q35 -device i8254 -m 2G -kernel $KERNEL -no-reboot -display none -serial stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 6: Q35 with HPET"
echo "=========================================="
timeout 15 qemu-system-x86_64 -M q35 -no-hpet -m 2G -kernel $KERNEL -no-reboot -display none -serial stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 7: Q35 with PIC fallback"
echo "=========================================="
timeout 15 qemu-system-x86_64 -M q35 -no-apic -m 2G -kernel $KERNEL -no-reboot -display none -serial stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 8: Using ISO (bootloader)"
echo "=========================================="
if [ -f "os/lux9.iso" ]; then
    timeout 15 qemu-system-x86_64 -drive file=os/lux9.iso,format=raw,media=disk -m 2G -no-reboot -display none -serial stdio || echo "Exit: $?"
else
    echo "ISO not available - make os first"
fi
