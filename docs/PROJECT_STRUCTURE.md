# Lux9 Project Structure

## Overview

The Lux9 project follows a clean, organized directory structure that separates concerns and makes navigation easy for developers.

## Directory Layout

```
lux9-kernel/
├── kernel/                 # Kernel source code
│   ├── 9front-port/       # Ported 9front kernel components
│   ├── 9front-pc64/       # Platform-specific code for x86_64
│   ├── include/           # Kernel header files
│   ├── libc9/             # Plan 9 standard library
│   └── borrowchecker.c    # Generic borrow checker implementation
├── docs/                  # Comprehensive documentation
├── shell_scripts/         # Development and testing utilities
├── drivers/               # Future driver implementations
├── userspace/             # User space components and servers
├── test/                  # Testing infrastructure
├── initrd/                # Initial ramdisk contents
├── gcc-plan9-plugin/      # GCC plugin for Plan 9 C compatibility
├── proofs/                # Formal verification (Coq proofs)
└── tmp/                   # Temporary files
```

## Kernel Directory

### 9front-port/
Contains the ported 9front kernel components:
- Process management (proc.c, sched.c, fork.c)
- Memory management (page.c, segment.c, xalloc.c, pageown.c)
- File system layer (chan.c, dev.c, cache.c)
- Device drivers (devcons.c, devmnt.c, devproc.c, devroot.c)
- I/O (qio.c, allocb.c)
- Synchronization (qlock.c, taslock.c)
- Real-time scheduling (edf.c)
- System calls (syscallfmt.c, sysfile.c, sysproc.c, syspageown.c)

### 9front-pc64/
Platform-specific code for x86_64 architecture:
- Boot and initialization (main.c, entry.S)
- Memory management (mmu.c, mmu_early.c)
- CPU features (fpu.c, trap.c, apic.c)
- Interrupts (i8259.c, archmp.c)
- Timers (i8253.c)
- Multiprocessing (squidboy.c)
- Console (cga.c, uart.c)
- Platform stubs (globals.c)

### include/
Kernel header files with clean separation:
- Core kernel interfaces (u.h, portlib.h, mem.h, dat.h, fns.h)
- Device interfaces (io.h, pci.h, sdhw.h)
- Memory safety (borrowchecker.h, pageown.h)
- File system (chan.h, dev.h, cache.h)
- Synchronization (qlock.h, lock.h)
- Utilities (libc.h, lib.h, error.h)

### libc9/
Plan 9 standard library implementation:
- String operations (kstr*.c, memcmp.c, etc.)
- Format functions (dofmt.c, vsnprint.c, sprint.c)
- 9P protocol (convD2M.c, convM2D.c, convM2S.c, convS2M.c)
- UTF-8 support (rune.c, utf*.c)
- Utilities (tokenize.c, strtol.c, atoi.c)

## Documentation Directory

Organized documentation files:
- **Technical Documentation**: PORTING-SUMMARY.md, PAGEOWN_BORROW_CHECKER.md
- **Project Vision**: SYSTEM_VISION.md, MILESTONE_ACHIEVED.md
- **Implementation Details**: MEMORY_SAFETY_MILESTONE.md, various component docs
- **Testing**: 9P_TESTING_*.md, TESTING_README.md
- **Development Guides**: PLAN9_C_GUIDE.md, DEBUG_*.md

## Shell Scripts Directory

Development and testing utilities:
- **Debugging**: debug.sh, debug_kernel.sh, mmu_debug.sh, quick_debug.sh
- **Testing**: test_9p_*.sh, test_*.sh
- **Running**: run_debug.sh, run_qemu_gdb.sh
- **Utilities**: Various helper scripts for development workflow

## Userspace Directory

User space components and servers:
- **Go servers**: ext4fs, memfs, and other file system servers
- **Testing utilities**: exchange_test, exchange_9p_test
- **Init utilities**: Basic user space initialization
- **Documentation**: Userspace-specific documentation

## Test Directory

Comprehensive testing infrastructure:
- **Test binaries**: Compiled test programs
- **Test data**: Sample files and test cases
- **Integration tests**: Multi-component test scenarios

## Build System

The project uses a clean GNU Make-based build system:
- **GNUmakefile**: Main build configuration
- **Parallel compilation**: Efficient multi-core builds
- **Dependency tracking**: Automatic header dependency management
- **Target organization**: Clear separation of kernel, ISO, and test targets

## Key Benefits of Clean Structure

### 1. Separation of Concerns
Each directory has a clear purpose, making it easy to locate specific components.

### 2. Easy Navigation
Consistent naming and organization reduce cognitive load for developers.

### 3. Scalability
Structure supports growth with new components and features.

### 4. Maintainability
Clean organization makes refactoring and updates straightforward.

### 5. Collaboration
Well-defined structure helps multiple developers work effectively.

## Future Expansion

The clean structure supports easy addition of new components:
- **New drivers**: Can be added to drivers/ directory
- **Additional architectures**: New platform directories under kernel/
- **More documentation**: Clear docs/ organization
- **Extended testing**: Comprehensive test/ infrastructure
- **Formal verification**: Dedicated proofs/ directory

This organized approach ensures the Lux9 project remains maintainable and extensible as it grows.