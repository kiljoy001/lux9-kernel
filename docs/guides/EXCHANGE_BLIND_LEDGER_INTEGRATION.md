# Design Document: Upgrade of Exchange Page System for Blind Ledger Integration

## 1. Introduction

This document outlines the detailed plan to upgrade the existing `exchange` page system in the Lux9 kernel. The primary goal is to fundamentally transform its security model from one based on direct physical memory addresses to one leveraging cryptographic capabilities managed by the Blind Ledger and Pebble Identity. This upgrade is critical for enforcing zero-knowledge addressing, fine-grained access control, and verifiable memory integrity across memory transfer operations.

## 2. Current State of the Exchange Page System

The current `exchange` page system, defined in `kernel/include/exchange.h` and implemented in `kernel/9front-port/exchange.c`, provides a mechanism for page-level memory sharing and transfer between processes.

*   **Core Abstraction:** `ExchangeHandle` is currently a `uintptr`, representing a raw physical address (`pa`).
*   **Ownership Management:** It relies heavily on the `pageown` system (`pageown.h`, `sys_pageown.c`) for tracking page ownership, borrowing, and basic access control.
*   **Mechanism:** Pages are "prepared" by unmapping them from the source process, and their physical address is returned as an `ExchangeHandle`. These handles can then be "accepted" by a destination process, which maps the physical page and acquires ownership via `pageown`.
*   **Limitations:**
    *   **Physical Address Exposure:** Directly exposing physical addresses (`uintptr`) to processes creates a significant security vulnerability, violating the principle of least privilege and zero-knowledge addressing.
    *   **Weak Access Control:** `pageown` provides basic ownership, but lacks the cryptographic backing and fine-grained capabilities of the Blind Ledger.
    *   **No Tamper Detection:** No built-in mechanism to cryptographically verify the integrity of exchanged memory.

## 3. Target State: Capability-Based Exchange Pages

The upgraded `exchange` system will operate entirely on cryptographic `UserCapability` hashes, integrating seamlessly with the Blind Ledger, Pebble Identity, Cryptographic Vault, and the Red/Blue CoW token system.

*   **Zero-Knowledge Addressing:** `ExchangeHandle`s will no longer reveal physical addresses. Instead, they will be opaque `UserCapability` hashes.
*   **Strong Access Control:** All exchange operations will be mediated and verified by the Blind Ledger, ensuring that only authorized processes can perform operations on the capabilities they hold.
*   **Verifiable Integrity:** The Merkle root of the Blind Ledger, stored in the Vault, will provide tamper detection for all memory pages managed by capabilities.
*   **CoW Support:** The system will natively support Copy-on-Write semantics using Red (shared, read-only) and Blue (private, writable) tokens.

## 4. Key Architectural Changes

### 4.1. Redefinition of `ExchangeHandle`

The most fundamental change:
*   **`kernel/include/exchange.h`:**
    ```c
    // Old: typedef uintptr ExchangeHandle;
    typedef UserCapability ExchangeHandle; // UserCapability is a cryptographically strong hash (e.g., BLAKE2b_256)
    ```

### 4.2. Role of `pageown` Transformed

The `pageown` system will evolve from directly managing physical page ownership to becoming an *interface layer* to the Blind Ledger and Pebble Identity.
*   **`pageown_is_owned`, `pageown_get_owner`, `pageown_acquire`, `pageown_transfer`**: These functions will no longer operate on raw physical addresses. Instead, they will become wrappers around (or be superseded by) Blind Ledger functions that query and manipulate `BlindLedgerEntry` instances based on `UserCapability` hashes.
*   **Pebble Identity Integration:** `pageown` will internally use Pebble functions to allocate and free 8-byte Pegs, which form the granular physical backing for capabilities.

### 4.3. Blind Ledger as the Source of Truth

The `blind_ledger.c` module will be the canonical repository for `BlindLedgerEntry` instances. Each `BlindLedgerEntry` will:
*   Map a `UserCapability` hash to its underlying physical memory (Pegs).
*   Store the associated `process_hash` and `leaf_hash` for Merkle tree generation.
*   Track the capability's owner, permissions, and CoW state.

### 4.4. Cryptographic Vault Integration

The Vault will be a critical component for `ExchangeHandle` security:
*   **Secret Generation:** The Vault will provide cryptographically secure random numbers (secrets) used in the minting of new `UserCapability` hashes by the Blind Ledger.
*   **Merkle Root Storage:** The Merkle root of the Blind Ledger will be securely stored and managed by the Vault, providing an unforgeable anchor for memory integrity.

## 5. Detailed Function-by-Function Transformation

The following describes the proposed new behavior for each `exchange` system function, integrating with the Blind Ledger, Pebble, and Vault.

### 5.1. `void exchangeinit(void)`

*   **Current Behavior:** Initializes the `prepared_lock` and prints a message. Uses `pageown` implicitly.
*   **Proposed New Behavior:**
    *   Initializes the Blind Ledger system (`blind_ledger_init()`).
    *   Initializes the Pebble Identity system (`pebble_init()`).
    *   Performs any necessary setup for the Cryptographic Vault interaction (e.g., establishing a secure channel).
    *   Initializes the `prepared_lock` (still needed for the `PreparedPage` tracking, which will store `UserCapability`s).
*   **Dependencies:** `blind_ledger_init()`, `pebble_init()`, Vault API.

### 5.2. `ExchangeHandle exchange_prepare(uintptr vaddr)`

*   **Current Behavior:** Takes a virtual address, gets its physical address, verifies `pageown` ownership, unmaps the page, stores `pa` in `PreparedPage`, and returns `pa`.
*   **Proposed New Behavior:**
    1.  **Validate `vaddr`:** (Same as current).
    2.  **Get `pa`:** Walk the current process's MMU to get the physical address (`pa`) corresponding to `vaddr`.
    3.  **Mint Capability:** Call a Blind Ledger function (e.g., `blind_ledger_mint_exchange_capability(current_proc, pa, vaddr, permissions)`). This function will:
        *   Verify `current_proc` currently owns `pa` via the Blind Ledger (replacing `pageown_is_owned` and `pageown_get_owner`).
        *   If `pa` is part of a Red (shared, read-only) token, it may trigger a CoW split if `permissions` request write access (resulting in a new Blue token).
        *   Allocate a new `PebbleBlack` token for `pa` (if not already tokenized or if CoW split occurs) via Pebble's batched allocation.
        *   Request a new secret from the Cryptographic Vault.
        *   Create a `BlindLedgerEntry` for `pa`, associated with `current_proc` as owner, specific permissions, and the Vault-generated secret.
        *   Generate the `UserCapability` hash (e.g., `H(secret || leaf_hash)`) for this new `BlindLedgerEntry`.
        *   Update the Merkle tree of the Blind Ledger.
    4.  **Unmap Page:** Clear the PTE for `vaddr` in `current_proc`'s page tables and flush TLB. The page is now managed by the capability system, not directly by the process's `vaddr`.
    5.  **Track Prepared Page:** Allocate a `PreparedPage` structure and store the newly minted `UserCapability` (not `pa`), `original_vaddr`, and `current_proc`. Add to `prepared_pages` list.
    6.  **Return `UserCapability`:** Return the `UserCapability` as the `ExchangeHandle`.
*   **Dependencies:** `blind_ledger_mint_exchange_capability()`, Cryptographic Vault API, Pebble allocation (batched).

### 5.3. `int exchange_accept(ExchangeHandle handle, uintptr dest_vaddr, int prot)`

*   **Current Behavior:** Takes a `pa` (`handle`), removes it from `prepared_pages`, maps `pa` to `dest_vaddr`, and acquires ownership via `pageown_acquire`.
*   **Proposed New Behavior:**
    1.  **Validate `handle` and `dest_vaddr`:** (Same as current).
    2.  **Verify & Accept Capability:** Call a Blind Ledger function (e.g., `blind_ledger_verify_and_accept_capability(current_proc, handle, dest_vaddr, prot)`). This function will:
        *   Verify the `UserCapability` (`handle`) against the Blind Ledger.
        *   Check if `current_proc` is allowed to accept this capability based on `prot` (e.g., if it's a Red token and `prot` includes write, trigger CoW logic).
        *   Retrieve the underlying physical address (`pa`) from the `BlindLedgerEntry` (which internally references the batched Pegs).
        *   Update the `BlindLedgerEntry` to reflect new owner (`current_proc`), potentially update `process_hash` and the Merkle tree.
    3.  **Map Page:** Map the retrieved `pa` to `dest_vaddr` in `current_proc`'s page tables with `prot` using `userpmap`.
    4.  **Remove from `prepared_pages`:** Find and remove the `PreparedPage` entry for this `UserCapability`.
    5.  **Return Status:** Return `EXCHANGE_OK` or an appropriate `ExchangeError`.
*   **Dependencies:** `blind_ledger_verify_and_accept_capability()`, Pebble for physical address resolution, MMU mapping functions.

### 5.4. `int exchange_cancel(ExchangeHandle handle)`

*   **Current Behavior:** Finds `pa` in `prepared_pages`, remaps it to `original_vaddr` for the owner, and frees `PreparedPage` entry.
*   **Proposed New Behavior:**
    1.  **Validate `handle`:** (Same as current).
    2.  **Find Prepared Page:** Find the `PreparedPage` entry associated with `handle` (`UserCapability`).
    3.  **Burn Capability:** Call a Blind Ledger function (e.g., `blind_ledger_cancel_capability(original_owner_proc, handle)`). This function will:
        *   Verify `original_owner_proc` has rights to cancel `handle`.
        *   Mark the `BlindLedgerEntry` as "burned" in the ledger.
        *   Request the Cryptographic Vault to securely destroy the associated secret.
        *   Update the Merkle tree.
        *   Free the underlying 8-byte Pegs via Pebble (using batched deallocation if the capability was the last owner of those Pegs).
        *   Retrieve the underlying `pa` from the now-burned `BlindLedgerEntry`.
    4.  **Remap Page:** Remap the retrieved `pa` to `pp->original_vaddr` for `pp->owner` (if applicable and still desired). This might not always be the case if the intention is to completely release the memory.
    5.  **Free `PreparedPage`:** Free the `PreparedPage` structure.
    6.  **Return Status:** Return `EXCHANGE_OK` or `EXCHANGE_EINVAL`.
*   **Dependencies:** `blind_ledger_cancel_capability()`, Cryptographic Vault API, Pebble deallocation (batched).

### 5.5. `int exchange_transfer(Proc *from, Proc *to, ExchangeHandle handle, uintptr to_vaddr)`

*   **Current Behavior:** Transfers ownership of `pa` via `pageown_transfer` from `from` to `to`, then maps `pa` into `to_vaddr` for `to`.
*   **Proposed New Behavior:**
    1.  **Validate parameters:** (Same as current).
    2.  **Transfer Capability:** Call a Blind Ledger function (e.g., `blind_ledger_transfer_capability(from, to, handle, to_vaddr)`). This function will:
        *   Verify `from` legitimately owns `handle`.
        *   Update the `BlindLedgerEntry` to set `to` as the new owner.
        *   Update the `process_hash` of the `BlindLedgerEntry` to reflect the new owner, triggering a Merkle tree update.
        *   Retrieve the underlying physical address (`pa`) from the ledger.
    3.  **Map Page:** Map the retrieved `pa` into `to`'s address space at `to_vaddr` using `userpmap`.
    4.  **Flush TLB:** (Same as current).
    5.  **Return Status:** Return `EXCHANGE_OK` or appropriate `ExchangeError`.
*   **Dependencies:** `blind_ledger_transfer_capability()`, Pebble for physical address resolution, MMU mapping functions.

### 5.6. `int exchange_is_valid(ExchangeHandle handle)`

*   **Current Behavior:** Checks if `pa` is within `pageownpool` bounds and not `POWN_FREE`.
*   **Proposed New Behavior:**
    1.  **Validate `handle`:** (Same as current).
    2.  **Query Blind Ledger:** Call a Blind Ledger function (e.g., `blind_ledger_is_capability_valid(handle)`). This function will:
        *   Check if `handle` (`UserCapability`) exists as an active `BlindLedgerEntry` in the ledger.
        *   Perform basic integrity checks on the `BlindLedgerEntry`.
    3.  **Return Status:** Return 1 if valid, 0 if not.
*   **Dependencies:** `blind_ledger_is_capability_valid()`.

### 5.7. `Proc* exchange_get_owner(ExchangeHandle handle)`

*   **Current Behavior:** Returns the `Proc*` from `pageown_get_owner(pa)`.
*   **Proposed New Behavior:**
    1.  **Validate `handle`:** (Same as current).
    2.  **Query Blind Ledger:** Call a Blind Ledger function (e.g., `blind_ledger_get_capability_owner(handle)`). This function will:
        *   Retrieve the `BlindLedgerEntry` for `handle`.
        *   Return the `Proc*` stored in the entry.
    3.  **Return Owner:** Return `Proc*` or `nil`.
*   **Dependencies:** `blind_ledger_get_capability_owner()`.

### 5.8. `int exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles)`

*   **Current Behavior:** Iterates over a range, calling `exchange_prepare` for each page.
*   **Proposed New Behavior:** (Largely similar, but calls the updated `exchange_prepare`):
    *   Iterate over the range `vaddr` to `vaddr + len`.
    *   For each page, call the updated `exchange_prepare(va)`.
    *   Store the returned `UserCapability` in the `handles` array.
    *   Implement robust rollback if any `exchange_prepare` call fails within the range (calling `exchange_cancel` for already prepared capabilities).
*   **Dependencies:** Updated `exchange_prepare()`, updated `exchange_cancel()`.

## 6. Integration Points (Internal Modules)

### 6.1. Pebble Identity (`kernel/pebble.c`, `kernel/include/pebble.h`)

*   **`pebble_black_alloc()` / `pebble_black_free()`:** These functions will be the interface for the Blind Ledger to allocate/deallocate spans of 8-byte Pegs when a `PebbleBlack` token is minted or burned.
*   **`PebbleBlack` struct:** Will need to store the `UserCapability` or a reference to its `BlindLedgerEntry`.

### 6.2. Blind Ledger Core (`kernel/9front-port/blind_ledger.c`, `kernel/include/blind_ledger.h`)

*   **`ledger_mint()`, `ledger_verify()`, `ledger_transfer()`, `ledger_burn()`:** These core functions will manage the lifecycle of `BlindLedgerEntry` instances.
*   **Hashing (`BLAKE2b`):** Used for generating `UserCapability` hashes, `leaf_hash`, `process_hash`, and Merkle tree nodes.
*   **Merkle Tree Management:** The Blind Ledger will maintain the Merkle tree of its entries, updating it on state changes and recomputing the root.

### 6.3. Cryptographic Vault

*   **`vault_generate_secret()`:** New API for the Blind Ledger to request secure secrets for `UserCapability` minting.
*   **`vault_store_merkle_root()`, `vault_retrieve_merkle_root()`:** API for the Blind Ledger to store and retrieve the Merkle root for tamper detection.
*   **`vault_destroy_secret()`:** API for securely erasing secrets when capabilities are burned.

### 6.4. CoW (Red/Blue Tokens)

*   **`BlindLedgerEntry`:** Will include fields to denote if a capability is a Red (shared, read-only) or Blue (private, writable) token.
*   **`ledger_mint()`/`ledger_verify()`:** Will incorporate logic to handle CoW semantics. For example, if a write attempt is made on a Red Token, `ledger_verify` might trigger a CoW event, leading to the creation of a new Blue Token.

## 7. Error Handling and Tamper Detection

*   **`ExchangeError` Codes:** These will be updated to reflect errors originating from the Blind Ledger or Pebble system (e.g., `EXCHANGE_ECAPINVALID`, `EXCHANGE_EPERMDENIED`).
*   **Merkle Root Mismatch:** As discussed, this will act as a "tripwire." Upon detection:
    *   Log event securely (signed by Vault).
    *   Alert system administrators.
    *   Transition system/affected processes to a "tainted" state, enforcing stricter policies or isolation, rather than an immediate hard halt.

## 8. Open Questions / Future Work (Updated)

*   **Performance Benchmarking:** Thorough benchmarking of hashing and Merkle tree updates with frequent capability operations will be crucial, especially considering the reliance on potential hardware acceleration.
*   **CoW Policy Details:** Define precise policies for CoW token lifecycle, including how many Red Tokens can exist for a given physical page and the exact triggering conditions for Red-to-Blue token transitions.
*   **Granularity Below Page Level (Addressed by Batching):** While the `exchange` system operates at page granularity, the Pebble Identity manages memory at 8-byte "Peg" granularity. The solution for this is **batching**:
    *   When a `UserCapability` representing a page (e.g., from `exchange_prepare`) is minted, the Blind Ledger will instruct Pebble to perform a *batched allocation* of `BY2PG / 8` (typically 512) contiguous Pegs.
    *   The `BlindLedgerEntry` for this capability will implicitly represent this batch of Pegs.
    *   This batching mechanism will also apply to deallocation and other bulk operations, significantly optimizing the interaction between page-granular and Peg-granular memory management. This ensures that while Pebble maintains fine-grained control, the higher-level `exchange` system can operate efficiently at page-level.
*   **Hashing Algorithm for Hardware Acceleration (BLAKE2b vs. SHA256):** The current design specifies BLAKE2b (from Monocypher) for cryptographic hashing. However, for "massive speed boosts" reliant on "hardware extensions in the cpu," SHA256 is often a more widely supported and accelerated primitive (e.g., Intel SHA Extensions, ARMv8 Crypto Extensions).
    *   **Consideration:** While BLAKE2b offers excellent software performance and security properties, the long-term architectural decision regarding the primary hashing algorithm should explicitly weigh its hardware acceleration potential against alternatives like SHA256.
    *   **Recommendation:** A crypto abstraction layer should be established to allow flexibility in swapping hashing algorithms. For critical, performance-sensitive paths, a decision may be needed to either:
        1.  Retain BLAKE2b and invest in specific hardware/software optimizations or custom acceleration.
        2.  Transition to SHA256 (or another hardware-accelerated algorithm) if its performance benefits through existing hardware extensions are substantial and meet security requirements. This would require careful re-evaluation of the cryptographic strength and formal verification aspects.
