#!/bin/bash
echo "Testing different QEMU machine types with ISO loader..."
ISO="lux9.iso"

echo "=========================================="
echo "Test 1: Classic PC (i440FX) with ISO"
echo "=========================================="
timeout 30 qemu-system-x86_64 -M pc-i440fx-noble -drive file=$ISO,format=raw,media=disk -m 2G -no-reboot -nographic -monitor none -serial mon:stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 2: MicroVM with ISO"
echo "=========================================="
timeout 30 qemu-system-x86_64 -M microvm -drive file=$ISO,format=raw,media=disk -m 2G -no-reboot -nographic -monitor none -serial mon:stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 3: Q35 with explicit PIC timer"
echo "=========================================="
timeout 30 qemu-system-x86_64 -M q35 -device i8254 -drive file=$ISO,format=raw,media=disk -m 2G -no-reboot -nographic -monitor none -serial mon:stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 4: Q35 with HPET disabled"
echo "=========================================="
timeout 30 qemu-system-x86_64 -M q35 -machine hpet=off -drive file=$ISO,format=raw,media=disk -m 2G -no-reboot -nographic -monitor none -serial mon:stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 5: Q35 with explicit APIC settings"
echo "=========================================="
timeout 30 qemu-system-x86_64 -M q35 -cpu host -drive file=$ISO,format=raw,media=disk -m 2G -no-reboot -nographic -monitor none -serial mon:stdio || echo "Exit: $?"

echo ""
echo "=========================================="
echo "Test 6: Q35 with older CPU emulation"
echo "=========================================="
timeout 30 qemu-system-x86_64 -M q35 -cpu qemu64 -drive file=$ISO,format=raw,media=disk -m 2G -no-reboot -nographic -monitor none -serial mon:stdio || echo "Exit: $?"
