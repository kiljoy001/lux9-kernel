# Lux9 Microkernel Operating System

Modern microkernel OS combining Plan 9 architecture with formal verification and multi-language safety.

## Overview

Lux9 is a microkernel operating system that extracts the core 9front kernel (15K LOC) and moves device drivers to userspace for isolation and safety.

**Key features:**
- Formal verification (1,175 lines Coq proofs)
- Memory safety (Go/Rust userspace, C only where necessary)
- Driver compatibility (virtio, NetBSD rump, Linux compat)
- Protocol translation (9P.e bridges 9P2000/u/L dialects)
- Zero-copy IPC with Rust-style borrow checker semantics

## Current Status

**Kernel:** ✅ Complete (25K LOC ported, bootable)  
**Proofs:** 🚧 In progress (1,175 LOC Coq)  
**Userspace:** 🚧 Partial (Go servers, ext4fs)  
**Drivers:** 📋 Design complete, implementation pending
**Memory Management:** ✅ Advanced page ownership tracking with borrow checker

## Quick Start

```bash
make          # Build kernel
make iso      # Create bootable ISO
make run      # Test in QEMU
cd proofs && make  # Verify proofs (requires Coq)
```

## Architecture

```
Kernel (C, 15K LOC) → 9pe-translator (Rust) → Servers (Go/C)
```

## Recent Improvements

- Completed migration of page ownership tracking to generic borrow checker
- Maintained full Plan 9 API compatibility while implementing modern memory safety
- Zero-copy IPC with formal borrow checking semantics
- Clean project structure with organized documentation and scripts

## Documentation

Detailed documentation available in `docs/`:
- `docs/PORTING-SUMMARY.md` - Complete porting process documentation
- `docs/PAGEOWN_BORROW_CHECKER.md` - Memory safety implementation details
- `docs/MEMORY_SAFETY_MILESTONE.md` - Recent borrow checker integration summary
- `docs/SYSTEM_VISION.md` - Overall project vision and roadmap
- `docs/PROJECT_STRUCTURE.md` - Clean directory organization guide

## Scripts

Development and testing scripts in `shell_scripts/`:
- Debugging utilities (`debug.sh`, `debug_kernel.sh`)
- Testing frameworks (`test_9p_*.sh`)
- QEMU runners (`run_debug.sh`, `run_qemu_gdb.sh`)

## License

9front MIT-style license. See individual components for details.
