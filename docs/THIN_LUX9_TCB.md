# Thin-Lux9 Core TCB (The 15k Target)

The following files constitute the minimal "Root of Trust" for the Lux9 kernel. These will be target for seL4-level Refinement Proofs.

## 1. Minimal Execution (TCB-EXEC)
- `kernel/9front-port/kthread.c`: [NEW] Minimal execution context and stack management.
- `kernel/9front-port/sched_verified.c`: [REDUCED] Minimal tick-based dispatcher (Refined).

## 2. Memory Management (TCB-MEM)
- `kernel/9front-port/alloc.c`: Basic kernel allocation.
- `kernel/9front-port/xalloc.c`: The verified allocator.
- `kernel/9front-port/page.c`: Physical page management.
- `kernel/9front-port/segment.c`: Virtual memory segments.
- `kernel/9front-port/fault.c`: Page fault handling.

## 3. Communication & Routing (TCB-IPC)
- `kernel/9front-port/chan.c`: The core 9P channel abstraction.
- `kernel/9front-port/sysfile.c`: 9P-to-System Call mapping.
- `kernel/9front-port/devmnt.c`: 9P mounting logic.
- `kernel/wasm/wasm_9p_integration.c`: The security bridge to WASM.

## 4. Fundamental Types & Utils (TCB-CORE)
- `kernel/include/dat.h`: Core data structures.
- `kernel/9front-port/rbtree.c`: Foundation for all kernel registries.
- `kernel/capability/lux_capability.c`: The capability security engine.

---

### Migration Targets (Non-TCB)
*Everything not listed above* is considered non-essential and will be moved to isolated WASM modules:
- **Process Management** (`proc.c`, `sysproc.c`, `pgrp.c`): Logic for `fork`/`exec`/PID moved to userspace.
- **Legacy Drivers** (`devsd.c`, `sdscsi.c`, `ahci.c`): Moved to userspace HAL.
- **Hardware Probing** (`pci.c`, `pciframework.c`): Moved to userspace HAL.
- **Security Extras** (`devtpm.c`, `blind_ledger.c`): Moved to isolation vaults.
- **Network & Console** (`devcons.c`, `devpipe.c`): Moved to userspace servers.
