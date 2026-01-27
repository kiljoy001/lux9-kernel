# Architecture Decision Record: WASM as Processes (Not Separate Instance Table)

**Date**: December 29, 2025
**Status**: ✅ **ACCEPTED** - Fundamental design change
**Deciders**: Scott (user), Claude Code (analysis)

---

## Context and Problem Statement

In the initial implementation of Phase 0 (WASM Layer 1 runtime), we created a **separate instance table** to manage WASM modules:

```c
#define MAX_WASM_INSTANCES 256

typedef struct WasmInstance {
    u64int instance_id;
    IM3Runtime runtime;
    IM3Module module;
    u8int *linear_memory;
    u32int memory_size;
    ulong capabilities;
    enum { ... } state;
    Lock lock;
} WasmInstance;

static WasmInstance instance_table[MAX_WASM_INSTANCES];
```

**User's Critical Question**: *"Why not use our proc and integrate wasm as processes into that instead of a separate system?"*

This challenged the fundamental premise: If WASM is the **main executable format** for this server OS (not just a plugin system), should WASM modules run as separate "instances" or as **first-class processes**?

---

## Analysis of Proc Structure

The kernel's `Proc` structure (`kernel/include/portdat.h`) ALREADY contains all the infrastructure we're duplicating in `WasmInstance`:

### 1. Capability System (Line 842)
```c
ulong capabilities;  /* Capability bitmap for hardware access */
```
**Exact same field** we added to `WasmInstance`!

### 2. Exchange Pages (Lines 698-708)
```c
void *exchange_channel;  /* ExchangeChannel from #X device */
u32int fid_counter;      /* Next FID to allocate */
vlong fid_offsets[256];  /* Offset tracking per FID */
```
Already provides 9P message infrastructure.

### 3. Resource Tracking (Lines 734, 850)
```c
ulong time[6];           /* User, Sys, Real; child U, S, R */
PebbleState pebble;      /* Pebble resource tracking */
```
Already tracks CPU time, memory usage, quotas.

### 4. Memory Segments (Line 724)
```c
Segment *seg[NSEG];      /* Process memory segments */
```
Already provides isolated address spaces - perfect for WASM linear memory!

### 5. State Management (Lines 693-694)
```c
int state;               /* Process states (Dead, Ready, Running, etc.) */
ushort state_trace;      /* FSM last 4 states */
```
Already has state machine (Dead, Ready, Running, etc.) - exact same as our `WasmInstance` states!

### 6. File Descriptors (Line 728)
```c
Fgrp *fgrp;              /* File descriptor group */
```
Already provides stdin/stdout/stderr via Chan* - WASM can use standard I/O!

### 7. Entry Point (Line 683)
```c
uintptr entry_point;     /* Entry point for process (ELF e_entry or UTZERO for a.out) */
```
Already supports different executable formats - just add WASM!

---

## Decision Drivers

### ❌ Problems with Separate Instance Table

1. **Duplicate Resource Tracking**:
   - `WasmInstance.capabilities` duplicates `Proc.capabilities`
   - `WasmInstance.state` duplicates `Proc.state`
   - Memory tracking duplicated from `Proc.seg[]`

2. **Artificial Limits**:
   - 256 instances is too small for server OS
   - Even 32K is a hard limit
   - Server OS needs thousands of processes

3. **No Standard Process Management**:
   - WASM instances don't show up in `ps`
   - Can't use `/proc/N/` to inspect WASM processes
   - No `fork()`, `exec()`, `wait()`, `kill()` support
   - Can't leverage existing scheduler

4. **Can't Reuse Kernel Infrastructure**:
   - Custom locking for instance table
   - Custom memory allocation
   - Custom state machine
   - Custom capability checking

5. **Wrong Model for Main Executable Format**:
   - If WASM is a **plugin system**: separate instances make sense
   - If WASM is the **main program format**: processes are correct

### ✅ Benefits of WASM as Processes

1. **Reuse ALL Existing Infrastructure**:
   - ✅ `up->capabilities` for permission checks (no duplication!)
   - ✅ `up->seg[LSEG]` for WASM linear memory
   - ✅ `up->state` for process state (Dead, Ready, Running)
   - ✅ Existing scheduler (no WASM-specific scheduling)
   - ✅ Existing memory allocator and quotas
   - ✅ Existing resource tracking (`up->time`, `up->pebble`)

2. **No Artificial Limits**:
   - As many WASM processes as kernel can handle
   - Same limit as native processes
   - Scales with RAM

3. **Standard Process Lifecycle**:
   - `exec("hello.wasm")` creates WASM process
   - `fork()` creates child WASM process
   - `wait()` waits for WASM child to exit
   - `kill(pid)` terminates WASM process
   - Process exit cleans up WASM runtime automatically

4. **Visibility and Debugging**:
   - WASM processes show up in `ps`
   - `/proc/N/mem` can read WASM linear memory
   - `/proc/N/status` shows WASM state
   - Standard debugging tools work

5. **File Descriptors Work Naturally**:
   - stdin/stdout/stderr via `up->fgrp`
   - WASM can `read(0)`, `write(1)`, `open()` files
   - No custom I/O plumbing needed

6. **Parent/Child Relationships**:
   - `up->parent` for process tree
   - `up->waitq` for collecting child exit status
   - WASM processes can spawn children

---

## Considered Alternatives

### Alternative 1: Expand Instance Table to 32K (REJECTED)
**Approach**: Keep separate table but increase size dynamically.

**Rejected Because**:
- Still duplicates resource tracking
- Still doesn't integrate with process management
- Still requires custom infrastructure
- Doesn't solve fundamental architectural mismatch

### Alternative 2: Hybrid - Processes + Instance Table (REJECTED)
**Approach**: One process manages multiple WASM instances via instance table.

**Rejected Because**:
- Complicated: two levels of management
- WASM instances still not first-class
- Doesn't leverage process isolation
- More complex than either pure approach

### Alternative 3: WASM as Processes (ACCEPTED ✅)
**Approach**: Each WASM module runs as a `Proc` with WASM execution context.

**Accepted Because**:
- Leverages ALL existing kernel infrastructure
- WASM is first-class executable format
- No duplication of capability/memory/state tracking
- Simpler overall architecture
- Scalable (no arbitrary limits)
- Standard tools work (`ps`, `/proc`, `kill`, etc.)

---

## Implementation Design

### Extend Proc Structure

Add WASM execution context to `kernel/include/portdat.h`:

```c
struct Proc {
    ... existing fields (label, timer, mach, text, user, etc.) ...

    /* CLR Thread-Local Storage (for managed code LocalDataStore) */
#define CLR_TLS_SLOTS 64
    void *clr_tls[CLR_TLS_SLOTS];
    int clr_tls_next_slot;

    /* WASM execution context (only populated if this is a WASM process) */
    struct {
        int initialized;         /* 1 if this is a WASM process, 0 otherwise */
        IM3Runtime runtime;      /* wasm3 runtime for this process */
        IM3Module module;         /* Loaded WASM module */
        u8int *linear_memory;    /* WASM linear memory (mapped to seg[LSEG]) */
        u32int memory_size;      /* Size of linear memory in bytes */
        u32int memory_pages;     /* Number of 64KB pages */
    } wasm;
} __attribute__((aligned(64)));
```

**Key Points**:
- `wasm.initialized` flag distinguishes WASM processes from native
- WASM fields ONLY populated for WASM processes (zero overhead for native)
- Linear memory mapped to `seg[LSEG]` segment (reuses existing segment system)
- Capability checks use existing `up->capabilities` field

### Execution Path

#### 1. Detect WASM Binary in exec()

```c
/* kernel/9front-port/sysproc.c */

void sys_exec(char *name, char **argv) {
    Chan *c = namec(name, Aopen, OEXEC, 0);

    /* Read magic number (first 4 bytes) */
    uchar magic[4];
    devtab[c->type]->read(c, magic, 4, 0);

    if (magic[0] == 0x00 && magic[1] == 'a' &&
        magic[2] == 's' && magic[3] == 'm') {
        /* WASM module - initialize WASM execution */
        exec_wasm_module(c, argv);
    } else if (magic[0] == 0x7f && magic[1] == 'E' &&
               magic[2] == 'L' && magic[3] == 'F') {
        /* Native ELF binary */
        exec_elf_binary(c, argv);
    } else {
        error("unknown executable format");
    }
}
```

#### 2. Load and Initialize WASM Module

```c
/* kernel/wasm/wasm_exec.c (new file) */

void exec_wasm_module(Chan *c, char **argv) {
    /* Load WASM module into memory */
    ulong size = c->qid.length;
    u8int *module_bytes = mallocz(size, 1);
    devtab[c->type]->read(c, module_bytes, size, 0);

    /* Initialize wasm3 runtime for THIS process */
    up->wasm.runtime = m3_NewRuntime(&wasm_runtime.env, 64 * 1024, nil);
    if (!up->wasm.runtime) {
        free(module_bytes);
        error("failed to create WASM runtime");
    }

    /* Parse and load WASM module */
    M3Result result = m3_ParseModule(&wasm_runtime.env, &up->wasm.module,
                                      module_bytes, size);
    if (result) {
        m3_FreeRuntime(up->wasm.runtime);
        free(module_bytes);
        error("WASM parse failed");
    }

    result = m3_LoadModule(up->wasm.runtime, up->wasm.module);
    if (result) {
        m3_FreeRuntime(up->wasm.runtime);
        free(module_bytes);
        error("WASM load failed");
    }

    /* Get linear memory and map to segment */
    up->wasm.linear_memory = m3_GetMemory(up->wasm.runtime,
                                          &up->wasm.memory_size, 0);
    up->wasm.memory_pages = up->wasm.memory_size / (64 * 1024);

    /* Map to LSEG segment for memory isolation */
    up->seg[LSEG] = newseg(SG_SHARED, (uintptr)up->wasm.linear_memory,
                           up->wasm.memory_size);

    up->wasm.initialized = 1;
    free(module_bytes);  /* Module copied into wasm3 runtime */

    /* Find _start function */
    IM3Function start_func;
    result = m3_FindFunction(&start_func, up->wasm.runtime, "_start");
    if (result) {
        /* Try main() if _start not found */
        result = m3_FindFunction(&start_func, up->wasm.runtime, "main");
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

#### 3. Simplified Tsyscall Handlers

```c
/* kernel/wasm/wasm_runtime.c - MUCH SIMPLER NOW! */

int sys_wasm_execute(Fcall *tx, Fcall *rx) {
    /* Check if current process is a WASM process */
    if (!up->wasm.initialized) {
        rx->type = Rerror;
        snprint(rx->ename, sizeof(rx->ename), "not a WASM process");
        return -1;
    }

    /* Check capability (reuses existing up->capabilities!) */
    if (!(up->capabilities & PERM_WASM_EXECUTE)) {
        rx->type = Rerror;
        snprint(rx->ename, sizeof(rx->ename), "no WASM execute permission");
        return -1;
    }

    /* Parse function name from sdata */
    u8int *sdata = (u8int *)tx->sdata;
    u32int func_name_len = *(u32int *)sdata;
    char *func_name = (char *)(sdata + 4);

    char func_name_buf[256];
    memmove(func_name_buf, func_name, func_name_len);
    func_name_buf[func_name_len] = '\0';

    /* Find function in THIS process's WASM runtime */
    IM3Function func;
    M3Result result = m3_FindFunction(&func, up->wasm.runtime, func_name_buf);
    if (result) {
        rx->type = Rerror;
        snprint(rx->ename, sizeof(rx->ename), "function not found: %s", result);
        return -1;
    }

    /* Execute function */
    result = m3_CallV(func);
    if (result) {
        rx->type = Rerror;
        snprint(rx->ename, sizeof(rx->ename), "execution failed: %s", result);
        return -1;
    }

    /* Get return value */
    uint64_t retval = 0;
    m3_GetResultsV(func, &retval);

    /* Send success reply */
    rx->type = Rsyscall;
    rx->tag = tx->tag;
    rx->scount = 8;
    PBIT64(rx->sdata, retval);
    return 0;
}
```

**Note**: No instance ID lookup! Just use `up->wasm` for current process.

#### 4. Process Exit Cleanup

```c
/* kernel/9front-port/proc.c */

void pexit(char *note, int freemem) {
    ... existing cleanup (segments, file descriptors, etc.) ...

    /* Clean up WASM runtime if this was a WASM process */
    if (up->wasm.initialized) {
        if (up->wasm.runtime) {
            m3_FreeRuntime(up->wasm.runtime);  /* Frees module too */
            up->wasm.runtime = nil;
            up->wasm.module = nil;
        }
        up->wasm.initialized = 0;
    }

    ... rest of exit cleanup ...
}
```

### User Experience

#### Running WASM Programs

```bash
# Compile WASM module (on development machine)
$ rustc --target wasm32-wasi hello.rs
$ ls -l hello.wasm
-rwxr-xr-x 1 user user 12345 Dec 29 hello.wasm

# Copy to lux9 kernel
$ cp hello.wasm /mnt/initrd/boot/

# Execute WASM program (on lux9)
$ ./hello.wasm
Hello from WASM!

# WASM process shows up in ps
$ ps
PID   USER    STATE   TEXT
1     root    Running init
2     root    Running wasm_server
42    user    Running hello.wasm    # ← WASM process!

# Inspect WASM process
$ cat /proc/42/status
hello.wasm 42: pc 0 Running
  mem: 65536 bytes (1 WASM pages)
  cpu: 123 ms user, 45 ms sys

# Read WASM linear memory
$ cat /proc/42/mem | hexdump -C

# Kill WASM process
$ kill 42
```

#### From Other Processes (9P)

```c
/* Layer 2 WASM server can spawn WASM processes */

// User writes to /wasm/ctl: "exec hello.wasm"
// WASM server forks and execs WASM module

int pid = fork();
if (pid == 0) {
    /* Child process */
    exec("/boot/hello.wasm", nil);
    exits("exec failed");
}

/* Parent waits for WASM child */
Waitmsg *w = wait();
print("WASM process %d exited: %s\n", w->pid, w->msg);
```

---

## Consequences

### Positive

1. **Leverages Existing Infrastructure**: No duplication of capability/memory/state systems
2. **Scalable**: No artificial limits on number of WASM processes
3. **Standard Tools Work**: `ps`, `/proc`, `kill`, `wait` all work with WASM
4. **Simpler Overall**: Less code, reuses proven kernel mechanisms
5. **First-Class WASM**: WASM programs are true processes, not "instances"
6. **Future-Proof**: Can add WASM JIT, debugging, profiling using existing proc infrastructure

### Negative

1. **More Complex Initial Integration**: Need to modify exec() path, not just add handlers
2. **WASM Linear Memory Mapping**: Requires careful segment management
3. **Fork/Exec Model Needs Thought**: WASM processes forking creates new runtimes

### Neutral

1. **Process Overhead**: Each WASM module is a full process (but this is THE RIGHT MODEL)
2. **Memory**: `sizeof(Proc)` increases by ~48 bytes for WASM fields (negligible)

---

## Migration Path

### Phase 1: Add WASM Fields to Proc (This Session)
1. ✅ Extend `Proc` structure in `portdat.h`
2. ✅ Add `wasm.initialized` flag
3. ✅ Define WASM context fields (runtime, module, memory)

### Phase 2: Implement exec_wasm_module() (Next Session)
1. Add WASM magic number detection to `sys_exec()`
2. Implement `exec_wasm_module()` in new file `wasm/wasm_exec.c`
3. Map WASM linear memory to `seg[LSEG]`

### Phase 3: Refactor Tsyscall Handlers
1. Remove `instance_table` global
2. Change handlers to use `up->wasm` instead of instance lookup
3. Remove instance ID from Tsyscall messages (no longer needed!)

### Phase 4: Add Process Exit Cleanup
1. Modify `pexit()` to check `up->wasm.initialized`
2. Free wasm3 runtime on process exit

### Phase 5: Testing
1. Test WASM binary execution
2. Verify process shows in `ps`
3. Test `/proc/N/` inspection
4. Test `kill` and `wait`

---

## Validation

This decision will be validated by:

1. ✅ WASM programs execute as processes
2. ✅ `ps` shows WASM processes
3. ✅ `/proc/N/mem` readable for WASM linear memory
4. ✅ No arbitrary instance limits
5. ✅ Resource quotas enforced automatically via existing Proc infrastructure
6. ✅ Capability checks use `up->capabilities` (no duplication)

---

## References

- **Proc Structure**: `kernel/include/portdat.h` lines 660-862
- **Segment Types**: `kernel/include/portdat.h` lines 381-398
- **Initial WASM Implementation**: `kernel/wasm/wasm_runtime.c` (to be refactored)
- **Phase 0 Status**: `PHASE0_STATUS.md`
- **Microkernel Architecture**: `MICROKERNEL_ARCHITECTURE.md`

---

## Conclusion

**WASM programs should be processes, not instance table entries.**

This decision:
- ✅ Aligns with Plan 9 philosophy (everything is a file, every program is a process)
- ✅ Leverages existing kernel infrastructure (no duplication)
- ✅ Scales without arbitrary limits
- ✅ Makes WASM a first-class executable format
- ✅ Simplifies overall architecture

**Next Steps**: Implement the Proc extension and refactor wasm_runtime.c to use processes instead of instance table.

---

*Decision Date: December 29, 2025*
*Impact: Fundamental change to WASM architecture - affects all of Phase 0*
*Status: ✅ **ACCEPTED** - Begin implementation immediately*
