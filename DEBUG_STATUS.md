# Boot Hang Debugging Status

## Symptoms
- Kernel halts shortly after `userinit()` completes.
- GDB shows the CPU spinning in `lock()` waiting on `mmupool`'s borrow-lock.
- `((BorrowLock*)&mmupool)->lock.key == 0xffff800000000001`, `lock.p` points to the stub Proc (pid 0), indicating the lock was taken during early boot and never released.

## Fixes Applied
- Refactored `mmualloc()` to drop the `mmupool` lock on every return path (b1d8a7d).
- Updated ISO (`lux9.iso` built at 2025-10-20 01:23) to include the new `mmualloc()` logic.

## Remaining Issue
- The lock is still owned immediately after boot, implying another mmupool acquisition (likely during slab priming in `preallocpages()`) still returns without calling `borrow_unlock()`.
- Need to protect remaining code paths (e.g., in `preallocpages()` and `mmuinit()` slab setup) with a cleanup block so the lock is always dropped.

## Next Steps
1. Instrument `borrow_lock`/`borrow_unlock` for `mmupool` using the GDB command file `mmupool.gdb`.
2. Re-run the boot; capture the last `borrow_lock` invocation without a matching `borrow_unlock` to identify the leaking code path.
3. Apply the same cleanup pattern there (`goto cleanup` or unlock-before-return) and rebuild.
