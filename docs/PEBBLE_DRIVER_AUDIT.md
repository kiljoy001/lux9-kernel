# Pebble Audit for Kernel-Resident Allocations

## Goal
Track long-lived ring-0 allocations with Pebble instead of leaving them on the
generic raw allocator.

This audit is about **kernel-resident objects**: structures that remain owned by
the kernel after the creating call returns. It is not a blanket requirement that
every temporary syscall scratch buffer be charged to the kernel Pebble state.

## Implemented

### Allocator surface
- `kernel/9front-port/alloc.c`
  - Added `xalloc_resident()`, `smalloc_resident()`, and `xfree_resident()`.
  - Kept `xalloc_driver()` / `xfree_driver()` for process-charged ring-0 work.
  - Pre-proc bootstrap resident allocations still fall back to `xallocz_raw()`
    on purpose; Pebble metadata is not reliable that early.

### Pebble state-aware helpers
- `kernel/pebble.c`
  - Added `pebble_kernel_state()`.
  - Added `pebble_black_alloc_in_state()` and
    `pebble_black_free_in_state()` so resident allocations can use a kernel
    Pebble state instead of the current process state.
  - Added `pebble_white_verify_in_state()` so WHITE token verification uses the
    same Pebble state that issued the token.
  - Kernel Pebble state is lazily initialized on first tracked resident use.

### Converted resident call sites
- `kernel/9front-pc64/irq.c`
  - `Vctl` allocations now use `xalloc_resident()` / `xfree_resident()`.
- `kernel/9front-pc64/archacpi.c`
  - `Bus`, `PCMPintr`, `Aintr`, `Apic`, and AML heap allocations now use the
    resident allocator.
- `kernel/9front-port/devregistry.c`
  - `Device` registry nodes now use the resident allocator.
- `kernel/9front-port/pci_stubs.c`
  - PCI device records, sizing tables, and the framework device array now use
    the resident allocator.
- `kernel/9front-port/devdma.c`
  - `DMAAlloc` tracking nodes now use `smalloc_resident()`.
- `kernel/9front-port/vault_syscalls.c`
  - `ProcessVault` objects now use `xalloc_resident()` / `xfree_resident()`.
- `kernel/9front-port/kernel_stubs.c`
  - Vault cleanup now frees resident `ProcessVault` objects correctly.
  - Argon2 scratch memory now uses `xalloc_driver()` / `xfree_driver()` rather
    than bypassing Pebble entirely.

## Deliberate boundary

### In scope
- Kernel-owned registries
- Driver/framework tables that outlive the creating call
- Shared kernel bookkeeping objects
- Kernel-only vault metadata

### Out of scope for this pass
- Temporary syscall response buffers
- Short-lived per-request scratch allocations
- Pure bootstrap allocations created before proc0 exists

Those remaining cases should be handled separately if the kernel later wants
full transient accounting, but they are not the same problem as long-lived
resident ownership.

## Verification
- Focused object rebuild:
  - `kernel/9front-port/alloc.o`
  - `kernel/pebble.o`
  - `kernel/9front-port/devregistry.o`
  - `kernel/9front-port/pci_stubs.o`
  - `kernel/9front-pc64/irq.o`
  - `kernel/9front-pc64/archacpi.o`
  - `kernel/9front-port/kernel_stubs.o`
  - `kernel/9front-port/vault_syscalls.o`
  - `kernel/9front-port/devdma.o`
- Full rebuild:
  - `make -f GNUmakefile iso`
- Full QEMU ISO boot:
  - `build/qemu-pebble-resident.log`
  - Reached `BOOT[proc0]: root namespace setup complete`
  - Reached `RESURRECTION: Started procd`
  - Reached `RESURRECTION: Started hal`
  - Reached `RESURRECTION: Started sophia`
  - Reached `RESURRECTION: Entering dynamic monitoring loop`
  - No `panic`, `PANIC`, `kernel fault`, `general protection`,
    `errstack underflow`, or `D2B: magic bad`
