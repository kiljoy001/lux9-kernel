# Lux9 Implementation Roadmap (WASM + Rump Edition)

> **Vision:** A High-Density WASM Hypervisor with a "Messy OS" Driver Layer (NetBSD Rump).
> **Target:** June 2026 (Public Beta / "Sock-Blowing" Demo)

## The 3-Layer Architecture

| Layer | Component | Tech Stack | Role | Status |
| :--- | :--- | :--- | :--- | :--- |
| **L0** | **Microkernel** | C / Assembly | Memory, Isolation, Exchange, blind-ledger | ✅ **Ready** |
| **L1** | **Hypervisor** | WASM3 / WASI | Universal Compute (The "Tenants") | 🚧 **In Progress** |
| **L2** | **Driver Server** | NetBSD (Rump) | "Messy" Hardware Support (USB, TCP/IP, FS) | 📅 **Next Up** |

---

## Phase 1: The Bridge & The Hypervisor (Jan - Feb)

**Goal:** Run **unmodified** WASM applications via a WASI-to-9P Shim.

### 1.1 WASI Implementation (The Bridge)
- [ ] **Wasi-Syscall-Shim:** Implement `wasi_snapshot_preview1` host functions in `kernel/wasm`.
    - Map `fd_read/write` → `9p_read/write`.
    - Map `path_open` → `9p_walk/open`.
    - Map `clock_time_get` → Kernel `ticks`.
- [ ] **WASM Process Init:** Ensure `proc0` can spawn a `.wasm` file directly as a process.

### 1.2 The "Polyglot" Proof
- [ ] **Rust Demo:** Compile `ls.rs` (using `wasi` target) -> Run on Lux9.
- [ ] **Go Demo:** Compile `server.go` (using `tinygo -target=wasi`) -> Run on Lux9.
- [ ] **C Demo:** Compile `hello.c` (using `clang --target=wasm32-wasi`) -> Run on Lux9.

**Milestone (Feb 28):** You can drag-and-drop a standard WASM file from the internet, and it runs in Lux9's terminal.

---

## Phase 2: The "Franken-Kernel" (March - April)

**Goal:** Integrate the **Monolithic NetBSD Rump Kernel** to handle drivers.

### 2.1 The Rump HAL (Hypercall Abstraction Layer)
- [ ] **librumpuser Port:** Implement the Rump HAL interface on top of Lux9.
    - `rumpuser_malloc` → Lux9 `pebble_alloc`.
    - `rumpuser_thread` → Lux9 `rfork(RFPROC)`.
    - `rumpuser_console` → Lux9 `devcons`.
- [ ] **9P Bridge:** Implement a 9P server *inside* the Rump kernel so it exposes its FS/Devices to Lux9.

### 2.2 The Integrated Build
- [ ] Build a single `rump_server.elf` containing:
    - NetBSD TCP/IP Stack.
    - NetBSD USB Stack.
    - NetBSD FFS/ZFS Filesystem.
- [ ] Boot sequence: `Kernel` -> `Rump Server` -> `WASM Init`.

**Milestone (April 30):** Lux9 boots, the Rump Server brings up the real Ethernet card, and a WASM app can make a socket call that is routed to the Rump Server.

---

## Phase 3: The "Sock-Blowing" Optimization (May - June)

**Goal:** Extreme Density & Performance for the June Reveal.

### 3.1 Zero-Copy Exchange Optimization
- [ ] **WASM Shared Memory:** Optimize `fork()` for WASM processes to share read-only code pages (CoW).
- [ ] **9P Zero-Copy:** Ensure `9p_read` from Rump directly maps pages into WASM memory without copying.

### 3.2 The Demo Build
- [ ] **Hypervisor Dashboard:** A visualizer showing 10,000 active processes.
- [ ] **Instant-Start:** Boot -> 500 Web Servers in < 2 seconds.
- [ ] **The "Universal" Demo:** Pipe data from a Python WASM script to a Rust WASM database.

---

## Phase 4: Research Recycling (Post-Beta)

**Goal:** Preserve and modernize the CLR and Symbolic Math systems as isolated userspace servers.

### 4.1 The CLR Server
- [ ] **Migration:** Move `old_cil_to_wasm` (Fruity/CLR) to `userspace/servers/clr`.
- [ ] **WASM Compilation:** Compile the C-based CLR interpreter to WASM.
- [ ] **Role:** Runs legacy C# binaries by JIT-compiling them to WASM at runtime (in userspace).

### 4.2 The Math Server
- [ ] **Symbolic Engine:** Package the Symbolic Math library as a standalone WASM service.
- [ ] **9P Interface:** Expose `/srv/math` to allow other processes to offload heavy symbolic computation.

---

## Phase 5: The Verification Triad (Continuous)

**Goal:** Maintain the "AI-Native" high assurance standard.

### 5.1 Formal Verification (Coq)
- [ ] **Scope:** Critical kernel data structures (Pebble, Exchange, Blind Ledger).
- [ ] **Action:** Update proof `proofs/clr/pebble_clr.v` whenever kernel logic changes.

### 5.2 Static Analysis (Frama-C)
- [ ] **Scope:** C Implementation safety (Pointer validity, Buffer overflows).
- [ ] **Action:** Run `frama-c -load-module ./Plan9.cmxs` on new C code.
- [ ] **ACSL:** Maintain `/*@ ... */` annotations in kernel headers.

---

## Technical Decisions
- **WASI:** Standard `wasi_snapshot_preview1` (upgrade to Preview 2 Component Model later).
- **Rump Kernel:** Monolithic (Single Server) for performance and simplicity.
- **IPC:** 9P is the universal language between L0, L1, and L2.
