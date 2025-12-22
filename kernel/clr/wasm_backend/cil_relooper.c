/* cil_relooper.c - Relooper algorithm implementation
 *
 * Converts arbitrary CIL control flow to structured WASM control flow.
 */

#include "cil_relooper.h"
#include "../il_parser.h"
#include "../il_to_fruity.h" /* For IL opcode definitions */
#include "wasm_buffer.h"

#ifndef nil
#define nil ((void *)0)
#endif

/* ========== WASM Opcodes ========== */
#define WASM_OP_UNREACHABLE 0x00
#define WASM_OP_NOP 0x01
#define WASM_OP_BLOCK 0x02
#define WASM_OP_LOOP 0x03
#define WASM_OP_IF 0x04
#define WASM_OP_ELSE 0x05
#define WASM_OP_END 0x0B
#define WASM_OP_BR 0x0C
#define WASM_OP_BR_IF 0x0D
#define WASM_OP_BR_TABLE 0x0E
#define WASM_OP_RETURN 0x0F

#define WASM_OP_LOCAL_GET 0x20
#define WASM_OP_LOCAL_SET 0x21

#define WASM_OP_I32_EQZ 0x45
#define WASM_OP_I64_EQZ 0x50
#define WASM_OP_I64_CONST 0x42
#define WASM_OP_I32_WRAP_I64 0xA7
#define WASM_OP_I64_EXTEND_I32_U 0xAD

#define WASM_TYPE_VOID 0x40

/* ========== CFG Building ========== */

/* Check if opcode is a branch instruction */
static int is_branch_opcode(u8int op) {
  switch (op) {
  case IL_BR_S:
  case IL_BRFALSE_S:
  case IL_BRTRUE_S:
  case IL_BEQ_S:
  case IL_BGE_S:
  case IL_BGT_S:
  case IL_BLE_S:
  case IL_BLT_S:
  case IL_BNE_UN_S:
  case IL_BR:
  case IL_BRFALSE:
  case IL_BRTRUE:
  case IL_BEQ:
  case IL_BGE:
  case IL_BGT:
  case IL_BLE:
  case IL_BLT:
  case IL_BNE_UN:
  case IL_LEAVE:
  case IL_LEAVE_S:
  case IL_RET:
  case IL_THROW:
  case IL_SWITCH:
    return 1;
  default:
    return 0;
  }
}

/* Check if opcode is conditional branch */
static int is_conditional_branch(u8int op) {
  switch (op) {
  case IL_BRFALSE_S:
  case IL_BRTRUE_S:
  case IL_BEQ_S:
  case IL_BGE_S:
  case IL_BGT_S:
  case IL_BLE_S:
  case IL_BLT_S:
  case IL_BNE_UN_S:
  case IL_BRFALSE:
  case IL_BRTRUE:
  case IL_BEQ:
  case IL_BGE:
  case IL_BGT:
  case IL_BLE:
  case IL_BLT:
  case IL_BNE_UN:
    return 1;
  default:
    return 0;
  }
}

/* Get branch offset size (1 for short, 4 for long) */
static int get_branch_size(u8int op) {
  switch (op) {
  case IL_BR_S:
  case IL_BRFALSE_S:
  case IL_BRTRUE_S:
  case IL_BEQ_S:
  case IL_BGE_S:
  case IL_BGT_S:
  case IL_BLE_S:
  case IL_BLT_S:
  case IL_BNE_UN_S:
  case IL_LEAVE_S:
    return 1;
  case IL_BR:
  case IL_BRFALSE:
  case IL_BRTRUE:
  case IL_BEQ:
  case IL_BGE:
  case IL_BGT:
  case IL_BLE:
  case IL_BLT:
  case IL_BNE_UN:
  case IL_LEAVE:
    return 4;
  default:
    return 0;
  }
}

/* Get operand size for non-branch opcodes */
static int get_operand_size(u16int op) {
  switch (op) {
  case IL_LDARG_S:
  case IL_LDARGA_S:
  case IL_STARG_S:
  case IL_LDLOC_S:
  case IL_LDLOCA_S:
  case IL_STLOC_S:
  case IL_LDC_I4_S:
    return 1;
  case IL_LDC_I4:
  case IL_CALL:
  case IL_CALLI:
  case IL_CALLVIRT:
  case IL_NEWOBJ:
  case IL_LDSTR:
  case IL_LDFLD:
  case IL_LDFLDA:
  case IL_STFLD:
  case IL_LDSFLD:
  case IL_LDSFLDA:
  case IL_STSFLD:
  case IL_NEWARR:
  case IL_BOX:
  case IL_UNBOX:
  case IL_UNBOX_ANY:
  case IL_CASTCLASS:
  case IL_ISINST:
  case IL_LDTOKEN:
  case IL_INITOBJ:
  case IL_SIZEOF:
  case IL_LDELEM:
  case IL_STELEM:
  case IL_LDELEMA:
  case IL_CPOBJ:
  case IL_LDOBJ:
  case IL_STOBJ:
    return 4;
  case IL_LDC_I8:
  case IL_LDC_R8:
    return 8;
  case IL_LDC_R4:
    return 4;
  default:
    /* Most opcodes have no operand */
    return 0;
  }
}

/* First pass: find all branch targets */
static void find_branch_targets(u8int *il, u32int il_size, u8int *is_target) {
  u32int offset = 0;

  memset(is_target, 0, il_size);

  while (offset < il_size) {
    u16int opcode = il[offset++];

    /* Handle 2-byte opcodes */
    if (opcode == 0xFE && offset < il_size) {
      opcode = (opcode << 8) | il[offset++];
    }

    if (is_branch_opcode((u8int)opcode)) {
      int size = get_branch_size((u8int)opcode);
      if (size > 0 && offset + size <= il_size) {
        s32int target_offset;
        if (size == 1) {
          target_offset = (s8int)il[offset];
        } else {
          target_offset = *(s32int *)&il[offset];
        }
        offset += size;

        /* Calculate absolute target */
        u32int target = offset + target_offset;
        if (target < il_size) {
          is_target[target] = 1;
        }
      }
    } else {
      /* Skip operand */
      offset += get_operand_size(opcode);
    }
  }
}

/* Build CFG from CIL bytecode */
int reloop_build_cfg(il_method_t *method, cil_cfg_t *cfg) {
  if (!method || !method->il_code || !cfg)
    return -1;

  u8int *il = method->il_code;
  u32int il_size = (u32int)method->il_code_size;

  memset(cfg, 0, sizeof(*cfg));

  /* Find all branch targets */
  u8int is_target[4096];
  if (il_size > 4096)
    return -2; /* Method too large */

  find_branch_targets(il, il_size, is_target);

  /* First block starts at 0 */
  is_target[0] = 1;

  /* Second pass: create basic blocks */
  u32int block_idx = 0;
  u32int offset = 0;

  while (offset < il_size && block_idx < RELOOP_MAX_BLOCKS) {
    /* Start new block */
    cil_block_t *block = &cfg->blocks[block_idx];
    block->id = block_idx;
    block->start_offset = offset;
    block->is_branch_target = is_target[offset];
    block->succ_count = 0;

    /* Map offset to block */
    cfg->offset_to_block[offset] = block_idx;

    /* Scan to end of block */
    while (offset < il_size) {
      u32int instr_start = offset;
      u16int opcode = il[offset++];

      /* Handle 2-byte opcodes */
      if (opcode == 0xFE && offset < il_size) {
        opcode = (opcode << 8) | il[offset++];
      }

      /* Check for branch */
      if (is_branch_opcode((u8int)opcode)) {
        int size = get_branch_size((u8int)opcode);

        block->branch_opcode = (u8int)opcode;

        if (size > 0) {
          if (size == 1) {
            block->branch_target = offset + (s8int)il[offset];
          } else {
            block->branch_target = offset + size + *(s32int *)&il[offset];
          }
          offset += size;
        }

        /* This ends the block */
        block->end_offset = offset;

        /* Add successors */
        if (opcode != IL_RET && opcode != IL_THROW) {
          /* Branch target is a successor */
          if (block->branch_target >= 0 &&
              (u32int)block->branch_target < il_size) {
            block->successors[block->succ_count++] =
                (u32int)block->branch_target;
          }

          /* Conditional branches also fall through */
          if (is_conditional_branch((u8int)opcode) && offset < il_size) {
            block->successors[block->succ_count++] = offset;
          }
        }

        break;
      }

      /* Skip operand */
      offset += get_operand_size(opcode);

      /* Check if next instruction is a branch target (ends this block) */
      if (offset < il_size && is_target[offset]) {
        block->end_offset = offset;
        /* Fallthrough successor */
        block->successors[block->succ_count++] = offset;
        break;
      }
    }

    /* Handle block ending at method end */
    if (block->end_offset == 0)
      block->end_offset = offset;

    block_idx++;
  }

  cfg->block_count = block_idx;
  cfg->entry_block = 0;

  /* Convert successor offsets to block indices */
  for (u32int i = 0; i < cfg->block_count; i++) {
    cil_block_t *block = &cfg->blocks[i];
    for (u32int j = 0; j < block->succ_count; j++) {
      u32int target_offset = block->successors[j];
      /* Find block containing this offset */
      for (u32int k = 0; k < cfg->block_count; k++) {
        if (cfg->blocks[k].start_offset == target_offset) {
          block->successors[j] = k;
          cfg->blocks[k].pred_count++;
          break;
        }
      }
    }
  }

  /* Identify loop headers (blocks with back edges) */
  for (u32int i = 0; i < cfg->block_count; i++) {
    cil_block_t *block = &cfg->blocks[i];
    for (u32int j = 0; j < block->succ_count; j++) {
      u32int succ = block->successors[j];
      if (succ <= i) { /* Back edge */
        cfg->blocks[succ].is_loop_header = 1;
        cfg->loop_headers[cfg->loop_header_count++] = succ;
      }
    }
  }

  print("RELOOP: Built CFG with %d blocks, %d loop headers\n", cfg->block_count,
        cfg->loop_header_count);

  return 0;
}

/* ========== Shape Analysis ========== */

static cil_shape_t *alloc_shape(reloop_ctx_t *ctx, shape_type_t type) {
  if (ctx->shape_count >= RELOOP_MAX_BLOCKS * 2)
    return nil;
  cil_shape_t *shape = &ctx->shapes[ctx->shape_count++];
  memset(shape, 0, sizeof(*shape));
  shape->type = type;
  return shape;
}

/* Improved shape analysis: detect loops, conditionals, and sequences */
cil_shape_t *reloop_analyze(cil_cfg_t *cfg, reloop_ctx_t *ctx) {
  if (!cfg || cfg->block_count == 0 || !ctx)
    return nil;

  ctx->cfg = cfg;
  ctx->shape_count = 0;

  /* Mark all blocks as unprocessed */
  for (u32int i = 0; i < cfg->block_count; i++) {
    ctx->processed[i] = 0;
  }

  /* Build shapes by processing blocks in order */
  cil_shape_t *root = nil;
  cil_shape_t **next_ptr = &root;

  for (u32int i = 0; i < cfg->block_count; i++) {
    if (ctx->processed[i])
      continue;

    cil_block_t *block = &cfg->blocks[i];
    cil_shape_t *shape;

    if (block->is_loop_header) {
      /* Create loop shape */
      shape = alloc_shape(ctx, SHAPE_LOOP);
      if (!shape)
        return nil;
      shape->loop.header_block = i;

      /* Find all blocks in the loop (those that can reach back to header) */
      cil_shape_t *inner = alloc_shape(ctx, SHAPE_SIMPLE);
      if (!inner)
        return nil;
      inner->simple.block_id = i;
      shape->loop.inner = inner;
      ctx->processed[i] = 1;

    } else if (block->succ_count == 2 &&
               is_conditional_branch(block->branch_opcode)) {
      /* Conditional branch - create if shape */
      shape = alloc_shape(ctx, SHAPE_IF);
      if (!shape)
        return nil;
      shape->cond.block_id = i;
      ctx->processed[i] = 1;

      /* Successors are: [0] = branch target, [1] = fallthrough */
      u32int then_block = block->successors[0];
      u32int else_block = block->successors[1];

      /* Create then shape if target not already processed */
      if (!ctx->processed[then_block] && then_block < cfg->block_count) {
        cil_shape_t *then_s = alloc_shape(ctx, SHAPE_SIMPLE);
        if (!then_s)
          return nil;
        then_s->simple.block_id = then_block;
        shape->cond.then_shape = then_s;
        ctx->processed[then_block] = 1;
      }

      /* Create else shape if fallthrough not already processed */
      if (!ctx->processed[else_block] && else_block < cfg->block_count &&
          else_block != then_block) {
        cil_shape_t *else_s = alloc_shape(ctx, SHAPE_SIMPLE);
        if (!else_s)
          return nil;
        else_s->simple.block_id = else_block;
        shape->cond.else_shape = else_s;
        ctx->processed[else_block] = 1;
      }

    } else {
      /* Simple block */
      shape = alloc_shape(ctx, SHAPE_SIMPLE);
      if (!shape)
        return nil;
      shape->simple.block_id = i;
      ctx->processed[i] = 1;
    }

    /* Link into sequence */
    *next_ptr = shape;
    if (shape->type == SHAPE_LOOP) {
      next_ptr = &shape->loop.next;
    } else if (shape->type == SHAPE_IF) {
      next_ptr = &shape->cond.next;
    } else if (shape->type == SHAPE_SEQUENCE) {
      next_ptr = &shape->seq.next;
    } else {
      /* For simple shapes, wrap in sequence to allow chaining */
      cil_shape_t *seq = alloc_shape(ctx, SHAPE_SEQUENCE);
      if (seq) {
        seq->seq.first = shape;
        *next_ptr = seq;
        next_ptr = &seq->seq.next;
      }
    }
  }

  return root;
}

/* ========== WASM Emission ========== */

/* Forward declaration */
static int emit_block_code(reloop_ctx_t *ctx, u32int block_id);

/* Emit a shape to WASM */
int reloop_emit(reloop_ctx_t *ctx, cil_shape_t *shape) {
  if (!ctx || !shape)
    return -1;

  wasm_buffer_t *out = ctx->output;

  switch (shape->type) {
  case SHAPE_SIMPLE:
    return emit_block_code(ctx, shape->simple.block_id);

  case SHAPE_LOOP: {
    /* Emit loop structure */
    wasm_emit_u8(out, WASM_OP_LOOP);
    wasm_emit_u8(out, WASM_TYPE_VOID); /* void result */
    ctx->label_depth++;

    /* Emit loop body */
    if (shape->loop.inner) {
      int err = reloop_emit(ctx, shape->loop.inner);
      if (err < 0)
        return err;
    }

    /* End loop */
    wasm_emit_u8(out, WASM_OP_END);
    ctx->label_depth--;

    /* Emit code after loop */
    if (shape->loop.next) {
      return reloop_emit(ctx, shape->loop.next);
    }
    return 0;
  }

  case SHAPE_IF: {
    /* Emit condition block code (but not branch) */
    int err = emit_block_code(ctx, shape->cond.block_id);
    if (err < 0)
      return err;

    /* Emit if structure */
    wasm_emit_u8(out, WASM_OP_IF);
    wasm_emit_u8(out, WASM_TYPE_VOID);
    ctx->label_depth++;

    /* Then branch */
    if (shape->cond.then_shape) {
      err = reloop_emit(ctx, shape->cond.then_shape);
      if (err < 0)
        return err;
    }

    /* Else branch */
    if (shape->cond.else_shape) {
      wasm_emit_u8(out, WASM_OP_ELSE);
      err = reloop_emit(ctx, shape->cond.else_shape);
      if (err < 0)
        return err;
    }

    wasm_emit_u8(out, WASM_OP_END);
    ctx->label_depth--;

    /* Code after if */
    if (shape->cond.next) {
      return reloop_emit(ctx, shape->cond.next);
    }
    return 0;
  }

  case SHAPE_SEQUENCE:
    if (shape->seq.first) {
      int err = reloop_emit(ctx, shape->seq.first);
      if (err < 0)
        return err;
    }
    if (shape->seq.next) {
      return reloop_emit(ctx, shape->seq.next);
    }
    return 0;

  case SHAPE_BREAK:
    /* Break to enclosing block */
    wasm_emit_u8(out, WASM_OP_BR);
    wasm_emit_uleb128(out, shape->branch.target_depth);
    return 0;

  case SHAPE_CONTINUE:
    /* Continue to loop header */
    wasm_emit_u8(out, WASM_OP_BR);
    wasm_emit_uleb128(out, shape->branch.target_depth);
    return 0;

  case SHAPE_MULTI: {
    /* Multi-entry using label dispatch
     *
     * Structure:
     * (loop $dispatch
     *   (block $L0
     *     (block $L1
     *       (block $L2
     *         local.get $label
     *         br_table $L0 $L1 $L2
     *       )
     *       ;; Label 2 code
     *       local.set $label 0
     *       br $dispatch
     *     )
     *     ;; Label 1 code
     *     local.set $label 1  ;; next label
     *     br $dispatch
     *   )
     *   ;; Label 0 code (fall through exits)
     * )
     */
    u32int count = shape->multi.handled_count;
    if (count == 0)
      return 0;

    /* Emit outer loop for dispatch */
    wasm_emit_u8(out, WASM_OP_LOOP);
    wasm_emit_u8(out, WASM_TYPE_VOID);
    ctx->label_depth++;

    /* Emit nested blocks for each label (in reverse order) */
    for (u32int i = 0; i < count; i++) {
      wasm_emit_u8(out, WASM_OP_BLOCK);
      wasm_emit_u8(out, WASM_TYPE_VOID);
      ctx->label_depth++;
    }

    /* Emit br_table dispatch */
    /* Load label variable (use local 0 as label var) */
    wasm_emit_u8(out, WASM_OP_LOCAL_GET);
    wasm_emit_uleb128(out, 0); /* Label variable in local 0 */
    wasm_emit_u8(out, WASM_OP_I32_WRAP_I64);

    /* br_table with targets */
    wasm_emit_u8(out, WASM_OP_BR_TABLE);
    wasm_emit_uleb128(out, count); /* Number of targets */
    for (u32int i = 0; i < count; i++) {
      wasm_emit_uleb128(out, i); /* Target label i */
    }
    wasm_emit_uleb128(out, 0); /* Default target */

    /* Emit code for each handled block (in reverse) */
    for (u32int i = count; i > 0; i--) {
      /* End block for label i-1 */
      wasm_emit_u8(out, WASM_OP_END);
      ctx->label_depth--;

      /* Emit block code */
      u32int block_id = shape->multi.handled[i - 1];
      int err = emit_block_code(ctx, block_id);
      if (err < 0)
        return err;

      /* If not last, set next label and continue dispatch */
      if (i > 1) {
        wasm_emit_u8(out, WASM_OP_I64_CONST);
        wasm_emit_sleb128(out, i - 2); /* Next label */
        wasm_emit_u8(out, WASM_OP_LOCAL_SET);
        wasm_emit_uleb128(out, 0);
        wasm_emit_u8(out, WASM_OP_BR);
        wasm_emit_uleb128(out, ctx->label_depth - 1); /* Back to loop */
      }
    }

    /* End dispatch loop */
    wasm_emit_u8(out, WASM_OP_END);
    ctx->label_depth--;

    /* Emit code after multi */
    if (shape->multi.next) {
      return reloop_emit(ctx, shape->multi.next);
    }
    return 0;
  }

  default:
    return -11;
  }
}

/* ========== Block Code Emission ========== */

/* External function to emit CIL opcodes (from cil_to_wasm.c) */
extern int cil_to_wasm_emit_one_opcode(wasm_buffer_t *buf, u8int *il,
                                       u32int *offset, u32int il_size);

/* Emit code for a single basic block */
static int emit_block_code(reloop_ctx_t *ctx, u32int block_id) {
  if (block_id >= ctx->cfg->block_count)
    return -1;

  cil_block_t *block = &ctx->cfg->blocks[block_id];
  wasm_buffer_t *out = ctx->output;
  il_method_t *method = ctx->method;

  u8int *il = method->il_code;
  u32int offset = block->start_offset;
  u32int end = block->end_offset;

  while (offset < end) {
    u16int opcode = il[offset];

    /* Handle branch opcodes specially */
    if (is_branch_opcode((u8int)opcode)) {
      /* Handle branch based on type */
      switch (opcode) {
      case IL_RET:
        wasm_emit_u8(out, WASM_OP_RETURN);
        offset++;
        break;

      case IL_BR_S:
      case IL_BR: {
        /* Unconditional branch - emit br to appropriate label */
        int size = get_branch_size((u8int)opcode);
        offset++; /* Skip opcode */

        s32int target;
        if (size == 1) {
          target = offset + 1 + (s8int)il[offset];
        } else {
          target = offset + 4 + *(s32int *)&il[offset];
        }
        offset += size;

        /* Find target block */
        u32int target_block = 0;
        for (u32int i = 0; i < ctx->cfg->block_count; i++) {
          if (ctx->cfg->blocks[i].start_offset == (u32int)target) {
            target_block = i;
            break;
          }
        }

        /* Check if back edge (loop continue) or forward (break) */
        if (target_block <= block_id) {
          /* Back edge - continue loop */
          wasm_emit_u8(out, WASM_OP_BR);
          wasm_emit_uleb128(out, 0); /* Innermost loop */
        } else {
          /* Forward - will fall through, no br needed here */
          /* The structure handles it */
        }
        break;
      }

      case IL_BRFALSE_S:
      case IL_BRFALSE:
      case IL_BRTRUE_S:
      case IL_BRTRUE: {
        /* Conditional branch */
        int size = get_branch_size((u8int)opcode);
        int is_true = (opcode == IL_BRTRUE_S || opcode == IL_BRTRUE);
        offset++; /* Skip opcode */

        s32int target;
        if (size == 1) {
          target = offset + 1 + (s8int)il[offset];
        } else {
          target = offset + 4 + *(s32int *)&il[offset];
        }
        offset += size;

        /* Convert i64 to i32 for branch condition */
        wasm_emit_u8(out, WASM_OP_I32_WRAP_I64);

        /* Invert condition if brfalse */
        if (!is_true) {
          wasm_emit_u8(out, WASM_OP_I32_EQZ);
        }

        /* Emit br_if */
        /* For now, branch to label 0 (innermost) */
        wasm_emit_u8(out, WASM_OP_BR_IF);
        wasm_emit_uleb128(out, 0);
        break;
      }

      case IL_THROW:
        wasm_emit_u8(out, WASM_OP_UNREACHABLE);
        offset++;
        break;

      default:
        /* Other branches - skip for now */
        offset++;
        offset += get_branch_size((u8int)opcode);
        break;
      }
    } else {
      /* Use existing opcode emitter */
      /* For now, inline basic emission */
      int err = cil_to_wasm_emit_one_opcode(out, il, &offset,
                                            (u32int)method->il_code_size);
      if (err < 0) {
        print("RELOOP: emit error %d at offset %d\n", err, offset);
        return err;
      }
    }
  }

  return 0;
}

/* ========== Main Entry Point ========== */

int reloop_compile_method(il_method_t *method, wasm_buffer_t *output) {
  if (!method || !output)
    return -1;

  /* Build CFG */
  cil_cfg_t cfg;
  int err = reloop_build_cfg(method, &cfg);
  if (err < 0) {
    print("RELOOP: Failed to build CFG: %d\n", err);
    return err;
  }

  /* If no branches, fall back to simple linear emit */
  if (cfg.block_count <= 1) {
    print("RELOOP: Single block, using linear emit\n");
    /* The existing cil_to_wasm_compile_method handles this */
    return 0;
  }

  /* Create context */
  reloop_ctx_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.cfg = &cfg;
  ctx.method = method;
  ctx.output = output;
  ctx.label_depth = 0;

  /* Analyze shapes */
  cil_shape_t *root = reloop_analyze(&cfg, &ctx);
  if (!root) {
    print("RELOOP: Failed to analyze shapes\n");
    return -3;
  }

  /* Emit WASM */
  err = reloop_emit(&ctx, root);
  if (err < 0) {
    print("RELOOP: Failed to emit WASM: %d\n", err);
    return err;
  }

  return 0;
}

/* ========== Debug Helpers ========== */

void reloop_dump_cfg(cil_cfg_t *cfg) {
  print("=== CFG Dump ===\n");
  print("Blocks: %d, Loop headers: %d\n", cfg->block_count,
        cfg->loop_header_count);

  for (u32int i = 0; i < cfg->block_count; i++) {
    cil_block_t *b = &cfg->blocks[i];
    print("  Block %d: [%d-%d) %s%s\n", i, b->start_offset, b->end_offset,
          b->is_loop_header ? "LOOP " : "",
          b->is_branch_target ? "TARGET" : "");
    print("    Successors:");
    for (u32int j = 0; j < b->succ_count; j++) {
      print(" %d", b->successors[j]);
    }
    print("\n");
  }
}

int reloop_is_branch_target(cil_cfg_t *cfg, u32int offset) {
  for (u32int i = 0; i < cfg->block_count; i++) {
    if (cfg->blocks[i].start_offset == offset &&
        cfg->blocks[i].is_branch_target)
      return 1;
  }
  return 0;
}

u32int reloop_get_block_at(cil_cfg_t *cfg, u32int offset) {
  for (u32int i = 0; i < cfg->block_count; i++) {
    if (cfg->blocks[i].start_offset <= offset &&
        offset < cfg->blocks[i].end_offset) {
      return i;
    }
  }
  return 0;
}
