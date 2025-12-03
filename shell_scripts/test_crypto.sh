#!/bin/bash
# Quick crypto testing script for Lux9 kernel

set -e

KERNEL="lux9.elf"
ISO="lux9.iso"
TIMEOUT=15

# Check if ISO exists, if not try to build it
if [ ! -f "$ISO" ]; then
    echo "ISO not found, attempting to build..."
    if [ ! -f "$KERNEL" ]; then
        echo "Error: $KERNEL not found. Run 'make' first."
        exit 1
    fi
    # Try to build ISO if userspace exists
    if [ -d "userspace/build" ] && [ -f "userspace/build/initrd.tar" ]; then
        make iso || echo "Warning: ISO build failed, trying kernel-only boot"
    else
        echo "Warning: No userspace found, using kernel-only boot method"
        # Use multiboot format with -kernel flag
        USE_KERNEL_BOOT=1
    fi
elif [ ! -f "$KERNEL" ]; then
    echo "Error: $KERNEL not found. Run 'make' first."
    exit 1
fi

if ! command -v qemu-system-x86_64 &> /dev/null; then
    echo "Error: qemu-system-x86_64 not found. Please install QEMU."
    exit 1
fi

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         Lux9 Kernel Hardware Crypto Test Suite            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

run_test() {
    local name="$1"
    local cpu="$2"
    local search="$3"
    local desc="$4"

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Test: $desc"
    echo "CPU:  $cpu"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    local logfile="/tmp/crypto_test_${name}.log"

    if [ -f "$ISO" ]; then
        # Boot from ISO
        timeout ${TIMEOUT}s qemu-system-x86_64 \
            -M q35 \
            -m 2G \
            -cdrom "$ISO" \
            -boot d \
            -no-reboot \
            -display none \
            -serial stdio \
            -cpu "$cpu" > "$logfile" 2>&1 || true
    else
        # Fallback to direct kernel boot (may not work with all QEMU versions)
        timeout ${TIMEOUT}s qemu-system-x86_64 \
            -M q35 \
            -m 2G \
            -kernel "$KERNEL" \
            -no-reboot \
            -display none \
            -serial stdio \
            -cpu "$cpu" > "$logfile" 2>&1 || true
    fi

    echo ""
    echo "Boot log excerpt:"
    echo "─────────────────────────────────────────────────────────────"
    grep -E "(cpuidentify|Crypto:|SHA|AES)" "$logfile" | head -20 || echo "No crypto output found"
    echo "─────────────────────────────────────────────────────────────"
    echo ""

    if grep -q "$search" "$logfile"; then
        echo "✅ PASS: Found expected output '$search'"
    else
        echo "❌ FAIL: Expected output '$search' not found"
        echo "Full log saved to: $logfile"
        return 1
    fi

    echo ""
    return 0
}

# Test Suite
echo ""

run_test "sw_fallback" \
    "qemu64,-sha-ni,-aes,-pclmulqdq" \
    "Using software SHA256" \
    "Software Crypto Fallback (No HW Acceleration)"

run_test "hw_full" \
    "max" \
    "SHA extensions available" \
    "Full Hardware Acceleration (SHA + AES)"

run_test "hw_sha_only" \
    "qemu64,+sha-ni,-aes" \
    "SHA extensions available" \
    "Hardware SHA Only (No AES)"

run_test "hw_aes_only" \
    "qemu64,-sha-ni,+aes" \
    "AES-NI available" \
    "Hardware AES Only (No SHA)"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    Test Summary                            ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║ All tests completed!                                       ║"
echo "║ Detailed logs available in /tmp/crypto_test_*.log         ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "To view detailed logs:"
echo "  cat /tmp/crypto_test_sw_fallback.log"
echo "  cat /tmp/crypto_test_hw_full.log"
echo ""
echo "To run with more options, see: CRYPTO_TESTING_STRATEGY.md"
echo ""
