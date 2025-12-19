/* fruity_sljit.c - Fruity IR to sljit JIT compiler bridge
 *
 * Translates Fruity IR to sljit operations, generating native machine code.
 * sljit handles the platform-specific code generation for x86-64, ARM, etc.
 */

/* Include sljit as all-in-one compilation unit */
#define SLJIT_CONFIG_AUTO 1

#ifdef USERSPACE_TEST
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define nil NULL
#define snprint snprintf
extern void *xalloc(size_t);
extern void xfree(void *);
#else
#include "../../include/dat.h"
#include "../../include/fns.h"

/* Standard C type compatibility for kernel mode */
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef unsigned long size_t;
#endif

/* sljit configuration for kernel use */
#ifndef USERSPACE_TEST
#define SLJIT_UTIL_STACK 0 /* No stack management in kernel */
#endif

#include "../sljit/sljit_src/sljitLir.c"

#include "../fruity/fruity_opcodes.h"
#include "fruity_sljit.h"

/* JIT context */
struct fruity_jit_ctx {
  struct sljit_compiler *compiler;

  /* Label map: Fruity block ID -> sljit label */
  struct sljit_label **labels;
  size_t label_count;

  /* Jump fixups needed */
  struct {
    struct sljit_jump *jump;
    uint32_t target_block_id;
  } *fixups;
  size_t fixup_count;
  size_t fixup_capacity;
};

/* ===== Context Management ===== */

fruity_jit_ctx_t *fruity_jit_create(void) {
  fruity_jit_ctx_t *ctx = xalloc(sizeof(fruity_jit_ctx_t));
  if (!ctx)
    return nil;

  memset(ctx, 0, sizeof(fruity_jit_ctx_t));
  return ctx;
}

void fruity_jit_destroy(fruity_jit_ctx_t *ctx) {
  if (!ctx)
    return;
  if (ctx->labels)
    xfree(ctx->labels);
  if (ctx->fixups)
    xfree(ctx->fixups);
  xfree(ctx);
}

/* ===== Helper Functions ===== */

static void add_fixup(fruity_jit_ctx_t *ctx, struct sljit_jump *jump,
                      uint32_t target) {
  if (ctx->fixup_count >= ctx->fixup_capacity) {
    size_t new_cap = ctx->fixup_capacity ? ctx->fixup_capacity * 2 : 16;
    ctx->fixups = realloc(ctx->fixups, new_cap * sizeof(ctx->fixups[0]));
    ctx->fixup_capacity = new_cap;
  }
  ctx->fixups[ctx->fixup_count].jump = jump;
  ctx->fixups[ctx->fixup_count].target_block_id = target;
  ctx->fixup_count++;
}

/* Map Fruity register to sljit register */
static sljit_s32 fruity_to_sljit_reg(int fruity_reg) {
  /* Use scratch registers R0-R5 for evaluation stack */
  switch (fruity_reg) {
  case 0:
    return SLJIT_R0;
  case 1:
    return SLJIT_R1;
  case 2:
    return SLJIT_R2;
  case 3:
    return SLJIT_R3;
  case 4:
    return SLJIT_R4;
  default:
    return SLJIT_R0;
  }
}

/* ===== Instruction Translation ===== */

static int emit_instruction(fruity_jit_ctx_t *ctx, struct sljit_compiler *C,
                            fruity_instruction_t *instr, int *sp) {
  sljit_s32 dst, src1, src2;

  switch (instr->opcode) {

  /* ===== Constants ===== */
  case FRUITY_LDC_I4:
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op1(C, SLJIT_MOV, dst, 0, SLJIT_IMM, instr->operand.value.i32);
    (*sp)++;
    break;

  case FRUITY_LDC_I8:
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op1(C, SLJIT_MOV, dst, 0, SLJIT_IMM, instr->operand.value.i64);
    (*sp)++;
    break;

  case FRUITY_LDNULL:
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op1(C, SLJIT_MOV, dst, 0, SLJIT_IMM, 0);
    (*sp)++;
    break;

  /* ===== Arithmetic ===== */
  case FRUITY_ADD:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_ADD, dst, 0, src1, 0, src2, 0);
    (*sp)++;
    break;

  case FRUITY_SUB:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_SUB, dst, 0, src1, 0, src2, 0);
    (*sp)++;
    break;

  case FRUITY_MUL:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_MUL, dst, 0, src1, 0, src2, 0);
    (*sp)++;
    break;

  case FRUITY_DIV:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op0(C, SLJIT_DIV_SW); /* Signed divide */
    (*sp)++;
    break;

  case FRUITY_NEG:
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_SUB, dst, 0, SLJIT_IMM, 0, src1, 0);
    (*sp)++;
    break;

  /* ===== Bitwise ===== */
  case FRUITY_AND:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_AND, dst, 0, src1, 0, src2, 0);
    (*sp)++;
    break;

  case FRUITY_OR:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_OR, dst, 0, src1, 0, src2, 0);
    (*sp)++;
    break;

  case FRUITY_XOR:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_XOR, dst, 0, src1, 0, src2, 0);
    (*sp)++;
    break;

  case FRUITY_SHL:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_SHL, dst, 0, src1, 0, src2, 0);
    (*sp)++;
    break;

  case FRUITY_SHR:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_ASHR, dst, 0, src1, 0, src2,
                   0); /* Arithmetic shift */
    (*sp)++;
    break;

  case FRUITY_SHR_UN:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2(C, SLJIT_LSHR, dst, 0, src1, 0, src2, 0); /* Logical shift */
    (*sp)++;
    break;

  /* ===== Comparison ===== */
  case FRUITY_CEQ:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2u(C, SLJIT_SUB | SLJIT_SET_Z, src1, 0, src2, 0);
    sljit_emit_op_flags(C, SLJIT_MOV, dst, 0, SLJIT_EQUAL);
    (*sp)++;
    break;

  case FRUITY_CLT:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2u(C, SLJIT_SUB | SLJIT_SET_SIG_LESS, src1, 0, src2, 0);
    sljit_emit_op_flags(C, SLJIT_MOV, dst, 0, SLJIT_SIG_LESS);
    (*sp)++;
    break;

  case FRUITY_CGT:
    (*sp)--;
    src2 = fruity_to_sljit_reg(*sp);
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op2u(C, SLJIT_SUB | SLJIT_SET_SIG_GREATER, src1, 0, src2, 0);
    sljit_emit_op_flags(C, SLJIT_MOV, dst, 0, SLJIT_SIG_GREATER);
    (*sp)++;
    break;

  /* ===== Stack Operations ===== */
  case FRUITY_DUP:
    src1 = fruity_to_sljit_reg(*sp - 1);
    dst = fruity_to_sljit_reg(*sp);
    sljit_emit_op1(C, SLJIT_MOV, dst, 0, src1, 0);
    (*sp)++;
    break;

  case FRUITY_POP:
    (*sp)--;
    break;

  /* ===== Local Variables ===== */
  case FRUITY_LOAD_LOCAL: {
    uint32_t idx = instr->operand.value.index;
    dst = fruity_to_sljit_reg(*sp);
    /* Locals are on stack, offset from frame pointer */
    sljit_emit_op1(C, SLJIT_MOV, dst, 0, SLJIT_MEM1(SLJIT_SP),
                   idx * sizeof(sljit_sw));
    (*sp)++;
    break;
  }

  case FRUITY_STORE_LOCAL: {
    uint32_t idx = instr->operand.value.index;
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    sljit_emit_op1(C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), idx * sizeof(sljit_sw),
                   src1, 0);
    break;
  }

  case FRUITY_LOAD_ARG: {
    uint32_t idx = instr->operand.value.index;
    dst = fruity_to_sljit_reg(*sp);
    /* Arguments are passed in Saved registers S0, S1, etc. */
    /* Note: We requested 2 saved registers in sljit_emit_enter */
    /* TODO: Spilling if > 2 args */
    sljit_emit_op1(C, SLJIT_MOV, dst, 0, SLJIT_S(idx), 0);
    (*sp)++;
    break;
  }

  /* ===== Control Flow ===== */
  case FRUITY_RET:
    if (*sp > 0) {
      /* Move result to return register */
      src1 = fruity_to_sljit_reg(*sp - 1);
      sljit_emit_op1(C, SLJIT_MOV, SLJIT_RETURN_REG, 0, src1, 0);
    }
    sljit_emit_return(C, SLJIT_MOV, SLJIT_RETURN_REG, 0);
    break;

  case FRUITY_JUMP:
    if (instr->operand.value.target) {
      struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_JUMP);
      add_fixup(ctx, jump, instr->operand.value.target->block_id);
    }
    break;

  case FRUITY_BTRUE:
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    sljit_emit_op2u(C, SLJIT_SUB | SLJIT_SET_Z, src1, 0, SLJIT_IMM, 0);
    if (instr->operand.value.target) {
      struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_NOT_EQUAL);
      add_fixup(ctx, jump, instr->operand.value.target->block_id);
    }
    break;

  case FRUITY_BFALSE:
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);
    sljit_emit_op2u(C, SLJIT_SUB | SLJIT_SET_Z, src1, 0, SLJIT_IMM, 0);
    if (instr->operand.value.target) {
      struct sljit_jump *jump = sljit_emit_jump(C, SLJIT_EQUAL);
      add_fixup(ctx, jump, instr->operand.value.target->block_id);
    }
    break;

  /* ===== Pebble Memory Operations ===== */
  case FRUITY_LIME: {
    /* Allocate: size → obj_ref */
    /* Call pebble_alloc(size) - external function call */
    (*sp)--;
    src1 = fruity_to_sljit_reg(*sp);

    /* Move arg to R0 for function call */
    if (src1 != SLJIT_R0)
      sljit_emit_op1(C, SLJIT_MOV, SLJIT_R0, 0, src1, 0);

    /* TODO: Call external pebble_alloc function */
    /* For now, just keep the size as result (placeholder) */
    dst = fruity_to_sljit_reg(*sp);
    (*sp)++;
    break;
  }

  case FRUITY_VANILLA:
    /* Share reference - no-op for now */
    break;

  case FRUITY_BURN:
    /* Release reference */
    (*sp)--;
    break;

  /* ===== NOP and Debug ===== */
  case FRUITY_NOP:
  case FRUITY_BREAK:
    /* No operation */
    break;

  default:
    /* Unsupported opcode - skip */
    break;
  }

  return 0;
}

/* Common emission logic for JIT and AOT */
static int emit_all_instructions(fruity_jit_ctx_t *ctx,
                                 struct sljit_compiler *C,
                                 fruity_function_t *func, char *err_buf,
                                 size_t err_len) {
  fruity_basic_block_t *block;
  fruity_instruction_t *instr;
  int sp = 0; /* Virtual stack pointer */

  /* Allocate label array */
  ctx->label_count = func->block_count;
  ctx->labels = xalloc(ctx->label_count * sizeof(struct sljit_label *));
  if (!ctx->labels) {
    return -1;
  }

  /* Emit function prologue */
  sljit_emit_enter(C, 0, SLJIT_ARGS1(W, W), /* 1 arg, returns word */
                   5,                       /* 5 scratch registers */
                   2,                       /* 2 saved registers */
                   func->local_count * sizeof(sljit_sw)); /* Stack for locals */

  /* Emit each basic block */
  size_t block_idx = 0;
  for (block = func->blocks_head; block != nil; block = block->next) {
    /* Define label for this block */
    ctx->labels[block_idx] = sljit_emit_label(C);

    /* Emit instructions */
    for (instr = block->instructions_head; instr != nil; instr = instr->next) {
      if (emit_instruction(ctx, C, instr, &sp) < 0) {
        snprint(err_buf, err_len, "Failed to emit instruction at offset %u",
                (unsigned)instr->msil_offset);
        return -1;
      }
    }

    block_idx++;
  }

  /* Resolve jump fixups */
  for (size_t i = 0; i < ctx->fixup_count; i++) {
    uint32_t target_id = ctx->fixups[i].target_block_id;
    if (target_id < ctx->label_count) {
      sljit_set_label(ctx->fixups[i].jump, ctx->labels[target_id]);
    }
  }

  return 0;
}

/* ===== Main Compilation Entry ===== */

int fruity_jit_compile(fruity_jit_ctx_t *ctx, fruity_function_t *func,
                       fruity_jit_result_t *result) {
  struct sljit_compiler *C;

  if (!ctx || !func || !result) {
    if (result) {
      snprint(result->error_msg, sizeof(result->error_msg),
              "Invalid parameters");
      result->success = -1;
    }
    return -1;
  }

  memset(result, 0, sizeof(*result));

  /* Create sljit compiler */
  C = sljit_create_compiler(nil);
  if (!C) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "Failed to create sljit compiler");
    result->success = -1;
    return -1;
  }
  ctx->compiler = C;

  /* Emit instructions */
  if (emit_all_instructions(ctx, C, func, result->error_msg,
                            sizeof(result->error_msg)) < 0) {
    sljit_free_compiler(C);
    result->success = -1;
    return -1;
  }

  /* Generate code */
  result->code = sljit_generate_code(C, 0, nil);
  if (!result->code) {
    sljit_free_compiler(C);
    snprint(result->error_msg, sizeof(result->error_msg),
            "Failed to generate native code");
    result->success = -1;
    return -1;
  }

  result->code_size = sljit_get_generated_code_size(C);
  result->success = 0;

  sljit_free_compiler(C);
  return 0;
}

int fruity_aot_compile(fruity_jit_ctx_t *ctx, fruity_function_t *func,
                       void **buffer, size_t *size) {
  struct sljit_compiler *C;
  char error_msg[256];

  if (!ctx || !func || !buffer || !size)
    return -1;

  *buffer = nil;
  *size = 0;

  C = sljit_create_compiler(nil);
  if (!C)
    return -1;
  ctx->compiler = C;

  if (emit_all_instructions(ctx, C, func, error_msg, sizeof(error_msg)) < 0) {
    sljit_free_compiler(C);
    return -1;
  }

  /* Serialize instead of generating code */
  sljit_uw buf_size;
  sljit_uw *buf = sljit_serialize_compiler(C, 0, &buf_size);

  sljit_free_compiler(C);

  if (!buf)
    return -1;

  *buffer = buf;
  *size = buf_size;
  return 0;
}

int fruity_aot_load(void *buffer, size_t size, fruity_jit_result_t *result) {
  struct sljit_compiler *C;

  if (!buffer || size == 0 || !result)
    return -1;
  memset(result, 0, sizeof(*result));

  /* Deserialize compiler from buffer */
  C = sljit_deserialize_compiler((sljit_uw *)buffer, size, 0, nil);
  if (!C) {
    snprint(result->error_msg, sizeof(result->error_msg),
            "Failed to deserialize compiler");
    result->success = -1;
    return -1;
  }

  /* Generate executable code from deserialized state */
  result->code = sljit_generate_code(C, 0, nil);
  if (!result->code) {
    sljit_free_compiler(C);
    snprint(result->error_msg, sizeof(result->error_msg),
            "Failed to generate code from deserialized AOT");
    result->success = -1;
    return -1;
  }

  result->code_size = sljit_get_generated_code_size(C);
  result->success = 0;

  sljit_free_compiler(C);
  return 0;
}

int fruity_jit_execute(fruity_jit_result_t *result, int64_t *args,
                       int arg_count, int64_t *ret_val) {
  if (!result || !result->code) {
    return -1;
  }

  /* Cast to function pointer and call */
  typedef sljit_sw (*jit_func_t)(sljit_sw);
  jit_func_t fn = (jit_func_t)result->code;

  sljit_sw arg0 = (arg_count > 0 && args) ? (sljit_sw)args[0] : 0;
  sljit_sw ret = fn(arg0);

  if (ret_val) {
    *ret_val = (int64_t)ret;
  }

  return 0;
}

void fruity_jit_free_code(fruity_jit_result_t *result) {
  if (result && result->code) {
    sljit_free_code(result->code, nil);
    result->code = nil;
    result->code_size = 0;
  }
}
