# 9front Microkernel Extraction Status

## Extracted Files (12,425 LOC)

### Port Layer (9front-port/) - 10,567 LOC
✓ `proc.c` (37,954 bytes) - Scheduler & process management
✓ `page.c` (5,983 bytes) - Page management  
✓ `segment.c` (18,629 bytes) - Memory segments
✓ `syscall.c` (145 lines) - Syscall dispatch
✓ `chan.c` (34,950 bytes) - Channel abstraction
✓ `dev.c` (9,106 bytes) - Device table
✓ `devmnt.c` (24,668 bytes) - 9P mount device
✓ `devproc.c` (31,756 bytes) - /proc filesystem
✓ `devroot.c` (4,370 bytes) - Root namespace
✓ `alloc.c` (6,485 bytes) - Memory allocator
✓ `alarm.c` (1,618 bytes) - Timer system
✓ `auth.c` (2,651 bytes) - Authentication

### Platform Layer (9front-pc64/) - 1,858 LOC
✓ `main.c` (6,640 bytes) - Entry point
✓ `mmu.c` (14,089 bytes) - Page tables & MMU
✓ `trap.c` (12,326 bytes) - Interrupts/exceptions/syscall entry
✓ `fpu.c` (9,614 bytes) - FPU/SSE support
✓ `squidboy.c` (2,417 bytes) - SMP CPU startup

### What's Missing
- APIC code (need from /sys/src/9/pc/)
- More headers (dat.h, fns.h, mem.h)
- Assembly files (.S)
- libc9 (Plan 9 C library)

## What We Have vs What We Need

### Already Working ✓
- **Scheduler**: Fair-share algorithm with EDF
- **Memory**: Segment-based management
- **Syscalls**: Plan 9 syscall table
- **9P**: In-kernel mount device
- **Devices**: Channel abstraction

### Needs Adaptation
- **Boot entry** (main.c): Replace Plan 9 boot with Limine
- **Device table** (dev.c): Remove in-kernel drivers
- **Headers**: Need to create compatibility layer

### Move to Userspace
All device drivers will become userspace 9P servers:
- Console → /dev/cons server
- Disk → /dev/sd* servers  
- Network → /net/* servers
- **VMX** → /mnt/vmx server (isolated!)

## Next Steps

1. Copy APIC/SMP code from /sys/src/9/pc/
2. Extract essential headers
3. Create Limine entry point (replace main.c init)
4. Build minimal kernel
5. Port userspace servers

## The Vision

```
9front Microkernel (12K LOC)
├─ Proven scheduler
├─ Proven memory management
├─ Plan 9 syscalls
├─ 9P protocol built-in
└─ Limine boot

    ↓ 9P

Userspace Servers
├─ devcons (console)
├─ devsd (storage)
├─ devnet (networking)
└─ vmx (virtualization) ← ISOLATED!
```

This is **9front made safe** - same great code, microkernel isolation.
