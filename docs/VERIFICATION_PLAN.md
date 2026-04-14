# Plan for Achieving High Quality Verification

This document outlines the specific steps required to elevate the verification quality of key kernel subsystems from "Medium/Low" to "High".

## 1. Goal Definition

"High Quality" verification in this project context is defined as:
*   **Functional Correctness:** Specifications (`requires`/`ensures`) describe *what* the function does, not just memory safety.
*   **Implementation Proof:** Loops have strong invariants proving termination and state preservation.
*   **State Modeling:** Global state (like the MMU configuration) is modeled abstractly.
*   **Traceability:** C code annotations link explicitly to Coq formal models.

## 2. Targeted Subsystems & Actions

### A. Memory Allocator (`kernel/9front-port/xalloc.c`)
*Current Status:* Medium. Interface contracts exist, basic loop invariants added.
*Missing:* Proof that the search algorithm is correct (completeness).

**Action Plan:**
1.  **Define Reachability:** Introduce an ACSL predicate `reachable(Hole *start, Hole *target)` to model the linked list.
2.  **Strengthen Loop Invariant:**
    *   Add: `\forall Hole *p; reachable(*l, p) && reachable(p, h) ==> p->size < size;`
    *   (Meaning: "Every hole we have skipped so far was too small.")
3.  **Prove Failure Case:** Ensure the `behavior failure` contract is provable by deducing from the loop invariant that if the loop finishes without return, no suitable hole exists.

### B. MMU Platform Layer (`kernel/9front-pc64/mmu.c`)
*Current Status:* Low/Improved. Interface contracts added, but rely on opaque globals.
*Missing:* Formal model of the physical memory map.

**Action Plan:**
1.  **Model Configuration:**
    *   Add ACSL logic to model `conf.mem` array: `logic integer ConfmemBase(integer i) = ...`
2.  **Verify Calculation:**
    *   In `ensure_phys_range`, add loop invariant: `max_physaddr == \max(0, \max_{k=0}^{i-1} (conf.mem[k].base + ...))`
3.  **Link to Coq:**
    *   Add reference to `proofs/mmu/mmu_model.v`.
    *   Example: `/*@ // Implements physical_address_valid from mmu_model.v */`

### C. Standard Library (`kernel/libc9/`)
*Current Status:* High (mostly).
*Refinement:* Ensure all string functions handle non-null-terminated buffer overreads (common source of bugs).

## 3. Execution Strategy

To execute this plan:
1.  Create a local `verification_logic.acsl` file to define helper predicates (like `reachable`).
2.  Iteratively apply stronger invariants to `xalloc.c` and run Frama-C (if available) or manual review against the logic.
3.  Update `docs/VERIFICATION_QUALITY.md` as files migrate to the "High" category.
