# Boot FPU/SHA Fix Plan

## Problem Summary
Hardware SHA is being executed before SSE/FPU is initialized. The call path is
`exchange_pool_init()` -> `ledger_mint()` -> `crypto_sha256()` ->
`sha256_transform_hw()` (XMM/SHA instructions). This happens before
`trapinit()` and `fpuinit()`, so the first SHA instruction faults with no
handler, causing a hang/reset.

## Goal
Ensure hardware SHA instructions are only executed after SSE/FPU is fully
initialized and the trap handlers exist, while preserving hardware SHA usage
once the system is ready.

## Required Changes

### 1) Add an explicit FPU/SSE readiness gate
- **Files**:
  - `kernel/9front-pc64/fpu.c`
  - `kernel/include/dat.h` (or another shared header for a boot flag)
  - `kernel/crypto/crypto.c`
- **What**:
  - Add a global flag (e.g. `fpu_ready` or `simd_ready`) set at the end of
    `fpuinit()` after CR4/XCR0 are configured and `_stts()` is called.
  - In `crypto_hw_sha_available()` (or in `crypto_sha256()` directly), require
    `fpu_ready == 1` in addition to `m->havesha`.
- **Why**:
  - Prevents HW SHA execution before SSE/FPU is safe to use.

### 2) Consider moving `fpuinit()` earlier (optional)
- **Files**:
  - `kernel/9front-pc64/main.c`
- **What**:
  - Move `fpuinit()` earlier if you want HW SHA available during
    `exchange_pool_init()`.
- **Constraints**:
  - Must happen after `xinit()` (allocator ready) and before any HW SHA usage.
  - Avoid running before `trapinit()` unless you are comfortable with
    exceptions being fatal during early boot.
- **Note**:
  - This is riskier than gating HW SHA and may not be necessary.

### 3) Add a boot arg override for SHA (optional safety valve)
- **Files**:
  - `kernel/crypto/crypto.c`
  - `kernel/9front-pc64/main.c` (if needed for boot arg propagation)
- **What**:
  - Add a `sha=0` or `nohwsha` boot arg to force software SHA even after FPU
    init.
- **Why**:
  - Lets you disable HW SHA on systems that misreport or have unstable SHA
    instructions.

### 4) Remove temporary debug prints once confirmed
- **Files**:
  - `kernel/crypto/crypto.c`
  - `kernel/9front-port/blind_ledger.c`
  - `kernel/exchange_pool.c`
  - `kernel/9front-port/exchange.c`
  - `kernel/9front-pc64/devarch.c`
- **What**:
  - Remove `uartputs(...)` and `print(...)` added for tracing (tags like
    `ledger_mint`, `exch_pool`, `sha256: hw start`).

## Testing Requirements

### Build the ISO
- **From repo root**:
  - `make iso -C kernel`
- **Expected**:
  - `kernel/lux9.iso` is created successfully.

### A) BIOS QEMU (baseline)
- **Command**:
  - `qemu-system-x86_64 -cdrom kernel/lux9.iso -boot d -M q35 -m 2G -display none -serial file:kernel/qemu.bios.log -no-reboot -no-shutdown`
- **Expected**:
  - Boot progresses past `BOOT_STATE: EXCHANGE` and `BOOT_STATE: USERINIT`.
  - No hangs at `sha256: hw start`.

### B) KVM QEMU (hardware features)
- **Command**:
  - `qemu-system-x86_64 -accel kvm -cpu host -cdrom kernel/lux9.iso -boot d -M q35 -m 2G -display none -serial file:kernel/qemu.kvm.log -no-reboot -no-shutdown`
- **Expected**:
  - Boot progresses with HW SHA enabled only after `fpuinit()`.
  - If `sha=0` boot arg is used, verify software SHA path only.

### C) Regression checks
- Ensure no earlier boot regressions (before `trapinit()`/`fpuinit()`):
  - `BOOT_STATE` sequence advances to `BOOT_STATE: SCHED`.
  - No unexpected traps in the serial log.

## Acceptance Criteria
- System boots past `exchange_pool_init()` without hang in both BIOS and KVM.
- HW SHA is not executed before FPU/SSE readiness.
- No new early-boot traps before `trapinit()`.
