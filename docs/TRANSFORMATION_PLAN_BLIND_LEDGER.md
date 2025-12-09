# Lux9 Transformation Plan: The Blind Ledger Architecture

## 1. Vision: The Cryptographic Microkernel

Lux9 is shifting from a traditional capability-based microkernel to a **Cryptographic Addressing Kernel**. The core tenet is that **Identity is Security**.

*   **Old World:** Access control via Lists (ACLs) or Tables (Capabilities) checking User ID against Resource ID.
*   **New World:** Access control via **Knowledge**. To possess the valid Hash of a resource is to own it. The Hash is mathematically unforgeable and tied to the specific Owner and Epoch.

## 2. Core Pillars

### 2.1 The Blind Ledger (Zero-Knowledge Addressing)
*   **Concept:** The kernel maintains a master table of resources, but users never see indexes or physical addresses.
*   **Mechanism:**
    *   **Kernel View:** `Table[Hash(Resource_ID)] = { PhysAddr, Owner_PID, Secret_Nonce }`
    *   **User View:** `Capability = H( H(Owner_PID | Secret_Nonce) | Resource_ID )`
*   **Verification:** Access grants are 0(1) hash verifications. `Expected_Cap == Provided_Cap`.
*   **Transfer:** Sender generates new Cap for Receiver using Receiver's public identity. Sender allows kernel to rotate the Nonce. Old Cap becomes garbage.

### 2.2 Pebble Identity (Immutable Topology)
*   **Concept:** Physical RAM is not a linear array. It is a collection of unique, immutable identities.
*   **Implementation:**
    *   At boot, every 4KB page (or 8-byte granule) is assigned a **Static Unique ID**.
    *   This ID is the leaf of a static **Merkle Tree** covering all RAM.
    *   **Benefit:** Robust protection against Use-After-Free and ABA problems. If a page is re-allocated, its "Epoch" changes, invalidating all old Merkle proofs/hashes.

### 2.3 The "Peg" & Bitmap Physics
*   **Granularity:** 1 White Token = **8 Bytes**.
*   **Tracking:** A global **Bitmap** replaces the linked lists for free space tracking.
    *   1 Bit = 1 Granule (8 bytes).
    *   **Reverse Bloom Filter / Hierarchical Bitmap:** Used to find run-lengths of `0`s (Free space) in O(1) or O(log N).
*   **Constraint:** White Tokens in the user process (`PebbleWhite` structs) now contain a `u64int index` pointing to this bitmap.

### 2.4 Exchange Integration
*   **Current:** `exchange_prepare(uintptr phys_addr)`
*   **Future:** `exchange_prepare(u64int capability_hash)`
*   **Impact:** Userspace drivers (like the future `ahci_driver`) never handle physical addresses. They handle opaque tokens. This creates perfect isolation.

## 3. Implementation Roadmap

### Phase 1: The Data Structures
1.  Define `struct BlindLedgerEntry` and `struct UserCapability`.
2.  Implement the Global Bitmap allocator (`kernel/bitmap.c`).
3.  Implement the Hashing Primitive (SipHash or Blake3).

### Phase 2: The Ledger Logic
1.  Implement `ledger_insert`, `ledger_lookup`, `ledger_transfer`.
2.  Replace `borrowpool` (Hash Map) with `BlindLedger` (B-Tree or Hash Map of Caps).
3.  Update `pebble.c` to use the Ledger for Black/Blue tokens.

### Phase 3: The Integration
1.  Update `exchange.c` to accept Capability Hashes.
2.  Update `pci_family.c` to issue Capabilities instead of mapping BARs directly.
3.  Update `pebble_issue_white` to scan the Bitmap.

### Phase 4: Verification
1.  Extend `proofs/broker_transfer.v` to model the Hash Hiding property (Zero Knowledge).
2.  Verify that `Old_Cap` cannot derive `New_Cap` without the Kernel Secret.

## 4. Security Implications

*   **Confused Deputy:** Solved. A deputy cannot be tricked into using its own authority to access a resource it "thinks" is yours, because it can't generate the valid Hash for your resource without your secret.
*   **Buffer Overflows:** Solved (in the physical domain). Adjacency in memory does not imply adjacency in the Hash space. You cannot "overflow" from one Pebble to another because you don't know the ID of the neighbor.
*   **Rowhammer / Side Channels:** Mitigated. The attacker doesn't know which rows are adjacent to their own victim rows because physical layout is abstracted by the Ledger.

---
*Drafted: December 7, 2025*
