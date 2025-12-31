# Lux9 Kernel Verification Status (Dec 2025)

## Executive Summary

Based on an analysis of the codebase, approximately **26.5%** of the kernel source code is formally verified using ACSL annotations and Coq proofs. The remaining **73.5%** comprises unverified platform code, legacy drivers, external runtimes (WASM), and standard library functions.

| Category | Lines of Code (LOC) | Percentage |
| :--- | :--- | :--- |
| **Verified** | ~17,686 | **26.5%** |
| **Unverified** | ~49,074 | **73.5%** |
| **Total** | ~66,760 | 100% |

## Verified Subsystems (High Assurance)

These components contain explicit ACSL contracts (`/*@ ... */`) and link to `proofs/` directory artifacts.

1.  **Core Kernel Logic**
    *   **Process Management**: `proc.c`, `proc_fsm.c` (Scheduling logic, State transitions)
    *   **Memory Allocator**: `xalloc.c`, `pebble.c` (Token invariants, conservation laws)
    *   **IPC**: `9p_router.c`, `msgord.c` (Message ordering, routing safety)
    *   **Scheduler**: `edf.c` (Earliest Deadline First policy)

2.  **Security Critical**
    *   **Kinetic Defense**: `pow_gate.c` (Cost/Benefit analysis)
    *   **Capabilities**: `blind_ledger.c`, `borrowchecker.c` (Ownership, Minting/Burning)
    *   **Consensus**: `consensus_depth.c`, `lock_dag.c`

3.  **Key Devices**
    *   **Filesystems**: `devmnt.c` (Mount), `devram.c` (Ramdisk), `devpipe.c`, `cache.c`

## Unverified Subsystems (Legacy / Hardware / External)

These components rely on traditional testing and manual review.

1.  **Platform Hardware Layer (`kernel/9front-pc64/`)**
    *   Boot code, Interrupt handling, APIC/IOAPIC, ACPI, low-level assembly (`entry.S`).
    *   *Reason*: Hardware-specific behaviors are difficult to model formally.

2.  **Device Families (`kernel/family/`)**
    *   PCI enumeration, device abstractions.
    *   *Status*: Architecture is sound, but implementation lacks proofs.

3.  **WASM Runtime (`kernel/wasm/`)**
    *   `wasm3` interpreter engine and integration bindings.
    *   *Risk*: Relies on the correctness of the external `wasm3` C codebase.

4.  **Standard Library (`kernel/libc9/`)**
    *   String manipulation (`strcpy`, `memset`), formatting.
    *   *Note*: Generally considered low-risk but technically unverified.

5.  **Syscall Handlers**
    *   `sysproc.c`, `sysfile.c` (Implementation details of syscalls).
    *   *Mitigation*: The logic *behind* them (processes, files) is largely verified in `proc.c` and `9p_router.c`.

## Roadmap to Higher Assurance

To increase the verified percentage:
1.  **Verify Family HAL**: Add ACSL contracts to `kernel/family/family.c`.
2.  **Verify Syscall Interface**: Formalize `sysproc.c`.
3.  **Sandboxing**: Isolate the unverified WASM runtime using the verified `borrowchecker` (in progress).
