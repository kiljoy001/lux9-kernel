# Lux9 Master TODO List

## Immediate Priorities (Phase 1 Cleanup & Verification)
- [ ] **MsgOrd + PoW Integration (Kinetic Defense)**
  - **Objective**: Integrate `kernel/pow_gate.c` logic into `kernel/msgord.c` to rate-limit IPC based on system load.
  - **Implementation**:
    - Add `POW_OP_MSGORD` to operation classes.
    - Modify `_msgord_submit` to accept a nonce and verify `pow_verify()`.
    - **CRITICAL**: Implement TCB Exemption. Checks `up->kp` (kernel process) to bypass PoW.
    - Drivers (user-space) MUST perform PoW to prevent spam/compromise flooding.
    - Update `OrdMsg` struct to carry nonce.
  - **Security**: Mitigates DoS/flooding of the consensus DAG.

- [ ] **Frama-C / Coq Consistency**
  - [ ] Use `scripts/framac_plan9_v2.sh` to verify `kernel/ghostdag_kernel.c` (legacy) vs `kernel/msgord.c` (active).
  - [ ] Ensure `proofs/msgord` theorems align with the "Lite" implementation in `msgord.c`.

- [ ] **CLR Interpreter Verification**
  - [ ] Complete Control Flow opcodes tests (Branching, Switch, Exception).
  - [ ] Complete Memory opcodes tests (Indirect access, Pointers).
  - [ ] Complete Object Model opcodes tests (Fields, Methods, Inheritance).

## Kernel Core
- [ ] **Memory Safety**
  - [ ] Verify `bootstrap_alloc.c` (currently unverified vs `xalloc.c`).
  - [ ] Extend Pebble tracking to all driver allocations.

- [ ] **Critical Bug Backlog**
  - [ ] **MsgOrd 9P payload lifetime**: `msgord_submit` stores a raw `Fcall *` and later dispatches it asynchronously; if the original buffer is stack-allocated or reused, this becomes UAF/data corruption. Ensure payloads are deep-copied or the exchange-page backing is pinned/refcounted. (`kernel/msgord.c:361`, `kernel/msgord.c:471`)
  - [ ] **Pebble white-token/budget leaks on early failure paths**: after `pebble_issue_white`, failures of `xallocz_raw` or `pebble_black_alloc` return without releasing the WHITE token or freeing raw memory, permanently shrinking available pool budget. Add cleanup on every failure path. (`kernel/9front-port/alloc.c:62`, `kernel/9front-port/alloc.c:81`)
  - [ ] **Static reply buffer in SYS_NSEC**: `static uchar nsec_reply[8]` is shared across callers, so concurrent SYS_NSEC replies can race and corrupt output. Use per-request storage or a thread-local buffer. (`kernel/9p_router.c:529`)

- [ ] **Subsystem TODO/FIXME Inventory (code scan)**
  - [ ] **9P WASM routing is stubbed**: implement proper server lookup/init and synchronous/async completion handling. (`kernel/9p_router.c:3639`, `kernel/9p_router.c:3648`, `kernel/9p_router.c:3675`)
  - [ ] **print ticks placeholder**: return actual tick count in `printticks`. (`kernel/9front-port/print.c:132`)
  - [ ] **BlindLedger indexing**: add UUID secondary index for O(log n) lookup. (`kernel/9front-port/blind_ledger.c:380`)
  - [ ] **devram capability disposal**: decide/implement "burn capability" behavior. (`kernel/9front-port/devram.c:169`)
  - [ ] **devexchange peer wiring**: connect to peer endpoint and implement pool resizing. (`kernel/9front-port/devexchange.c:798`, `kernel/9front-port/devexchange.c:815`)
  - [ ] **WASM 9P integration routing**: implement actual routing to WASM server. (`kernel/wasm/wasm_9p_integration.c:253`)
  - [ ] **WASM capability bindings**: read name strings from WASM linear memory. (`kernel/wasm/wasm_capability_bindings.c:79`, `kernel/wasm/wasm_capability_bindings.c:108`)
  - [ ] **WASM runtime isolation**: allocate isolated segment, map/unmap linear memory, and parse args/return values. (`kernel/wasm/wasm_runtime.c:43`, `kernel/wasm/wasm_runtime.c:91`, `kernel/wasm/wasm_runtime.c:210`, `kernel/wasm/wasm_runtime.c:309`, `kernel/wasm/wasm_runtime.c:323`, `kernel/wasm/wasm_runtime.c:372`)
  - [ ] **WASM fileserver functionality**: load/parse module, prepare exchange pages, and marshal request/response payloads with correct sizes and utilization tracking. (`kernel/wasm/wasm_fileserver.c:67`, `kernel/wasm/wasm_fileserver.c:94`, `kernel/wasm/wasm_fileserver.c:152`, `kernel/wasm/wasm_fileserver.c:228`, `kernel/wasm/wasm_fileserver.c:387`, `kernel/wasm/wasm_fileserver.c:397`, `kernel/wasm/wasm_fileserver.c:412`)
  - [ ] **WASI shim directory flag**: set `is_dir` correctly. (`kernel/wasm/wasi_lux9_shim.c:235`)
  - [ ] **9front-pc64 globals routing**: wire to `9p_router`. (`kernel/9front-pc64/globals.c:457`)
  - [ ] **CGA mapping**: handle screen memory not mapped yet. (`kernel/9front-pc64/cga.c:158`)
  - [ ] **Crypto arch detection**: enhance automatic detection coverage (likely upstream). (`kernel/crypto/sph_types.h:997`)

- [ ] **Scheduler**
  - [ ] Formalize ULE scheduler status (currently "experimental/unverified").
  - [ ] Decide on ULE vs EDF as primary verified scheduler.

## User Space
- [ ] **Drivers**
  - [ ] Audit user-space drivers for PoW compliance (they should calculate nonces).
  - [ ] Implement `libmsgord` helper for user-space PoW calculation.

## Testing/Tooling
- [ ] **CLR/QBE test stubs**
  - [ ] Replace stub `pebble_alloc` and complete argument handling in QBE toolchain tests. (`kernel/test/temp_src/qbe_compile.c:118`, `kernel/test/temp_src/fruity_to_qbe.c:115`, `kernel/test/temp_src/fruity_to_qbe.c:191`)
  - [ ] Implement module/function/block verification in `fruity_ir`. (`kernel/test/temp_src/fruity_ir.c:493`, `kernel/test/temp_src/fruity_ir.c:500`, `kernel/test/temp_src/fruity_ir.c:507`)

## Documentation
- [ ] Update `SECURITY.md` with the "Kinetic Defense" IPC model.
- [ ] Mark `ghostdag_kernel.c` as deprecated/historical in comments.
