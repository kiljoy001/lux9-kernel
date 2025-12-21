/* il_to_fruity.c - IL → Fruity IR Converter
 *
 * Converts .NET IL bytecode (stack-based) to Fruity IR (explicit Pebble
 * operations).
 */

#if defined(KERNEL) || defined(__PLAN9_KERNEL__)
/* Manual Plan 9 Types (avoiding include maze) */
#define _U_H_
#define IL_SHR_UN 0x64
#define nil ((void *)0)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
typedef u32int Rune;
#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#define USED(x)                                                                \
  if (x) {                                                                     \
  }

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;

#include "../../port/lib.h"
#include "../9front-pc64/mem.h"
#include "dat.h"
#include "fns.h"

/* Critical: Use consistent allocator throughout - xallocz/xfree.
 * The pool allocator (malloc/calloc/free) uses different headers than
 * xalloc, so mixing them causes xfree panics. */
#define calloc(n, sz) xallocz((n) * (sz), 1)
#define free(p) xfree(p)

/* strdup uses pool allocator internally, so we need our own xstrdup */
static inline char *xstrdup(const char *s) {
  if (s == nil)
    return nil;
  ulong len = strlen(s) + 1;
  char *copy = xalloc(len);
  if (copy)
    memmove(copy, s, len);
  return copy;
}
#define strdup(s) xstrdup(s)

#else
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Userspace mocks */
extern void *xalloc(size_t size);
extern void xfree(void *ptr);
#define snprint snprintf
#endif

#include "fruity/fruity_ir.h"
#include "il_to_fruity.h"

/* Maximum type stack depth for static tracking */
#define IL_TYPE_STACK_MAX 256

/* Internal context for conversion */
typedef struct il_to_fruity_ctx {
  il_assembly_t *assembly;
  il_method_t *method;

  /* Basic block tracking */
  fruity_basic_block_t **blocks;
  size_t block_count;
  size_t block_capacity;

  /* Branch targets (IL offsets that start new blocks) */
  uint32_t *branch_targets;
  size_t branch_target_count;
  size_t branch_target_capacity;

  /* Current function being built */
  fruity_function_t *current_function;

  /* Type stack for reference tracking during translation */
  stack_entry_type_t type_stack[IL_TYPE_STACK_MAX];
  int type_stack_top;

  /* Error tracking */
  il_to_fruity_error_t last_error;
} il_to_fruity_ctx_t;

/* Type stack helpers */
static void type_stack_push(il_to_fruity_ctx_t *ctx, stack_entry_type_t type) {
  if (ctx->type_stack_top < IL_TYPE_STACK_MAX) {
    ctx->type_stack[ctx->type_stack_top++] = type;
  }
}

static stack_entry_type_t type_stack_pop(il_to_fruity_ctx_t *ctx) {
  if (ctx->type_stack_top > 0) {
    return ctx->type_stack[--ctx->type_stack_top];
  }
  return STACK_UNKNOWN;
}

static stack_entry_type_t type_stack_peek(il_to_fruity_ctx_t *ctx) {
  if (ctx->type_stack_top > 0) {
    return ctx->type_stack[ctx->type_stack_top - 1];
  }
  return STACK_UNKNOWN;
}

/* Helper: Error strings */
const char *il_to_fruity_error_string(il_to_fruity_error_t error) {
  switch (error) {
  case IL_TO_FRUITY_OK:
    return "success";
  case IL_TO_FRUITY_ERROR_INVALID_IL:
    return "invalid IL bytecode";
  case IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE:
    return "unsupported IL opcode";
  case IL_TO_FRUITY_ERROR_STACK_UNDERFLOW:
    return "stack underflow";
  case IL_TO_FRUITY_ERROR_OUT_OF_MEMORY:
    return "out of memory";
  case IL_TO_FRUITY_ERROR_METADATA:
    return "metadata error";
  case IL_TO_FRUITY_ERROR_CFG:
    return "control flow graph error";
  default:
    return "unknown error";
  }
}

/* Helper: Add branch target */
static int add_branch_target(il_to_fruity_ctx_t *ctx, uint32_t offset) {
  /* Check if already exists */
  for (size_t i = 0; i < ctx->branch_target_count; i++) {
    if (ctx->branch_targets[i] == offset)
      return 0;
  }

  /* Expand capacity if needed */
  if (ctx->branch_target_count >= ctx->branch_target_capacity) {
    size_t new_capacity =
        ctx->branch_target_capacity ? ctx->branch_target_capacity * 2 : 16;
    uint32_t *new_targets = xalloc(new_capacity * sizeof(uint32_t));
    if (new_targets == NULL)
      return -1;
    if (ctx->branch_targets) {
      memmove(new_targets, ctx->branch_targets,
              ctx->branch_target_count * sizeof(uint32_t));
      xfree(ctx->branch_targets);
    }
    ctx->branch_targets = new_targets;
    ctx->branch_target_capacity = new_capacity;
  }

  ctx->branch_targets[ctx->branch_target_count++] = offset;
  return 0;
}

/* Helper: Is offset a branch target? */
static int is_branch_target(il_to_fruity_ctx_t *ctx, uint32_t offset) {
  for (size_t i = 0; i < ctx->branch_target_count; i++) {
    if (ctx->branch_targets[i] == offset)
      return 1;
  }
  return 0;
}

/*@ requires \valid(ctx);
    allocates  \result;
    assigns    ctx->blocks, ctx->block_count, ctx->block_capacity;
    assigns    ctx->last_error;
    behavior   success:
      assumes  xalloc can allocate;
      ensures  \result != \null;
      ensures  \result->block_id == block_id;
      ensures  \result->instructions_head == \null;
      ensures  \result->instructions_tail == \null;
      ensures  \result->instruction_count == 0;
      ensures  ctx->block_count == \old(ctx->block_count) + 1;
    behavior   failure:
      assumes  xalloc cannot allocate;
      ensures  \result == \null;
      ensures  ctx->last_error == IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
*/
/* Helper: Create new basic block */
static fruity_basic_block_t *create_basic_block(il_to_fruity_ctx_t *ctx,
                                                uint32_t block_id) {
  fruity_basic_block_t *block = xalloc(sizeof(fruity_basic_block_t));
  if (block == NULL) {
    ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
    return NULL;
  }
  memset(block, 0, sizeof(fruity_basic_block_t));

  block->block_id = block_id;
  block->instructions_head = NULL;
  block->instructions_tail = NULL;
  block->instruction_count = 0;

  /* Expand blocks array if needed */
  if (ctx->block_count >= ctx->block_capacity) {
    size_t new_capacity = ctx->block_capacity ? ctx->block_capacity * 2 : 16;
    fruity_basic_block_t **new_blocks =
        xalloc(new_capacity * sizeof(fruity_basic_block_t *));
    if (new_blocks == NULL) {
      xfree(block);
      ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
      return NULL;
    }
    if (ctx->blocks) {
      memmove(new_blocks, ctx->blocks,
              ctx->block_count * sizeof(fruity_basic_block_t *));
      xfree(ctx->blocks);
    }
    ctx->blocks = new_blocks;
    ctx->block_capacity = new_capacity;
  }

  ctx->blocks[ctx->block_count++] = block;
  return block;
}

/*@ requires \valid(block);
    requires \valid(instr);
    requires instr->next == \null;
    requires instr->prev == \null || instr->prev == block->instructions_tail;
    assigns  block->instructions_head, block->instructions_tail;
    assigns  block->instruction_count;
    assigns  block->instructions_tail->next \from instr;
    assigns  instr->prev, instr->next;
    ensures  block->instructions_tail == instr;
    ensures  block->instruction_count == \old(block->instruction_count) + 1;
    ensures  \result == 0;
*/
/* Helper: Add instruction to block */
static int add_instruction_to_block(fruity_basic_block_t *block,
                                    fruity_instruction_t *instr) {
  instr->next = NULL;
  instr->prev = block->instructions_tail;

  if (block->instructions_tail)
    block->instructions_tail->next = instr;
  else
    block->instructions_head = instr;

  block->instructions_tail = instr;
  block->instruction_count++;
  return 0;
}

/*@ allocates \result;
    assigns   \result \from opcode, operand, il_offset;
    behavior  success:
      assumes  xalloc can allocate sizeof(fruity_instruction_t) bytes;
      ensures  \result != \null;
      ensures  \result->opcode == opcode;
      ensures  \result->msil_offset == il_offset;
      ensures  \result->next == \null;
      ensures  \result->prev == \null;
    behavior  failure:
      assumes  xalloc cannot allocate memory;
      ensures  \result == \null;
*/
/* Helper: Create Fruity instruction */
static fruity_instruction_t *create_fruity_instruction(fruity_opcode_t opcode,
                                                       fruity_operand_t operand,
                                                       uint32_t il_offset) {
  fruity_instruction_t *instr = xalloc(sizeof(fruity_instruction_t));
  if (instr)
    memset(instr, 0, sizeof(fruity_instruction_t));
  if (instr == NULL)
    return NULL;

  instr->opcode = opcode;
  instr->operand = operand;
  instr->msil_offset = il_offset;
  return instr;
}

/* Phase 1: Identify basic block boundaries */
static int identify_basic_blocks(il_to_fruity_ctx_t *ctx) {
  const uint8_t *il = ctx->method->il_code;
  size_t il_size = ctx->method->il_code_size;
  size_t offset = 0;

  /* Offset 0 is always a block start */
  if (add_branch_target(ctx, 0) != 0)
    return -1;

  /* Scan IL to find branch targets */
  while (offset < il_size) {
    uint8_t opcode = il[offset];

    /* Handle branch instructions */
    switch (opcode) {
    case IL_BR_S:
    case IL_BRFALSE_S:
    case IL_BRTRUE_S:
    case IL_BEQ_S:
    case IL_BGE_S:
    case IL_BGT_S:
    case IL_BLE_S:
    case IL_BLT_S:
    case IL_BNE_UN_S:
      /* Short branch: 1-byte offset */
      if (offset + 2 > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }
      int8_t short_offset = (int8_t)il[offset + 1];
      uint32_t target = offset + 2 + short_offset;
      if (add_branch_target(ctx, target) != 0)
        return -1;
      /* Instruction after branch is also a block start */
      if (add_branch_target(ctx, offset + 2) != 0)
        return -1;
      offset += 2;
      break;

    case IL_BR:
    case IL_BRFALSE:
    case IL_BRTRUE:
    case IL_BEQ:
    case IL_BGE:
    case IL_BGT:
    case IL_BLE:
    case IL_BLT:
    case IL_BNE_UN:
      /* Long branch: 4-byte offset */
      if (offset + 5 > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }
      int32_t long_offset = *(int32_t *)&il[offset + 1];
      uint32_t long_target = offset + 5 + long_offset;
      if (add_branch_target(ctx, long_target) != 0)
        return -1;
      /* Instruction after branch is also a block start */
      if (add_branch_target(ctx, offset + 5) != 0)
        return -1;
      offset += 5;
      break;

    case IL_JMP:
      /* jmp is a terminator */
      if (offset + 5 > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }
      if (add_branch_target(ctx, offset + 5) != 0)
        return -1;
      offset += 5;
      break;

    case IL_SWITCH: {
      if (offset + 5 > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }
      uint32_t n = *(uint32_t *)&il[offset + 1];
      uint32_t switch_len = 5 + n * 4;
      if (offset + switch_len > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }

      uint32_t base_offset = offset + switch_len;
      for (uint32_t i = 0; i < n; i++) {
        int32_t target_delta = *(int32_t *)&il[offset + 5 + i * 4];
        if (add_branch_target(ctx, base_offset + target_delta) != 0)
          return -1;
      }
      /* Fallthrough target (default) */
      if (add_branch_target(ctx, base_offset) != 0)
        return -1;

      offset += switch_len;
      break;
    }

    case IL_RET:
    case IL_THROW:
    case IL_ENDFINALLY:
      /* Return/throw/endfinally ends a block; next instruction (if any) starts
       * new block */
      if (offset + 1 < il_size) {
        if (add_branch_target(ctx, offset + 1) != 0)
          return -1;
      }
      offset += 1;
      break;

    case IL_LEAVE_S:
      /* leave.s: Short branch out of protected region */
      if (offset + 2 > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }
      int8_t leave_s_offset = (int8_t)il[offset + 1];
      uint32_t leave_s_target = offset + 2 + leave_s_offset;
      if (add_branch_target(ctx, leave_s_target) != 0)
        return -1;
      if (add_branch_target(ctx, offset + 2) != 0)
        return -1;
      offset += 2;
      break;

    case IL_LEAVE:
      /* leave: Long branch out of protected region */
      if (offset + 5 > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }
      int32_t leave_offset = *(int32_t *)&il[offset + 1];
      uint32_t leave_target = offset + 5 + leave_offset;
      if (add_branch_target(ctx, leave_target) != 0)
        return -1;
      if (add_branch_target(ctx, offset + 5) != 0)
        return -1;
      offset += 5;
      break;

    /* Single-byte instructions */
    case IL_NOP:
    case IL_LDARG_0:
    case IL_LDARG_1:
    case IL_LDARG_2:
    case IL_LDARG_3:
    case IL_LDLOC_0:
    case IL_LDLOC_1:
    case IL_LDLOC_2:
    case IL_LDLOC_3:
    case IL_STLOC_0:
    case IL_STLOC_1:
    case IL_STLOC_2:
    case IL_STLOC_3:
    case IL_LDNULL:
    case IL_LDC_I4_M1:
    case IL_LDC_I4_0:
    case IL_LDC_I4_1:
    case IL_LDC_I4_2:
    case IL_LDC_I4_3:
    case IL_LDC_I4_4:
    case IL_LDC_I4_5:
    case IL_LDC_I4_6:
    case IL_LDC_I4_7:
    case IL_LDC_I4_8:
    case IL_DUP:
    case IL_POP:
    case IL_LDIND_I1:
    case IL_LDIND_U1:
    case IL_LDIND_I2:
    case IL_LDIND_U2:
    case IL_LDIND_I4:
    case IL_LDIND_U4:
    case IL_LDIND_I8:
    case IL_LDIND_I:
    case IL_LDIND_R4:
    case IL_LDIND_R8:
    case IL_LDIND_REF:
    case IL_STIND_REF:
    case IL_STIND_I1:
    case IL_STIND_I2:
    case IL_STIND_I4:
    case IL_STIND_I8:
    case IL_STIND_R4:
    case IL_STIND_R8:
    case IL_STIND_I:
    case IL_ADD:
    case IL_SUB:
    case IL_MUL:
    case IL_DIV:
    case IL_DIV_UN:
    case IL_REM:
    case IL_REM_UN:
    case IL_AND:
    case IL_OR:
    case IL_XOR:
    case IL_SHL:
    case IL_SHR:
    case IL_SHR_UN:
    case IL_NEG:
    case IL_NOT:
    case IL_CKFINITE:
    case IL_BREAK:
    case 0x8e: /* ldlen */
    case 0x9a: /* ldelem.ref */
    case 0xa2: /* stelem.ref */
    case IL_CONV_I1:
    case IL_CONV_I2:
    case IL_CONV_I4:
    case IL_CONV_I8:
    case IL_CONV_U1:
    case IL_CONV_U2:
    case IL_CONV_U4:
    case IL_CONV_U8:
    case IL_CONV_I:
    case IL_CONV_U:
    case IL_CONV_R4:
    case IL_CONV_R8:
    case IL_CONV_R_UN:
    case IL_CONV_OVF_I1:
    case IL_CONV_OVF_U1:
    case IL_CONV_OVF_I2:
    case IL_CONV_OVF_U2:
    case IL_CONV_OVF_I4:
    case IL_CONV_OVF_U4:
    case IL_CONV_OVF_I8:
    case IL_CONV_OVF_U8:
    case IL_CONV_OVF_I:
    case IL_CONV_OVF_U:
    case IL_CONV_OVF_I1_UN:
    case IL_CONV_OVF_I2_UN:
    case IL_CONV_OVF_I4_UN:
    case IL_CONV_OVF_I8_UN:
    case IL_CONV_OVF_U1_UN:
    case IL_CONV_OVF_U2_UN:
    case IL_CONV_OVF_U4_UN:
    case IL_CONV_OVF_U8_UN:
    case IL_CONV_OVF_I_UN:
    case IL_CONV_OVF_U_UN:
      offset += 1;
      break;

    case IL_LDC_R4: /* 4-byte operand */
      offset += 1 + 4;
      break;

    case IL_LDC_R8: /* 8-byte operand */
      offset += 1 + 8;
      break;

    /* Two-byte instructions */
    case IL_LDARG_S:
    case 0x0F: /* ldarga.s */
    case 0x10: /* starg.s */
    case IL_LDLOC_S:
    case IL_LDLOCA_S:
    case IL_STLOC_S:
    case IL_LDC_I4_S:
      offset += 2;
      break;

    /* Five-byte instructions */
    case IL_LDC_I4:
    case IL_CALL:
    case IL_CALLVIRT:
    case IL_CALLI:
    case IL_LDSTR:
    case 0x7E: /* IL_LDSFLD */
      // printf("DEBUG: Found LDSFLD at %x\n", (unsigned)offset);
    case IL_NEWOBJ:
    case IL_LDFLD:
    case IL_STFLD:
    case IL_NEWARR:
    case IL_LDELEMA:
    case IL_BOX:
    case IL_UNBOX:
    case IL_UNBOX_ANY:
    case IL_CASTCLASS:
    case IL_ISINST:
    case IL_CPOBJ:
    case IL_LDOBJ:
    case IL_STOBJ:
    case IL_MKREFANY:
    case IL_REFANYVAL:
    case 0x80: /* stsfld */
      offset += 5;
      break;

    /* Nine-byte instructions */
    case IL_LDC_I8:
      offset += 5;
      break;

    case IL_LDTOKEN:
      /* ldtoken: 1 byte opcode + 4 byte operand */
      offset += 5;
      break;

    /* Two-byte opcode prefix */
    case 0xFE:
      if (offset + 2 > il_size) {
        ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
        return -1;
      }
      uint8_t op2 = il[offset + 1];

      switch (op2) {
      case 0x06:     /* ldftn */
      case 0x07:     /* ldvirtftn */
      case 0x09:     /* ldarg */
      case 0x0A:     /* ldarga */
      case 0x0B:     /* starg */
      case 0x0C:     /* ldloc */
      case 0x0D:     /* ldloca */
      case 0x0E:     /* stloc */
      case 0x15:     /* initobj */
      case 0x16:     /* constrained. */
      case 0x1C:     /* sizeof */
        offset += 6; // 2-byte opcode + 4-byte operand
        break;

      case 0x1D:     /* refanytype */
      case 0x1E:     /* readonly. */
      case 0x13:     /* volatile. */
      case 0x14:     /* tail. */
        offset += 2; // 2-byte opcode, no operand
        break;

      case 0x19:     /* no. */
      case 0x12:     /* unaligned. */
        offset += 3; // 2-byte opcode + 1-byte operand
        break;

      case 0x0F:     /* localloc */
      case 0x1A:     /* rethrow */
      case 0x00:     /* arglist */
      case 0x17:     /* cpblk */
      case 0x18:     /* initblk */
      case 0x01:     /* ceq */
      case 0x02:     /* cgt */
      case 0x03:     /* cgt.un */
      case 0x04:     /* clt */
      case 0x05:     /* clt.un */
        offset += 2; // 2-byte opcode, no operand
        break;

      default:
        /* Unknown two-byte opcode */
        ctx->last_error = IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE;
        return -1;
      }
      break;

    default:
      /* Unknown/unsupported opcode */
      ctx->last_error = IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE;
      return -1;
    }
  }

  return 0;
}

/* Helper: Sort comparison for uint32 */
static int compare_uint32(const void *a, const void *b) {
  uint32_t ua = *(const uint32_t *)a;
  uint32_t ub = *(const uint32_t *)b;
  return (ua < ub) ? -1 : (ua > ub) ? 1 : 0;
}

/* Helper: Get basic block starting at offset (Binary Search) */
static fruity_basic_block_t *get_block_at_offset(il_to_fruity_ctx_t *ctx,
                                                 uint32_t offset) {
  if (ctx->block_count == 0)
    return NULL;

  size_t left = 0;
  size_t right = ctx->block_count - 1;

  while (left <= right) {
    size_t mid = left + (right - left) / 2;
    fruity_basic_block_t *block = ctx->blocks[mid];

    if (block->start_offset == offset) {
      return block;
    }

    if (block->start_offset < offset) {
      left = mid + 1;
    } else {
      if (mid == 0)
        break;
      right = mid - 1;
    }
  }
  return NULL;
}

/* Phase 2: Translate IL instructions to Fruity IR */
static int translate_instruction(il_to_fruity_ctx_t *ctx,
                                 fruity_basic_block_t *block, const uint8_t *il,
                                 size_t *offset_ptr, size_t il_size) {
  size_t offset = *offset_ptr;
  uint8_t opcode = il[offset];
  fruity_instruction_t *instr = NULL;
  fruity_operand_t operand;

  memset(&operand, 0, sizeof(operand));
  operand.type = FRUITY_OP_NONE;

  switch (opcode) {
  case IL_NOP:
    instr = create_fruity_instruction(FRUITY_NOP, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LDC_I4_M1:
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = -1;
    instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LDC_I4_0:
  case IL_LDC_I4_1:
  case IL_LDC_I4_2:
  case IL_LDC_I4_3:
  case IL_LDC_I4_4:
  case IL_LDC_I4_5:
  case IL_LDC_I4_6:
  case IL_LDC_I4_7:
  case IL_LDC_I4_8:
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode - IL_LDC_I4_0;
    instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
    type_stack_push(ctx, STACK_VALUE);
    *offset_ptr += 1;
    break;

  case IL_LDC_R4:
    operand.type = FRUITY_OP_IMM_R32;
    operand.value.r32 = *(float *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDC_R4, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_LDC_R8:
    operand.type = FRUITY_OP_IMM_R64;
    operand.value.r64 = *(double *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDC_R8, operand, offset);
    *offset_ptr += 9;
    break;

  case IL_LDC_I4_S:
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = (int8_t)il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
    *offset_ptr += 2;
    break;

  case IL_LDC_I4:
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = *(int32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDC_I4, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_LDC_I8:
    operand.type = FRUITY_OP_IMM_I64;
    operand.value.i64 = *(int64_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDC_I8, operand, offset);
    *offset_ptr += 9;
    break;

  case IL_LDNULL:
    instr = create_fruity_instruction(FRUITY_LDNULL, operand, offset);
    type_stack_push(ctx, STACK_REF); /* null is technically a ref */
    *offset_ptr += 1;
    break;

  case IL_DUP:
    /* Type-aware flavor injection:
     * If stack top is reference type → FRUITY_VANILLA (addref)
     * If stack top is value type → FRUITY_DUP (copy)
     * UNKNOWN types treated as refs (conservative/safe)
     */
    {
      stack_entry_type_t top_type = type_stack_peek(ctx);
      if (top_type == STACK_REF || top_type == STACK_UNKNOWN) {
        instr = create_fruity_instruction(FRUITY_VANILLA, operand, offset);
      } else {
        instr = create_fruity_instruction(FRUITY_DUP, operand, offset);
      }
      /* DUP duplicates the top, so push same type again */
      type_stack_push(ctx, top_type);
    }
    *offset_ptr += 1;
    break;

  case IL_POP:
    /* Type-aware flavor injection:
     * If stack top is reference type → FRUITY_BURN (release)
     * If stack top is value type → FRUITY_POP (discard)
     * UNKNOWN types treated as refs (conservative/safe)
     */
    {
      stack_entry_type_t top_type = type_stack_pop(ctx);
      if (top_type == STACK_REF || top_type == STACK_UNKNOWN) {
        instr = create_fruity_instruction(FRUITY_BURN, operand, offset);
      } else {
        instr = create_fruity_instruction(FRUITY_POP, operand, offset);
      }
    }
    *offset_ptr += 1;
    break;

  case IL_LDLOC_0:
  case IL_LDLOC_1:
  case IL_LDLOC_2:
  case IL_LDLOC_3:
    operand.type = FRUITY_OP_LOCAL;
    operand.value.index = opcode - IL_LDLOC_0;
    instr = create_fruity_instruction(FRUITY_LOAD_LOCAL, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LDLOC_S:
    operand.type = FRUITY_OP_LOCAL;
    operand.value.index = il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LOAD_LOCAL, operand, offset);
    *offset_ptr += 2;
    break;

  case IL_STLOC_0:
  case IL_STLOC_1:
  case IL_STLOC_2:
  case IL_STLOC_3:
    operand.type = FRUITY_OP_LOCAL;
    operand.value.index = opcode - IL_STLOC_0;
    instr = create_fruity_instruction(FRUITY_STORE_LOCAL, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_STLOC_S:
    operand.type = FRUITY_OP_LOCAL;
    operand.value.index = il[offset + 1];
    instr = create_fruity_instruction(FRUITY_STORE_LOCAL, operand, offset);
    *offset_ptr += 2;
    break;

  case IL_LDARG_0:
  case IL_LDARG_1:
  case IL_LDARG_2:
  case IL_LDARG_3:
    operand.type = FRUITY_OP_ARG;
    operand.value.index = opcode - IL_LDARG_0;
    instr = create_fruity_instruction(FRUITY_LOAD_ARG, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LDARG_S:
    operand.type = FRUITY_OP_ARG;
    operand.value.index = il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LOAD_ARG, operand, offset);
    *offset_ptr += 2;
    break;

  case IL_ADD:
    instr = create_fruity_instruction(FRUITY_ADD, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_SUB:
    instr = create_fruity_instruction(FRUITY_SUB, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_MUL:
    instr = create_fruity_instruction(FRUITY_MUL, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_DIV:
    instr = create_fruity_instruction(FRUITY_DIV, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_DIV_UN:
    instr = create_fruity_instruction(FRUITY_DIV_UN, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_REM:
    instr = create_fruity_instruction(FRUITY_REM, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_REM_UN:
    instr = create_fruity_instruction(FRUITY_REM_UN, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_AND:
    instr = create_fruity_instruction(FRUITY_AND, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_OR:
    instr = create_fruity_instruction(FRUITY_OR, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_XOR:
    instr = create_fruity_instruction(FRUITY_XOR, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_SHL:
    instr = create_fruity_instruction(FRUITY_SHL, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_SHR:
    instr = create_fruity_instruction(FRUITY_SHR, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_SHR_UN:
    instr = create_fruity_instruction(FRUITY_SHR_UN, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_NOT:
    instr = create_fruity_instruction(FRUITY_NOT, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_CPOBJ:
    /* cpobj: Copy value type */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_CPOBJ, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_BREAK: // Add BREAK opcode
    instr = create_fruity_instruction(FRUITY_BREAK, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_NEG:
    instr = create_fruity_instruction(FRUITY_NEG, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_CALL:
  case IL_CALLVIRT:
    /* call/callvirt: Method call (virtual uses vtable lookup) */
    operand.type = FRUITY_OP_METHOD;
    operand.value.token = *(uint32_t *)&il[offset + 1]; // THIS LINE!
    instr = create_fruity_instruction(FRUITY_CALL, operand, offset);
    /* Mark callvirt for runtime dispatch */
    if (opcode == IL_CALLVIRT && instr)
      instr->pebble_effects.creates_white = 0; /* Tag for vtable lookup */
    *offset_ptr += 5;
    break;

  case IL_CALLI:
    /* calli: Indirect method call */
    operand.type =
        FRUITY_OP_METHOD; // Operand is metadata token for callsite signature
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_CALLI, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_RET:
    instr = create_fruity_instruction(FRUITY_RET, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_NEWOBJ:
    operand.type = FRUITY_OP_METHOD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_NEWOBJ, operand, offset);
    if (instr)
      instr->pebble_effects.creates_white = 1;
    type_stack_push(ctx, STACK_REF); /* newobj produces a reference */
    *offset_ptr += 5;
    break;

  case IL_NEWARR:
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_NEWARR, operand, offset);
    if (instr)
      instr->pebble_effects.creates_white = 1;
    type_stack_pop(ctx);             /* pop the length (value) */
    type_stack_push(ctx, STACK_REF); /* push the array ref */
    *offset_ptr += 5;
    break;

  case IL_LDSTR: {
    /* String loading: token is index into #US heap */
    uint32_t us_token = *(uint32_t *)&il[offset + 1];
    /* Extract index from token (low 24 bits) */
    uint32_t us_index = us_token & 0x00FFFFFF;

    /* Store string index in operand for later resolution */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = us_index;

    instr = create_fruity_instruction(FRUITY_LOAD_STRING, operand, offset);
    if (instr)
      instr->pebble_effects.creates_white = 1;

    type_stack_push(ctx, STACK_REF); /* ldstr produces a string reference */
    *offset_ptr += 5;
    break;
  }

  /* Branch instructions */
  case IL_BR_S:
  case IL_BR: {
    int32_t delta = (opcode == IL_BR_S) ? (int8_t)il[offset + 1]
                                        : *(int32_t *)&il[offset + 1];
    uint32_t instr_len = (opcode == IL_BR_S) ? 2 : 5;
    uint32_t target = offset + instr_len + delta;

    fruity_basic_block_t *target_block = get_block_at_offset(ctx, target);
    if (!target_block) {
      ctx->last_error = IL_TO_FRUITY_ERROR_CFG;
      return -1;
    }

    operand.type = FRUITY_OP_BRANCH;
    operand.value.target = target_block;
    instr = create_fruity_instruction(FRUITY_JUMP, operand, offset);
    *offset_ptr += instr_len;
    break;
  }

  case IL_BRFALSE_S:
  case IL_BRFALSE: {
    int32_t delta = (opcode == IL_BRFALSE_S) ? (int8_t)il[offset + 1]
                                             : *(int32_t *)&il[offset + 1];
    uint32_t instr_len = (opcode == IL_BRFALSE_S) ? 2 : 5;
    uint32_t target = offset + instr_len + delta;

    fruity_basic_block_t *target_block = get_block_at_offset(ctx, target);
    if (!target_block) {
      ctx->last_error = IL_TO_FRUITY_ERROR_CFG;
      return -1;
    }

    operand.type = FRUITY_OP_BRANCH;
    operand.value.target = target_block;
    instr = create_fruity_instruction(FRUITY_BFALSE, operand, offset);
    *offset_ptr += instr_len;
    break;
  }

  case IL_BRTRUE_S:
  case IL_BRTRUE: {
    int32_t delta = (opcode == IL_BRTRUE_S) ? (int8_t)il[offset + 1]
                                            : *(int32_t *)&il[offset + 1];
    uint32_t instr_len = (opcode == IL_BRTRUE_S) ? 2 : 5;
    uint32_t target = offset + instr_len + delta;

    fruity_basic_block_t *target_block = get_block_at_offset(ctx, target);
    if (!target_block) {
      ctx->last_error = IL_TO_FRUITY_ERROR_CFG;
      return -1;
    }

    operand.type = FRUITY_OP_BRANCH;
    operand.value.target = target_block;
    instr = create_fruity_instruction(FRUITY_BTRUE, operand, offset);
    *offset_ptr += instr_len;
    break;
  }

  case IL_BEQ_S:
  case IL_BEQ:
  case IL_BNE_UN_S:
  case IL_BNE_UN:
  case IL_BGE_S:
  case IL_BGE:
  case IL_BGT_S:
  case IL_BGT:
  case IL_BLE_S:
  case IL_BLE:
  case IL_BLT_S:
  case IL_BLT: {
    /* Generic handler for conditional branches */
    int32_t delta;
    uint32_t instr_len;
    fruity_opcode_t f_op;

    /* Determine length and delta */
    /* Note: This is a bit repetitive, could be cleaner */
    if (opcode == IL_BEQ_S || opcode == IL_BNE_UN_S || opcode == IL_BGE_S ||
        opcode == IL_BGT_S || opcode == IL_BLE_S || opcode == IL_BLT_S) {
      delta = (int8_t)il[offset + 1];
      instr_len = 2;
    } else {
      delta = *(int32_t *)&il[offset + 1];
      instr_len = 5;
    }

    /* Map opcode */
    uint16_t f_op_val;
    switch (opcode) {
    case IL_BEQ_S:
    case IL_BEQ:
      f_op_val = FRUITY_BEQ;
      break;
    case IL_BNE_UN_S:
    case IL_BNE_UN:
      f_op_val = FRUITY_BNE;
      break; /* Treating UN same as normal for now */
    case IL_BGE_S:
    case IL_BGE:
      f_op_val = FRUITY_BGE;
      break;
    case IL_BGT_S:
    case IL_BGT:
      f_op_val = FRUITY_BGT;
      break;
    case IL_BLE_S:
    case IL_BLE:
      f_op_val = FRUITY_BLE;
      break;
    case IL_BLT_S:
    case IL_BLT:
      f_op_val = FRUITY_BLT;
      break;
    default:
      f_op_val = FRUITY_NOP;
      break; /* Should not happen */
    }

    uint32_t target = offset + instr_len + delta;
    fruity_basic_block_t *target_block = get_block_at_offset(ctx, target);
    if (!target_block) {
      ctx->last_error = IL_TO_FRUITY_ERROR_CFG;
      return -1;
    }

    operand.type = FRUITY_OP_BRANCH;
    operand.value.target = target_block;
    instr = create_fruity_instruction(f_op_val, operand, offset);
    *offset_ptr += instr_len;
    break;
  }

  /* ===== Type Operations ===== */
  case IL_CASTCLASS:
    /* castclass: Cast object to type (throws if invalid) */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_CASTCLASS, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_ISINST:
    /* isinst: Type check (returns null if invalid) */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_ISINST, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_BOX:
    /* box: Box value type to reference type */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    /* Boxing allocates a new object (creates white token) */
    instr = create_fruity_instruction(FRUITY_BOX, operand, offset);
    if (instr)
      instr->pebble_effects.creates_white = 1;
    *offset_ptr += 5;
    break;

  case IL_UNBOX:
    /* unbox: Get pointer to value inside boxed object */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_UNBOX, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_UNBOX_ANY:
    /* unbox.any: Unbox and copy value */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_UNBOX_ANY, operand, offset);
    *offset_ptr += 5;
    break;

  /* ===== Field Access ===== */
  case IL_LDFLD:
    /* ldfld: Load field from object */
    operand.type = FRUITY_OP_FIELD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LOAD_FIELD, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_STFLD:
    /* stfld: Store to object field */
    operand.type = FRUITY_OP_FIELD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_STORE_FIELD, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_LDSFLD:
    /* ldsfld: Load static field */
    operand.type = FRUITY_OP_FIELD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LOAD_STATIC, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_STSFLD:
    /* stsfld: Store to static field */
    operand.type = FRUITY_OP_FIELD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_STORE_STATIC, operand, offset);
    *offset_ptr += 5;
    break;

  /* ===== Array Operations ===== */
  case IL_LDLEN:
    /* ldlen: Load array length */
    instr = create_fruity_instruction(FRUITY_LDLEN, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LDELEM_I1:
  case IL_LDELEM_U1:
  case IL_LDELEM_I2:
  case IL_LDELEM_U2:
  case IL_LDELEM_I4:
  case IL_LDELEM_U4:
  case IL_LDELEM_I8:
  case IL_LDELEM_I:
  case IL_LDELEM_R4:
  case IL_LDELEM_R8:
  case IL_LDELEM_REF:
    /* ldelem.*: Load array element */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode; /* Encode element type in operand */
    instr = create_fruity_instruction(FRUITY_LDELEM, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LDELEM:
    /* ldelem: Load array element with type token */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDELEM, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_STELEM_I:
  case IL_STELEM_I1:
  case IL_STELEM_I2:
  case IL_STELEM_I4:
  case IL_STELEM_I8:
  case IL_STELEM_R4:
  case IL_STELEM_R8:
  case IL_STELEM_REF:
    /* stelem.*: Store array element */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode;
    instr = create_fruity_instruction(FRUITY_STELEM, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_STELEM:
    /* stelem: Store array element with type token */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_STELEM, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_LDELEMA:
    /* ldelema: Load element address */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDELEMA, operand, offset);
    *offset_ptr += 5;
    break;

  /* ===== Exception Handling ===== */
  case IL_THROW:
    /* throw: Pop exception reference and dispatch */
    instr = create_fruity_instruction(FRUITY_THROW, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LEAVE_S: {
    /* leave.s: Exit try/catch, branch to target (short) */
    int8_t delta = (int8_t)il[offset + 1];
    uint32_t target = offset + 2 + delta;
    fruity_basic_block_t *target_block = get_block_at_offset(ctx, target);
    if (!target_block) {
      ctx->last_error = IL_TO_FRUITY_ERROR_CFG;
      return -1;
    }
    operand.type = FRUITY_OP_BRANCH;
    operand.value.target = target_block;
    instr = create_fruity_instruction(FRUITY_LEAVE, operand, offset);
    *offset_ptr += 2;
    break;
  }

  case IL_LEAVE: {
    /* leave: Exit try/catch, branch to target (long) */
    int32_t delta = *(int32_t *)&il[offset + 1];
    uint32_t target = offset + 5 + delta;
    fruity_basic_block_t *target_block = get_block_at_offset(ctx, target);
    if (!target_block) {
      ctx->last_error = IL_TO_FRUITY_ERROR_CFG;
      return -1;
    }
    operand.type = FRUITY_OP_BRANCH;
    operand.value.target = target_block;
    instr = create_fruity_instruction(FRUITY_LEAVE, operand, offset);
    *offset_ptr += 5;
    break;
  }

  case IL_ENDFINALLY:
    /* endfinally/endfault: Resume exception dispatch or normal flow */
    instr = create_fruity_instruction(FRUITY_ENDFINALLY, operand, offset);
    *offset_ptr += 1;
    break;

  /* ===== Conversion Opcodes ===== */
  case IL_CONV_I1:
  case IL_CONV_I2:
  case IL_CONV_I4:
  case IL_CONV_U1:
  case IL_CONV_U2:
  case IL_CONV_U4:
    /* Convert to 32-bit integer (signed/unsigned handled by target type in IR)
     */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode;
    instr = create_fruity_instruction(FRUITY_CONV_I4, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_CONV_I8:
  case IL_CONV_U8:
  case IL_CONV_I:
  case IL_CONV_U:
    /* Convert to 64-bit integer */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode;
    instr = create_fruity_instruction(FRUITY_CONV_I8, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_CONV_R4:
    /* Convert to float32 */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode;
    instr = create_fruity_instruction(FRUITY_CONV_R4, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_CONV_R8:
  case IL_CONV_R_UN:
    /* Convert to float64 */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode;
    instr = create_fruity_instruction(FRUITY_CONV_R8, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_CONV_OVF_I1:
  case IL_CONV_OVF_U1:
  case IL_CONV_OVF_I2:
  case IL_CONV_OVF_U2:
  case IL_CONV_OVF_I4:
  case IL_CONV_OVF_U4:
  case IL_CONV_OVF_I8:
  case IL_CONV_OVF_U8:
  case IL_CONV_OVF_I:
  case IL_CONV_OVF_U:
  case IL_CONV_OVF_I1_UN:
  case IL_CONV_OVF_I2_UN:
  case IL_CONV_OVF_I4_UN:
  case IL_CONV_OVF_I8_UN:
  case IL_CONV_OVF_U1_UN:
  case IL_CONV_OVF_U2_UN:
  case IL_CONV_OVF_U4_UN:
  case IL_CONV_OVF_U8_UN:
  case IL_CONV_OVF_I_UN:
  case IL_CONV_OVF_U_UN:
    /* conv.ovf.*: Overflow-checking conversion */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode;
    instr = create_fruity_instruction(FRUITY_CONV_I4, operand, offset);
    *offset_ptr += 1;
    break;

  /* ===== Switch Statement ===== */
  case IL_SWITCH: {
    /* switch: Jump table - n targets followed by n int32 offsets */
    if (offset + 5 > il_size) {
      ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
      return -1;
    }
    uint32_t n = *(uint32_t *)&il[offset + 1];
    uint32_t switch_len = 5 + n * 4;
    if (offset + switch_len > il_size) {
      ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
      return -1;
    }

    /* Allocate switch targets */
    fruity_switch_targets_t *targets =
        calloc(1, sizeof(fruity_switch_targets_t));
    if (!targets) {
      ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
      return -1;
    }
    targets->count = n;
    targets->targets = calloc(n, sizeof(fruity_basic_block_t *));
    if (!targets->targets) {
      free(targets);
      ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
      return -1;
    }

    uint32_t base_offset = offset + switch_len;
    for (uint32_t i = 0; i < n; i++) {
      int32_t target_delta = *(int32_t *)&il[offset + 5 + i * 4];
      fruity_basic_block_t *bb =
          get_block_at_offset(ctx, base_offset + target_delta);
      if (!bb) {
        free(targets->targets);
        free(targets);
        ctx->last_error = IL_TO_FRUITY_ERROR_CFG;
        return -1;
      }
      targets->targets[i] = bb;
    }

    operand.type = FRUITY_OP_SWITCH;
    operand.value.switch_targets = targets;
    instr = create_fruity_instruction(FRUITY_SWITCH, operand, offset);
    *offset_ptr += switch_len;
    break;
  }

  /* ===== Indirect Load/Store ===== */
  case IL_LDIND_I1:
  case IL_LDIND_U1:
  case IL_LDIND_I2:
  case IL_LDIND_U2:
  case IL_LDIND_I4:
  case IL_LDIND_U4:
  case IL_LDIND_I8:
  case IL_LDIND_I:
  case IL_LDIND_R4:
  case IL_LDIND_R8:
  case IL_LDIND_REF:
    /* ldind.*: Load value indirectly through pointer */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode; /* Encode type in opcode */
    instr = create_fruity_instruction(FRUITY_LOAD_IND, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_STIND_REF:
  case IL_STIND_I1:
  case IL_STIND_I2:
  case IL_STIND_I4:
  case IL_STIND_I8:
  case IL_STIND_R4:
  case IL_STIND_R8:
  case IL_STIND_I:
    /* stind.*: Store value indirectly through pointer */
    operand.type = FRUITY_OP_IMM_I32;
    operand.value.i32 = opcode; /* Encode type in opcode */
    instr = create_fruity_instruction(FRUITY_STORE_IND, operand, offset);
    *offset_ptr += 1;
    break;

  /* ===== Address-of Operations ===== */
  case IL_LDLOCA_S:
    /* ldloca.s: Load address of local variable (short) */
    operand.type = FRUITY_OP_LOCAL;
    operand.value.index = il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LOAD_LOCAL_ADDR, operand, offset);
    *offset_ptr += 2;
    break;

  case IL_LDARGA_S:
    /* ldarga.s: Load address of argument (short) */
    operand.type = FRUITY_OP_ARG;
    operand.value.index = il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LOAD_ARG_ADDR, operand, offset);
    *offset_ptr += 2;
    break;

  case IL_STARG_S:
    /* starg.s: Store to argument (short) */
    operand.type = FRUITY_OP_ARG;
    operand.value.index = il[offset + 1];
    instr = create_fruity_instruction(FRUITY_STORE_ARG, operand, offset);
    *offset_ptr += 2;
    break;

  case IL_LDFLDA:
    /* ldflda: Load field address */
    operand.type = FRUITY_OP_FIELD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDFLDA, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_LDSFLDA:
    /* ldsflda: Load static field address */
    operand.type = FRUITY_OP_FIELD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDFLDA, operand,
                                      offset); // Reuse LDFLDA for now
    *offset_ptr += 5;
    break;

    /* ===== Object Operations ===== */

  case IL_LDOBJ:
    /* ldobj: Load value type */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LOAD_FIELD, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_STOBJ:
    /* stobj: Store value type */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_STORE_FIELD, operand, offset);
    *offset_ptr += 5;
    break;

  /* ===== Overflow-checked Arithmetic ===== */
  case IL_ADD_OVF:
    instr = create_fruity_instruction(FRUITY_ADD_OVF, operand, offset);
    *offset_ptr += 1;
    break;
  case IL_ADD_OVF_UN:
    instr = create_fruity_instruction(FRUITY_ADD_OVF_UN, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_SUB_OVF:
    instr = create_fruity_instruction(FRUITY_SUB_OVF, operand, offset);
    *offset_ptr += 1;
    break;
  case IL_SUB_OVF_UN:
    instr = create_fruity_instruction(FRUITY_SUB_OVF_UN, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_MUL_OVF:
    instr = create_fruity_instruction(FRUITY_MUL_OVF, operand, offset);
    *offset_ptr += 1;
    break;
  case IL_MUL_OVF_UN:
    instr = create_fruity_instruction(FRUITY_MUL_OVF_UN, operand, offset);
    *offset_ptr += 1;
    break;

  /* ===== Rare/Specialized Opcodes ===== */
  case IL_JMP:
    /* jmp: Tail call jump to method */
    operand.type = FRUITY_OP_METHOD;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_JMP, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_CKFINITE:
    /* ckfinite: Check for finite float (throws on NaN/Inf) */
    instr = create_fruity_instruction(FRUITY_CKFINITE, operand, offset);
    *offset_ptr += 1;
    break;

  case IL_LDTOKEN:
    /* ldtoken: Load runtime type/method/field handle */
    operand.type = FRUITY_OP_TOKEN;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_LDTOKEN, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_REFANYVAL:
    /* refanyval: Extract value from typed reference */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_REFANYVAL, operand, offset);
    *offset_ptr += 5;
    break;

  case IL_MKREFANY:
    /* mkrefany: Create typed reference */
    operand.type = FRUITY_OP_TYPE;
    operand.value.token = *(uint32_t *)&il[offset + 1];
    instr = create_fruity_instruction(FRUITY_MKREFANY, operand, offset);
    *offset_ptr += 5;
    break;

  /* ===== Two-byte Opcodes (0xFE prefix) ===== */
  case 0xFE: {
    if (offset + 2 > il_size) {
      ctx->last_error = IL_TO_FRUITY_ERROR_INVALID_IL;
      return -1;
    }
    uint8_t op2 = il[offset + 1];

    switch (op2) {
    case 0x01: /* ceq */
      instr = create_fruity_instruction(FRUITY_CEQ, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x02: /* cgt */
    case 0x03: /* cgt.un */
      instr = create_fruity_instruction(FRUITY_CGT, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x04: /* clt */
    case 0x05: /* clt.un */
      instr = create_fruity_instruction(FRUITY_CLT, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x06: /* ldftn */
      operand.type = FRUITY_OP_METHOD;
      operand.value.token = *(uint32_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_LDFTN, operand, offset);
      *offset_ptr += 6;
      break;

    case 0x07: /* ldvirtftn */
      operand.type = FRUITY_OP_METHOD;
      operand.value.token = *(uint32_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_LDVIRTFTN, operand, offset);
      *offset_ptr += 6;
      break;

    case 0x09: /* ldarg */
      operand.type = FRUITY_OP_ARG;
      operand.value.index = *(uint16_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_LOAD_ARG, operand, offset);
      *offset_ptr += 4;
      break;

    case 0x0A: /* ldarga */
      operand.type = FRUITY_OP_ARG;
      operand.value.index = *(uint16_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_LOAD_ARG, operand, offset);
      *offset_ptr += 4;
      break;

    case 0x0B: /* starg */
      operand.type = FRUITY_OP_ARG;
      operand.value.index = *(uint16_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_STORE_ARG, operand, offset);
      *offset_ptr += 4;
      break;

    case 0x0C: /* ldloc */
      operand.type = FRUITY_OP_LOCAL;
      operand.value.index = *(uint16_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_LOAD_LOCAL, operand, offset);
      *offset_ptr += 4;
      break;

    case 0x0D: /* ldloca */
      operand.type = FRUITY_OP_LOCAL;
      operand.value.index = *(uint16_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_LOAD_LOCAL, operand, offset);
      *offset_ptr += 4;
      break;

    case 0x0E: /* stloc */
      operand.type = FRUITY_OP_LOCAL;
      operand.value.index = *(uint16_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_STORE_LOCAL, operand, offset);
      *offset_ptr += 4;
      break;

    case 0x0F: /* localloc */
      instr = create_fruity_instruction(FRUITY_LIME, operand, offset);
      /* BEVIS: PoW check required for dynamic stack allocation
       * Uses POW_OP_STACK_ALLOC (5) - cheaper than heap but still costs */
      if (instr) {
        instr->pebble_effects.creates_white = 1;
        instr->pebble_effects.pow_op_class = 5; /* POW_OP_STACK_ALLOC */
      }
      *offset_ptr += 2;
      break;

    case 0x15: /* initobj */
      operand.type = FRUITY_OP_TYPE;
      operand.value.token = *(uint32_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_INITOBJ, operand, offset);
      *offset_ptr += 6;
      break;

    case 0x17: /* cpblk */
      instr = create_fruity_instruction(FRUITY_MEMCPY, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x18: /* initblk */
      instr = create_fruity_instruction(FRUITY_MEMSET, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x1A: /* rethrow */
      instr = create_fruity_instruction(FRUITY_RETHROW, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x1C: /* sizeof */
      operand.type = FRUITY_OP_TYPE;
      operand.value.token = *(uint32_t *)&il[offset + 2];
      instr = create_fruity_instruction(FRUITY_SIZEOF, operand, offset);
      *offset_ptr += 6;
      break;

    case 0x00: /* arglist */
      instr = create_fruity_instruction(FRUITY_ARGLIST, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x1D: /* refanytype */
      instr = create_fruity_instruction(FRUITY_REFANYTYPE, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x16: /* constrained. */
      operand.type = FRUITY_OP_TYPE;
      operand.value.token = *(uint32_t *)&il[offset + 2];
      instr =
          create_fruity_instruction(FRUITY_PREFIX_CONSTRAINED, operand, offset);
      *offset_ptr += 6;
      break;

    case 0x1E: /* readonly. */
      instr =
          create_fruity_instruction(FRUITY_PREFIX_READONLY, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x19: /* no. */
      operand.type = FRUITY_OP_IMM_I32;
      operand.value.i32 = il[offset + 2];
      instr = create_fruity_instruction(FRUITY_PREFIX_NO, operand, offset);
      *offset_ptr += 3;
      break;

    case 0x12: /* unaligned. */
      operand.type = FRUITY_OP_IMM_I32;
      operand.value.i32 = il[offset + 2];
      instr =
          create_fruity_instruction(FRUITY_PREFIX_UNALIGNED, operand, offset);
      *offset_ptr += 3;
      break;

    case 0x13: /* volatile. */
      instr =
          create_fruity_instruction(FRUITY_PREFIX_VOLATILE, operand, offset);
      *offset_ptr += 2;
      break;

    case 0x14: /* tail. */
      instr = create_fruity_instruction(FRUITY_PREFIX_TAIL, operand, offset);
      *offset_ptr += 2;
      break;

    default:
      /* Unknown two-byte opcode */
      ctx->last_error = IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE;
      return -1;
    }
    break;
  }

  default:
#ifndef KERNEL
    printf("Unsupported opcode: 0x%02x at offset 0x%04x\n", opcode,
           (unsigned int)offset);
#endif
    ctx->last_error = IL_TO_FRUITY_ERROR_UNSUPPORTED_OPCODE;
    return -1;
  }

  if (instr == NULL) {
    ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
    return -1;
  }

  if (add_instruction_to_block(block, instr) != 0) {
    free(instr);
    ctx->last_error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
    return -1;
  }

  return 0;
}

/* Convert IL method to Fruity function */
/* Convert IL method to Fruity function */
fruity_function_t *il_to_fruity_convert_method(il_assembly_t *assembly,
                                               il_method_t *method,
                                               il_to_fruity_error_t *error) {
  extern void uartputs(char *, int);
  char debug_buf[128];
  il_to_fruity_ctx_t ctx;
  fruity_function_t *func = NULL;

  snprint(debug_buf, sizeof(debug_buf), "DEBUG: il_to_fruity ENTER method=%s\n",
          method->name);
  uartputs(debug_buf, strlen(debug_buf));

  memset(&ctx, 0, sizeof(ctx));

  ctx.assembly = assembly;
  ctx.method = method;
  ctx.last_error = IL_TO_FRUITY_OK;

  /* Phase 1: Identify basic blocks (targets) */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity Phase 1: identify basic blocks\n");
  uartputs(debug_buf, strlen(debug_buf));
  if (identify_basic_blocks(&ctx) != 0) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: il_to_fruity identify_basic_blocks FAILED\n");
    uartputs(debug_buf, strlen(debug_buf));
    if (error)
      *error = ctx.last_error;
    goto cleanup;
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity found %d branch targets\n",
          (int)ctx.branch_target_count);
  uartputs(debug_buf, strlen(debug_buf));

  /* Create function */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity creating function structure\n");
  uartputs(debug_buf, strlen(debug_buf));
  func = calloc(1, sizeof(fruity_function_t));
  if (func == NULL) {
    if (error)
      *error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
    goto cleanup;
  }

  func->name = strdup(method->name ? method->name : "unnamed");
  func->method_token = method->method_token; /* Use token from il_method_t */
  ctx.current_function = func;

  /* Phase 1.5: Sort targets and create blocks */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity Phase 1.5: sorting and creating blocks\n");
  uartputs(debug_buf, strlen(debug_buf));
  qsort(ctx.branch_targets, ctx.branch_target_count, sizeof(uint32_t),
        compare_uint32);

  uint32_t block_id = 0;
  for (size_t i = 0; i < ctx.branch_target_count; i++) {
    uint32_t target = ctx.branch_targets[i];

    /* Skip duplicates */
    if (i > 0 && target == ctx.branch_targets[i - 1])
      continue;

    fruity_basic_block_t *block = create_basic_block(&ctx, block_id++);
    if (block == NULL) {
      if (error)
        *error = ctx.last_error;
      goto cleanup;
    }
    block->start_offset = target;
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity created %d blocks\n", (int)ctx.block_count);
  uartputs(debug_buf, strlen(debug_buf));

  /* Phase 2: Translate instructions per block */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity Phase 2: translating instructions\n");
  uartputs(debug_buf, strlen(debug_buf));
  for (size_t i = 0; i < ctx.block_count; i++) {
    fruity_basic_block_t *current_block = ctx.blocks[i];
    size_t offset = current_block->start_offset;
    size_t end_offset = (i + 1 < ctx.block_count)
                            ? ctx.blocks[i + 1]->start_offset
                            : method->il_code_size;

    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: il_to_fruity translating block %d offset=%d-%d\n", (int)i,
            (int)offset, (int)end_offset);
    uartputs(debug_buf, strlen(debug_buf));

    while (offset < end_offset) {
      if (translate_instruction(&ctx, current_block, method->il_code, &offset,
                                method->il_code_size) != 0) {
        snprint(
            debug_buf, sizeof(debug_buf),
            "DEBUG: il_to_fruity translate_instruction FAILED at offset %d\n",
            (int)offset);
        uartputs(debug_buf, strlen(debug_buf));
        if (error)
          *error = ctx.last_error;
        goto cleanup;
      }
    }
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity translation complete\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Link blocks into function - same as before */
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: il_to_fruity linking blocks\n");
  uartputs(debug_buf, strlen(debug_buf));
  func->block_count = ctx.block_count;
  for (size_t i = 0; i < ctx.block_count; i++) {
    fruity_basic_block_t *block = ctx.blocks[i];
    block->next = (i + 1 < ctx.block_count) ? ctx.blocks[i + 1] : NULL;
    block->prev = (i > 0) ? ctx.blocks[i - 1] : NULL;
  }
  if (ctx.block_count > 0) {
    func->blocks_head = ctx.blocks[0];
    func->blocks_tail = ctx.blocks[ctx.block_count - 1];
  }
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: il_to_fruity blocks linked\n");
  uartputs(debug_buf, strlen(debug_buf));

  if (error)
    *error = IL_TO_FRUITY_OK;

cleanup:
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: il_to_fruity cleanup\n");
  uartputs(debug_buf, strlen(debug_buf));
  if (ctx.branch_targets) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: il_to_fruity freeing branch_targets %p\n",
            ctx.branch_targets);
    uartputs(debug_buf, strlen(debug_buf));
    xfree(ctx.branch_targets);
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: il_to_fruity branch_targets freed\n");
    uartputs(debug_buf, strlen(debug_buf));
  }
  if (ctx.blocks) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: il_to_fruity freeing blocks %p\n", ctx.blocks);
    uartputs(debug_buf, strlen(debug_buf));
    xfree(ctx.blocks);
    snprint(debug_buf, sizeof(debug_buf), "DEBUG: il_to_fruity blocks freed\n");
    uartputs(debug_buf, strlen(debug_buf));
  }

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: il_to_fruity returning func=%p\n", func);
  uartputs(debug_buf, strlen(debug_buf));
  return func;
}

/* Convert entire assembly to Fruity module */
fruity_module_t *il_to_fruity_convert_assembly(il_assembly_t *assembly,
                                               il_to_fruity_error_t *error) {
  fruity_module_t *module;
  fruity_function_t *func, *prev_func;
  il_to_fruity_error_t method_error;
  size_t i;

  if (assembly == NULL) {
    if (error)
      *error = IL_TO_FRUITY_ERROR_INVALID_IL;
    return NULL;
  }

  /* Allocate module */
  module = calloc(1, sizeof(fruity_module_t));
  if (module == NULL) {
    if (error)
      *error = IL_TO_FRUITY_ERROR_OUT_OF_MEMORY;
    return NULL;
  }

  /* Set module name from assembly (use "assembly" as default) */
  module->name = strdup("assembly");
  module->version = 1;
  module->functions_head = NULL;
  module->functions_tail = NULL;
  module->function_count = 0;
  module->metadata = assembly; /* Preserve assembly for token resolution */

  /* Convert each method */
  prev_func = NULL;
  for (i = 0; i < assembly->method_count; i++) {
    il_method_t *method = &assembly->methods[i];

    /* Skip methods without IL code (abstract, extern, etc.) */
    if (method->il_code == NULL || method->il_code_size == 0)
      continue;

    /* Convert method to Fruity function */
    func = il_to_fruity_convert_method(assembly, method, &method_error);
    if (func == NULL) {
      /* Log error but continue with other methods */
      continue;
    }

    /* Link function into module's list */
    func->next = NULL;
    func->prev = prev_func;

    if (prev_func != NULL)
      prev_func->next = func;
    else
      module->functions_head = func;

    module->functions_tail = func;
    module->function_count++;
    prev_func = func;
  }

  /* Check if we have at least one function */
  if (module->function_count == 0) {
    if (error)
      *error = IL_TO_FRUITY_ERROR_METADATA;
    if (module->name)
      free(module->name);
    free(module);
    return NULL;
  }

  if (error)
    *error = IL_TO_FRUITY_OK;
  return module;
}

/* Free Fruity function */
void fruity_free_function(fruity_function_t *func) {
  if (func == NULL)
    return;

  /* Free blocks and instructions */
  for (fruity_basic_block_t *block = func->blocks_head; block != NULL;) {
    fruity_basic_block_t *next_block = block->next;

    for (fruity_instruction_t *instr = block->instructions_head;
         instr != NULL;) {
      fruity_instruction_t *next_instr = instr->next;
      free(instr);
      instr = next_instr;
    }

    free(block);
    block = next_block;
  }

  if (func->name)
    free(func->name);
  free(func);
}

/* Free Fruity module */
void fruity_free_module(fruity_module_t *mod) {
  if (mod == NULL)
    return;

  /* Free functions */
  for (fruity_function_t *func = mod->functions_head; func != NULL;) {
    fruity_function_t *next = func->next;
    fruity_free_function(func);
    func = next;
  }

  if (mod->name)
    free(mod->name);
  free(mod);
}
