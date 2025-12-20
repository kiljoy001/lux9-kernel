/* llvm_to_fruity.c - LLVM IR to Fruity IR Translator
 *
 * Translates parsed LLVM modules to Fruity IR with Pebble memory semantics.
 * Key feature: alloca/malloc -> FRUITY_LIME (white pebble creation)
 */

#include "../include/portlib.h"
#include "../include/u.h"
#include "fruity/fruity_ir.h"
#include "fruity/fruity_opcodes.h"
#include "llvm_parser.h"

/* ========== TRANSLATION CONTEXT ========== */

typedef struct {
  llvm_module_t *llvm_mod;
  fruity_module_t *fruity_mod;
  fruity_function_t *current_func;
  fruity_basic_block_t *current_block;

  /* SSA value mapping: LLVM value ID -> Fruity temp ID */
  u32int *value_map;
  u32int value_map_size;
  u32int next_temp;
} llvm_fruity_ctx_t;

/* ========== OPCODE TRANSLATION TABLE ========== */

static fruity_opcode_t llvm_to_fruity_opcode(llvm_opcode_t llvm_op) {
  switch (llvm_op) {
  case LLVM_ADD:
    return FRUITY_ADD;
  case LLVM_SUB:
    return FRUITY_SUB;
  case LLVM_MUL:
    return FRUITY_MUL;
  case LLVM_SDIV:
    return FRUITY_DIV;
  case LLVM_UDIV:
    return FRUITY_DIV; /* TODO: unsigned variant */
  case LLVM_SREM:
    return FRUITY_REM;
  case LLVM_UREM:
    return FRUITY_REM;
  case LLVM_AND:
    return FRUITY_AND;
  case LLVM_OR:
    return FRUITY_OR;
  case LLVM_XOR:
    return FRUITY_XOR;
  case LLVM_SHL:
    return FRUITY_SHL;
  case LLVM_LSHR:
    return FRUITY_SHR;
  case LLVM_ASHR:
    return FRUITY_SAR;
  case LLVM_RET:
    return FRUITY_RET;
  case LLVM_LOAD:
    return FRUITY_LOAD_LOCAL;
  case LLVM_STORE:
    return FRUITY_STORE_LOCAL;
  case LLVM_BR:
    return FRUITY_JUMP;
  case LLVM_CALL:
    return FRUITY_CALL;
  case LLVM_ICMP:
    return FRUITY_CEQ; /* Default, refined by predicate */
  default:
    return FRUITY_NOP;
  }
}

/* ========== INSTRUCTION TRANSLATION ========== */

static fruity_instruction_t *
translate_instruction(llvm_fruity_ctx_t *ctx, llvm_instruction_t *llvm_instr) {
  fruity_instruction_t *instr = fruity_instruction_create(FRUITY_NOP);
  if (!instr)
    return nil;

  switch (llvm_instr->opcode) {
  case LLVM_ADD:
  case LLVM_SUB:
  case LLVM_MUL:
  case LLVM_SDIV:
  case LLVM_UDIV:
  case LLVM_SREM:
  case LLVM_UREM:
  case LLVM_AND:
  case LLVM_OR:
  case LLVM_XOR:
  case LLVM_SHL:
  case LLVM_LSHR:
  case LLVM_ASHR:
    /* Binary operations */
    instr->opcode = llvm_to_fruity_opcode(llvm_instr->opcode);
    /* Operands handled by QBE via temp IDs */
    break;

  case LLVM_ALLOCA:
    /* CRITICAL: This is where Pebble semantics are injected!
     * LLVM alloca -> Fruity LIME (create white pebble)
     *
     * This ensures all stack allocations are tracked by Pebble,
     * giving C++ code memory safety guarantees.
     */
    instr->opcode = FRUITY_LIME;
    instr->pebble_effects.creates_white = 1;
    break;

  case LLVM_LOAD:
    /* Load from pointer - may need VANILLA for ref counting */
    instr->opcode = FRUITY_LOAD_LOCAL;
    if (llvm_instr->operand_count > 0) {
      instr->operand.type = FRUITY_OP_LOCAL;
      instr->operand.value.index = ctx->value_map[llvm_instr->operand_ids[0]];
    }
    break;

  case LLVM_STORE:
    /* Store to pointer */
    instr->opcode = FRUITY_STORE_LOCAL;
    if (llvm_instr->operand_count > 1) {
      instr->operand.type = FRUITY_OP_LOCAL;
      instr->operand.value.index = ctx->value_map[llvm_instr->operand_ids[1]];
    }
    break;

  case LLVM_RET:
    instr->opcode = FRUITY_RET;
    break;

  case LLVM_BR:
    if (llvm_instr->operand_count == 1) {
      /* Unconditional branch */
      instr->opcode = FRUITY_JUMP;
      /* Target block resolved later */
    } else {
      /* Conditional branch */
      instr->opcode = FRUITY_BTRUE;
      /* Condition and targets resolved later */
    }
    break;

  case LLVM_CALL:
    instr->opcode = FRUITY_CALL;
    if (llvm_instr->operand_count > 0) {
      instr->operand.type = FRUITY_OP_METHOD;
      instr->operand.value.token = llvm_instr->extra.call.callee_id;
    }
    break;

  case LLVM_ICMP:
    /* Integer comparison - depends on predicate */
    /* Predicate values: eq=32, ne=33, ugt=34, uge=35, ult=36, ule=37,
     *                   sgt=38, sge=39, slt=40, sle=41 */
    switch (llvm_instr->extra.cmp.cmp_predicate) {
    case 32:
      instr->opcode = FRUITY_CEQ;
      break;
    case 33:
      instr->opcode = FRUITY_CNE;
      break;
    case 38:
      instr->opcode = FRUITY_CGT;
      break;
    case 39:
      instr->opcode = FRUITY_CGE;
      break;
    case 40:
      instr->opcode = FRUITY_CLT;
      break;
    case 41:
      instr->opcode = FRUITY_CLE;
      break;
    default:
      instr->opcode = FRUITY_CEQ;
      break;
    }
    break;

  case LLVM_PHI:
    /* PHI nodes need special handling - resolved during SSA elimination */
    instr->opcode = FRUITY_NOP; /* Placeholder */
    break;

  case LLVM_GEP:
    /* GetElementPtr - pointer arithmetic */
    /* Translate to address calculation */
    instr->opcode = FRUITY_ADD; /* Simplified */
    break;

  case LLVM_ZEXT:
  case LLVM_SEXT:
  case LLVM_TRUNC:
  case LLVM_BITCAST:
    /* Type conversions - mostly no-op at Fruity level */
    instr->opcode = FRUITY_NOP;
    break;

  default:
    instr->opcode = FRUITY_NOP;
    break;
  }

  /* Map result to temp ID */
  if (llvm_instr->result_id < ctx->value_map_size) {
    ctx->value_map[llvm_instr->result_id] = ctx->next_temp++;
  }

  return instr;
}

/* ========== BLOCK TRANSLATION ========== */

static fruity_basic_block_t *translate_block(llvm_fruity_ctx_t *ctx,
                                             llvm_basic_block_t *llvm_bb) {
  fruity_basic_block_t *bb = fruity_function_add_block(ctx->current_func);
  if (!bb)
    return nil;

  bb->block_id = llvm_bb->id;
  ctx->current_block = bb;

  for (llvm_instruction_t *llvm_instr = llvm_bb->instructions_head; llvm_instr;
       llvm_instr = llvm_instr->next) {

    fruity_instruction_t *instr = translate_instruction(ctx, llvm_instr);
    if (instr) {
      fruity_block_add_instruction(bb, instr);
    }
  }

  return bb;
}

/* ========== FUNCTION TRANSLATION ========== */

static fruity_function_t *translate_function(llvm_fruity_ctx_t *ctx,
                                             llvm_function_t *llvm_func) {
  fruity_function_t *func = fruity_function_create(
      llvm_func->name ? llvm_func->name : "anonymous", llvm_func->id);
  if (!func)
    return nil;

  ctx->current_func = func;
  func->arg_count = llvm_func->param_count;
  func->local_count = llvm_func->local_count;

  /* Allocate value map for SSA resolution */
  ctx->value_map_size = llvm_func->local_count + 256;
  ctx->value_map = smalloc(ctx->value_map_size * sizeof(u32int));
  if (!ctx->value_map) {
    fruity_function_destroy(func);
    return nil;
  }
  memset(ctx->value_map, 0, ctx->value_map_size * sizeof(u32int));
  ctx->next_temp = 1;

  /* Translate each basic block */
  for (llvm_basic_block_t *llvm_bb = llvm_func->blocks_head; llvm_bb;
       llvm_bb = llvm_bb->next) {

    fruity_basic_block_t *bb = translate_block(ctx, llvm_bb);
    if (!bb) {
      free(ctx->value_map);
      fruity_function_destroy(func);
      return nil;
    }
  }

  free(ctx->value_map);
  ctx->value_map = nil;

  return func;
}

/* ========== MODULE TRANSLATION ========== */

fruity_module_t *llvm_to_fruity(llvm_module_t *llvm_mod, char *errbuf,
                                ulong errbuf_size) {
  if (!llvm_mod) {
    if (errbuf)
      snprint(errbuf, errbuf_size, "NULL LLVM module");
    return nil;
  }

  fruity_module_t *fruity_mod =
      fruity_module_create(llvm_mod->name ? llvm_mod->name : "llvm_module");
  if (!fruity_mod) {
    if (errbuf)
      snprint(errbuf, errbuf_size, "Failed to create Fruity module");
    return nil;
  }

  llvm_fruity_ctx_t ctx = {.llvm_mod = llvm_mod,
                           .fruity_mod = fruity_mod,
                           .current_func = nil,
                           .current_block = nil,
                           .value_map = nil,
                           .value_map_size = 0,
                           .next_temp = 1};

  /* Translate each function */
  for (llvm_function_t *llvm_func = llvm_mod->functions_head; llvm_func;
       llvm_func = llvm_func->next) {

    fruity_function_t *func = translate_function(&ctx, llvm_func);
    if (!func) {
      if (errbuf)
        snprint(errbuf, errbuf_size, "Failed to translate function %ud",
                llvm_func->id);
      fruity_module_destroy(fruity_mod);
      return nil;
    }

    fruity_module_add_function(fruity_mod, func);
  }

  return fruity_mod;
}

/* ========== FULL PIPELINE ========== */

int llvm_compile_bitcode(u8int *bc_data, ulong bc_size, uintptr asm_page,
                         char *errbuf, ulong errbuf_size) {
  /* 1. Parse LLVM bitcode */
  llvm_module_t *llvm_mod = nil;
  if (llvm_parse_bitcode(bc_data, bc_size, &llvm_mod, errbuf, errbuf_size) <
      0) {
    return -1;
  }

  /* 2. Translate to Fruity IR */
  fruity_module_t *fruity_mod = llvm_to_fruity(llvm_mod, errbuf, errbuf_size);
  llvm_module_destroy(llvm_mod);

  if (!fruity_mod) {
    return -1;
  }

  /* 3. Continue through existing pipeline:
   *    fruity_to_qbe() -> qbe_compile_page()
   *
   * For now, just return success - full integration pending.
   */

  fruity_module_destroy(fruity_mod);
  return 0;
}
