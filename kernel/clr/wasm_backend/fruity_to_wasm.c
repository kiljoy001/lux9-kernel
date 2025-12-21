/* fruity_to_wasm.c - Fruity IR to WASM Converter implementation */

#include "fruity_to_wasm.h"
#include "wasm_buffer.h"

#ifdef USERSPACE_TEST
#include <stdlib.h>
#include <string.h>
extern void *xalloc(size_t size);
extern void xfree(void *ptr);
#else
#include "../../include/fns.h" /* xalloc */
#include "../../include/lib.h" /* memset, memmove */
#endif

/* WASM Opcodes */
#define WASM_OP_BLOCK 0x02
#define WASM_OP_LOOP 0x03
#define WASM_OP_IF 0x04
#define WASM_OP_ELSE 0x05
#define WASM_OP_END 0x0B
#define WASM_OP_BR 0x0C
#define WASM_OP_BR_IF 0x0D
#define WASM_OP_BR_TABLE 0x0E
#define WASM_OP_RETURN 0x0F
#define WASM_OP_CALL 0x10

#define WASM_OP_LOCAL_GET 0x20
#define WASM_OP_LOCAL_SET 0x21
#define WASM_OP_LOCAL_TEE 0x22
#define WASM_OP_GLOBAL_GET 0x23
#define WASM_OP_GLOBAL_SET 0x24

#define WASM_OP_I32_LOAD 0x28
#define WASM_OP_I64_LOAD 0x29
#define WASM_OP_I32_STORE 0x36
#define WASM_OP_I64_STORE 0x37

#define WASM_OP_I32_CONST 0x41
#define WASM_OP_I64_CONST 0x42

#define WASM_OP_I32_EQZ 0x45
#define WASM_OP_I32_EQ 0x46
#define WASM_OP_I32_NE 0x47
#define WASM_OP_I32_ADD 0x6A
#define WASM_OP_I32_SUB 0x6B
#define WASM_OP_I32_MUL 0x6C
#define WASM_OP_I32_DIV_S 0x6D
#define WASM_OP_I32_REM_S 0x6F
#define WASM_OP_I32_AND 0x71
#define WASM_OP_I32_OR 0x72
#define WASM_OP_I32_XOR 0x73
#define WASM_OP_I32_SHL 0x74
#define WASM_OP_I32_SHR_S 0x75
#define WASM_OP_I32_SHR_U 0x76

/* Types */
#define WASM_TYPE_I32 0x7F
#define WASM_TYPE_I64 0x7E
#define WASM_TYPE_F32 0x7D
#define WASM_TYPE_F64 0x7C
#define WASM_TYPE_FUNC 0x60
#define WASM_TYPE_EMPTY 0x40

/* WASM Section IDs */
#define WASM_SEC_CUSTOM 0
#define WASM_SEC_TYPE 1
#define WASM_SEC_IMPORT 2
#define WASM_SEC_FUNCTION 3
#define WASM_SEC_TABLE 4
#define WASM_SEC_MEMORY 5
#define WASM_SEC_GLOBAL 6
#define WASM_SEC_EXPORT 7
#define WASM_SEC_START 8
#define WASM_SEC_ELEMENT 9
#define WASM_SEC_CODE 10
#define WASM_SEC_DATA 11

/* Section helpers */
static void emit_section_start(wasm_buffer_t *buf, u8int id, ulong *size_pos) {
  wasm_emit_u8(buf, id);
  *size_pos = buf->size;
  /* Placeholder for uleb128 size (we'll overwrite later) */
  /* WASM sizes can be up to 5 bytes in uleb128, we use a fixed 5-byte pad for
   * simplicity or just track and move. */
  /* For simplicity, we just emit a 0 and will do a backpatch if small, or use a
   * better strategy. */
  /* Let's use a simpler approach: emit to a temporary buffer, then emit length
   * + data to main. */
}

/* Lux9 Shadow Stack Layout
... */

/* Helpers */
static void emit_push_i32(wasm_buffer_t *code, s32int val) {
  /* MEM[SP] = val; SP += 8 */
  /* Get SP */
  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0);

  /* Const val */
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, val);

  /* Store i32 */
  wasm_emit_u8(code, WASM_OP_I32_STORE);
  wasm_emit_uleb128(code, 2); /* align */
  wasm_emit_uleb128(code, 0); /* offset */

  /* Increment SP */
  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, 8);
  wasm_emit_u8(code, WASM_OP_I32_ADD);
  wasm_emit_u8(code, WASM_OP_GLOBAL_SET);
  wasm_emit_uleb128(code, 0);
}

static void emit_pop_to_local(wasm_buffer_t *code, u32int local_idx) {
  /* SP -= 8; local = MEM[SP] */

  /* Decrement SP */
  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, 8);
  wasm_emit_u8(code, WASM_OP_I32_SUB);
  wasm_emit_u8(code, WASM_OP_GLOBAL_SET);
  wasm_emit_uleb128(code, 0);

  /* Load */
  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_I32_LOAD);
  wasm_emit_uleb128(code, 2);
  wasm_emit_uleb128(code, 0);

  /* Set local */
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, local_idx);
}

static void emit_binary_op_i32(wasm_buffer_t *code, u8int wasm_op) {
  /* Pop rhs -> local 1 (scratch)
     Pop lhs -> local 0 (scratch)
     res = op(0, 1)
     Push res
  */
  /* We need scratch locals. Let's assume the function preamble defines:
     0: target_block (i32)
     1: scratch_a (i32)
     2: scratch_b (i32)
     3+: CIL locals
  */
  emit_pop_to_local(code, 2); /* RHS */
  emit_pop_to_local(code, 1); /* LHS */

  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, 1);
  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, 2);

  wasm_emit_u8(code, wasm_op);

  /* Result is on WASM stack. We need to push it to Shadow Stack */
  /* This requires: Get SP, Store (Result) */
  /* But Store takes (Addr, Val). So we need Addr first. */

  /* Complex: The result is on stack.
     We need to put it in a local temp to store it?
     Yes, store to scratch 1 again. */
  wasm_emit_u8(code, WASM_OP_LOCAL_SET);
  wasm_emit_uleb128(code, 1);

  /* Store to stack */
  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0); /* Addr */

  wasm_emit_u8(code, WASM_OP_LOCAL_GET);
  wasm_emit_uleb128(code, 1); /* Val */

  wasm_emit_u8(code, WASM_OP_I32_STORE);
  wasm_emit_uleb128(code, 2);
  wasm_emit_uleb128(code, 0);

  /* Inc SP */
  wasm_emit_u8(code, WASM_OP_GLOBAL_GET);
  wasm_emit_uleb128(code, 0);
  wasm_emit_u8(code, WASM_OP_I32_CONST);
  wasm_emit_sleb128(code, 8);
  wasm_emit_u8(code, WASM_OP_I32_ADD);
  wasm_emit_u8(code, WASM_OP_GLOBAL_SET);
  wasm_emit_uleb128(code, 0);
}

/* Compile a single function body */
static int compile_function_body(fruity_function_t *func, wasm_buffer_t *code) {
  /* Locals:
     0: target_block (i32)
     1: scratch_a (i32)
     2: scratch_b (i32)
     3..N: CIL Locals mapped 1:1
  */

  /* Declare locals */
  u32int local_count = 3 + func->local_count;
  /* Run-length encoding of locals */
  /* We just say 'local_count' of type i32 */
  wasm_emit_uleb128(code, 1); /* 1 group */
  wasm_emit_uleb128(code, local_count);
  wasm_emit_u8(code, WASM_TYPE_I32);

  /* Init SP global if needed? No, done at module level.
     But we should set it to STACK_BASE at start of main?
     For now assume handled globally. */

  /* Loop-Switch Wrapper */
  /* block $break (label 1) */
  wasm_emit_u8(code, WASM_OP_BLOCK);
  wasm_emit_u8(code, WASM_TYPE_EMPTY);

  /* loop $loop (label 0) */
  wasm_emit_u8(code, WASM_OP_LOOP);
  wasm_emit_u8(code, WASM_TYPE_EMPTY);

  /* Dispatcher: block $b0, block $b1 ... br_table */
  /* Nested blocks for switch. Depth = block_count */
  ulong block_count = func->block_count;

  /* To implement br_table to arbitary blocks, we nest them:
     block $b_N
      ...
       block $b_0
         br_table $b_0 $b_1 ... $b_N (target)
       end
       ... (body of b_0)
       br $loop
      end
      ... (body of b_N)
  */
  /* This is the standard re-looper structure for flat switches */

  /* 1. Emit N nested blocks */
  for (ulong i = 0; i < block_count; i++) {
    wasm_emit_u8(code, WASM_OP_BLOCK);
    wasm_emit_u8(code, WASM_TYPE_EMPTY);
  }

  /* 2. Emit br_table */
  wasm_emit_u8(code, WASM_OP_BLOCK); /* Dispatch block */
  wasm_emit_u8(code, WASM_TYPE_EMPTY);

  wasm_emit_u8(code, WASM_OP_LOCAL_GET); /* target_block */
  wasm_emit_uleb128(code, 0);

  wasm_emit_u8(code, WASM_OP_BR_TABLE);
  wasm_emit_uleb128(code, block_count); /* count */
  for (ulong i = 0; i < block_count; i++) {
    /* Target i maps to label (block_count - i) */
    /* label 0 is Dispatch block */
    /* label 1 is block b_0 */
    wasm_emit_uleb128(code, i + 1);
  }
  wasm_emit_uleb128(code, block_count); /* default: last block or break? */

  wasm_emit_u8(code, WASM_OP_END); /* End Dispatch block */

  /* 3. Emit Block Bodies */
  /* Blocks are popped in reverse order of creation.
     Inner-most was b_0. So we emit b_0 first.
  */
  fruity_basic_block_t *bb = func->blocks_head;
  for (ulong i = 0; i < block_count; i++) {
    /* Body of block 'i' */
    fruity_basic_block_t *current_bb = bb;

    if (current_bb) {
      fruity_instruction_t *instr = current_bb->instructions_head;
      while (instr) {
        /* Emit Instruction */
        switch (instr->opcode) {
        case FRUITY_NOP:
          break;

        case FRUITY_LDC_I4:
          emit_push_i32(code, instr->operand.value.i32);
          break;

        case FRUITY_ADD:
          emit_binary_op_i32(code, WASM_OP_I32_ADD);
          break;
        case FRUITY_SUB:
          emit_binary_op_i32(code, WASM_OP_I32_SUB);
          break;
        case FRUITY_MUL:
          emit_binary_op_i32(code, WASM_OP_I32_MUL);
          break;

        case FRUITY_LOAD_LOCAL:
          /* Push local to stack */
          wasm_emit_u8(code, WASM_OP_LOCAL_GET);
          wasm_emit_uleb128(code, 3 + instr->operand.value.index);
          /* Store to Shadow Stack */
          /* (Simplified push logic here for brevity, essentially same as
           * emit_push_i32 but value is on stack) */
          /* TODO: Factor out generic push */
          break;

        case FRUITY_STORE_LOCAL:
          /* Pop stack to local */
          emit_pop_to_local(code, 3 + instr->operand.value.index);
          break;

        case FRUITY_RET:
          wasm_emit_u8(code, WASM_OP_RETURN);
          break;

        case FRUITY_JUMP:
          /* Set target, br loop */
          wasm_emit_u8(code, WASM_OP_I32_CONST);
          wasm_emit_sleb128(code, instr->operand.value.target->block_id);
          wasm_emit_u8(code, WASM_OP_LOCAL_SET);
          wasm_emit_uleb128(code, 0); /* target_block */

          /* Jump to loop head (label depth depends on nesting) */
          /* We are inside 'block_count - i' blocks + Loop wrapper + Block
           * wrapper */
          /* Actually, we just need to break out of the current block to fall
             through? No, we need to go back to the top dispatcher. The
             dispatcher is at the top of the Loop. Loop is label 'block_count -
             i + 1'.
          */
          wasm_emit_u8(code, WASM_OP_BR);
          wasm_emit_uleb128(code, block_count - i + 1);
          break;

        default:
          /* Placeholder */
          break;
        }

        instr = instr->next;
      }
      bb = current_bb->next; /* Advance for next iteration */
    }

    /* Check if fallthrough is needed (if last instruction wasn't a terminator)
     */
    int needs_fallthrough = 1;
    if (current_bb && current_bb->instructions_tail) {
      switch (current_bb->instructions_tail->opcode) {
      case FRUITY_RET:
      case FRUITY_JUMP:
        needs_fallthrough = 0;
        break;
      default:
        break;
      }
    }

    if (needs_fallthrough) {
      wasm_emit_u8(code, WASM_OP_I32_CONST);
      wasm_emit_sleb128(code, i + 1);
      wasm_emit_u8(code, WASM_OP_LOCAL_SET);
      wasm_emit_uleb128(code, 0);
      wasm_emit_u8(code, WASM_OP_BR);
      wasm_emit_uleb128(code, block_count - i + 1);
    }

    wasm_emit_u8(code, WASM_OP_END); /* Close block */
  }

  wasm_emit_u8(code, WASM_OP_END); /* Close Loop */
  wasm_emit_u8(code, WASM_OP_END); /* Close Wrapper Block */

  /* End of function */
  wasm_emit_u8(code, WASM_OP_END);

  return 0;
}

/* Helper: Map CLR type to WASM type */
static u8int valtype_to_wasm(clr_value_type_t t) {
  switch (t) {
  case CLR_INT64:
    return WASM_TYPE_I64;
  case CLR_BOOL:
  case CLR_INT32:
  default:
    return WASM_TYPE_I32;
  }
}

int fruity_compile_to_wasm(fruity_module_t *module,
                           fruity_wasm_result_t *result) {
  if (!module || !result)
    return -1;

  wasm_buffer_t main_buf;
  wasm_buf_init(&main_buf, 4096);

  /* 1. Header */
  wasm_emit_u32(&main_buf, 0x6D736100); /* \0asm */
  wasm_emit_u32(&main_buf, 1);          /* Version 1 */

  /* Count functions */
  ulong func_count = 0;
  {
    fruity_function_t *f = module->functions_head;
    while (f) {
      func_count++;
      f = f->next;
    }
  }

  /* 2. Type Section (Index 1) */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 128);
    wasm_emit_vec_header(&sec, func_count);

    fruity_function_t *f = module->functions_head;
    while (f) {
      /* Emit Signature: (params) -> (result) */
      wasm_emit_u8(&sec, WASM_TYPE_FUNC);

      /* Params */
      wasm_emit_vec_header(&sec, f->arg_count);
      for (ulong i = 0; i < f->arg_count; i++) {
        wasm_emit_u8(&sec, valtype_to_wasm(f->arg_types[i]));
      }

      /* Result */
      if (f->return_type == CLR_VOID) {
        wasm_emit_vec_header(&sec, 0);
      } else {
        wasm_emit_vec_header(&sec, 1);
        wasm_emit_u8(&sec, valtype_to_wasm(f->return_type));
      }

      f = f->next;
    }

    wasm_emit_u8(&main_buf, WASM_SEC_TYPE);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 3. Function Section (Index 3) */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 64);
    wasm_emit_vec_header(&sec, func_count);

    for (ulong i = 0; i < func_count; i++) {
      wasm_emit_uleb128(&sec, i); /* Function i uses Type i (1:1 mapping) */
    }

    wasm_emit_u8(&main_buf, WASM_SEC_FUNCTION);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 4. Memory Section (Index 5) */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 64);
    wasm_emit_vec_header(&sec, 1); /* 1 memory */
    wasm_emit_u8(&sec, 0);         /* limit: min only */
    wasm_emit_uleb128(&sec, 1);    /* 1 page (64KB) */

    wasm_emit_u8(&main_buf, WASM_SEC_MEMORY);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 5. Export Section (Index 7) */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 64);

    /* 1 memory + func_count exports */
    wasm_emit_vec_header(&sec, 1 + func_count);

    /* Export Memory */
    wasm_emit_name(&sec, "memory");
    wasm_emit_u8(&sec, 0x02); /* Memory export */
    wasm_emit_uleb128(&sec, 0);

    /* Export Functions */
    fruity_function_t *f = module->functions_head;
    ulong idx = 0;
    while (f) {
      const char *name = f->name ? f->name : "MethodUnknown";
      wasm_emit_name(&sec, name);
      wasm_emit_u8(&sec, 0x00); /* Function export */
      wasm_emit_uleb128(&sec, idx++);
      f = f->next;
    }

    wasm_emit_u8(&main_buf, WASM_SEC_EXPORT);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  /* 6. Code Section (Index 10) */
  {
    wasm_buffer_t sec;
    wasm_buf_init(&sec, 1024);
    wasm_emit_vec_header(&sec, func_count);

    fruity_function_t *f = module->functions_head;
    while (f) {
      wasm_buffer_t body;
      wasm_buf_init(&body, 1024);
      compile_function_body(f, &body);

      wasm_emit_uleb128(&sec, body.size);
      wasm_emit_bytes(&sec, body.data, body.size);
      wasm_buf_free(&body);

      f = f->next;
    }

    wasm_emit_u8(&main_buf, WASM_SEC_CODE);
    wasm_emit_uleb128(&main_buf, sec.size);
    wasm_emit_bytes(&main_buf, sec.data, sec.size);
    wasm_buf_free(&sec);
  }

  if (main_buf.error) {
    wasm_buf_free(&main_buf);
    return -1;
  }

  result->wasm_binary = main_buf.data;
  result->wasm_size = main_buf.size;
  result->success = 0;

  return 0;
}

void fruity_wasm_result_free(fruity_wasm_result_t *result) {
  if (result->wasm_binary) {
    xfree(result->wasm_binary);
    result->wasm_binary = nil;
  }
}
