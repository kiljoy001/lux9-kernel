# Type-Aware Stack Translation - Implementation Guide

## Current Status

✅ **Infrastructure Complete:**
- FRUITY_DUP_REF and FRUITY_POP_REF opcodes defined
- QBE emission handlers implemented
- Opcode table entries added

⚠️ **Still Needed:**
Type tracking in `il_to_fruity_ctx_t` to decide which flavor to emit.

---

## Implementation Steps

### 1. Add Stack Type Tracking to Context

```c
// In il_to_fruity.c, line ~60 - add to il_to_fruity_ctx_t:

typedef struct il_to_fruity_ctx {
  // ... existing fields ...

  /* Compile-time stack type tracking for GC barriers */
  fruity_type_t *stack_types;   /* Type of each stack slot */
  size_t stack_depth;            /* Current stack depth */
  size_t stack_capacity;         /* Allocated capacity */
} il_to_fruity_ctx_t;
```

### 2. Initialize Stack in Context Creation

```c
// In il_to_fruity_begin() or similar:

ctx->stack_capacity = 128;  // Max stack depth
ctx->stack_types = xalloc(sizeof(fruity_type_t) * ctx->stack_capacity);
ctx->stack_depth = 0;
```

### 3. Implement Type Classification

```c
// Add helper function:

static int is_reference_type(fruity_type_t type) {
  switch (type) {
    case FRUITY_TYPE_OBJECT:
    case FRUITY_TYPE_STRING:
    case FRUITY_TYPE_ARRAY:
      return 1;
    default:
      return 0;
  }
}
```

### 4. Track Stack Effects for Every Instruction

For each IL instruction, update `stack_types`:

```c
// Example for ldarg.0 (loads argument):
case IL_LDARG_0:
  // Push argument type onto compile-time stack
  push_stack_type(ctx, ctx->method->arg_types[0]);
  // ... emit fruity instruction ...
  break;

// Example for add (pops 2, pushes 1):
case IL_ADD:
  pop_stack_type(ctx);  // Discard right operand type
  fruity_type_t left_type = pop_stack_type(ctx);
  push_stack_type(ctx, left_type);  // Result has left's type
  // ... emit fruity instruction ...
  break;
```

### 5. Update DUP/POP Translation

Replace lines 693-707 in `il_to_fruity.c`:

```c
case IL_DUP:
  if (ctx->stack_depth > 0) {
    fruity_type_t top_type = peek_stack_type(ctx);

    if (is_reference_type(top_type)) {
      instr = create_fruity_instruction(FRUITY_DUP_REF, operand, offset);
    } else {
      instr = create_fruity_instruction(FRUITY_DUP, operand, offset);
    }

    // Duplicate the type on compile-time stack
    push_stack_type(ctx, top_type);
  }
  *offset_ptr += 1;
  break;

case IL_POP:
  if (ctx->stack_depth > 0) {
    fruity_type_t top_type = pop_stack_type(ctx);

    if (is_reference_type(top_type)) {
      instr = create_fruity_instruction(FRUITY_POP_REF, operand, offset);
    } else {
      instr = create_fruity_instruction(FRUITY_POP, operand, offset);
    }
  }
  *offset_ptr += 1;
  break;
```

---

## Type Information Sources

### Option A: Use ECMA-335 Metadata

Parse method signatures from IL assembly metadata:
- `method->arg_types[]` - argument types
- `method->local_types[]` - local variable types
- Token resolution for field/method types

### Option B: Use IL Stack Transition Rules

ECMA-335 Partition III defines stack transitions for each opcode:
- `ldarg.0` pushes type of arg 0
- `ldc.i4` pushes int32
- `newobj` pushes object reference
- `add` pops 2, pushes numeric result

Implement a table-driven stack simulator.

---

## Testing

Add test case in `kernel/test/test_opcodes_phase1.c`:

```c
void test_dup_ref_emission(void) {
  /* IL sequence that duplicates a reference:
   *   newobj MyClass::.ctor
   *   dup              <- Should emit FRUITY_DUP_REF
   *   stloc.0
   */

  // Verify fruity IR contains FRUITY_DUP_REF, not FRUITY_DUP
  // Verify QBE contains "call $clr_object_addref"
}
```

---

## Proof Obligation

Once implemented, prove in Coq:

```coq
(* In proofs/clr/type_safety.v *)

Theorem dup_ref_preserves_refcount : forall st,
  is_reference_type (stack_top st) = true ->
  translate_dup st = FRUITY_DUP_REF ->
  refcount_after st = refcount_before st + 1.
Proof.
  (* Prove DUP_REF correctly increments refcount via white token *)
Qed.

Theorem dup_value_no_refcount : forall st,
  is_reference_type (stack_top st) = false ->
  translate_dup st = FRUITY_DUP ->
  refcount_after st = refcount_before st.
Proof.
  (* Prove DUP on value types doesn't touch refcount *)
Qed.
```

This ensures GC correctness is **formally verified**, not just tested.

---

## Estimated Effort

- Stack tracking infrastructure: ~150 LOC
- Type classification logic: ~50 LOC
- IL opcode stack effect table: ~300 LOC
- Testing: ~100 LOC
- Coq proofs: ~200 LOC

**Total: ~800 LOC to complete type-aware translation**

---

## Priority

**HIGH** - Without this, reference types will leak when duplicated on the stack, breaking the Pebble white token accounting and eventually exhausting memory.
