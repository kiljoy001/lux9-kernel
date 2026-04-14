#!/bin/bash
# Setup script for lux9-kernel development environment

echo "=== Setting up lux9-kernel development environment ==="

# Check for required packages
echo "Checking for required tools..."

# Try to find QEMU
if ! command -v qemu-system-x86_64 &> /dev/null; then
    echo "QEMU not found in PATH. Checking local installations..."
    QEMU_PATHS=(
        "/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64"
        "/home/scott/Repo/gnumach/qemu-9.1.0-x64-install/bin/qemu-system-x86_64"
        "/home/scott/Repo/gnumach/qemu-9.1.0/install-root/usr/local/bin/qemu-system-x86_64"
        "/home/scott/Repo/qemu-10.1.0-rc1/build/qemu-system-x86_64"
    )
    
    for path in "${QEMU_PATHS[@]}"; do
        if [ -f "$path" ]; then
            echo "Found QEMU at $path"
            echo "Adding to PATH..."
            export PATH="$(dirname $path):$PATH"
            break
        fi
    done
    
    if ! command -v qemu-system-x86_64 &> /dev/null; then
        echo "Warning: qemu-system-x86_64 not found. Please install QEMU package:"
        echo "  Ubuntu/Debian: sudo apt install qemu-system-x86"
        echo "  Fedora: sudo dnf install qemu-system-x86"
        echo "  Arch: sudo pacman -S qemu-full"
    fi
else
    echo "✓ QEMU found: $(which qemu-system-x86_64)"
fi

# Check for xorriso
if ! command -v xorriso &> /dev/null; then
    echo "Warning: xorriso not found. Required for ISO creation."
    echo "Please install xorriso package:"
    echo "  Ubuntu/Debian: sudo apt install xorriso"
    echo "  Fedora: sudo dnf install xorriso"
    echo "  Arch: sudo pacman -S xorriso"
else
    echo "✓ xorriso found: $(which xorriso)"
fi

# Check for build tools
if ! command -v gcc &> /dev/null; then
    echo "Error: gcc not found. Please install build-essential package."
    exit 1
else
    echo "✓ GCC found: $(gcc --version | head -1)"
fi

if ! command -v make &> /dev/null; then
    echo "Error: make not found. Please install build-essential package."
    exit 1
else
    echo "✓ Make found: $(make --version | head -1)"
fi

if ! command -v ld &> /dev/null; then
    echo "Error: ld not found. Please install binutils package."
    exit 1
else
    echo "✓ Linker found: $(ld --version | head -1)"
fi

# Check for Limine
if [ -d "boot/limine" ] && [ -f "boot/limine/limine-bios.sys" ]; then
    echo "✓ Limine bootloader found in boot/limine/"
else
    echo "Warning: Limine bootloader not found. Building kernel will work, but ISO creation may not be bootable."
    echo "To fix this, run: git submodule update --init --recursive"
fi

echo ""
echo "=== Environment Setup Complete ==="
echo "You can now build the kernel with: make"
echo "Create ISO with: make iso"
echo "Run kernel with: make run"
echo "Run with console output: make direct-run"
