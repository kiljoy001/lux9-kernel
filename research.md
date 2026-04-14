# Boot Failure Research (QEMU + Limine)

## Environment

- Target environment: QEMU with Limine bootloader (ISO boot).
- Build commands: `make clean`, `make`, `make iso`.
- Repo file count: 11983 files found via `find . -type f`.

## Boot Log Capture (for analytical analysis)

### Build

```bash
make clean
make
make iso
```

### Boot + record log

Use ISO boot to exercise Limine (not `-kernel`). Example from docs:

```bash
qemu-system-x86_64 \
  -cpu max \
  -cdrom lux9.iso -boot d \
  -M q35 -m 2G \
  -display none -serial stdio
```

To record the boot log to a file, redirect serial to a file (example path):

```bash
qemu-system-x86_64 \
  -cpu max \
  -cdrom lux9.iso -boot d \
  -M q35 -m 2G \
  -display none -serial file:kernel/qemu.log
```

Notes:

- `GNUmakefile` provides `make run`, but it boots with `-kernel` and bypasses Limine.
- `make run` logs to `qemu.log` in the repo root (not Limine/ISO).

## Evidence From Logs

### QEMU log (ISO boot)

From `kernel/qemu.log`:

- `suicide: sys: trap: general protection violation pc=0x200009`
- `PANIC: boot process died: sys: trap: general protection violation pc=0x200009`

### Boot check debug log

From `kernel/boot_check_debug.txt`:

- Repeated failures: `fixfault: pageown_acquire failed for stack/bss pa=0x331af000` (and many following pages).
- Final panic: `PANIC: freepages: release failed pa=0x331af000 ... OwnerPID: -1, CurrentPID: 2.`

### Boot check final log

- No `PANIC`/`freepages` hits found via `rg --text -n` in `kernel/boot_check_final.txt`.
- Treat this log as incomplete or produced by a different run until verified.

## Files Involved (boot, faults, ownership, build)

### Build/boot orchestration

- `GNUmakefile` (ISO creation, run targets, QEMU log location).
- `docs/reference/CRYPTO_QUICK_REFERENCE.md` (ISO boot commands).
- `docs/guides/DEBUG_HHDM_MMU.md` (build + ISO boot instructions).

### Boot and process initialization

- `kernel/9front-pc64/boot.c` (early boot, Limine handoff; not re-read in this pass).
- `kernel/9front-pc64/main.c` (boot sequencing; not re-read in this pass).
- `kernel/9front-port/userinit.c` (proc0 setup, P9SEG stub, mmuswitch, init0).
- `kernel/9front-port/sysproc.c` (sysrfork uses P9SEG stub and exchange ownership).

### Fault handling and lazy exchange mapping

- `kernel/9front-port/fault.c` (stack/bss faults, `mapphys` lazy allocation, exchange page ownership acquisition).
- `kernel/9front-port/segment.c` (segment freeing path calls `freepages`).

### Page ownership and allocator paths

- `kernel/9front-port/page.c` (`newpage`, `freepages`, ownership release, token tracking).
- `kernel/9front-port/pageown.c` (borrow checker wrapper: acquire/release keyed by HHDM VA).
- `kernel/9front-port/alloc.c` (post-boot ownership registration gated by boot state).

### Trap/exception path

- `kernel/9front-pc64/trap.c` (trap names, general protection violation handling).

## Observed Blockers (from logs)

1. Repeated `pageown_acquire` failures on stack/bss pages during early user faults.
2. `freepages` panic with `OwnerPID: -1` when releasing a page owned by PID 2.
3. `boot process died` due to a user-mode general protection violation at `pc=0x200009`.

## Code Observations Tied to Blockers

### Ownership acquisition vs release key

- `pageown_acquire` uses HHDM VA key (`pa + saved_limine_hhdm_offset`).
- `pageown_release` uses `kaddr(pa)` to compute the HHDM VA.
- If `saved_limine_hhdm_offset` and `kaddr()` diverge (or are uninitialized), ownership can be acquired under one key and released under another.

### `newpage` ownership acquisition is fatal

- `newpage` in `kernel/9front-port/page.c` panics if `pageown_acquire` fails.
- Logs show repeated `pageown_acquire failed for stack/bss` without an immediate panic.
- That log string does not exist in the current source tree, suggesting a different binary or local patch during that run.

### `freepages` ownership release path

- `freepages` checks `pageown_is_owned()` and then attempts `pageown_release`.
- If `pageown_get_owner()` returns `nil`, the logic checks for system-owned pages; otherwise it panics.
- The log shows `OwnerPID: -1`, which implies `pageown_get_owner()` returned a non-nil owner with pid -1.

### P9 exchange page lazy allocation

- `proc_setup_p9seg_stub` sets `pseg->pa = 0` and `SG_PHYSICAL` for lazy allocation.
- `mapphys` allocates a new page and calls `pageown_acquire` using `saved_limine_hhdm_offset`.
- If `saved_limine_hhdm_offset` is not initialized or differs from `kaddr()` expectations, ownership tracking can desync.

### `putpage` and ownership release

- `putpage` explicitly disables pageown release (`if (0 && ...)`), but `freepages` still performs release checks. This could allow pages to move into free lists without a symmetric release path and later fail ownership checks.

## Working Hypotheses

1. **Ownership key mismatch between acquire and release**
   - Evidence: `pageown_acquire` uses `saved_limine_hhdm_offset`, but `pageown_release` uses `kaddr()`. Logs show `OwnerPID: -1` in `freepages` panic.
   - What would prove this wrong: Instrument both paths to show identical HHDM VAs for the same PA during acquire and release in the failing run.

2. **Stack/bss ownership acquisition happens before borrow checker is ready**
   - Evidence: repeated `pageown_acquire failed for stack/bss` in logs without `newpage` panic implies a different error path or older build.
   - What would prove this wrong: reproduce the failure on the current source; if it panics in `newpage`, then the log is from a different tree or binary.

3. **User-space page tables or text mapping incomplete during init**
   - Evidence: `pc=0x200009` general protection violation, alongside heavy fault activity in user-space addresses.
   - What would prove this wrong: show valid PTEs for `UTZERO..` and stack at time of `init0()` transition, and that the trap is caused by a different exception (e.g., invalid instruction in init image).

4. **System-owned pages reported with `OwnerPID: -1` but not treated as system-owned**
   - Evidence: `freepages` only uses system release if `owner == nil` and `borrow_is_owned_by_system(...)` is true. Log shows non-nil owner with pid -1.
   - What would prove this wrong: confirm that `borrow_is_owned_by_system(...)` returns true for those pages or that `OwnerPID` is not -1 in a fresh run.

## Discrepancies to Resolve

- The log string `fixfault: pageown_acquire failed for stack/bss` is not present in the current tree. This suggests a different build or uncommitted change produced the log.
- `kernel/boot_check_final.txt` did not show panic markers with current grep method; treat it as lower-confidence evidence until verified.

## Coverage Warning

I found 11983 files but only examined 15 (code, docs, and logs) during this pass. Additional source coverage is required before final conclusions.

## What Would Prove My Assessment Wrong?

- A fresh Limine ISO boot using the current tree that does not reproduce `pageown_acquire failed` or `freepages` panics, yet still hits the same user-space GPF.
- Instrumentation showing `saved_limine_hhdm_offset` and `kaddr()` are consistent for all affected PAs.
- Evidence that the `pc=0x200009` GPF originates from a bad init image or unrelated user payload rather than page table/ownership issues.
