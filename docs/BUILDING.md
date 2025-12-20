# Lux9 Build System

## Overview

Lux9 provides a portable build system that works across different Linux distributions. The build system uses standard GNU make and requires common development tools.

## Prerequisites

### Required Tools

- **GCC** - C compiler (GCC 9+ recommended)
- **GNU Make** - Build system
- **GNU ld** - Linker
- **QEMU** - For testing (qemu-system-x86_64)
- **xorriso** - For ISO creation (optional)

### Installation Commands

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential qemu-system-x86 xorriso
```

**Fedora:**
```bash
sudo dnf install @development-tools qemu-system-x86 xorriso
```

**Arch Linux:**
```bash
sudo pacman -S base-devel qemu-full xorriso
```

## Quick Start

```bash
# Clone repository
git clone https://github.com/kiljoy001/lux9-kernel.git
cd lux9-kernel

# Initialize submodules (including Limine bootloader)
git submodule update --init --recursive

# Build kernel
make

# Create bootable ISO
make iso

# Run in QEMU
make run

# Run with console output
make direct-run
```

## Build System Details

### Main Build Targets

- `make` - Build the kernel (lux9.elf)
- `make clean` - Clean build artifacts
- `make iso` - Create bootable ISO image
- `make run` - Run kernel in QEMU (output to file)
- `make direct-run` - Run kernel in QEMU (console output)
- `make count` - Count lines of code
- `make list` - List source files
- `make test-build` - Test compilation

### Environment Setup

Run the setup script to verify your environment:

```bash
./scripts/setup.sh
```

This script checks for required tools and sets up PATH variables.

## Kernel Architecture

The kernel is built as a freestanding x86_64 ELF executable with the following characteristics:

- **Architecture:** x86_64
- **ABI:** Custom kernel ABI (Plan 9 compatible)
- **Memory Model:** Higher Half Direct Mapping (HHDM)
- **No:** Standard library, exceptions, RTTI, or dynamic linking

## Directory Structure

```
lux9-kernel/
├── kernel/              # Kernel source code
│   ├── 9front-port/     # 9front compatibility layer
│   ├── 9front-pc64/     # x86_64 platform code
│   ├── libc9/           # Minimal C library
│   └── libmemdraw/      # Graphics library
├── port/                # Shared headers
├── real_drivers/        # Kernel-space drivers
├── userspace/           # Userspace components
├── boot/                # Bootloader (Limine)
├── docs/                # Documentation
└── scripts/             # Build and development scripts
```

## Build Configuration

The build system is configured through `GNUmakefile` with the following key settings:

### Compiler Flags
- **Standard:** GNU11 C standard
- **Optimization:** -O0 (debug build)
- **Debugging:** Full debug symbols (-g3)
- **Architecture:** x86_64 freestanding
- **Warnings:** Strict warning levels

### Linker Configuration
- **Script:** kernel/linker.ld
- **Memory Layout:** Higher half kernel (0xffffffff80110000)
- **No:** Standard libraries or dynamic linking

## ISO Creation

The ISO creation process:

1. Builds the kernel
2. Creates initrd from userspace components
3. Uses Limine bootloader for boot support
4. Creates hybrid BIOS/UEFI bootable ISO

Requirements for ISO creation:
- xorriso (for ISO filesystem creation)
- Limine bootloader files
- QEMU for testing

## Testing and Development

### QEMU Integration

The build system includes QEMU integration for easy testing:

```bash
make run          # Run with output to qemu.log
make direct-run   # Run with console output
```

### Debugging

GDB debugging scripts are available in `gdb_scripts/`:

```bash
# Debug with GDB
gdb lux9.elf -x gdb_scripts/gdb_memory.gdb
```

## Troubleshooting

### Common Issues

1. **Missing tools:** Run `./scripts/setup.sh` to check environment
2. **QEMU not found:** Install qemu-system-x86 package
3. **xorriso missing:** Install xorriso for ISO creation
4. **Limine issues:** RunLet me create a new BUILDING.md file with comprehensive build instructions:

<execute_command>
<command>echo '# Building Lux9

## Prerequisites

### Required Tools
- GCC (for kernel compilation)
- GNU Make
- GNU ld (linker)
- QEMU (for testing - optional but recommended)
- xorriso (for ISO creation - optional)

### Optional Tools
- clang (for Limine bootloader)
- GDB (for debugging)

## Quick Start

### Build the kernel:
```bash
make
```

### Run in QEMU:
```bash
make run
```

### Create bootable ISO:
```bash
make iso
```

## Setup Script

Run the setup script to check your environment:
```bash
./scripts/setup.sh
```

This script will:
- Check for required build tools
- Locate QEMU installations
- Verify Limine bootloader availability
- Provide installation hints for missing tools

## Build System Overview

The build system uses GNU Make with the following key targets:

- `make` - Build the kernel (lux9.elf)
- `make clean` - Remove build artifacts
- `make iso` - Create bootable ISO image
- `make run` - Run kernel in QEMU
- `make direct-run` - Run kernel with console output
- `make count` - Count lines of code
- `make list` - List source files

## Kernel Build Process

The kernel is built as a freestanding x86_64 ELF executable:
- No standard library dependencies
- Custom libc implementation in `kernel/libc9/`
- Plan 9 compatible kernel interfaces
- Memory safety through borrow checker

## ISO Creation

The ISO creation process:
1. Builds the kernel
2. Creates initrd from userspace files
3. Uses Limine bootloader (if available)
4. Generates bootable ISO with xorriso

## Portable Build System

This build system is designed to be portable:
- Uses standard GNU tools (gcc, make, ld)
- Auto-detects QEMU installations
- Flexible Limine handling
- Clear error messages for missing tools
- Cross-platform compatible Makefiles

## Troubleshooting

### Missing QEMU
If QEMU is not found, the setup script will look for local installations in:
- `/home/scott/Repo/gnumach/qemu-9.1.0/build/`
- `/home/scott/Repo/gnumach/qemu-9.1.0-x64-install/bin/`
- `/home/scott/Repo/gnumach/qemu-9.1.0/install-root/usr/local/bin/`
- `/home/scott/Repo/qemu-10.1.0-rc1/build/`

### Missing xorriso
For ISO creation, install xorriso:
```bash
# Ubuntu/Debian
sudo apt install xorriso

# Fedora
sudo dnf install xorriso

# Arch
sudo pacman -S xorriso
```

### Missing Limine
The Limine bootloader is included as a git submodule:
```bash
git submodule update --init --recursive
```

## Development Workflow

1. Modify source files
2. `make` to rebuild kernel
3. `make run` to test changes
4. `make iso` to create distributable image

The build system automatically detects changes and rebuilds only modified files.' > BUILDING.md</command>
<requires_approval>false</requires_approval>
</execute_command>
