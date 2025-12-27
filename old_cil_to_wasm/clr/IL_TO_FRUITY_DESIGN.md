# IL → Fruity IR Converter Design

## Goal
Convert .NET IL bytecode (stack-based) to Fruity IR (explicit Pebble operations).

## Architecture

### Input
- `il_method_t` from IL parser
- IL bytecode as byte stream
- Metadata tokens (strings, types, methods)

### Output
- `fruity_function_t` with CFG
- Basic blocks with Fruity instructions
- Explicit LIME/VANILLA/BURN operations

## IL → Fruity Mapping

### Constants
```
IL                    →  Fruity IR
─────────────────────────────────────────
ldc.i4.0              →  FRUITY_LDC_I4(0)
ldc.i4.1              →  FRUITY_LDC_I4(1)
ldc.i4 <value>        →  FRUITY_LDC_I4(value)
ldc.i8 <value>        →  FRUITY_LDC_I8(value)
ldc.r4 <value>        →  FRUITY_LDC_R4(value)
ldc.r8 <value>        →  FRUITY_LDC_R8(value)
ldnull                →  FRUITY_LDNULL
```

### Arithmetic
```
IL                    →  Fruity IR
─────────────────────────────────────────
add                   →  FRUITY_ADD
sub                   →  FRUITY_SUB
mul                   →  FRUITY_MUL
div                   →  FRUITY_DIV
rem                   →  FRUITY_REM
neg                   →  FRUITY_NEG
and                   →  FRUITY_AND
or                    →  FRUITY_OR
xor                   →  FRUITY_XOR
not                   →  FRUITY_NOT
shl                   →  FRUITY_SHL
shr                   →  FRUITY_SHR
```

### Comparison
```
IL                    →  Fruity IR
─────────────────────────────────────────
ceq                   →  FRUITY_CEQ
clt                   →  FRUITY_CLT
cgt                   →  FRUITY_CGT
```

### Locals & Arguments
```
IL                    →  Fruity IR
─────────────────────────────────────────
ldloc.0               →  FRUITY_LOAD_LOCAL(0)
ldloc.1               →  FRUITY_LOAD_LOCAL(1)
ldloc.s <index>       →  FRUITY_LOAD_LOCAL(index)
stloc.0               →  FRUITY_STORE_LOCAL(0)
stloc.1               →  FRUITY_STORE_LOCAL(1)
stloc.s <index>       →  FRUITY_STORE_LOCAL(index)
ldarg.0               →  FRUITY_LOAD_ARG(0)
ldarg.1               →  FRUITY_LOAD_ARG(1)
ldarg.s <index>       →  FRUITY_LOAD_ARG(index)
starg.s <index>       →  FRUITY_STORE_ARG(index)
```

### Object Operations
```
IL                    →  Fruity IR
─────────────────────────────────────────
newobj <token>        →  FRUITY_LIME(type_token)
ldfld <token>         →  FRUITY_LOAD_FIELD(field_token)
stfld <token>         →  FRUITY_STORE_FIELD(field_token)
ldstr <token>         →  FRUITY_LIME(string_type)
                         + FRUITY_LOAD_STRING(token)
```

### Stack Operations
```
IL                    →  Fruity IR
─────────────────────────────────────────
dup                   →  FRUITY_DUP (VANILLA for refs)
pop                   →  FRUITY_POP (BURN for refs)
```

### Control Flow
```
IL                    →  Fruity IR
─────────────────────────────────────────
call <token>          →  FRUITY_CALL(method_token)
ret                   →  FRUITY_RET
br <target>           →  FRUITY_JUMP(target_block)
br.s <target>         →  FRUITY_JUMP(target_block)
beq <target>          →  FRUITY_BEQ(target_block)
bne.un <target>       →  FRUITY_BNE(target_block)
blt <target>          →  FRUITY_BLT(target_block)
ble <target>          →  FRUITY_BLE(target_block)
bgt <target>          →  FRUITY_BGT(target_block)
bge <target>          →  FRUITY_BGE(target_block)
brtrue <target>       →  FRUITY_BTRUE(target_block)
brfalse <target>      →  FRUITY_BFALSE(target_block)
```

### Arrays
```
IL                    →  Fruity IR
─────────────────────────────────────────
newarr <token>        →  FRUITY_NEWARR(element_type)
ldlen                 →  FRUITY_LDLEN
ldelem <type>         →  FRUITY_LDELEM
stelem <type>         →  FRUITY_STELEM
ldelema <token>       →  FRUITY_LDELEMA(element_type)
```

### Conversions
```
IL                    →  Fruity IR
─────────────────────────────────────────
conv.i4               →  FRUITY_CONV_I4
conv.i8               →  FRUITY_CONV_I8
conv.r4               →  FRUITY_CONV_R4
conv.r8               →  FRUITY_CONV_R8
```

## Implementation Strategy

### Phase 1: Basic Block Identification
1. Scan IL bytecode for branch targets
2. Create basic blocks at:
   - Method entry point (offset 0)
   - Branch targets
   - Instructions after branches
3. Build CFG edges

### Phase 2: Instruction Translation
For each basic block:
1. Decode IL instruction
2. Map to Fruity opcode(s)
3. Handle stack semantics
4. Create Fruity instruction node
5. Link into basic block

### Phase 3: Reference Tracking
1. Track value types vs reference types on stack
2. Insert VANILLA for reference duplication
3. Insert BURN for reference disposal
4. Insert LEMON for ownership transfer

### Phase 4: Metadata Resolution
1. Resolve type tokens
2. Resolve method tokens
3. Resolve field tokens
4. Resolve string literals

## API Design

```c
/* il_to_fruity.h */

/* Convert IL method to Fruity function */
fruity_function_t* il_to_fruity_convert_method(
    il_assembly_t *assembly,
    il_method_t *method
);

/* Convert entire assembly to Fruity module */
fruity_module_t* il_to_fruity_convert_assembly(
    il_assembly_t *assembly
);

/* Free Fruity function */
void fruity_free_function(fruity_function_t *func);

/* Free Fruity module */
void fruity_free_module(fruity_module_t *mod);
```

## Example Translation

### F# Source
```fsharp
[<EntryPoint>]
let main args =
    printfn "Hello from F#!"
    0
```

### IL Bytecode
```
IL_0000: ldstr        0x70000001   // "Hello from F#!"
IL_0005: call         0x0A000006   // PrintfModule::PrintFormatLine
IL_000a: ldc.i4.0                  // Push 0
IL_000b: ret                       // Return
```

### Fruity IR (pseudo-code)
```
block_0:
    %0 = FRUITY_LIME(type=String)          ; Allocate string object
    %1 = FRUITY_LOAD_STRING(0x70000001)     ; Load "Hello from F#!"
    FRUITY_CALL(0x0A000006, %1)             ; Call printf
    FRUITY_BURN(%1)                         ; Release string ref
    %2 = FRUITY_LDC_I4(0)                   ; Load constant 0
    FRUITY_RET(%2)                          ; Return 0
```

## Type Tracking

We need to track types on the stack to determine when to use VANILLA/BURN:

```c
typedef enum {
    STACK_VALUE,    // Value type (int32, float, etc.)
    STACK_REF,      // Reference type (objects, strings)
    STACK_UNKNOWN   // Unknown (conservative)
} stack_entry_type_t;

typedef struct {
    stack_entry_type_t type;
    uint32_t type_token;  // ECMA-335 metadata token
} stack_entry_t;
```

## Error Handling

```c
typedef enum {
    IL_TO_FRUITY_OK,
    IL_TO_FRUITY_ERROR_INVALID_IL,
    IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE,
    IL_TO_FRUITY_ERROR_STACK_UNDERFLOW,
    IL_TO_FRUITY_ERROR_OUT_OF_MEMORY,
    IL_TO_FRUITY_ERROR_METADATA,
} il_to_fruity_error_t;
```

## Next Steps

1. Implement `il_to_fruity.h` API
2. Implement basic block builder
3. Implement instruction translator
4. Test with simple F# program
5. Verify Fruity IR correctness

## Test Plan

### Test 1: Constants
```fsharp
let x = 42
```
Expected: FRUITY_LDC_I4(42)

### Test 2: Arithmetic
```fsharp
let x = 5 + 3
```
Expected: FRUITY_LDC_I4(5), FRUITY_LDC_I4(3), FRUITY_ADD

### Test 3: Conditionals
```fsharp
if x > 0 then 1 else 0
```
Expected: FRUITY_CGT, FRUITY_BTRUE

### Test 4: Function Call
```fsharp
printfn "Hello"
```
Expected: FRUITY_LIME, FRUITY_LOAD_STRING, FRUITY_CALL, FRUITY_BURN

### Test 5: Local Variables
```fsharp
let x = 42
let y = x + 1
```
Expected: FRUITY_LDC_I4(42), FRUITY_STORE_LOCAL(0), FRUITY_LOAD_LOCAL(0), etc.

---

**Status**: Design complete, ready for implementation
**Next**: Implement `il_to_fruity.c` with basic block builder
