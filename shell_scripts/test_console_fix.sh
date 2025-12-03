#!/bin/bash
# test_console_fix.sh - Test the console fixes
# This tests both UART console and framebuffer detection

echo "=== Testing Lux9 Console Fixes ==="
echo ""
echo "This test checks:"
echo "1. UART console always works (even if framebuffer fails)"
echo "2. Enhanced framebuffer debugging messages"
echo "3. QEMU VGA output support"
echo ""

cd /home/scott/Repo/lux9-kernel

# Build kernel if needed
if [ ! -f "kernel/lux9.elf" ]; then
    echo "Building kernel..."
    make kernel 2>&1 | tail -20
fi

# Test with VGA output (should detect framebuffer or fallback to UART)
echo "=== Test 1: VGA Mode (should show graphics or fallback to UART) ==="
echo "Starting QEMU with VGA support..."
echo "Look for console output in QEMU graphics window AND serial console"
echo "You should see:"
echo "  - UART serial messages in serial console"
echo "  - Either framebuffer messages OR UART fallback messages"
echo "  - Console should work regardless of framebuffer status"
echo ""
echo "Press Ctrl+C to stop, then check the logs..."

timeout 30 qemu-system-x86_64 \
    -kernel kernel/lux9.elf \
    -m 512M \
    -vga std \
    -display gtk \
    -serial stdio \
    -no-reboot || echo "QEMU stopped"

echo ""
echo "=== Test 2: Serial-Only Mode (UART only) ==="
echo "Starting QEMU in serial-only mode (UART console only)..."
echo "All output should appear in serial console window"
echo ""

timeout 30 qemu-system-x86_64 \
    -kernel kernel/lux9.elf \
    -m 512M \
    -serial stdio \
    -display none \
    -no-reboot || echo "QEMU stopped"

echo ""
echo "=== Analysis ==="
echo "Check the console output for:"
echo "✅ UART initialization messages"
echo "✅ Framebuffer detection debugging (detailed info about Limine response)"
echo "✅ Either framebuffer success OR clean fallback to UART"
echo "✅ Console should work in both tests"
echo ""
echo "Expected debug messages:"
echo "  save_framebuffer_info: checking limine_framebuffer=..."
echo "  save_framebuffer_info: response=..."
echo "  save_framebuffer_info: framebuffer_count=..."
echo ""
echo "If framebuffer works:"
echo "  fbconsoleinit: using saved framebuffer info"
echo "  fbconsole: console upgraded to graphics"
echo ""
echo "If framebuffer fails:"
echo "  fbconsole: no saved framebuffer info, using UART console"
echo "  Console still works via UART!"