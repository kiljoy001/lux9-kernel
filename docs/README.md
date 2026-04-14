# Lux9 Microkernel: Architecture & Implementation Guide

*Version 0.9.0 - Auto-Generated Architectural Audit*

## 1. System Architecture

If you look through the source code, you'll notice Lux9 isn't trying to be a typical Unix clone. It fundamentally rejects two things: standard device drivers and garbage collection.

Instead of drivers, it uses what it calls **Families**. The idea here is that the kernel shouldn't care about the specific vendor of a network card; it only cares that it speaks "PCI" or "USB". The kernel provides a generic routing layer, and "Families" plug into that to handle the specifics.

For memory, it uses **Pebble Physics**. Instead of a heap where you `malloc` and hope for the best, memory allocation is treated like a token economy. You have to "buy" memory with a reservation token (White), and then you "own" it (Black). If you want to share it, you "lend" it (Blue). This makes memory leaks nearly impossible by design because you can't allocate without a reservation, and you can't lose track of a token without breaking the economy rules.

Finally, the whole thing is wrapped in a **Congruent 9P** interface. This means the kernel's internal state isn't just *exposed* as a file system—it *is* a file system. If you see a directory in `/dev/family/PCI`, that directory structure *is* the kernel's data structure for that device.

### Architectural Hierarchy

```text
[ User Space / CLR Runtime ]
         ^
         | 9P / IPC
         v
[ Congruent 9P Router (#F) ] <-> [ /dev/family ]
         |
[ Family HAL Registry ]
    |
    +--- [ PCI Family ] <-> [ Hardware Bus ]
    |       ^
    |       +--- [ Transaction Manager (Multi-Device) ]
    |       +--- [ Channel Manager (Resource Binding) ]
    |
    +--- [ Stubbed Families: USB, I2C, SPI ]
         |
[ Pebble Physics & Borrow Checker ] <--- [ Fruity IR (Analysis) ]
    (Manages all memory tokens and resource ownership)
```

---

## 2. Subsystem Deep Dives

### 2.1 Pebble Physics (Memory Model)
**Files:** `kernel/pebble.c`, `kernel/pebble.h`

The memory model in Lux9 is pretty unique. It's built around the idea that memory management is a state machine, not a janitorial task.

It starts with a **White Token**. This is just a reservation—a promise that you *can* have memory. In the CLR integration code, these White tokens effectively act as object references. Once you verify that token, you can exchange it for a **Black Token**, which is the actual physical memory.

But here is where it gets interesting for concurrency: if you want to let someone else look at your memory, you issue a **Blue Token**. This is a "borrow." The code explicitly tracks these borrows. If you need to try something risky (like a speculative transaction), you create a **Red Token**, which is a snapshot of the Blue token's state. If the transaction fails, the system uses that Red token to roll everything back. It's basically version control for RAM.

### 2.2 The Borrow Checker
**Files:** `kernel/borrowchecker.c`

This is effectively a runtime implementation of Rust's borrow checker, written in C. It uses a global hash map called `borrowpool` to track every single resource key (usually an address).

The rules are strictly enforced: a resource can be `FREE`, `EXCLUSIVE` (owned by one process), `SHARED_OWNED` (read-only by many), or `MUT_LENT` (loaned out mutably). There is a function called `borrow_broker_transfer` that really highlights the security model: you can't just hand off ownership. You need a "Two-Factor" check involving a Capability Key and a Nonce. This prevents a process from accidentally (or maliciously) dumping resources onto a process that didn't ask for them.

### 2.3 Family HAL (Hardware Abstraction)
**Files:** `kernel/family/family.c`, `kernel/family/devfamily.c`

The HAL is designed around a registry. It doesn't know about specific hardware; it knows about protocols. When the system boots, different "Families" (like PCI) register themselves.

Everything interacts through **Channels**. You don't open a "device"; you allocate a Channel ID (which is just a 64-bit integer). This ID binds your process to a specific hardware resource. The `struct FamilyExchangePage` is the key interface here—it's a table of function pointers that lets the generic kernel talk to any registered family without knowing how it works internally.

### 2.4 Fruity IR (Intermediate Representation)
**Files:** `kernel/clr/fruity/fruity_ir.h`

Fruity is the intermediate representation used by the compiler, but it's not just for optimization. It explicitly models the Pebble token game.

The instructions themselves track "Pebble Effects"—things like `creates_white` or `burns_white`. This means the compiler knows exactly how memory tokens are flowing through the program before it even runs. It also has specific operations for the transactional memory model, with opcodes like `CHERRY` (start snapshot) and `BERRY` (end transaction).

---

## 3. The 9P File System Specification

The system exposes a Congruent 9P file server at `/dev/family` (Device `#F`).

### 3.1 Virtual File Tree

```text
/dev/family/
├── ctl                # Global control file (read/write)
├── bus                # Bus listing (read-only)
├── PCI/               # The PCI Family Directory (Implied routing)
│   ├── ctl            # Global PCI control
│   ├── bus            # PCI Bus enumeration
│   ├── 0000.00.1f.2/  # Device Directory (Domain.Bus.Dev.Func)
│   │   ├── config     # Raw Configuration Space (RW)
│   │   ├── ctl        # Device control
│   │   └── ...
└── ...
```

### 3.2 Protocol Behavior

*   **Root Generator (`pcigen` in `pci_9p.c`):**
    *   `Qdir` (Root): Exposes `ctl`, `bus`, and device directories.
    *   `Qdevbase + i`: Device directories map to registry index `i`.
    *   `Qdevconfig + i`: Exposes the raw PCI configuration space. Reads < 256 bytes return config bytes.
    *   `Qdevctl + i`: Exposes device-specific control commands.

---

## 4. API Reference

### 4.1 Global API Functions

| Subsystem | Function | Description |
| :--- | :--- | :--- |
| **Family** | `family_init` | Initializes the registry and locks. |
| **Family** | `family_register` | Registers a `FamilyOps` struct under a generic name (e.g., "PCI"). |
| **Pebble** | `pebble_issue_white` | Creates a reservation token. |
| **Pebble** | `pebble_black_alloc` | Converts a verified White token to physical memory. |
| **Borrow** | `borrow_acquire` | Takes exclusive ownership of a resource key. |
| **Borrow** | `borrow_broker_transfer` | Securely moves ownership between processes. |
| **PCI** | `pcifamily_init` | Scans PCI bus, builds topology, and calls `family_register`. |
| **PCI** | `pci_create_transaction` | Starts a multi-device atomic transaction. |

### 4.2 Key Structures

#### `struct FamilyExchangePage`
The primary interface object for a registered family.
*   `family_type`: Enum identifier (e.g., `FAMILY_PCI`).
*   `ops`: Function pointers (`walk`, `open`, `scan_devices`, etc.).
*   `stats`: Channel and memory usage statistics.
*   `channel_mgr`: Pointer to the channel management subsystem.

#### `struct PCIDeviceDescriptor`
Represents a detected PCI device.
*   `address`: Domain, Bus, Device, Function.
*   `vendor_id` / `device_id`: Hardware identifiers.
*   `bars`: Array of Base Address Registers (validity, type, size).
*   `bound_channel_id`: The 64-bit Family Channel ID currently owning this device.

#### `struct MultiDevicePebbleChain`
Manages atomic transactions across multiple devices.
*   `tx_id`: Unique transaction identifier.
*   `pebbles`: Array of pebble handles involved.
*   `devices`: Array of devices locked in this transaction.

---

## 5. Initialization Sequence

The boot sequence is fairly straightforward, mostly focused on setting up the registries.

1.  **Kernel Boot**: The kernel starts up and calls the subsystem initializers.
2.  **`family_init()`**: This clears out the global `family_registry` and sets up the locks and ID counters. At this point, the HAL is ready but empty.
3.  **`pcifamily_init()`**: This is where the work happens. It allocates the global PCI context and immediately runs `scan_pci_bus(0)` to recursively find every physical device plugged into the machine.
4.  **Registration**: Once the scan is done, it calls `family_register(FAMILY_PCI, ...)` to plug the PCI subsystem into the generic HAL.
5.  **Channel Manager**: Finally, `setup_pci_channel_manager` runs to initialize the allocators. Now the system is ready to hand out Channel IDs to processes that ask for them.

### 5.1 Console Output Evolution During Boot

The Lux9 kernel implements a sophisticated console output evolution during the boot sequence, transitioning from basic UART to full framebuffer console. This evolution ensures reliable output throughout all boot phases.

![Console Evolution](../console_evolution.svg)

**Key Evolution Stages:**

1. **UART Console (Earliest Boot)**
   - **Function**: `i8250console()` (line 593 in `main.c`)
   - **Output**: `uartputs("TEST: main() started\n", 21)` (line 594)
   - **Characteristics**: 
     - Polled I/O at 115200 baud
     - Reliable before memory setup
     - Character-by-character output
     - Used in `mach0init()`, `i8250console()`, early boot logging

2. **Serial Console (Buffered Output)**
   - **Function**: `bootargsinit()` (lines 27-126 in `boot.c`)
   - **Output**: `uartputs("bootargsinit: cmdline found: ", 31)` (line 61)
   - **Characteristics**:
     - UART interrupts enabled
     - Ring buffer support
     - Interrupt-driven I/O
     - Higher throughput than polled mode

3. **Framebuffer Console (High-Speed Output)**
   - **Function**: `bootscreeninit()` → `fbconsoleinit()` (lines 304-311 in `main.c`)
   - **Transition**: `console_ready = 1` (line 309)
   - **Characteristics**:
     - Direct framebuffer access
     - DMA acceleration
     - Pixel-perfect output
     - Rich text formatting

4. **Full Console System**
   - **Smart Boot Logging**: `boot_log()` function (lines 86-100 in `main.c`)
   - **Auto-Switching**: Automatically selects UART vs `print()` based on `console_ready` flag
   - **Seamless Transition**: Preserves all output during console evolution

**Console Ready State Machine:**
- `console_ready = 0`: Only UART output available (`uartputs`)
- `console_ready = 1`: Framebuffer + `print()` available
- `boot_log()` automatically handles the transition

This evolution ensures that debug output is never lost, even during the critical early boot phases when only UART is available.

---

## 6. Formal Verification (Coq)

The developers didn't just write this code and hope it works; they proved parts of it. In the `proofs/` directory, there are Coq files like `sip_model.v`.

These files formally define the semantics of the System Integration Point (SIP) and the Borrow Checker. They use machine integers (Z) to model PIDs and Page IDs, and they define the exact states (`Exclusive`, `SharedOwned`, etc.) that we see in the C code.

The proofs explicitly verify the state transitions. For example, they prove that if you transition from `Exclusive` to `SharedOwned` using `BorrowShared`, you cannot accidentally end up with two exclusive owners. It's a mathematical guarantee that the C code's logic is sound.

---

## 7. Security Architecture

Looking at the code, specifically `kernel/borrowchecker.c` and `kernel/pebble.c`, the security model relies on strict ownership tracking rather than traditional access control lists (ACLs).

**Observed Mechanisms:**

1.  **Runtime Ownership Enforcement:**
    The kernel implements a "Borrow Checker" in C. Before a process can touch a resource (like a memory page or a lock), it must hold a valid record in the `borrowpool`. If `borrow_acquire` sees that a resource is already `EXCLUSIVE` to another process, it denies access immediately. This moves typical compile-time safety checks into the runtime environment.

2.  **The Token Game:**
    Memory isn't just allocated; it's exchanged. In `pebble.c`, we see that a process must first "reserve" capability (White token) before it can "own" memory (Black token). This two-step process means a process cannot simply grab memory; it must have a valid reservation first, which limits the blast radius of runaway allocations.

3.  **Two-Factor Ownership Transfer:**
    There is a specific function, `borrow_broker_transfer`, which handles moving a resource from Process A to Process B. It doesn't just change the owner field; it checks a `key_cap` (Capability Key) and a `nonce`. This suggests that simply knowing the address of a resource isn't enough to take it over—you need the cryptographic capability token to prove you are the intended recipient.

4.  **Hardware Isolation:**
    The `pci_channel.c` file shows that devices are bound to "Channels." Once a device is bound to a channel ID, other processes cannot access it without holding that specific channel handle. This creates hard isolation between different drivers or subsystems trying to talk to the same hardware.

**In Summary:**
The system trades the flexibility of "anyone can access anything if they have root" for a strict "you can only touch what you hold a token for" model. It prevents entire classes of race conditions and use-after-free bugs by making them impossible to represent in the kernel's internal state machine.