# WASM Layer 1 Runtime - Remaining Implementation Tasks

## Status: Initial Implementation Complete (with TODOs)

The Layer 1 WASM runtime has been created with the following files:
- `wasm_runtime.c` - Core runtime implementation
- `wasm_runtime.h` - API header
- Boot integration in `main.c`
- Makefile integration
- 9P router integration in `9p_router.c`

## Compilation Errors to Fix

### 1. Capability System Integration
**Files**: `wasm_runtime.c` lines 167, 267

**Current code**:
```c
if (!cap_check(up->capability, PERM_WASM_COMPILE))
```

**Needs to be**:
```c
if (!(up->capabilities & PERM_WASM_COMPILE))
```

**Reason**: `up->capabilities` is a `ulong` bitmap, not a pointer. Direct bitwise AND check needed.

### 2. Return Value Encoding
**Files**: `wasm_runtime.c` lines 252, 351, 400

**Current code**:
```c
rx->retval = 0;
```

**Needs to be**:
```c
rx->type = Rsyscall;
rx->tag = tx->tag;
rx->scount = 8;
PBIT64(rx->sdata, (u64int)retval);
```

**Reason**: Fcall doesn't have `retval` field. Return values are encoded in `sdata` buffer using PBIT64/PBIT32 macros.

### 3. m3_Call Signature
**File**: `wasm_runtime.c` line 328

**Current code**:
```c
result = m3_Call(func);
```

**Needs investigation**: Check wasm3.h for correct m3_Call signature. May need:
```c
result = m3_Call(func, 0, nil);  // argc=0, argv=nil
```

### 4. Segment Allocation (Temporary Workaround)
**File**: `wasm_runtime.c` line 89

**Current code**:
```c
wasm_runtime.seg = newseg(SG_WASM, WASMSIZE, 0);
```

**TODO**: Proper segment isolation requires:
1. Define SG_WASM in kernel segment type enum
2. Implement proper segment memory isolation
3. Map WASM linear memory to isolated segment
4. Implement exchange page backing for WASM memory

**Temporary workaround**: Comment out segment allocation, use malloc for now:
```c
// wasm_runtime.seg = newseg(SG_WASM, WASMSIZE, 0);
// print("wasm_runtime: allocated isolated segment at %p, size %llu\\n",
//       wasm_runtime.seg, WASMSIZE);
```

### 5. Missing Function Declarations
**Needed**: Ensure these kernel functions are accessible:
- `lock()` / `unlock()` - should be in fns.h
- `PBIT64()` / `PBIT32()` - should be in portlib.h
- `print()` - should be in fns.h
- `snprint()` - should be in fns.h
- `memset()` / `memmove()` - should be in portlib.h

## Next Steps

1. **Fix compilation errors** (above changes)
2. **Test kernel build** (`make clean && make`)
3. **Verify boot sequence** (check for "WASM3 Runtime Initialized" message)
4. **Create Layer 2 WASM server** (Rust → WASM module)
5. **Implement 9P filesystem interface** (/wasm/modules, /wasm/instances)
6. **End-to-end testing** (load WASM module, execute function)

## Architecture Notes

The current implementation establishes the foundation for the microkernel architecture:

- **Layer 1 (Kernel)**: wasm_runtime.c provides isolated wasm3 execution
- **Layer 2 (WASM Server)**: To be implemented as WASM module itself
- **Communication**: Via Tsyscall(SYS_WASM_*) messages

The Layer 2 WASM server will:
1. Manage /wasm/ filesystem via 9P
2. Create/destroy WASM instances
3. Delegate actual execution to Layer 1 via Tsyscall
4. Enforce quotas and capabilities
5. Provide user-facing WASM service

This separation ensures:
- WASM runtime has NO direct kernel access
- All operations go through capability-checked Tsyscall boundary
- Layer 2 can be restarted by resurrection server without crashing kernel
- Clean microkernel architecture with minimal Layer 1
