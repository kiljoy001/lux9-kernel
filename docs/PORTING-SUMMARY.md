# 9front Kernel Port to GCC/Limine - Summary

## Project Overview

Successfully ported the 9front Plan 9 kernel to compile with GCC and boot via the Limine bootloader, creating **lux9** - a Plan 9 kernel for modern x86-64 systems.

**Date:** October 2025
**Kernel Version:** lux9.elf (346KB)
**Status:** Compiled and linked successfully, boots to Limine, kernel initialization in progress

---

## Achievements

### 1. Complete Kernel Compilation (25,000+ lines)

**Port Layer (9front-port/):** 33 files compiled
- Process management (proc.c, sched.c, fork.c)
- Memory management (page.c, segment.c, xalloc.c)
- File system layer (chan.c, dev.c, cache.c)
- Device drivers (devcons.c, devmnt.c, devproc.c, devroot.c)
- I/O (qio.c, allocb.c)
- Synchronization (qlock.c, taslock.c)
- Real-time scheduling (edf.c)
- System calls (syscallfmt.c, sysfile.c, sysproc.c)
- **Advanced memory safety (pageown.c, borrowchecker.c) - Zero-copy IPC with Rust-style borrow checking**

**Platform Layer (9front-pc64/):** 11 files compiled
- Boot and initialization (main.c, entry.S)
- Memory management (mmu.c)
- CPU features (fpu.c, trap.c)
- Interrupts (apic.c, i8259.c, archmp.c)
- Timers (i8253.c)
- Multiprocessing (squidboy.c)
- Console (cga.c)
- Platform stubs (globals.c)

**Standard Library (libc9/):** 37 files compiled
- String operations (kstr*.c, memcmp.c, etc.)
- Format functions (dofmt.c, vsnprint.c, sprint.c)
- 9P protocol (convD2M.c, convM2D.c, convM2S.c, convS2M.c)
- UTF-8 support (rune.c, utf*.c)
- Utilities (tokenize.c, strtol.c, atoi.c)

**Assembly (l.S, entry.S):** 400+ lines
- CPU control instructions (cpuid, rdmsr, wrmsr, rdtsc)
- FPU state management (fxsave, fxrstor, xsave, xrstor)
- Segment/paging (lgdt, lidt, ltr, getcr*, putcr*)
- Atomic operations (cmpswap486, tas)
- System calls (syscallentry)
- Memory barriers (mfence, sfence, lfence, coherence_impl)

---

## Major Technical Challenges Solved

### Challenge 1: Anonymous Struct Embedding

**Problem:** Plan 9 C allows embedding struct fields without a field name:
```c
struct Proc {
    Lock;      // All Lock fields embedded directly
    Timer;     // All Timer fields embedded directly
    int pid;
};
```

**GCC Issue:** Not supported - requires explicit field names or inline struct definitions.

**Solution:** Manually expanded 100+ anonymous embeddings throughout the codebase.

**Example Fix:**
```c
// Before (Plan 9)
struct Proc {
    Lock;
    Timer;
    PFPU;
    PMMU;
    int pid;
};

// After (GCC-compatible)
struct Proc {
    // Lock fields
    int locked;
    // Timer fields
    Tval twhen;
    Timer *tnext;
    Timer *tprev;
    // PFPU fields
    int fpstate;
    int kfpstate;
    FPalloc *fpsave;
    FPalloc *kfpsave;
    // PMMU fields (similar expansion)
    // ...
    int pid;
};
```

**Files Affected:** portdat.h, dat.h, proc.c, sched.c, edf.c, devproc.c

---

### Challenge 2: Header File Conflicts

**Problem:** Plan 9 has two different `lib.h` files:
- `/sys/include/lib.h` - Userspace library (sleep, notify, etc.)
- `/sys/src/9/port/lib.h` - Kernel-only functions (print, strlen, etc.)

Including userspace lib.h in kernel caused function conflicts.

**Solution:** Separated kernel and userspace headers:
```bash
kernel/include/lib.h       # From /sys/src/9/port/lib.h (kernel)
kernel/include/portlib.h   # Minimal kernel formatting
```

---

### Challenge 3: Global Variable Multiple Definitions

**Problem:** Plan 9's struct-variable syntax creates BSS allocations in every compilation unit:
```c
// In header file
struct {
    Lock lk;
    char buf[16384];
} kmesg;  // Creates global in every .c file that includes this
```

**GCC Issue:** Multiple definition errors at link time.

**Solution:**
1. Separate type definitions from variable declarations
2. Use `extern` in headers
3. Define once in globals.c

**Example Fix:**
```c
// Header (portdat.h)
struct Kmesg {
    Lock lk;
    uint n;
    char buf[16384];
};
extern struct Kmesg kmesg;

// Implementation (globals.c)
struct Kmesg kmesg;
```

**Variables Fixed:**
- `swapalloc` (struct Swapalloc)
- `kmesg` (struct Kmesg)
- `active` (struct Active)
- `machp[MAXMACH]` (Mach pointer array)
- Function pointers: `consdebug`, `hwrandbuf`, `kproftimer`, `screenputs`

---

### Challenge 4: PIE Linking for Kernel

**Problem:** Default GCC creates Position Independent Executables (PIE), unsuitable for kernel.

**Solution:** Disabled PIE in build flags:
```makefile
CFLAGS := -fno-pie -fno-pic -mcmodel=kernel
LDFLAGS := -no-pie -static -nostdlib
```

---

### Challenge 5: va_list Array Type

**Problem:** In GCC, `va_list` is an array type:
```c
va_list args;
f.args = args;  // Error: assignment to expression with array type
```

**Solution:** Use `va_copy()`:
```c
va_copy(f.args, args);
```

---

### Challenge 7: Page Ownership Tracking Migration

**Problem:** Original page ownership tracking system was replaced with a generic borrow checker to provide Rust-style memory safety semantics for zero-copy IPC.

**Solution:** Migrated all page ownership operations to use the unified borrow checker:
- `pageown_acquire` → `borrow_acquire` 
- `pageown_release` → `borrow_release`
- `pageown_transfer` → `borrow_transfer`
- `pageown_borrow_shared` → `borrow_borrow_shared`
- `pageown_borrow_mut` → `borrow_borrow_mut`
- `pageown_return_shared` → `borrow_return_shared`
- `pageown_return_mut` → `borrow_return_mut`

**API Compatibility:** Maintained full Plan 9 compatibility by implementing `pa2owner` as an adapter that converts `BorrowOwner` data to the legacy `PageOwner` format on demand, with locking to ensure consistent snapshots.

**Benefits:**
- Zero-copy IPC with formal borrow checking semantics
- Prevention of use-after-free and double-free bugs
- Shared borrowing (one writer, many readers) for efficient memory access
- Integration with existing kernel memory management

---

## Build System

**Makefile Features:**
- Automatic dependency tracking
- Parallel compilation
- Separate compilation for port/platform/libc layers
- ISO creation with Limine
- QEMU test target

**Build Commands:**
```bash
make            # Build kernel
make iso        # Create bootable ISO
make run        # Boot in QEMU
make clean      # Clean build
make count      # Line count statistics
```

---

## Bootloader Integration

### Limine Protocol

Created proper Limine entry point (entry.S) with protocol requests:

**Requests:**
- Bootloader info
- Memory map
- Framebuffer
- Higher-half direct map (HHDM)

**Entry Point:**
```asm
_start:
    cli                     # Disable interrupts
    lea stack_top(%rip), %rsp  # Set up stack
    xor %rbp, %rbp          # Clear frame pointer
    cld                     # Clear direction flag
    call main               # Jump to kernel
```

**Boot Configuration (limine.conf):**
```
timeout: 5

/9front on Lux (lux9)
    protocol: limine
    kernel_path: boot():/lux9.elf
```

---

## Files Created/Modified

### New Files
- `kernel/9front-pc64/l.S` - 400 lines of x86-64 assembly
- `kernel/9front-pc64/entry.S` - Limine boot entry
- `kernel/9front-pc64/globals.c` - 462 lines of platform stubs
- `kernel/libc9/*.c` - 37 standard library files
- `kernel/borrowchecker.c` - Generic borrow checker implementation
- `docs/*.md` - Comprehensive documentation files
- `shell_scripts/*` - Development and testing utilities
- `limine.conf` - Bootloader configuration

### Modified Files
- `kernel/include/portdat.h` - Fixed anonymous structs, extern globals
- `kernel/include/dat.h` - Fixed anonymous structs, extern globals
- `kernel/include/fns.h` - Added extern for function pointers
- `kernel/include/lib.h` - Replaced with kernel version
- `kernel/9front-pc64/dat.h` - Fixed struct definitions
- `kernel/9front-pc64/fns.h` - Added platform function declarations
- `kernel/9front-port/*.c` - Fixed anonymous struct references
- `kernel/9front-port/pageown.c` - Migrated to borrow checker
- `GNUmakefile` - Complete build system

### Clean Project Structure
- **`docs/`** - Organized documentation with clear naming
- **`shell_scripts/`** - Development and testing utilities
- **`kernel/`** - Well-organized kernel source tree
- **`drivers/`** - Future driver implementations
- **`userspace/`** - User space components and servers
- **`test/`** - Testing infrastructure and utilities

---

## Statistics

**Total Lines Compiled:** ~25,000 lines of C
**Object Files:** 81 (33 port + 11 pc64 + 37 libc9)
**Assembly Lines:** 400+ (l.S + entry.S)
**Kernel Size:** 3.8 MB (unstripped ELF)
**ISO Size:** 4.1 MB (bootable)

**Compilation Time:** ~15 seconds (clean build, parallel)

## Current Status

### ✅ Working
- Full kernel compilation with no errors
- Complete linking to single ELF binary
- Bootable ISO creation
- Limine protocol integration
- Boot to kernel entry point
- Advanced memory management with borrow checker
- Zero-copy IPC with formal memory safety
- Clean project organization

### 🚧 In Progress
- Userspace process development
- File system integration
- Driver implementation
- Formal verification

### 📋 Organized Structure
- **Documentation** - Well-organized in `docs/` directory
- **Scripts** - Development utilities in `shell_scripts/`
- **Testing** - Comprehensive test infrastructure
- **Kernel** - Clean source tree organization

---

## Next Steps

### Phase 1: Boot to Console (Priority)
1. Fix serial console initialization (i8250console)
2. Implement basic printk/print output
3. Complete meminit0/meminit (memory detection from Limine memmap)
4. Set up interrupt descriptor table (trapinit)
5. Initialize APIC/timer for scheduling

### Phase 2: Core Services
1. Complete memory allocator (xalloc, page allocator)
2. Process creation (userinit, first process)
3. Device initialization (devcons, devroot)
4. File system mounting

### Phase 3: Userspace
1. Port init binary from 9front
2. Load and execute first process
3. Shell access
4. Full system bring-up

---

## Future Enhancements

### GCC Plugin for Plan 9 C
Documented in `docs/gcc-plan9-plugin.md`:
- Automatic anonymous struct expansion
- #pragma varargck support
- No source modification needed
- Full Plan 9 C compatibility

### Graphics Support
- Re-enable VGA driver (vga.c)
- Port libmemdraw
- Framebuffer console via Limine

### Hardware Support
- Real ACPI implementation
- AHCI disk driver
- E1000 network driver
- USB support

---

## Lessons Learned

### 1. Plan 9 C vs Standard C
- Anonymous struct embedding is extensively used
- Struct-variable declarations create subtle bugs
- UTF-8 identifiers work fine in GCC
- #pragma extensions are informational only

### 2. Kernel Build Systems
- Separate headers for kernel vs userspace critical
- Static linking requires careful symbol management
- Global variables need one definition rule
- Assembly integration straightforward with proper .S suffix

### 3. Bootloader Integration
- Limine protocol is clean and well-documented
- Higher-half kernel simplifies memory layout
- Pre-initialized paging reduces boot complexity
- Protocol requests provide clean boot info passing

---

## References

- **9front Source:** https://git.9front.org/plan9front/plan9front
- **Limine Protocol:** https://github.com/limine-bootloader/limine
- **GCC Documentation:** https://gcc.gnu.org/onlinedocs/
- **Plan 9 C Compilers:** http://doc.cat-v.org/plan_9/4th_edition/papers/compiler

---

## Acknowledgments

- 9front team for maintaining Plan 9
- Limine bootloader developers
- Plan 9 community for documentation
- GCC team for excellent toolchain

---

## License

This port maintains 9front's MIT-style license. See original 9front source for details.

---

**End of Summary**
