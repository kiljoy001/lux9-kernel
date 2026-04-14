# Ownership Tokens + RFMEM Isolation (Plan Only)

## Goal
Unblock resurrection by isolating exchange pages under RFMEM, and define a long-term ownership-flip model using UUIDv8 tokens (vault-signed, procd-verified, kernel-enforced).

## Scope (Now vs Later)
Now (focus):
- Fix the rfork hang by ensuring exchange pages are never shared under RFMEM.

Later (design hardening):
- Segment-level ownership tokens (UUIDv8).
- Procd-verified tokens; kernel-enforced access.
- Optional policy bits aligned with borrow checker states.

## A. Immediate Fix: Isolate Exchange Pages Under RFMEM

### Problem
sys_rfork(RFPROC | RFMEM) shares segments, so the exchange page gets shared. This causes deadlock/hang and breaks isolation.

### Minimal fix (mechanism only)
In the RFMEM path of sysrfork:
- Invalidate the exchange page PTE before procfork.
- Clear parent's exchange page tracking (up->p9page = nil; up->p9page_phys = 0).
- Create a stub exchange segment for the child (proc_setup_p9seg_stub(p)).
- Do not transfer exchange page ownership between parent/child.

Result: Parent and child will fault and get fresh exchange pages even with RFMEM.

### Validation
- Boot reaches:
  - RESURRECTION: starting managed services
  - RESURRECTION: Starting Procd Service...

## B. Long-Term Design: Segment-Level Ownership Tokens

### Design goals
- Per-segment tokens (not per page).
- Token ownership flips control access.
- Kernel stays light; procd handles policy.

### Token model (UUIDv8)
- Token encodes:
  - segment_id
  - generation
  - policy bits (borrow states / permissions)
  - entropy/nonce

### Ownership flips
- Explicit syscall: sys_seg_transfer(token, target_pid2)
- Kernel only accepts flips from procd.
- Kernel registry:
  - token -> {owner_pid2, seg_id, size, perms, gen, state}

### Vault + procd
- Vault holds private key for signing tokens.
- procd holds public key registry.
- procd validates signatures and requests kernel registration/flip.

## C. Policy / Mechanism Split

Kernel (mechanism)
- Enforces access control by ownership.
- Maintains token registry.
- No signature verification.

procd (policy)
- Validates signatures.
- Decides when transfers are allowed.
- Mediates registration/flip requests.

## D. Open Decisions
1. Segment size: fixed (start) vs dynamic (later).
2. Token format: exact UUIDv8 bit layout.
3. Borrow-state bit encoding.
4. Exec behavior: rotate keys, invalidate old tokens.

## E. Risks
- RFMEM isolation could break legacy assumptions.
- Token registry must be robust and minimal to avoid kernel bloat.
- procd must be available early enough for token validation.

## Next Steps (when ready)
1. Implement RFMEM exchange isolation.
2. Validate resurrection/procd startup.
3. Formalize token layout and registry.
4. Add procd verification flow.
