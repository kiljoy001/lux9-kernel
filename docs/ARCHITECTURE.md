# Lux9 Kernel Architecture

## 1. Architecture Overview

Lux9 represents a radical departure from traditional kernel designs by integrating high-level language concepts directly into the kernel's memory management and hardware abstraction layers. The architecture is built upon four pillars:

1.  **Pebble**: A deterministic, token-based memory management system that replaces garbage collection and manual memory management with a formal "token game".
2.  **Family**: A 9P-based Hardware Abstraction Layer (HAL) that groups devices into "families" (PCI, USB, etc.) rather than individual drivers.
3.  **Congruent 9P**: A file-system interface where the directory structure strictly mirrors the kernel's internal object state.
4.  **Borrow Checker**: A kernel-level implementation of Rust-style ownership and borrowing rules for resources.

### 1.1 The Pebble Memory Model

Pebble replaces the traditional `malloc`/`free` or Garbage Collection model with a token exchange system.

| Token | Color | Semantics |
| :--- | :--- | :--- |
| **White** | Reservation | A capability/reservation to allocate memory or a reference to an object. Contains a generation ID. |
| **Black** | Ownership | Represents actual allocated memory (the "backing store"). Has a budget cost. |
| **Blue** | Borrow | A borrowed reference to Black memory. Links back to the Black token. |
| **Red** | Snapshot | A copy/snapshot of Blue memory. Used for speculative execution, transactional rollbacks, and persistence. |

**Lifecycle:**
1.  **Issue**: A process requests a **White** token (`pebble_issue_white`).
2.  **Verify**: The White token is verified against the budget (`pebble_white_verify`), converting it to a pending state.
3.  **Alloc**: The pending state is converted to a **Black** token (`pebble_black_alloc`), allocating physical memory.
4.  **Borrow**: **Blue** tokens are created to allow other parts of the system to access the memory without transferring ownership.
5.  **Snapshot**: A **Red** token is created from a Blue token to capture state for potential rollback (`pebble_red_copy`).

### 1.2 Family HAL (Hardware Abstraction Layer)

The Family system abstracts hardware not as individual devices, but as "Families" of protocols. It uses a registry system where families (PCI, USB, I2C) register themselves as 9P file servers.

*   **Registry**: A global `FamilyRegistry` tracks active families.
*   **Channels**: Access to devices is mediated via 64-bit "Channel IDs".
*   **Congruent 9P Router**: The `#F` device (`/dev/family/`) acts as a router. Accessing `/dev/family/PCI` delegates operations to the registered PCI family subsystem.

---

## 2. 9P File Tree Structure

The Lux9 kernel exposes its internal state via a congruent 9P file system.

### 2.1 `/dev/family/` (#F)

The Family device serves as the root for all hardware interaction.

| Path | Description |
| :--- | :--- |
| `/dev/family/` | Root directory listing all registered families. |
| `/dev/family/ctl` | Control file for global family management. |
| `/dev/family/PCI/` | Root for the PCI family subsystem. |
| `/dev/family/PCI/ctl` | PCI-specific control (scan bus, etc.). |
| `/dev/family/PCI/{bus}.{dev}.{func}/` | Directory representing a specific PCI device. |
| `/dev/family/PCI/.../config` | Raw PCI configuration space. |
| `/dev/family/PCI/.../bar{N}` | Memory mapped I/O for BARs. |

### 2.2 `/dev/sip/` (System Integration Point)

The SIP device (referenced in Pebble tests) handles token issuance.

| Path | Description |
| :--- | :--- |
| `/dev/sip/issue` | Write to this file to request a new **White** token. |

---

## 3. API Reference

### 3.1 Pebble API (`kernel/pebble.c`)

| Function | Description |
| :--- | :--- |
| `PebbleWhite* pebble_issue_white(PebbleState *ps, void *data, ulong size)` | Issues a new White token (reservation). |
| `int pebble_white_verify(PebbleWhite *white, void **black_handle)` | Verifies a White token, preparing it for allocation. |
| `int pebble_black_alloc(uintptr size, void **handle)` | Allocates physical memory, returning a Black token handle. |
| `int pebble_black_free(void *handle)` | Frees a Black token and its associated memory. |
| `int pebble_red_copy(PebbleBlue *blue, PebbleRed **red)` | Creates a Red snapshot from a Blue reference. |
| `int pebble_blue_discard(PebbleBlue *blue)` | Discards a Blue reference. |

### 3.2 Borrow Checker API (`kernel/borrowchecker.c`)

Implements Rust-like ownership rules for kernel resources.

| Function | Description |
| :--- | :--- |
| `enum BorrowError borrow_acquire(Proc *p, uintptr key)` | Acquires exclusive ownership of a resource key for a process. |
| `enum BorrowError borrow_release(Proc *p, uintptr key)` | Releases ownership. |
| `enum BorrowError borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key)` | Grants shared (read-only) access to another process. |
| `enum BorrowError borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key)` | Grants mutable (exclusive) access to another process. |
| `enum BorrowError borrow_broker_transfer(Proc *sender, Proc *receiver, uintptr phys_addr, struct IdentKey cap)` | Atomically transfers ownership using 2FA capability keys. |

### 3.3 Family API (`kernel/family/family.c`)

| Function | Description |
| :--- | :--- |
| `int family_register(enum DeviceFamily type, struct FamilyOps *ops, char *name)` | Registers a new device family (e.g., PCI) with the system. |
| `struct FamilyExchangePage* family_lookup(enum DeviceFamily type)` | Retrieves the exchange page for a registered family. |
| `uint64_t generate_channel_id(void)` | Generates a system-unique channel ID for device access. |

### 3.4 CLR Integration (`kernel/clr/clr-kernel/clr_pebble_integration.c`)

Maps the CLR Object System onto Pebble tokens.

| Function | Description |
| :--- | :--- |
| `clr_object_t* clr_object_alloc(...)` | Allocates a CLR object by issuing a White token and converting to Black. |
| `PebbleWhite* clr_object_addref(...)` | Adds a reference by issuing a *new* White token pointing to existing Black memory. |
| `int clr_object_snapshot(...)` | Creates a Red token to snapshot an object for speculative execution. |
| `int clr_object_commit(...)` | Commits speculation by discarding the Red snapshot. |
| `int clr_object_rollback(...)` | Rolls back object state using the Red snapshot. |

---

## 4. Implementation Details

### 4.1 Zero-Copy IPC & Speculation
The kernel leverages the **Red/Blue** token mechanism to implement zero-copy IPC and speculative execution.
*   **IPC**: A process can pass a **Blue** token to another process. The receiver gets read access without data copying.
*   **Speculation**: Before modifying a shared object, a **Red** snapshot is taken. If the transaction fails, the state is rolled back using `pebble_red_copy` logic (restoring Blue from Red).

### 4.2 The "Heap is the Game"
In the CLR integration, there is no distinct "Heap". The "Heap" is simply the aggregate state of the Pebble token game.
*   **Reference Counting**: Implemented by counting active **White** tokens for a specific **Black** token.
*   **Safety**: The Borrow Checker ensures that no two processes hold mutable references to the same Black token simultaneously unless explicitly brokered.
