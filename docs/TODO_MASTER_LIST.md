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

- [ ] **Scheduler**
  - [ ] Formalize ULE scheduler status (currently "experimental/unverified").
  - [ ] Decide on ULE vs EDF as primary verified scheduler.

## User Space
- [ ] **Drivers**
  - [ ] Audit user-space drivers for PoW compliance (they should calculate nonces).
  - [ ] Implement `libmsgord` helper for user-space PoW calculation.

## Documentation
- [ ] Update `SECURITY.md` with the "Kinetic Defense" IPC model.
- [ ] Mark `ghostdag_kernel.c` as deprecated/historical in comments.
