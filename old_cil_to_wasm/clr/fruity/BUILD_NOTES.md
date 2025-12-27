# Fruity IR Build Notes

## Pure Build System - No Linux Contamination

The Fruity IR subsystem is built using **only Plan 9 native types** from `u.h`.

### Type System

We use Plan 9 fixed-width types instead of Linux `<stdint.h>`:

| Linux Type | Plan 9 Type | Size |
|------------|-------------|------|
| `int32_t`  | `s32int`    | 4 bytes |
| `int64_t`  | `s64int`    | 8 bytes |
| `uint32_t` | `u32int`    | 4 bytes |
| `uint64_t` | `u64int`    | 8 bytes |
| `bool`     | `int` (0/1) | native |

### No External Dependencies

**What we DON'T use:**
- ❌ `<stdint.h>` - Linux standard integers
- ❌ `<stdbool.h>` - Linux boolean type
- ❌ `<stdlib.h>` - Linux standard library
- ❌ `<string.h>` - Linux string functions

**What we DO use:**
- ✅ `u.h` - Plan 9 universal header (native types)
- ✅ `lib.h` - Plan 9 library functions
- ✅ `mem.h` - Plan 9 memory management
- ✅ `dat.h` - Plan 9 data structures
- ✅ `fns.h` - Plan 9 function declarations

### AOT Compilation Strategy

The Fruity IR is designed for **Ahead-of-Time (AOT) compilation**, not JIT:

```
Development Machine (has LLVM):
  C# Source → Roslyn → MSIL
      ↓
  fruity-lower → Fruity IR
      ↓
  fruity-opt → Optimized Fruity IR
      ↓
  fruity-aot (uses LLVM) → Native ELF binary
      ↓
  Deploy to target

Target Machine (Lux9 kernel):
  Load ELF binary (no LLVM needed)
  Execute native code
  Pebble syscalls via kernel ABI
```

### Kernel ABI

The generated native code calls back into the kernel via a clean C ABI:

```c
// Defined by kernel, called by generated code
void* lux_alloc(u32int size, u32int type_id);
void  lux_token_mint(void* black_ptr, void** out_white);
void  lux_token_burn(void* white_token);
void  lux_exchange_send(void* white_token, u32int channel_id);
void  lux_snapshot(void* white_token);
void  lux_commit(void* white_token);
void  lux_rollback(void* white_token);
```

These map directly to Fruity opcodes:
- `FRUITY_LIME` → `lux_alloc()`
- `FRUITY_VANILLA` → `lux_token_mint()`
- `FRUITY_BURN` → `lux_token_burn()`
- `FRUITY_GRAPE` → `lux_exchange_send()`
- `FRUITY_CHERRY` → `lux_snapshot()`
- `FRUITY_BERRY` → `lux_commit()`
- `FRUITY_ROLLBACK` → `lux_rollback()`

### Why AOT, Not JIT?

**JIT Problems:**
- Large runtime overhead (LLVM/JIT engine in kernel)
- Unpredictable compilation pauses
- Writable code pages (security risk)
- Complex porting to different architectures

**AOT Benefits:**
- ✅ Fast boot (no compilation at runtime)
- ✅ Deterministic performance
- ✅ Small kernel (no JIT engine)
- ✅ W^X memory protection
- ✅ LLVM only on dev machine, not target

### Build Output

```bash
$ make -C kernel/clr/fruity
ar rcs fruity.a fruity_ir.o

$ ls -lh kernel/clr/fruity/fruity.a
-rw-rw-r-- 1 scott scott 15K Dec  7 23:11 fruity.a
```

The `fruity.a` library contains:
- IR data structures (instruction, basic block, function, module)
- IR construction functions
- IR traversal and analysis
- Opcode metadata
- Debug printing

This library is linked into the kernel for runtime support, but **code generation happens offline** using the `fruity-aot` tool.
