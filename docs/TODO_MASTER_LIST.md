# Lux9 Master TODO List

## Immediate Priorities (Phase 1 Cleanup & Verification)


## Kernel Core
- [ ] **Memory Safety**
  - [ ] Verify `bootstrap_alloc.c` (currently unverified vs `xalloc.c`).
  - [ ] Extend Pebble tracking to all driver allocations.


- [ ] **Subsystem TODO/FIXME Inventory (code scan)**
  - [ ] **WASI completeness**: implement missing WASI syscalls (args/env, clocks, random, fd flags/advise, full path ops, fs sync/stat, polling) and normalize errno mappings for common runtimes. (`kernel/wasm/wasi_lux9_shim.c`, `kernel/wasm/wasm_runtime.c`)
  - [ ] **WASM memory model hardening**: finalize arena-based linear memory, enforce guard pages on grow/shrink, and remove any kernel-heap fallbacks in wasm3 alloc paths. (`kernel/wasm/wasm_runtime.c`, `kernel/wasm/wasm_runtime/wasm3/m3_core.c`)
  - [ ] **WASM syscall surface**: complete `Tsyscall` coverage for WASI-hosted ops and align ABI/errno conversions. (`kernel/9p_router.c`, `kernel/wasm/wasi_lux9_shim.c`)
  - [ ] **WASM namespace plumbing**: finalize `/wasm` preopens, fd rights inheritance, and per-process namespace isolation. (`kernel/9front-port/devroot.c`, `kernel/9front-port/userinit.c`, `kernel/wasm/wasi_lux9_shim.c`)
  - [ ] **WASM pebble accounting**: ensure all linear memory growth and host allocations are charged/returned to pebble. (`kernel/wasm/wasm_runtime.c`, `kernel/9front-port/page.c`)
  - [ ] **WASM IPC integration**: define ring/exchange IPC interface for WASM and wire to syscall entrypoints. (`kernel/9p_router.c`, `kernel/9front-port/devexchange.c`, `kernel/9front-port/devring.c`)
  - [ ] **WASM trap/exception handling**: map traps to consistent WASI errno or exit status and avoid kernel panics on malformed modules. (`kernel/wasm/wasm_runtime.c`, `kernel/wasm/wasm_runtime/wasm3`)
  - [ ] **9P WASM routing is stubbed**: implement proper server lookup/init and synchronous/async completion handling. (`kernel/9p_router.c:3639`, `kernel/9p_router.c:3648`, `kernel/9p_router.c:3675`)
  - [ ] **print ticks placeholder**: return actual tick count in `printticks`. (`kernel/9front-port/print.c:132`)
  - [ ] **BlindLedger indexing**: add UUID secondary index for O(log n) lookup. (`kernel/9front-port/blind_ledger.c:380`)
  - [ ] **devram capability disposal**: decide/implement "burn capability" behavior. (`kernel/9front-port/devram.c:169`)
  - [ ] **devexchange peer wiring**: connect to peer endpoint and implement pool resizing. (`kernel/9front-port/devexchange.c:798`, `kernel/9front-port/devexchange.c:815`)
  - [ ] **WASM 9P integration routing**: implement actual routing to WASM server. (`kernel/wasm/wasm_9p_integration.c:253`)
  - [ ] **WASM capability bindings**: read name strings from WASM linear memory. (`kernel/wasm/wasm_capability_bindings.c:79`, `kernel/wasm/wasm_capability_bindings.c:108`)
  - [ ] **WASM runtime isolation**: allocate isolated segment, map/unmap linear memory, and parse args/return values. (`kernel/wasm/wasm_runtime.c:43`, `kernel/wasm/wasm_runtime.c:91`, `kernel/wasm/wasm_runtime.c:210`, `kernel/wasm/wasm_runtime.c:309`, `kernel/wasm/wasm_runtime.c:323`, `kernel/wasm/wasm_runtime.c:372`)
  - [ ] **WASM fileserver functionality**: load/parse module, prepare exchange pages, and marshal request/response payloads with correct sizes and utilization tracking. (`kernel/wasm/wasm_fileserver.c:67`, `kernel/wasm/wasm_fileserver.c:94`, `kernel/wasm/wasm_fileserver.c:152`, `kernel/wasm/wasm_fileserver.c:228`, `kernel/wasm/wasm_fileserver.c:387`, `kernel/wasm/wasm_fileserver.c:397`, `kernel/wasm/wasm_fileserver.c:412`)
  - [ ] **WASI/Rump POSIX bridge**: implement remaining `/srv/rump/posix/*` ops.
    - [x] sockets (accept, recv, send, shutdown)
    - [x] path ops (mkdir, rmdir, unlink, rename, symlink, readlink)
    - [x] fd ops (sync, set_size)
    - [ ] poll (poll_oneoff)
    - [ ] times (fd_set_times, path_set_times)
    (`userspace/rump/rump_server.c`)
  - [ ] **9front-pc64 globals routing**: wire to `9p_router`. (`kernel/9front-pc64/globals.c:457`)
  - [ ] **CGA mapping**: handle screen memory not mapped yet. (`kernel/9front-pc64/cga.c:158`)
  - [ ] **Crypto arch detection**: enhance automatic detection coverage (likely upstream). (`kernel/crypto/sph_types.h:997`)

- [ ] **Scheduler**
  - [ ] Formalize ULE scheduler status (currently "experimental/unverified").
  - [ ] Decide on ULE vs EDF as primary verified scheduler.

## User Space
- [ ] **Drivers**
  - [ ] Audit user-space drivers for PoW compliance (link against `liblux`).

## Testing/Tooling

## Documentation
- [ ] Update `SECURITY.md` with the "Kinetic Defense" IPC model.
- [ ] Mark `ghostdag_kernel.c` as deprecated/historical in comments.
