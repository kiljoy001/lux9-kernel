# Verification Quality Audit (Jan 2026)

## Overview
This document supplements `VERIFICATION_STATUS.md` by analyzing the *quality* and *depth* of the formal verification artifacts, not just their presence.

## 1. Coq Proof Quality
**Rating: High**

The `proofs/` directory contains high-quality, mechanically checked Coq proofs. 
*   **Hardware Abstraction (`proofs/hardware_abstraction.v`):**
    *   **Status:** Excellent.
    *   **Details:** Defines rigorous `HardwareProfile` models for x86_64, ARM64, and RISC-V. Proves safety properties (`cross_architecture_safety`) using standard tactics without relying on `admit` or `Axiom`.
    *   **Axioms:** None found in the abstraction logic.

*   **Axiom Usage:**
    *   Axioms are used appropriately for:
        *   **Crypto Primitives:** `elligator_inv`, `crypto_correctness` (Standard practice to assume crypto math correctness).
        *   **Physical Constants:** `BY2PG > 0` (Page size positive).

## 2. ACSL Contract Quality
**Rating: Mixed (Medium to Low)**

While many files are marked "Verified" due to the presence of `/*@` annotations, the depth of these contracts varies significantly.

### A. High Quality (Standard Library & Core Logic)
*   **Examples:** `kernel/libc9/kstrcpy.c`, `kernel/proc_fsm.c`.
*   **Characteristics:**
    *   Full functional correctness specs (`requires`, `ensures`).
    *   Loop invariants provided (e.g., `loop invariant s1 >= os1` in `strcpy`).
    *   Traceability to Coq proofs (e.g., comments linking to `proc_state_dag.v`).

### B. Medium Quality (Memory Management)
*   **Examples:** `kernel/9front-port/xalloc.c`.
*   **Characteristics:**
    *   Strong Interface Contracts: `xalloc_internal` has detailed `behavior` clauses and alignment checks.
    *   **Weak Implementation Verification:** The core free-list traversal loop (`for (h = *l; h; h = h->link)`) **lacks loop invariants**. This implies the verification tool cannot fully prove the safety of the list traversal or the preservation of list invariants during the search, limiting the guarantee to "If it returns, it returns valid memory," but not "it will safely search the list."

### C. Low Quality (Platform Layer)
*   **Examples:** `kernel/9front-pc64/mmu.c`.
*   **Characteristics:**
    *   Sparse annotations. A 1400-line file may contain only 1-3 contracts.
    *   Trivial contracts (e.g., `extern void uartputs... assigns \nothing;`).
    *   Most internal static functions (`dbghex`, `ensure_phys_range`) are unverified.
    *   **Metric Inflation:** Counting these files as "Verified" inflates the coverage statistics (73%) without providing proportional assurance.

## 3. Verification Challenges: Timeouts
**Status: High Ambition / High Complexity**

Several subsystems, notably `kernel/borrowchecker.c` and `kernel/clr/qbe/pebble.c`, frequently timeout in automated verification scripts. Analysis reveals that these timeouts are **not** due to poor ACSL quality, but rather:
*   **State Machine Explosion:** Components like the borrow checker implement complex FSMs (e.g., `borrow_fsm_transition`) that require the prover to explore many branching paths.
*   **Dynamic Data Structures:** Reasoning about pointer aliasing and heap-allocated linked lists (e.g., `BorrowOwner` hash tables) is computationally expensive for automated provers.
*   **Global Invariants:** Maintaining predicates like `borrow_pool_valid` across all modifications involves significant inter-procedural reasoning.
*   **Cryptographic Integration:** The use of nonces and secure hashing increases the logical depth required for verification.

These timeouts indicate that the verification effort has reached the limits of general-purpose automated provers and requires further decomposition into smaller lemmas or more manual proof hints (Coq).

## 4. Recommendations
1.  **Strengthen Allocator Proofs:** Add loop invariants to `xalloc_internal` to prove free-list integrity.
2.  **Audit Platform Contracts:** Re-classify files with only trivial `extern` contracts as "Partially Verified" or "Interface Verified" rather than fully verified.
3.  **Expand Platform Verification:** Add comprehensive ACSL contracts to MMU and driver code beyond trivial extern declarations.
