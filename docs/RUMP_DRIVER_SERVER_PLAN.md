# Full Rump Driver Server Plan

## 1) Define the target architecture + interfaces
- Contract:
  - Rump kernel runs in userspace as `rump_server`.
  - Rump serves `/srv/rump/posix/*` via 9P.
  - `rumpuser_*` routes all device access through HAL (HAL is the broker).
- Device scope:
  - Storage: AHCI only (skip IDE).
  - Console: tty/serial.
  - Network: enable whatever Rump can provide ("all rump can give us").

## 2) Bring up rumpuser backend (core runtime)
- Implement real time + sleep in `userspace/rump/rumpuser_lux9.c`:
  - `rumpuser_clock_gettime`, `rumpuser_clock_sleep`.
- Implement threading + locks:
  - `rumpuser_thread_create/join/exit`
  - `rumpuser_mutex_*`, `rumpuser_rw_*`, `rumpuser_cv_*`
  - If no full scheduler yet, use a minimal kernel-assisted thread API or
    single-threaded stub + guard.
- Implement randomness (`rumpuser_getrandom`).

## 3) Implement block I/O and file I/O bridges
- Implement storage hooks in `userspace/rump/rumpuser_lux9.c`:
  - `rumpuser_bio`, `rumpuser_iovread`, `rumpuser_iovwrite`
- Bind to HAL:
  - Use the family interface (or existing HAL syscalls) to issue read/write to
    AHCI/IDE.
  - Define request/response buffers, alignment rules, and error mappings.

## 4) Make the rump 9P POSIX bridge complete
- Validate existing 9P server in `userspace/rump/rump_server.c`.
- Implement missing operations currently stubbed:
  - fd sync/tell, set times, poll, sockets (if needed).
- Ensure `/srv/rump/posix/*` is mounted and live:
  - Verify service init and mount points.

## 5) Integrate into boot/services
- Confirm `rump_server` is launched by `boot/services.conf` and
  `initrd/boot/services.conf`.
- Ensure `rump_server` can access required devices and memory.
- Add logging to validate startup and basic IO.

## 6) WASI POSIX path verification
- Confirm `PERM_WASM_POSIX` is granted to WASM processes that should use rump.
- Validate `/wasm/posix` works and routes to `/srv/rump/posix/*`.

## 7) Testing plan
- Minimal functional tests:
  - `stat`, `read`, `write`, `readdir`, `seek`, `rename`.
- If storage is wired:
  - Create file, write data, reboot, read back.
- Optional network tests if enabled.

## 8) Cleanup + docs
- Document the final runtime contract:
  - rumpuser API mapping
  - HAL bridging details
  - Error translation and limitations
