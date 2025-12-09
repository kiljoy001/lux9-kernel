# CLR Current State Assessment

## What We Have ✅

### 1. Fruity IR (Complete)
**Location:** `kernel/clr/fruity/`

**Files:**
- `fruity_ir.h` - IR data structures
- `fruity_ir.c` - IR manipulation functions
- `fruity_opcodes.h` - Opcode definitions
- `fruity_cbor.c` - CBOR serialization

**Capabilities:**
- ✅ In-memory IR representation (linked lists)
- ✅ Pebble-aware opcodes (LIME, VANILLA, BURN, CHERRY, BERRY)
- ✅ IPC opcodes (GRAPE, LEMON)
- ✅ Control flow (CALL, RET, JUMP, BEQ, BNE)
- ✅ Stack operations (DUP, POP, LDLOC, STLOC)
- ✅ Arithmetic (ADD, SUB, MUL)
- ✅ CBOR serialization for IR persistence

### 2. Fruity IR → QBE Backend (Complete)
**Location:** `kernel/clr/fruity/fruity_to_qbe.c`

**Capabilities:**
- ✅ Converts Fruity IR to QBE IL text format
- ✅ Zero-copy I/O via exchange pages
- ✅ Runtime ABI declarations for Pebble integration
- ✅ Maps Fruity opcodes to QBE IL

**Example output:**
```qbe
# QBE IL generated from Fruity IR
export function l $lux_alloc(w %size, w %type) { @start ret 0 }
export function $lux_token_mint(l %ptr) { @start ret }
export function $lux_token_burn(l %ptr) { @start ret }
```

### 3. QBE Compiler (Complete)
**Location:** `kernel/clr/qbe/`

**Capabilities:**
- ✅ Full QBE compiler integrated
- ✅ AMD64 backend (amd64/targ.c, amd64/emit.c)
- ✅ Register allocation
- ✅ SSA optimization
- ✅ Gas assembler output

**Pipeline:**
```
QBE IL text → QBE compiler → x86-64 assembly
```

### 4. CIL Runtime (Complete)
**Location:** `kernel/clr/clr-implementation/`

**Files:**
- `clr_runtime.h` - CIL interpreter header
- `clr_runtime.c` - CIL interpreter implementation

**Capabilities:**
- ✅ Stack-based CIL execution
- ✅ Type-checked operations
- ✅ Formally verified (from Coq proofs)
- ✅ Supported opcodes:
  - NOP, DUP, POP
  - LDC_I4 (load constant)
  - ADD, SUB, MUL
  - LDLOC, STLOC (locals)
  - BR, RET (control flow)

## What's Missing ❌

### 1. IL Bytecode Parser (CRITICAL GAP)
**Need:** Parse .NET PE/COFF files to extract IL bytecode

**.NET file format:**
```
PE/COFF File
├── DOS Header
├── PE Header
├── Section Headers
├── .text section (code)
├── .rsrc section (resources)
└── Metadata Tables
    ├── MethodDef table
    ├── TypeDef table
    ├── Assembly table
    └── IL bytecode streams
```

**What we need to parse:**
- PE/COFF headers
- CLI metadata header
- Metadata tables (MethodDef, TypeDef, etc.)
- IL bytecode stream for each method

**Current status:** ❌ Not implemented

### 2. IL → Fruity IR Converter (CRITICAL GAP)
**Need:** Convert CIL bytecode to Fruity IR

**Mapping examples:**
```
CIL Instruction          →  Fruity IR
─────────────────────────────────────────
ldloc.0                  →  FRUITY_LOAD_LOCAL 0
ldloc.1                  →  FRUITY_LOAD_LOCAL 1
add                      →  FRUITY_ADD
stloc.2                  →  FRUITY_STORE_LOCAL 2
ldc.i4.5                 →  FRUITY_LOAD_CONST 5 (need to add)
newobj <constructor>     →  FRUITY_LIME (allocate)
callvirt <method>        →  FRUITY_CALL
ret                      →  FRUITY_RET
```

**Current status:** ❌ Not implemented

### 3. F# Runtime Integration
**Need:** Minimal FSharp.Core support

**Required F# types:**
- `Option<'T>` (Some/None)
- `Result<'T,'E>` (Ok/Error)
- `List<'T>` (cons list)
- Basic string operations

**Current status:** ❌ Not implemented

### 4. Memory Allocation Integration
**Need:** Connect IL object allocation to Pebble

**When F# does:**
```fsharp
let x = Some 42
```

**IL bytecode:**
```
newobj instance void Option<int32>::.ctor(int32)
```

**Should translate to:**
```
1. FRUITY_LIME (pebble_black_token)
2. FRUITY_VANILLA (get white token)
3. Store value at address
4. Return capability
```

**Current status:** ⚠️ Partially implemented (LIME opcode exists, not connected)

## Pipeline Status

### Current Complete Path ✅
```
Fruity IR (in-memory)
    ↓
fruity_to_qbe.c
    ↓
QBE IL (text format)
    ↓
QBE compiler
    ↓
x86-64 assembly
    ↓
Native code
```

### Missing Path ❌
```
F# source code
    ↓
fsc compiler (external)
    ↓
.NET IL bytecode (.dll)
    ↓
❌ IL Parser (MISSING)
    ↓
❌ IL → Fruity IR (MISSING)
    ↓
Fruity IR (in-memory)
    ↓
✅ fruity_to_qbe.c
    ↓
✅ QBE compiler
    ↓
✅ x86-64 assembly
```

## Immediate Blockers

### Blocker 1: No IL Parser
**Problem:** Can't read .NET assemblies

**Solution:** Implement PE/COFF parser with CLI metadata reader

**Estimated effort:** 500-800 lines of C

**Priority:** **CRITICAL** - Blocks entire pipeline

### Blocker 2: No IL → Fruity IR Converter
**Problem:** Can't translate IL bytecode to our IR

**Solution:** Write IL opcode → Fruity opcode mapper

**Estimated effort:** 300-500 lines of C

**Priority:** **CRITICAL** - Blocks entire pipeline

### Blocker 3: Missing CIL Opcodes
**Problem:** Current CIL runtime only has 11 opcodes, need ~100+

**Solution:** Extend clr_runtime.c with more opcodes

**Estimated effort:** 50 lines per opcode × 50 opcodes = ~2500 lines

**Priority:** **HIGH** - Needed for real programs

## Test Case: Minimal F# Program

### Input: hello.fs
```fsharp
[<EntryPoint>]
let main args =
    printfn "Hello from F#!"
    0
```

### Compile to IL
```bash
fsc hello.fs
# Produces: hello.dll (contains IL bytecode)
```

### IL Bytecode (Expected)
```
.method public static int32 main(string[] args)
{
    .entrypoint
    ldstr "Hello from F#!"
    call void [FSharp.Core]Microsoft.FSharp.Core.PrintfModule::PrintFormatLine<class [FSharp.Core]Microsoft.FSharp.Core.Unit>(class [FSharp.Core]Microsoft.FSharp.Core.PrintfFormat<class [FSharp.Core]Microsoft.FSharp.Core.Unit,class [mscorlib]System.IO.TextWriter,class [FSharp.Core]Microsoft.FSharp.Core.Unit,class [FSharp.Core]Microsoft.FSharp.Core.Unit>)
    ldc.i4.0
    ret
}
```

### What We Need to Support This

1. **IL Parser:**
   - Read hello.dll PE/COFF format
   - Extract MethodDef for `main`
   - Parse IL bytecode stream

2. **IL → Fruity IR:**
   - `ldstr` → FRUITY_LOAD_STRING (need to add)
   - `call` → FRUITY_CALL
   - `ldc.i4.0` → FRUITY_LOAD_CONST 0
   - `ret` → FRUITY_RET

3. **Runtime Support:**
   - Implement `printfn` syscall
   - String allocation
   - Console output

## Recommended Implementation Order

### Phase 1: IL Parser (Week 1)
**Goal:** Read .NET assemblies

**Tasks:**
1. Implement PE/COFF header parser
2. Implement CLI metadata header parser
3. Implement MethodDef table reader
4. Implement IL bytecode stream reader
5. Test: Parse hello.dll, print IL opcodes

**Deliverable:** Can read IL bytecode from .NET assembly

### Phase 2: IL → Fruity IR (Week 2)
**Goal:** Convert IL to our IR

**Tasks:**
1. Create IL opcode → Fruity opcode mapping table
2. Implement converter for basic opcodes (ld*, st*, add, sub, etc.)
3. Implement call/ret handling
4. Implement branch handling
5. Test: Convert hello.dll IL → Fruity IR

**Deliverable:** Can convert IL bytecode to Fruity IR

### Phase 3: Extended Runtime (Week 3)
**Goal:** Support enough opcodes for real programs

**Tasks:**
1. Add string operations (ldstr, newstr, etc.)
2. Add comparison ops (ceq, cgt, clt, etc.)
3. Add conversion ops (conv.i4, conv.i8, etc.)
4. Add array operations (newarr, ldelem, stelem)
5. Test: Run hello.fs compiled binary

**Deliverable:** Can execute simple F# programs

### Phase 4: F# Core Types (Week 4)
**Goal:** Support F# Option, Result, List

**Tasks:**
1. Implement Option<'T> type
2. Implement Result<'T,'E> type
3. Implement List<'T> type
4. Test: F# program using Options and Results

**Deliverable:** Can use F# idioms

## Files We Need to Create

### 1. kernel/clr/il_parser.h
```c
// PE/COFF + CLI metadata parser
typedef struct {
    uint8_t *data;
    size_t size;
} il_assembly_t;

typedef struct {
    char *name;
    uint8_t *il_code;
    size_t il_code_size;
} il_method_t;

il_assembly_t* il_parse_assembly(const char *path);
il_method_t* il_get_method(il_assembly_t *asm, const char *name);
void il_free_assembly(il_assembly_t *asm);
```

### 2. kernel/clr/il_parser.c
```c
// ~500-800 lines
// Implement PE/COFF parsing
// Implement CLI metadata parsing
// Extract IL bytecode
```

### 3. kernel/clr/il_to_fruity.h
```c
// IL → Fruity IR converter
fruity_module_t* il_to_fruity(il_assembly_t *asm);
fruity_function_t* il_method_to_fruity(il_method_t *method);
```

### 4. kernel/clr/il_to_fruity.c
```c
// ~300-500 lines
// Map IL opcodes to Fruity opcodes
// Handle stack semantics
// Convert control flow
```

## Success Criteria

**Milestone 1: IL Parser**
- [ ] Can parse .NET PE/COFF file
- [ ] Can extract method metadata
- [ ] Can read IL bytecode stream
- [ ] Test: Print IL opcodes from hello.dll

**Milestone 2: IL → Fruity IR**
- [ ] Can convert basic IL to Fruity IR
- [ ] Can handle method calls
- [ ] Can handle branches
- [ ] Test: Convert hello.dll to Fruity IR

**Milestone 3: Code Generation**
- [ ] Can generate QBE IL from Fruity IR
- [ ] Can compile QBE IL to x86-64
- [ ] Can execute native code
- [ ] Test: Run "Hello from F#!" natively

**Milestone 4: F# Support**
- [ ] Can compile F# programs
- [ ] Can use F# Option types
- [ ] Can use F# Result types
- [ ] Test: F# program with pattern matching

## Next Immediate Action

**START HERE:** Implement IL parser (kernel/clr/il_parser.c)

**Why:** This unblocks the entire pipeline. Without IL parsing, we can't read F# compiled output.

**First test:**
```bash
# Compile F# to IL
fsc hello.fs  # Produces hello.dll

# Parse IL (our code)
./test_il_parser hello.dll
# Expected output:
# Method: main
# IL_0000: ldstr "Hello from F#!"
# IL_0005: call PrintfModule::PrintFormatLine
# IL_000a: ldc.i4.0
# IL_000b: ret
```

**Once IL parser works, implement IL → Fruity IR converter.**
