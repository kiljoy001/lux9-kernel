/* cil_domtree.c - Dominator Tree Implementation
 *
 * Implements dominator tree construction and reverse postorder numbering
 * for Ramsey's "Beyond Relooper" algorithm.
 */

#include "cil_domtree.h"


/* Kernel print function */
extern int print(char *fmt, ...);

/* ===== CFG Building ===== */

/* Check if opcode is a branch instruction */
static int is_branch_op(u8int op) {
  switch (op) {
  case 0x2B:
  case 0x2C:
  case 0x2D:
  case 0x2E:
  case 0x2F: /* br.s, brfalse.s, etc */
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x33:
  case 0x34:
  case 0x35:
  case 0x36:
  case 0x37:
  case 0x38:
  case 0x39:
  case 0x3A:
  case 0x3B:
  case 0x3C:
  case 0x3D:
  case 0x3E:
  case 0x3F:
  case 0x40:
  case 0x41:
  case 0x42:
  case 0x43:
  case 0x44: /* br, brfalse, etc */
  case 0x45:
  case 0x2A: /* ret */
    return 1;
  default:
    return 0;
  }
}

/* Check if branch is conditional (has 2 successors) */
static int is_conditional_op(u8int op) {
  switch (op) {
  case 0x2C:
  case 0x2D: /* brfalse.s, brtrue.s */
  case 0x2E:
  case 0x2F:
  case 0x30:
  case 0x31:
  case 0x32: /* beq.s, bge.s, etc */
  case 0x33:
  case 0x34:
  case 0x35:
  case 0x36:
  case 0x37:
  case 0x38:
  case 0x39: /* brfalse, brtrue */
  case 0x3E:
  case 0x3F:
  case 0x40:
  case 0x41:
  case 0x42: /* beq, bge, bgt, etc */
  case 0x43:
  case 0x44:
    return 1;
  default:
    return 0;
  }
}

/* Get branch offset size (1 for short .s, 4 for long) */
static int branch_offset_size(u8int op) {
  if (op >= 0x2B && op <= 0x37)
    return 1; /* Short branches */
  if (op >= 0x38 && op <= 0x45)
    return 4; /* Long branches */
  return 0;
}

/* Get operand size for non-branch opcodes */
static int get_op_size(u16int op) {
  /* Handle 2-byte opcodes */
  if ((op >> 8) == 0xFE) {
    u8int op2 = op & 0xFF;
    switch (op2) {
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C: /* ldarg, ldarga, starg, ldloc */
    case 0x0D:
    case 0x0E: /* ldloca, stloc */
      return 2;
    case 0x16: /* constrained. */
    case 0x1C: /* sizeof */
      return 4;
    default:
      return 0;
    }
  }

  /* Single byte opcodes */
  switch (op) {
  case 0x0E:
  case 0x0F:
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13: /* ldarg.s, etc */
  case 0x1F: /* ldc.i4.s */
  case 0xFE: /* prefix */
    return 1;
  case 0x20: /* ldc.i4 */
  case 0x22: /* ldc.r4 */
  case 0x27:
  case 0x28:
  case 0x29: /* jmp, call, calli */
  case 0x6F:
  case 0x70:
  case 0x71:
  case 0x72:
  case 0x73: /* callvirt, cpobj, etc */
  case 0x74:
  case 0x75:
  case 0x79:
  case 0x7B:
  case 0x7C: /* castclass, isinst, box, ldfld, etc */
  case 0x7D:
  case 0x7E:
  case 0x7F:
  case 0x80: /* stfld, ldsfld, ldsflda, stsfld */
  case 0x8C:
  case 0x8D:
  case 0xA3:
  case 0xA4:
  case 0xA5: /* box, newarr, ldelem, etc */
  case 0xC2: /* refanyval */
  case 0xC6: /* mkrefany */
  case 0xD0: /* ldtoken */
    return 4;
  case 0x21:
  case 0x23: /* ldc.i8, ldc.r8 */
    return 8;
  default:
    return 0;
  }
}

/* Build CFG from CIL bytecode */
int domtree_build_cfg(u8int *il, u32int il_size, dt_cfg_t *cfg) {
  if (!il || !cfg || il_size == 0)
    return -1;

  /* Initialize */
  for (u32int i = 0; i < MAX_BLOCKS; i++) {
    cfg->blocks[i].n_succ = 0;
    cfg->blocks[i].n_pred = 0;
  }
  cfg->n_blocks = 0;
  cfg->entry_block = 0;
  cfg->il = il;
  cfg->il_size = il_size;

  /* First pass: find all branch targets */
  u8int is_target[4096] = {0};
  is_target[0] = 1; /* Entry is always a target */

  u32int offset = 0;
  while (offset < il_size) {
    u8int op = il[offset];

    if (op == 0xFE && offset + 1 < il_size) {
      /* Two-byte opcode */
      u16int op2 = (0xFE << 8) | il[offset + 1];
      offset += 2 + get_op_size(op2);
      continue;
    }

    if (is_branch_op(op)) {
      int off_size = branch_offset_size(op);
      if (off_size > 0 && op != 0x2A) { /* Not ret */
        s32int branch_off = 0;
        if (off_size == 1) {
          branch_off = (s8int)il[offset + 1];
        } else {
          branch_off = *(s32int *)&il[offset + 1];
        }
        u32int target = offset + 1 + off_size + branch_off;
        if (target < il_size) {
          is_target[target] = 1;
        }
        /* Fallthrough for conditional branches */
        if (is_conditional_op(op)) {
          u32int fallthrough = offset + 1 + off_size;
          if (fallthrough < il_size) {
            is_target[fallthrough] = 1;
          }
        }
      }
      offset += 1 + off_size;
    } else {
      offset += 1 + get_op_size(op);
    }
  }

  /* Second pass: create basic blocks */
  u32int block_id = 0;
  u32int block_start = 0;

  for (offset = 0; offset < il_size && block_id < MAX_BLOCKS;) {
    /* Start new block if this is a target */
    if (is_target[offset] && offset > block_start) {
      /* End previous block */
      cfg->blocks[block_id].id = block_id;
      cfg->blocks[block_id].start_offset = block_start;
      cfg->blocks[block_id].end_offset = offset;
      cfg->blocks[block_id].terminator = TERM_FALLTHROUGH;
      cfg->blocks[block_id].succ[0] = block_id + 1;
      cfg->blocks[block_id].n_succ = 1;
      block_id++;
      block_start = offset;
    }

    u8int op = il[offset];
    int op_size = 1;

    if (op == 0xFE && offset + 1 < il_size) {
      u16int op2 = (0xFE << 8) | il[offset + 1];
      op_size = 2 + get_op_size(op2);
    } else if (is_branch_op(op)) {
      int off_size = branch_offset_size(op);
      op_size = 1 + off_size;

      /* This ends the current block */
      cfg->blocks[block_id].id = block_id;
      cfg->blocks[block_id].start_offset = block_start;
      cfg->blocks[block_id].end_offset = offset + op_size;
      cfg->blocks[block_id].branch_offset = offset;

      if (op == 0x2A) { /* ret */
        cfg->blocks[block_id].terminator = TERM_RETURN;
        cfg->blocks[block_id].n_succ = 0;
      } else if (is_conditional_op(op)) {
        cfg->blocks[block_id].terminator = TERM_CONDITIONAL;
        /* Compute targets */
        s32int branch_off = (off_size == 1) ? (s8int)il[offset + 1]
                                            : *(s32int *)&il[offset + 1];
        u32int true_target = offset + op_size + branch_off;
        u32int false_target = offset + op_size;
        /* These will be resolved to block IDs later */
        cfg->blocks[block_id].succ[0] = true_target;  /* Temp: offset */
        cfg->blocks[block_id].succ[1] = false_target; /* Temp: offset */
        cfg->blocks[block_id].n_succ = 2;
      } else {
        cfg->blocks[block_id].terminator = TERM_UNCONDITIONAL;
        s32int branch_off = (off_size == 1) ? (s8int)il[offset + 1]
                                            : *(s32int *)&il[offset + 1];
        u32int target = offset + op_size + branch_off;
        cfg->blocks[block_id].succ[0] = target; /* Temp: offset */
        cfg->blocks[block_id].n_succ = 1;
      }

      block_id++;
      block_start = offset + op_size;
    } else {
      op_size = 1 + get_op_size(op);
    }

    offset += op_size;
  }

  /* Handle last block if not terminated by branch */
  if (block_start < il_size && block_id < MAX_BLOCKS) {
    cfg->blocks[block_id].id = block_id;
    cfg->blocks[block_id].start_offset = block_start;
    cfg->blocks[block_id].end_offset = il_size;
    cfg->blocks[block_id].terminator = TERM_RETURN;
    cfg->blocks[block_id].n_succ = 0;
    block_id++;
  }

  cfg->n_blocks = block_id;

  /* Resolve offset-based successors to block IDs */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dt_basic_block_t *b = &cfg->blocks[i];
    for (u32int s = 0; s < b->n_succ; s++) {
      u32int target_offset = b->succ[s];
      /* Find block containing this offset */
      for (u32int j = 0; j < cfg->n_blocks; j++) {
        if (cfg->blocks[j].start_offset == target_offset) {
          b->succ[s] = j;
          break;
        }
      }
    }
  }

  /* Build predecessor lists */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dt_basic_block_t *b = &cfg->blocks[i];
    for (u32int s = 0; s < b->n_succ; s++) {
      u32int succ_id = b->succ[s];
      if (succ_id < cfg->n_blocks) {
        dt_basic_block_t *succ = &cfg->blocks[succ_id];
        if (succ->n_pred < MAX_DOM_CHILDREN) {
          succ->pred[succ->n_pred++] = i;
        }
      }
    }
  }

  return 0;
}

/* ===== Dominator Tree Construction ===== */

/* Simple O(n²) dominator computation using dataflow */
int domtree_build(dt_cfg_t *cfg, domtree_t *tree) {
  if (!cfg || !tree || cfg->n_blocks == 0)
    return -1;

  tree->n_nodes = cfg->n_blocks;
  tree->root = 0;

  /* Initialize: each node is its own dominator */
  u32int dom[MAX_BLOCKS];
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dom[i] = i;
    tree->nodes[i].block_id = i;
    tree->nodes[i].idom = i;
    tree->nodes[i].n_children = 0;
    tree->nodes[i].rpo = 0;
    tree->nodes[i].is_loop_header = 0;
    tree->nodes[i].is_merge_node = 0;
  }

  /* Entry node dominates only itself */
  dom[0] = 0;

  /* Iterate until fixed point */
  int changed = 1;
  while (changed) {
    changed = 0;
    for (u32int i = 1; i < cfg->n_blocks; i++) { /* Skip entry */
      dt_basic_block_t *b = &cfg->blocks[i];
      if (b->n_pred == 0)
        continue;

      /* New idom = intersection of all predecessors' dominators */
      u32int new_idom = b->pred[0];
      for (u32int p = 1; p < b->n_pred; p++) {
        u32int pred = b->pred[p];
        /* Intersect: find common dominator */
        u32int a = new_idom, b_val = pred;
        while (a != b_val) {
          while (a > b_val)
            a = dom[a];
          while (b_val > a)
            b_val = dom[b_val];
        }
        new_idom = a;
      }

      if (dom[i] != new_idom) {
        dom[i] = new_idom;
        changed = 1;
      }
    }
  }

  /* Build tree structure from idom relation */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    tree->nodes[i].idom = dom[i];
  }

  /* Add children to each node */
  for (u32int i = 1; i < cfg->n_blocks; i++) {
    u32int parent = dom[i];
    if (parent != i && tree->nodes[parent].n_children < MAX_DOM_CHILDREN) {
      tree->nodes[parent].children[tree->nodes[parent].n_children++] = i;
    }
  }

  return 0;
}

/* ===== Reverse Postorder Numbering ===== */

static u32int rpo_counter;
static u8int visited[MAX_BLOCKS];

static void rpo_dfs(dt_cfg_t *cfg, domtree_t *tree, u32int block) {
  if (visited[block])
    return;
  visited[block] = 1;

  dt_basic_block_t *b = &cfg->blocks[block];
  for (u32int i = 0; i < b->n_succ; i++) {
    rpo_dfs(cfg, tree, b->succ[i]);
  }

  tree->nodes[block].rpo = rpo_counter--;
}

void domtree_compute_rpo(dt_cfg_t *cfg, domtree_t *tree) {
  rpo_counter = cfg->n_blocks - 1;
  for (u32int i = 0; i < MAX_BLOCKS; i++)
    visited[i] = 0;

  rpo_dfs(cfg, tree, 0);
}

/* ===== Mark Special Nodes ===== */

void domtree_mark_special_nodes(dt_cfg_t *cfg, domtree_t *tree) {
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dt_basic_block_t *b = &cfg->blocks[i];
    domtree_node_t *n = &tree->nodes[i];

    /* Count forward in-edges */
    u32int forward_in = 0;
    for (u32int p = 0; p < b->n_pred; p++) {
      u32int pred = b->pred[p];
      if (tree->nodes[pred].rpo < n->rpo) {
        forward_in++;
      } else {
        /* Back edge: this is a loop header */
        n->is_loop_header = 1;
      }
    }

    /* Merge node has 2+ forward in-edges */
    n->is_merge_node = (forward_in >= 2) ? 1 : 0;
  }
}

/* ===== Helpers ===== */

domtree_node_t *domtree_get_node(domtree_t *tree, u32int block_id) {
  if (block_id >= tree->n_nodes)
    return 0;
  return &tree->nodes[block_id];
}

/* ===== Translation Context ===== */

void ctx_init(translation_ctx_t *ctx) {
  ctx->depth = 0;
  ctx->has_fallthrough = 0;
}

void ctx_push(translation_ctx_t *ctx, ctx_type_t type, u32int label) {
  if (ctx->depth < MAX_CTX_DEPTH) {
    ctx->frames[ctx->depth].type = type;
    ctx->frames[ctx->depth].label = label;
    ctx->depth++;
  }
}

void ctx_pop(translation_ctx_t *ctx) {
  if (ctx->depth > 0)
    ctx->depth--;
}

int ctx_index_of(translation_ctx_t *ctx, u32int label) {
  /* Search from innermost to outermost */
  for (int i = (int)ctx->depth - 1; i >= 0; i--) {
    ctx_frame_t *f = &ctx->frames[i];
    if ((f->type == CTX_LOOP_HEADED_BY || f->type == CTX_BLOCK_FOLLOWED_BY) &&
        f->label == label) {
      return (int)ctx->depth - 1 - i;
    }
  }
  return -1; /* Not found */
}

/* ===== Debug ===== */

void domtree_dump(domtree_t *tree, dt_cfg_t *cfg) {
  print("=== Dominator Tree (%d nodes) ===\n", tree->n_nodes);
  for (u32int i = 0; i < tree->n_nodes; i++) {
    domtree_node_t *n = &tree->nodes[i];
    dt_basic_block_t *b = &cfg->blocks[i];
    print("  Block %d: RPO=%d idom=%d [%d-%d)", i, n->rpo, n->idom,
          b->start_offset, b->end_offset);
    if (n->is_loop_header)
      print(" LOOP");
    if (n->is_merge_node)
      print(" MERGE");
    if (n->n_children > 0) {
      print(" children=[");
      for (u32int c = 0; c < n->n_children; c++) {
        print("%d", n->children[c]);
        if (c + 1 < n->n_children)
          print(",");
      }
      print("]");
    }
    print("\n");
  }
}
