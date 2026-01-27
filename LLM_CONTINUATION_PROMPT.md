# LLM Continuation Prompt: WASM Microkernel Implementation

**Use this prompt to continue the WASM microkernel architecture implementation work.**

---

## Overall Goal

Transform the lux9-kernel into a **3-layer microkernel architecture** where WASM is the primary executable format. The kernel is a Plan 9-based OS with 9P protocol for all communication.

### Critical Context

1. **This is a Plan 9-based kernel**, not Linux
2. **WASM is the MAIN executable format**, not a plugin system
3. **Security-first design**: Phase ordering matters (establish capability boundaries BEFORE resurrection server)
4. **9P protocol for everything**: All communication via Tsyscall/Rsyscall messages over exchange pages
5. **WASM-as-processes architecture**: WASM programs run as first-class processes (not separate instance table)

---

## The 3-Layer Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Layer 3+: User Applications                           │
│  - shell, user programs, development tools             │
└────────────────┬────────────────────────────────────────┘
                 │ 9P protocol (Tread/Twrite)
┌────────────────▼────────────────────────────────────────┐
│  Layer 2: System Services (ALL AS WASM MODULES!)       │
│                                                         │
│  init (native C) → starts resurrection server →        │
│    ├─ Resurrection Server (native C)                   │
│    ├─ WASM Server (WASM module!) - manages /wasm/      │
│    ├─ HAL Server (WASM module!) - hardware access      │
│    ├─ File Server (WASM module!) - filesystem          │
│    └─ Capability Server (WASM module!) - caps          │
│                                                         │
│  All communicate via 9P + exchange pages                │
└────────────────┬────────────────────────────────────────┘
                 │ Tsyscall/Rsyscall
┌────────────────▼────────────────────────────────────────┐
│  Layer 1: Kernel (MINIMAL HYPERVISOR!)                 │
│  - Memory management (mmap, segments, borrow checker)  │
│  - CPU scheduling                                       │
│  - Resource limits (quotas, capabilities)              │
│  - IPC primitives (exchange pages, doorbells)          │
│  - wasm3 runtime (ISOLATED, capability-restricted)     │
│                                                         │
│  NO family system in kernel (moves to Layer 2!)        │
└─────────────────────────────────────────────────────────┘
```

### Bootstrap Sequence

1. **Kernel boots** → initializes wasm3 runtime (Layer 1)
2. **init starts** (first Layer 2 process, native C)
3. **init spawns resurrection server** (Layer 2, native C)
4. **Resurrection server starts other Layer 2 services** (all WASM modules!)
5. **Layer 3+ applications use Layer 2 services** via 9P

---

## Phase Plan (Security-First Ordering!)

**CRITICAL**: Phase 0 MUST come before Phase 1 to establish capability boundaries!

### Phase 0: WASM Split (Layer 1 runtime + Layer 2 server) - **IN PROGRESS (75% complete)**
**Goal**: Separate mechanism (Layer 1 runtime) from policy (Layer 2 server)
**Status**: Layer 1 refactored to WASM-as-processes, Layer 2 server pending

### Phase 1: Build Resurrection Server (AFTER Phase 0)
**Goal**: Monitor/restart Layer 2 services
**Status**: Not started

### Phase 2: 9P Filesystem for WASM
**Goal**: Implement `/wasm/` filesystem interface
**Status**: Not started

### Phase 3: Move Family System to Layer 2 HAL
**Goal**: Remove hardware abstraction from kernel
**Status**: Not started

### Phase 4: Resource Quotas
**Goal**: Enforce CPU/memory limits
**Status**: Not started

### Phase 5: Integration Testing
**Goal**: End-to-end validation
**Status**: Not started

---

## Critical Architectural Decision: WASM-as-Processes

### The Problem (User's Question)

Original implementation used a **separate instance table**:
```c
static WasmInstance instance_table[256];  // ← WRONG for server OS!
```

**User asked**: *"Why not use our proc and integrate wasm as processes into that instead of a separate system?"*

### The Solution (Implemented)

**WASM programs ARE processes**, not instance table entries:

```c
/* kernel/include/portdat.h */
struct Proc {
    ... existing fields (capabilities, seg[], state, etc.) ...

    /* WASM execution context (only for WASM processes) */
    struct {
        int initialized;         /* 1 if WASM process, 0 for native */
        void *runtime;           /* IM3Runtime - wasm3 runtime */
        void *module;            /* IM3Module - loaded WASM module */
        u8int *linear_memory;    /* WASM linear memory */
        u32int memory_size;
        u32int memory_pages;
    } wasm;
};
```

### Why This Is Correct

✅ **Reuses existing infrastructure**:
- `up->capabilities` for permission checks
- `up->seg[LSEG]` for WASM linear memory
- `up->state` for process state
- Existing scheduler, memory allocator, resource limits

✅ **No artificial limits**: As many WASM processes as kernel can handle

✅ **Standard process lifecycle**:
- `exec("hello.wasm")` creates WASM process
- WASM processes show up in `ps`
- Can `fork()`, `wait()`, `kill()` WASM processes
- `/proc/N/mem` can read WASM linear memory

✅ **Simpler**: 100 fewer lines of code

**See**: `ADR_WASM_AS_PROCESSES.md` for full rationale

---

## What Has Been Accomplished

### ✅ Completed

1. **Architecture Documentation** (`MICROKERNEL_ARCHITECTURE.md`):
   - 3-layer model defined
   - Bootstrap sequence documented
   - Shows how previous research (CIL/QBE/SLJIT) will become Layer 2 servers

2. **Layer 1 WASM Runtime** (`kernel/wasm/wasm_runtime.c`):
   - Isolated wasm3 runtime in kernel (443 lines)
   - Three Tsyscall handlers: SYS_WASM_COMPILE, SYS_WASM_EXECUTE, SYS_WASM_DESTROY
   - Capability enforcement at Tsyscall boundary

3. **Proc Structure Extended** (`kernel/include/portdat.h`):
   - Added `wasm` struct to `Proc` (48 bytes per process)
   - Zero overhead for native processes

4. **Refactored to WASM-as-Processes**:
   - Removed instance table and management functions
   - Handlers now use `up->wasm` instead of instance lookup
   - Message format simplified (no instance_id needed)

5. **Boot Integration** (`kernel/9front-pc64/main.c`):
   - Calls `wasm_runtime_init()` during kernel boot
   - Prints initialization messages

6. **9P Routing** (`kernel/9p_router.c`):
   - Routes SYS_WASM_* syscalls to handlers
   - Integrated with existing Tsyscall infrastructure

7. **Build System** (`kernel/Makefile`):
   - WASM files added to build
   - Compilation rules for wasm/*.c files

8. **Compilation**: ✅ `wasm_runtime.o` compiles cleanly

### 📄 Documentation Created

- **`MICROKERNEL_ARCHITECTURE.md`** - Overall architecture, bootstrap sequence, Layer 2 services
- **`PHASE0_STATUS.md`** - Phase 0 implementation status, what's done, what's next
- **`ADR_WASM_AS_PROCESSES.md`** - Architecture decision record for WASM-as-processes
- **`SESSION_SUMMARY_WASM_AS_PROCESSES.md`** - Refactoring session summary
- **`kernel/wasm/TODO_LAYER1_FIXES.md`** - Technical notes on Layer 1 fixes
- **`LLM_CONTINUATION_PROMPT.md`** - This file!

---

## What Needs To Be Done Next

### Immediate: Complete Phase 0

#### 1. Test Full Kernel Build (15 minutes)

```bash
cd /home/scott/Repo/lux9-kernel/kernel
make clean && make -j4
```

**Expected**: Kernel builds with WASM Layer 1 integrated
**Expected output**: "=== WASM3 Runtime Initialized ===" during boot
**If errors**: Fix compilation issues, likely header conflicts or missing declarations

#### 2. Implement exec_wasm_module() (2-3 hours)

Create new file: `kernel/wasm/wasm_exec.c`

**Goal**: Detect WASM binaries in exec() and initialize as WASM processes

```c
/* kernel/wasm/wasm_exec.c */

#include "../include/u.h"
#include "../include/portlib.h"
#include "../include/dat.h"
#include "../include/fns.h"

/* Detect WASM binary and execute */
void exec_wasm_module(Chan *c, char **argv) {
    /* Read WASM module into memory */
    ulong size = c->qid.length;
    u8int *module_bytes = mallocz(size, 1);
    devtab[c->type]->read(c, module_bytes, size, 0);

    /* Initialize wasm3 runtime for THIS process */
    extern WasmRuntime wasm_runtime;  // Global env from wasm_runtime.c

    up->wasm.runtime = m3_NewRuntime(wasm_runtime.env, 64 * 1024, nil);
    if (!up->wasm.runtime)
        error("failed to create WASM runtime");

    /* Parse and load WASM module */
    M3Result result = m3_ParseModule(wasm_runtime.env,
                                     (IM3Module*)&up->wasm.module,
                                     module_bytes, size);
    if (result) {
        m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
        free(module_bytes);
        error("WASM parse failed");
    }

    result = m3_LoadModule((IM3Runtime)up->wasm.runtime,
                          (IM3Module)up->wasm.module);
    if (result) {
        m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
        free(module_bytes);
        error("WASM load failed");
    }

    /* Get linear memory and map to segment */
    up->wasm.linear_memory = m3_GetMemory((IM3Runtime)up->wasm.runtime,
                                          &up->wasm.memory_size, 0);
    up->wasm.memory_pages = up->wasm.memory_size / (64 * 1024);

    /* TODO: Map to seg[LSEG] for proper isolation */
    // up->seg[LSEG] = newseg(SG_SHARED, (uintptr)up->wasm.linear_memory,
    //                        up->wasm.memory_size);

    up->wasm.initialized = 1;
    free(module_bytes);  /* Module copied into wasm3 runtime */

    /* Find _start function */
    IM3Function start_func;
    result = m3_FindFunction(&start_func, (IM3Runtime)up->wasm.runtime, "_start");
    if (result) {
        /* Try main() if _start not found */
        result = m3_FindFunction(&start_func, (IM3Runtime)up->wasm.runtime, "main");
        if (result)
            error("no _start or main function in WASM module");
    }

    /* Execute WASM _start() - this becomes the process's main loop */
    result = m3_CallV(start_func);
    if (result) {
        print("WASM execution failed: %s\n", result);
        exits("WASM error");
    }

    /* _start returned - process exits normally */
    exits(nil);
}
```

**Modify `kernel/9front-port/sysproc.c`**:

```c
/* In sys_exec() function, add WASM detection: */

void sys_exec(char *name, char **argv) {
    Chan *c = namec(name, Aopen, OEXEC, 0);

    /* Read magic number (first 4 bytes) */
    uchar magic[4];
    devtab[c->type]->read(c, magic, 4, 0);

    if (magic[0] == 0x00 && magic[1] == 'a' &&
        magic[2] == 's' && magic[3] == 'm') {
        /* WASM module - initialize WASM execution */
        extern void exec_wasm_module(Chan *, char **);
        exec_wasm_module(c, argv);
    } else if (magic[0] == 0x7f && magic[1] == 'E' &&
               magic[2] == 'L' && magic[3] == 'F') {
        /* Native ELF binary */
        // ... existing ELF exec code ...
    } else {
        error("unknown executable format");
    }
}
```

**Add to Makefile**:
```makefile
WASM_C := wasm/wasm_runtime.c wasm/wasm_exec.c ...
WASM_O := wasm_runtime.o wasm_exec.o ...
```

#### 3. Add WASM Cleanup to pexit() (30 minutes)

**Modify `kernel/9front-port/proc.c`**:

```c
void pexit(char *note, int freemem) {
    ... existing cleanup (close files, free segments, etc.) ...

    /* Clean up WASM runtime if this was a WASM process */
    if (up->wasm.initialized) {
        if (up->wasm.runtime) {
            /* Include wasm3 header */
            extern void m3_FreeRuntime(void *);
            m3_FreeRuntime(up->wasm.runtime);
            up->wasm.runtime = nil;
            up->wasm.module = nil;
        }
        up->wasm.linear_memory = nil;
        up->wasm.memory_size = 0;
        up->wasm.memory_pages = 0;
        up->wasm.initialized = 0;
    }

    ... rest of exit cleanup ...
}
```

#### 4. Create Layer 2 WASM Server (1-2 days)

**Goal**: Layer 2 WASM server is ITSELF a WASM module that manages other WASM modules!

Create: `userspace/wasm_server/wasm_server.rs`

```rust
// userspace/wasm_server/wasm_server.rs
// Compiled to wasm_server.wasm and run by Layer 1 runtime!

use wasm_bindgen::prelude::*;
use std::collections::HashMap;

#[wasm_bindgen]
pub struct WasmServer {
    instances: HashMap<u64, Instance>,
}

struct Instance {
    pid: u64,           // Process ID of WASM process
    module_path: String,
    state: InstanceState,
}

enum InstanceState {
    Loading,
    Ready,
    Running,
    Exited,
}

#[wasm_bindgen]
impl WasmServer {
    pub fn new() -> WasmServer {
        WasmServer {
            instances: HashMap::new(),
        }
    }

    // Called via 9P write to /wasm/ctl
    pub fn create_instance(&mut self, module_path: &str) -> u64 {
        let instance_id = next_id();

        // Load module from filesystem (via 9P read)
        let module_bytes = read_file(module_path);

        // Fork and exec WASM module as new process
        let pid = fork();
        if pid == 0 {
            // Child process: exec WASM module
            exec(module_path, &[]);
            // If exec returns, error
            exit(1);
        }

        // Parent: track the instance
        self.instances.insert(instance_id, Instance {
            pid,
            module_path: module_path.to_string(),
            state: InstanceState::Loading,
        });

        instance_id
    }

    // Called via 9P write to /wasm/instances/X/ctl
    pub fn control_instance(&mut self, instance_id: u64, cmd: &str) {
        match cmd {
            "start" => { /* Already started by fork/exec */ },
            "stop" => {
                if let Some(inst) = self.instances.get(&instance_id) {
                    kill(inst.pid, SIGTERM);
                }
            },
            "kill" => {
                if let Some(inst) = self.instances.get(&instance_id) {
                    kill(inst.pid, SIGKILL);
                }
            },
            _ => {},
        }
    }

    // 9P operations exported to kernel
    #[wasm_bindgen]
    pub fn p9_read(&self, qid_path: u64, offset: u64, count: u32) -> Vec<u8> {
        // Handle reads from /wasm/ filesystem
        // ...
    }

    #[wasm_bindgen]
    pub fn p9_write(&mut self, qid_path: u64, data: &[u8]) -> u32 {
        // Handle writes to /wasm/ filesystem
        // ...
    }
}

// Helper: Read file via 9P
fn read_file(path: &str) -> Vec<u8> {
    // Open file via 9P Topen
    // Read via 9P Tread
    // Return bytes
    vec![]  // Placeholder
}

// Helper: Fork process
fn fork() -> u64 {
    // Send Tsyscall(SYS_FORK) to kernel
    // Return child PID
    0  // Placeholder
}

// Helper: Exec WASM module
fn exec(path: &str, args: &[&str]) {
    // Send Tsyscall(SYS_EXEC) to kernel
    // This will call exec_wasm_module() in kernel
}

// Helper: Kill process
fn kill(pid: u64, signal: i32) {
    // Send Tsyscall(SYS_KILL) to kernel
}
```

**Compile**:
```bash
cd userspace/wasm_server
cargo build --target wasm32-wasi --release
# Output: target/wasm32-wasi/release/wasm_server.wasm
```

**Integrate with boot**: Copy `wasm_server.wasm` to initrd, start from init

#### 5. Implement 9P Filesystem Interface (1-2 days)

**Goal**: Serve `/wasm/` filesystem from Layer 2 WASM server

**Filesystem Structure**:
```
/wasm/
    modules/
        hello.wasm          ← WASM modules (read from initrd)
        calculator.wasm
    instances/
        hello.0/            ← Running instances (processes)
            ctl             ← Control: start/stop/kill
            stdin           ← Input stream
            stdout          ← Output stream
            stderr          ← Error stream
            mem             ← Linear memory (read/write)
    ctl                     ← Global control (create instance)
```

**Mount**: `mount /srv/wasm /wasm wasm`

---

## Important Technical Details

### Codebase Location

```
/home/scott/Repo/lux9-kernel/kernel/
├── 9front-pc64/       # x86-64 platform code
│   └── main.c         # Kernel boot (calls wasm_runtime_init)
├── 9front-port/       # Portable kernel code
│   ├── proc.c         # Process management (need to add WASM cleanup)
│   └── sysproc.c      # System calls (need to add exec_wasm_module)
├── include/
│   ├── portdat.h      # Proc structure (WASM fields added here)
│   ├── fcall.h        # 9P message structures
│   └── fns.h          # Function declarations
├── wasm/
│   ├── wasm_runtime.c # Layer 1 WASM runtime (DONE)
│   ├── wasm_runtime.h # API header (DONE)
│   ├── wasm_exec.c    # WASM exec handler (TODO)
│   └── wasm_runtime/
│       └── wasm3/     # wasm3 library
│           ├── wasm3.h
│           └── m3_*.c
├── 9p_router.c        # 9P message routing (WASM handlers added)
└── Makefile           # Build system (WASM files added)
```

### Key Structures

**Proc** (`kernel/include/portdat.h` lines 660-875):
```c
struct Proc {
    Label sched;
    Timer timer;
    Mach *mach;
    char *text;
    char *user;

    ulong capabilities;      /* Capability bitmap */
    Segment *seg[NSEG];      /* Memory segments */
    Fgrp *fgrp;              /* File descriptors */
    int state;               /* Process state */

    /* WASM execution context (lines 867-874) */
    struct {
        int initialized;
        void *runtime;       /* IM3Runtime */
        void *module;        /* IM3Module */
        u8int *linear_memory;
        u32int memory_size;
        u32int memory_pages;
    } wasm;
};
```

**Fcall** (`kernel/include/fcall.h`):
```c
struct Fcall {
    uchar type;
    u32int fid;
    u16int tag;
    union {
        struct { /* Tsyscall/Rsyscall */
            u16int scallnr;      /* Syscall number */
            u32int scount;       /* sdata byte count */
            uchar sdata[8192];   /* Syscall data */
        };
        struct { /* Tread/Rread */
            vlong offset;
            u32int count;
            uchar *data;
        };
        /* ... other 9P message types ... */
    };
};
```

### Syscall Numbers

```c
/* kernel/wasm/wasm_runtime.h */
#define SYS_WASM_COMPILE  100
#define SYS_WASM_EXECUTE  101
#define SYS_WASM_DESTROY  102

/* Capability permissions */
#define PERM_WASM_COMPILE  (1UL << 16)
#define PERM_WASM_EXECUTE  (1UL << 17)
```

### Message Formats (After Refactoring)

**SYS_WASM_COMPILE** (Tsyscall):
```
sdata = [module_size:4] [module_bytes:n]
        ↑ u32int       ↑ raw WASM bytes
```

**SYS_WASM_EXECUTE** (Tsyscall):
```
sdata = [func_name_len:4] [func_name:n]
        ↑ u32int          ↑ function name string
```

**SYS_WASM_DESTROY** (Tsyscall):
```
sdata = (none needed - operates on current process)
```

All return `pid` (not instance_id) in Rsyscall.

### wasm3 API

```c
/* Environment (global, shared by all processes) */
IM3Environment m3_NewEnvironment(void);
void m3_FreeEnvironment(IM3Environment env);

/* Runtime (per-process) */
IM3Runtime m3_NewRuntime(IM3Environment env, u32int stackSize, void *userdata);
void m3_FreeRuntime(IM3Runtime runtime);

/* Module */
M3Result m3_ParseModule(IM3Environment env, IM3Module *module,
                        const u8int *bytes, u32int length);
M3Result m3_LoadModule(IM3Runtime runtime, IM3Module module);

/* Execution */
M3Result m3_FindFunction(IM3Function *func, IM3Runtime runtime, const char *name);
M3Result m3_CallV(IM3Function func, ...);  /* Variadic args */
M3Result m3_GetResultsV(IM3Function func, ...);  /* Get return values */

/* Memory */
u8int *m3_GetMemory(IM3Runtime runtime, u32int *size, u32int index);
```

**Note**: `M3Result` is `const char *` (nil on success, error message on failure)

---

## Common Pitfalls to Avoid

### 1. Header Conflicts

**Problem**: wasm3.h includes system headers that conflict with kernel headers

**Solution**: Already fixed in `kernel/wasm/wasm_runtime/wasm3/wasm3.h`:
```c
#ifdef __PLAN9_KERNEL__
/* Kernel provides these types in u.h and portlib.h */
#else
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#endif
```

### 2. Type Mismatches

**Problem**: wasm3 uses C99 types (`uint32_t`), kernel uses Plan 9 types (`u32int`)

**Solution**: Typedefs in `wasm_runtime.c`:
```c
typedef u8int uint8_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef usize size_t;
```

### 3. Return Value Encoding

**Problem**: Fcall doesn't have `retval` field

**Solution**: Use `sdata` buffer with PBIT64 macro:
```c
rx->type = Rsyscall;
rx->tag = tx->tag;
rx->scount = 8;
PBIT64(rx->sdata, retval);  /* Encodes u64 into bytes */
```

### 4. Capability Field Name

**Problem**: Easy to use wrong field name

**Correct**: `up->capabilities` (plural, bitmap)
**Wrong**: `up->capability` (doesn't exist)

### 5. Process Context

**Problem**: Forgetting that handlers run in process context

**Remember**:
- `up` is current process (`extern Proc *up;`)
- WASM state is in `up->wasm`
- Capability checks use `up->capabilities`
- No instance_id needed - just use current process!

---

## Testing Strategy

### Unit Tests

1. **Compile wasm_runtime.o**: `make wasm_runtime.o`
2. **Compile wasm_exec.o**: `make wasm_exec.o`
3. **Full kernel build**: `make clean && make -j4`

### Integration Tests

1. **Boot kernel in QEMU**:
   ```bash
   qemu-system-x86_64 -kernel lux9.elf -initrd initrd.tar \
       -m 512M -serial stdio -nographic
   ```

2. **Check boot messages**:
   - Should see: "=== Initializing WASM3 Runtime (Layer 1) ==="
   - Should see: "=== WASM3 Runtime Initialized ==="

3. **Test WASM execution** (once exec_wasm_module is done):
   ```bash
   # In QEMU/kernel
   ./hello.wasm
   # Should execute WASM program and print output
   ```

4. **Verify process visibility**:
   ```bash
   ps  # Should show WASM processes
   cat /proc/N/status  # Should show WASM process state
   ```

### Example WASM Test Module

```wat
;; hello.wasm - Simple test module
(module
  (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (memory 1)
  (export "memory" (memory 0))

  ;; Write "Hello from WASM!\n" to stdout
  (func $main (export "_start")
    ;; ... WASM code to write to stdout ...
  )
)
```

Compile: `wat2wasm hello.wat -o hello.wasm`

---

## Key Documents to Read

**MUST READ** (in order):

1. **`MICROKERNEL_ARCHITECTURE.md`** - Overall architecture, bootstrap, layers
2. **`ADR_WASM_AS_PROCESSES.md`** - Why WASM-as-processes is correct
3. **`PHASE0_STATUS.md`** - Current status, what's done, what's next
4. **`SESSION_SUMMARY_WASM_AS_PROCESSES.md`** - Refactoring session details

**Reference**:

5. **`kernel/wasm/TODO_LAYER1_FIXES.md`** - Technical notes on Layer 1
6. **`kernel/include/portdat.h`** - Proc structure definition
7. **`kernel/wasm/wasm_runtime.c`** - Layer 1 runtime implementation
8. **`kernel/9p_router.c`** - 9P message routing

---

## Success Criteria

Phase 0 is complete when:

1. ✅ Kernel builds cleanly
2. ✅ Kernel boots and prints "WASM3 Runtime Initialized"
3. ✅ Can execute WASM binaries: `./hello.wasm`
4. ✅ WASM processes show up in `ps`
5. ✅ Can read WASM process memory via `/proc/N/mem`
6. ✅ WASM processes exit cleanly (pexit cleans up)
7. ✅ Layer 2 WASM server starts and serves `/wasm/` filesystem

---

## Your Task

**Continue the implementation from where it was left off.**

Current state:
- ✅ Layer 1 runtime refactored to WASM-as-processes
- ✅ Proc structure extended with WASM fields
- ✅ Tsyscall handlers use `up->wasm`
- ✅ wasm_runtime.o compiles cleanly

Next steps:
1. Test full kernel build
2. Implement exec_wasm_module()
3. Add WASM cleanup to pexit()
4. Create Layer 2 WASM server
5. Implement 9P filesystem

**Remember**:
- This is Plan 9, not Linux
- WASM is the main executable format
- Everything communicates via 9P
- Read the documents first!

Good luck! 🚀
