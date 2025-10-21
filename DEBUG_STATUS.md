# Boot Hang Debugging Status

## Symptoms
- Kernel halts shortly after `userinit()` completes.
- GDB shows the CPU spinning in `lock()` waiting on `mmupool`'s borrow-lock.
- `((BorrowLock*)&mmupool)->lock.key == 0xffff800000000001`, `lock.p` points to the stub Proc (pid 0), indicating the lock was taken during early boot and never released.
- Serial output shows repeated `ML` markers at hang point, indicating MMU Lock (borrow_lock) operations.

## Fixes Applied
- Refactored `mmualloc()` to drop the `mmupool` lock on every return path (b1d8a7d).
- Updated ISO (`lux9.iso` built at 2025-10-20 01:23) to include the new `mmualloc()` logic.
- Additional debugging: Added detailed serial debug output to trace pmap execution, confirmed hang occurs in mmuwalk()->mmucreate()->mmualloc() path.

## Remaining Issue
- The lock is still owned immediately after boot, implying another mmupool acquisition (likely during slab priming in `preallocpages()` or early `mmualloc()` calls) still returns without calling `borrow_unlock()`.
- Debug output shows execution reaches pmap() in proc0(), gets to mmuwalk() loop (markers '1','2','3' visible), but hangs before marker '4', indicating the hang is in mmuwalk() calling mmucreate() which calls mmualloc().
- Serial output shows `ML23MLMLML4ML5` pattern at hang point, indicating repeated MMU Lock (borrow_lock) attempts.
- Need to protect remaining code paths (e.g., in `preallocpages()` and `mmuinit()` slab setup) with a cleanup block so the lock is always dropped.

## Next Steps
1. Instrument `borrow_lock`/`borrow_unlock` for `mmupool` using the GDB command file `mmupool.gdb`.
2. Re-run the boot; capture the last `borrow_lock` invocation without a matching `borrow_unlock` to identify the leaking code path.
3. Apply the same cleanup pattern there (`goto cleanup` or unlock-before-return) and rebuild.
4. Alternative investigation: Use QEMU deterministic replay (already captured) to examine exact state when hang occurs.
5. Direct QEMU monitor inspection of lock state while hung to see `mmupool.lock.key` value.
