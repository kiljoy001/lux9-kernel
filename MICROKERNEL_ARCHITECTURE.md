# Lux9 Microkernel Architecture

## Executive Summary

Lux9 uses a **3-layer OSI-style microkernel architecture** where the kernel is a minimal hypervisor and all OS services (including WASM execution, HAL, file servers, and future CIL/QBE/SLJIT runtimes) run as **isolated Layer 2 servers** communicating via **9P protocol + exchange pages**.

**Critical Design Principle**: "WASM has no superpowers" - it's strictly an execution environment with no OS abilities. All system services run under capability-enforced boundaries.

## Architecture Layers

```
┌─────────────────────────────────────────────────┐
│  Layer 3+: User Applications                   │
│  - shell, user programs                        │
│  - symbolic computing, development tools       │
│  - CIL programs compiled to WASM               │
└────────────────┬────────────────────────────────┘
                 │ 9P protocol (Tread/Twrite)
┌────────────────▼────────────────────────────────┐
│  Layer 2: System Services (ALL OS BASICS!)     │
│                                                 │
│  ┌─────────────────────────────────────────┐   │
│  │  init (BOOTSTRAPS everything!)          │   │
│  │  - First process started by kernel      │   │
│  │  - Starts resurrection server            │   │
│  └────────────┬────────────────────────────┘   │
│               │ spawns                          │
│               ▼                                 │
│  ┌─────────────────────────────────────────┐   │
│  │  Resurrection Server (native C)         │   │
│  │  - Monitors/restarts other Layer 2 svrs │   │
│  └────────────┬────────────────────────────┘   │
│               │ manages                         │
│               ▼                                 │
│  ┌──────────────────────┐  ┌─────────────────┐ │
│  │  WASM Server         │  │  HAL Server     │ │
│  │  (WASM module!)      │  │  (WASM module!) │ │
│  │  - Manages /wasm/    │  │  - Family system│ │
│  │  - Instance mgmt     │  │  - PCI/USB/I2C  │ │
│  └──────────┬───────────┘  └────────┬────────┘ │
│             │                       │          │
│  ┌──────────▼───────────┐  ┌────────▼────────┐ │
│  │  File Server         │  │  Capability Svr │ │
│  │  (WASM module!)      │  │  (WASM module!) │ │
│  │  - Persistent storage│  │  - User-facing  │ │
│  └──────────┬───────────┘  └────────┬────────┘ │
│             │                       │          │
│  ┌──────────▼───────────┐  ┌────────▼────────┐ │
│  │  CIL Runtime Server  │  │  QBE/SLJIT Svr  │ │
│  │  (WASM module!)      │  │  (WASM module!) │ │
│  │  - CIL → WASM        │  │  - JIT compiler │ │
│  │  - Execution         │  │  - Optimization │ │
│  └──────────────────────┘  └─────────────────┘ │
│                                                 │
│  ALL Layer 2 services communicate via           │
│  9P protocol + exchange pages                   │
└─────────────┼───────────────────────────────────┘
              │ Tsyscall/Rsyscall via exchange pages
┌─────────────▼───────────────────────────────────┐
│  Layer 1: Kernel (MINIMAL HYPERVISOR!)         │
│  ┌──────────────────────────────────────────┐  │
│  │  Memory Management                       │  │
│  │  - mmap/munmap for WASM linear memory    │  │
│  │  - Exchange page allocation              │  │
│  │  - Borrow checker                        │  │
│  ├──────────────────────────────────────────┤  │
│  │  CPU Scheduling                          │  │
│  │  - Schedule Layer 2 servers as processes │  │
│  │  - Preemption, timeslicing               │  │
│  ├──────────────────────────────────────────┤  │
│  │  Resource Limits                         │  │
│  │  - CPU quotas, memory limits             │  │
│  │  - Capability enforcement                │  │
│  ├──────────────────────────────────────────┤  │
│  │  IPC Primitives                          │  │
│  │  - Exchange pages                        │  │
│  │  - 9P message routing                    │  │
│  │  - Doorbells                             │  │
│  ├──────────────────────────────────────────┤  │
│  │  WASM3 Runtime (isolated)                │  │
│  │  - Executes Layer 2 WASM modules         │  │
│  │  - Capability-restricted                 │  │
│  │  - NO direct kernel access               │  │
│  └──────────────────────────────────────────┘  │
│                                                 │
│  Family system REMOVED from kernel (moved to   │
│  Layer 2 HAL server!) - Kernel simplified!     │
└─────────────────────────────────────────────────┘
```

## Current Status (Phase 0 Starting)

### What Works
- ✅ **9P Protocol**: Complete Tread/Twrite/Tsyscall/Rsyscall implementation
- ✅ **Exchange Pages**: Zero-copy IPC with capability protection
- ✅ **Capability System**: UUID-based monotonic derivation chains
- ✅ **Borrow Checker**: Rust-style ownership tracking
- ✅ **WASM3 Runtime**: v0.5.1 integrated (10,519 lines)
- ✅ **Family System**: Comprehensive HAL supporting PCI/USB/I2C/SPI/DMA/IRQ/TPM
- ✅ **Userspace init**: Working example using Tsyscall messages
- ✅ **msgord**: Message ordering infrastructure

### What's Happening Now (Phase 0)
- **Splitting WASM3** into:
  - **Layer 1**: Isolated runtime (kernel execution, capability-restricted)
  - **Layer 2**: WASM server AS WASM MODULE (manages /wasm/, delegates to Layer 1)

### Current Integration Files
Located in `kernel/wasm/`:
- `wasm_fileserver.c` - Current monolithic server (being split)
- `wasm_9p_integration.c` - 9P + capability validation (reusing)
- `wasm_capability_bindings.c` - WASM import functions (reusing)
- `wasm_runtime/wasm3/*.c` - Core wasm3 runtime (moving to isolated segment)

## Communication Model

**CRITICAL**: No custom syscalls, no protocol extensions. Use existing infrastructure:

### 9P Message Types (Already Implemented)
- `Tread/Rread` - Read from filesystem
- `Twrite/Rwrite` - Write to filesystem
- `Tsyscall/Rsyscall` - System calls via exchange pages
- All other standard 9P operations

### Tsyscall Messages (scallnr values)
```c
// Existing syscalls (use these):
#define SYS_MMAP      1   // Allocate memory
#define SYS_MUNMAP    2   // Free memory
#define SYS_READ      3   // Read from fd
#define SYS_WRITE     4   // Write to fd

// Add for Layer 2 servers (just new constants, no protocol changes):
#define SYS_WASM_EXECUTE  100  // Execute WASM function
#define SYS_MMAP_DEVICE   101  // Map physical memory (for HAL)
#define HAL_GPIO_SET      200  // GPIO control (HAL server)
#define HAL_I2C_READ      201  // I2C read (HAL server)
// ... etc
```

### Layer 2 ↔ Layer 1 Communication Pattern
```c
// Example: WASM server allocates memory via Tsyscall

// 1. Build Tsyscall message
Fcall tx = {
    .type = Tsyscall,
    .scallnr = SYS_MMAP,
    .scount = 8,  // size of sdata
    // sdata: [size:8]
};

// 2. Marshal to exchange page
convS2M(&tx, exchange_page, PAGE_SIZE);

// 3. Ring doorbell
ctl->doorbell = 1;
__asm__ volatile("syscall");

// 4. Wait for Rsyscall reply
Fcall rx;
convM2S(exchange_page, PAGE_SIZE, &rx);

// 5. Get allocated address
void *addr = (void *)rx.retval;
```

## Phase 0: WASM Split (SECURITY FOUNDATION!)

**Status**: Starting now

**Why First**: Establishes capability boundaries BEFORE deploying resurrection server. If resurrection server is built while everything is kernel-privileged, it becomes an attack vector.

### Tasks

1. **Isolate wasm3 runtime in kernel** (Layer 1):
   ```c
   // kernel/wasm/wasm_runtime.c (new file)

   typedef struct WasmRuntime {
       Segment *wasm_seg;       // Isolated address space
       Capability *cap;         // Limited capabilities
       IM3Environment *env;     // wasm3 environment
       Lock lock;
   } WasmRuntime;

   // Kernel exposes wasm3 via Tsyscall messages
   void sys_wasm_execute(Fcall *tx) {
       // tx->sdata: [module_id:8] [function:str] [args:n]

       // Check capability
       if (!cap_check(up->capability, PERM_WASM_EXECUTE))
           error("no wasm execute permission");

       // Execute in isolated wasm3 runtime
       M3Result result = m3_Call(wasm_runtime.env, ...);

       // Send Rsyscall reply
       Fcall rx = {.type = Rsyscall, .retval = result};
       send_reply(up->session, &rx);
   }
   ```

2. **Create Layer 2 WASM Server** (as WASM module):
   ```rust
   // userspace/wasm_server/wasm_server.rs
   // Compiled to wasm_server.wasm!

   use wasm_bindgen::prelude::*;

   #[wasm_bindgen]
   pub struct WasmServer {
       instances: HashMap<u64, Instance>,
   }

   #[wasm_bindgen]
   impl WasmServer {
       // Called via 9P write to /wasm/ctl
       pub fn create_instance(&mut self, module_path: &str) -> u64 {
           // Load module from filesystem (via 9P)
           let module_bytes = read_file(module_path);

           // Ask Layer 1 to compile (via Tsyscall)
           syscall_wasm_compile(instance_id, &module_bytes);

           self.instances.insert(instance_id, Instance::new());
           instance_id
       }

       // Exports 9P operations
       #[wasm_bindgen]
       pub fn p9_read(qid_path: u64, offset: u64, count: u32) -> Vec<u8> {
           // Read from instance stdout, etc.
       }

       #[wasm_bindgen]
       pub fn p9_write(qid_path: u64, data: &[u8]) -> u32 {
           // Write to instance stdin, control, etc.
       }
   }
   ```

3. **Reuse existing infrastructure**:
   - Exchange pages (already implemented)
   - 9P routing (kernel/9p_router.c)
   - Capability validation (wasm_9p_integration.c)
   - Borrow checker enforcement

## Phase 1: Resurrection Server

**Status**: After Phase 0

**Purpose**: MINIX-style service monitor that restarts crashed Layer 2 servers

**Why After Phase 0**: Runs as native C at Layer 2, WITHOUT kernel privileges. Needs capability model established first.

### Key Properties
- Monitors all Layer 2 servers via 9P control files
- Checkpoints WASM server state (WASM linear memory snapshots)
- Restarts from last known good state
- Enforces service dependencies (e.g., HAL before file server)

## Future Layer 2 Servers

### CIL Runtime Server
Based on previous research (`qbe + sljit` integration):

```rust
// userspace/cil_server/cil_server.rs
// Compiled to cil_server.wasm!

#[wasm_bindgen]
pub struct CilServer {
    assemblies: HashMap<u64, Assembly>,
}

#[wasm_bindgen]
impl CilServer {
    // Load CIL assembly
    pub fn load_assembly(&mut self, dll_path: &str) -> u64 {
        // Parse CIL bytecode
        // Convert to WASM via CIL → WASM compiler
        // Delegate execution to Layer 1 wasm3 runtime
    }

    // Execute CIL method
    pub fn invoke_method(&mut self, asm_id: u64, method: &str) -> Vec<u8> {
        // Translate CIL call to WASM function call
        // Use Tsyscall(SYS_WASM_EXECUTE)
    }
}
```

**Mounts at**: `/cil/assemblies/`, `/cil/instances/`

### QBE/SLJIT JIT Server
Based on previous QBE SSA IR and SLJIT integration:

```rust
// userspace/jit_server/jit_server.rs
// Compiled to jit_server.wasm!

#[wasm_bindgen]
pub struct JitServer {
    ir_modules: HashMap<u64, QbeModule>,
}

#[wasm_bindgen]
impl JitServer {
    // Compile QBE IR to native code via SLJIT
    pub fn compile_qbe(&mut self, ir: &str) -> u64 {
        // Parse QBE SSA IR
        // Generate SLJIT instructions
        // Allocate executable memory via Tsyscall(SYS_MMAP)
        // Return compiled function handle
    }

    // Execute JIT-compiled code
    pub fn execute(&mut self, func_id: u64, args: &[u8]) -> Vec<u8> {
        // Call compiled function
        // Sandboxed via capability enforcement
    }
}
```

**Mounts at**: `/jit/modules/`, `/jit/exec/`

### Symbolic Computing Server
For theorem proving, symbolic math:

```rust
// userspace/symbolic_server/symbolic_server.rs

#[wasm_bindgen]
pub struct SymbolicServer {
    proofs: HashMap<u64, ProofState>,
}

#[wasm_bindgen]
impl SymbolicServer {
    // Load Coq proof
    pub fn load_proof(&mut self, proof_path: &str) -> u64 {
        // Parse Coq vernacular
        // Track proof state
    }

    // Execute tactic
    pub fn apply_tactic(&mut self, proof_id: u64, tactic: &str) -> String {
        // Apply Coq tactic
        // Return new proof state
    }
}
```

**Mounts at**: `/symbolic/proofs/`, `/symbolic/compute/`

## Filesystem Layout

```
/
├── wasm/                      # WASM Server (Layer 2)
│   ├── modules/
│   │   ├── hello.wasm
│   │   ├── calculator.wasm
│   │   └── cil_server.wasm    # CIL runtime AS WASM
│   └── instances/
│       └── hello.0/
│           ├── ctl            # Control: start/stop/status
│           ├── mem            # Linear memory (r/w)
│           ├── stdin          # Input stream
│           ├── stdout         # Output stream
│           └── stderr         # Error stream
│
├── dev/
│   └── hal/                   # HAL Server (Layer 2, WASM module)
│       ├── pci/
│       │   ├── 0000:00:1f.2/
│       │   │   ├── config     # PCI config space
│       │   │   ├── bar0       # BAR mappings
│       │   │   └── ctl        # Device control
│       │   └── devices        # List all PCI devices
│       ├── usb/
│       ├── i2c/
│       └── gpio/
│
├── cil/                       # CIL Runtime Server (Layer 2)
│   ├── assemblies/
│   │   └── myapp.dll
│   └── instances/
│       └── myapp.0/
│           ├── ctl
│           └── stdout
│
├── jit/                       # QBE/SLJIT Server (Layer 2)
│   ├── modules/
│   │   └── optimized.qbe
│   └── exec/
│       └── func123/ctl
│
└── srv/
    ├── resurrection           # Resurrection Server
    ├── cap                    # Capability Server
    └── compute                # Symbolic Computing
```

## Security Model

### Capability Enforcement

All Layer 2 servers run under capability restrictions:

```c
// Example: HAL server needs HARDWARE_ACCESS capability

// At boot (in init):
Capability *hal_cap = cap_create_module("hal_server");
cap_add_permission(hal_cap, PERM_HARDWARE_ACCESS);

// When HAL server calls SYS_MMAP_DEVICE:
void sys_mmap_device(Fcall *tx) {
    // Check capability BEFORE mapping hardware
    if (!cap_check(up->capability, PERM_HARDWARE_ACCESS))
        error("permission denied");

    // Map physical memory to Layer 2 process
    void *vaddr = map_physical_to_layer2(phys_addr, size, prot);

    Fcall rx = {.type = Rsyscall, .retval = (long)vaddr};
    send_reply(up->session, &rx);
}
```

### Isolation Properties
- ❌ Layer 2 servers CANNOT access kernel memory
- ❌ Layer 2 servers CANNOT bypass capabilities
- ❌ Layer 2 server crash does NOT crash kernel
- ✅ Layer 2 servers communicate ONLY via 9P + exchange pages
- ✅ Resurrection server restarts crashed services

## Bootstrap Sequence

```
1. Kernel Boot (Layer 1):
   - Initialize memory management
   - Initialize CPU scheduler
   - Start init as first userspace process (Layer 2)

2. init Starts (Layer 2):
   - Mount root filesystem
   - Spawn resurrection server
   - Wait for resurrection to mount /srv/resurrection

3. Resurrection Server (Layer 2):
   - Mount 9P server at /srv/resurrection
   - Register critical services:
     * wasm_server.wasm
     * hal_server.wasm
     * file_server.wasm
     * capability_server.wasm
   - Start all services
   - Monitor and restart on crash

4. Layer 2 Servers Start:
   - WASM server mounts /wasm
   - HAL server mounts /dev/hal
   - File server mounts /
   - Each exports 9P filesystem interface

5. Layer 3+ Applications:
   - shell uses /wasm, /dev/hal, / via 9P
   - User programs access services via standard file operations
```

## Implementation Status

### Completed
- [x] 9P protocol implementation
- [x] Exchange pages + borrow checker
- [x] Capability system with monotonic derivation
- [x] WASM3 runtime integration (v0.5.1)
- [x] Family system (comprehensive HAL)
- [x] Userspace init working with Tsyscall
- [x] Architecture plan documented

### Phase 0: WASM Split (In Progress)
- [ ] Isolate wasm3 runtime in kernel segment
- [ ] Implement Tsyscall(SYS_WASM_EXECUTE) handler
- [ ] Create Layer 2 WASM server (as WASM module)
- [ ] Provide 9P filesystem interface from Layer 2
- [ ] Restrict wasm3 to exchange page communication
- [ ] Capability enforcement at Tsyscall boundary

### Phase 1: Resurrection Server (Next)
- [ ] Build native C resurrection server
- [ ] Service registration and monitoring
- [ ] WASM checkpoint/restore mechanism
- [ ] 9P control interface (/srv/resurrection)

### Phase 2-5: See MICROKERNEL_ARCHITECTURE.md

### Future Phases
- [ ] CIL Runtime Server (reuse CIL → WASM research)
- [ ] QBE/SLJIT JIT Server (reuse QBE + SLJIT integration)
- [ ] Symbolic Computing Server
- [ ] Additional Layer 2 services as needed

## Performance Considerations

### Overhead Sources
- Syscall cost: ~100-200ns per Tsyscall (acceptable for microkernel)
- IPC overhead: Exchange page writes (zero-copy design mitigates)
- Context switches: Layer 1 ↔ Layer 2 transitions

### Optimizations
- ✅ Exchange pages eliminate memory copying
- ✅ msgord provides lock-free message ordering
- ✅ Batch operations where possible
- ✅ JIT compilation in wasm3 runtime (fast execution)

## Developer Guide

### Adding a New Layer 2 Server

1. **Create server as WASM module**:
   ```bash
   cd userspace/myserver
   cargo new --lib myserver
   # Add wasm-bindgen dependency
   ```

2. **Implement 9P exports**:
   ```rust
   #[wasm_bindgen]
   pub fn p9_read(qid_path: u64, offset: u64, count: u32) -> Vec<u8> {
       // Your read implementation
   }

   #[wasm_bindgen]
   pub fn p9_write(qid_path: u64, data: &[u8]) -> u32 {
       // Your write implementation
   }
   ```

3. **Build to WASM**:
   ```bash
   cargo build --target wasm32-unknown-unknown --release
   ```

4. **Register with resurrection server**:
   ```bash
   echo "register myserver /boot/myserver.wasm" > /srv/resurrection/ctl
   ```

5. **Start service**:
   ```bash
   echo "start myserver" > /srv/resurrection/ctl
   ```

### Debugging Layer 2 Servers

Since servers are WASM modules:
- ✅ Can inspect WASM linear memory
- ✅ Can checkpoint entire server state
- ✅ Can restart from last checkpoint
- ✅ Deterministic execution (WASM)
- ✅ No C code corruption issues

## References

### Key Files
- `kernel/9p_router.c` - 9P message routing
- `kernel/libc9/convS2M.c` - 9P message serialization
- `kernel/wasm/` - Current WASM3 integration (being split)
- `kernel/family/` - Family system (moving to Layer 2)
- `userspace/lib9p_syscall/init.c` - Tsyscall usage example

### Architecture Documents
- `/home/scott/.claude/plans/shiny-hopping-yeti.md` - Detailed implementation plan
- `MICROKERNEL_ARCHITECTURE.md` - This document

### Related Research
- CIL execution via QBE + SLJIT
- CIL to WASM compilation
- Plan 9 9P protocol
- Pebble capability system
- MINIX resurrection server
- Rust borrow checker

## Contact

For questions about this architecture, see:
- Architecture plan: `/home/scott/.claude/plans/shiny-hopping-yeti.md`
- Current work: Phase 0 (WASM split) in progress
- Next agent: Continue from Phase 0 split implementation
