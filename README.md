# Lux9 Microkernel Operating System

Modern microkernel OS combining Plan 9 architecture with formal verification and multi-language safety.

## Overview

Lux9 is a microkernel operating system that extracts the core 9front kernel (15K LOC) and moves device drivers to userspace for isolation and safety.

**Key features:**
- Formal verification (1,175 lines Coq proofs)
- Memory safety (Go/Rust userspace, C only where necessary)
- Driver compatibility (virtio, NetBSD rump, Linux compat)
- Protocol translation (9P.e bridges 9P2000/u/L dialects)

## Current Status

**Kernel:** ✅ Complete (25K LOC ported, bootable)  
**Proofs:** 🚧 In progress (1,175 LOC Coq)  
**Userspace:** 🚧 Partial (Go servers, ext4fs)  
**Drivers:** 📋 Design complete, implementation pending

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

See `docs/` for detailed documentation.

## License

9front MIT-style license. See individual components for details.
