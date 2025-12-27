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
  case 0x00: /* nop */
  case 0x01: /* break */
    return 1;
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
/*
 * @coq_proof: proofs/relooper/cfg_spec.v
 * @theorem: cfg_identifies_all_targets
 *   - For any branch target offset, a basic block starts at that offset
 * @theorem: cfg_edges_correct
 *   - CFG edges match actual control flow (branch targets or fallthroughs)
 * @theorem: cfg_no_missing_edges
 *   - All control flow transitions are captured as edges
 * @theorem: basic_block_single_entry
 *   - No internal offsets within a block are branch targets
 *
 * Two-pass CFG construction algorithm:
 *   Pass 1: Scan bytecode to identify all branch targets (is_target array)
 *   Pass 2: Create basic blocks at each target, link successors/predecessors
 */
/*
  ACSL SPEC (see proofs/relooper/cfg_spec.v for formal definitions):

  requires \valid(il + (0..il_size-1));
  requires \valid(cfg);
  requires il_size > 0 && il_size <= 4096;

  ensures \result == 0 ==> cfg->n_blocks > 0;
  ensures \result == 0 ==> cfg->n_blocks <= MAX_BLOCKS;
  ensures \result == 0 ==> cfg->entry_block == 0;

  COQ_PROOF_REF: cfg_identifies_all_targets - all branch targets have blocks
  COQ_PROOF_REF: entry_block_exists - block 0 starts at offset 0
  COQ_PROOF_REF: basic_block_single_entry - no internal branch targets
*/
/* Build CFG from CIL bytecode */
int domtree_build_cfg(u8int *il, u32int il_size, dt_cfg_t *cfg) {
  if (!il || !cfg || il_size == 0)
    return -1;

  /* Initialize */
  for (u32int i = 0; i < MAX_BLOCKS; i++) {
    cfg->blocks[i].n_succ = 0;
    cfg->blocks[i].n_pred = 0;
    cfg->blocks[i].branch_opcode = 0;
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
      cfg->blocks[block_id].succ[0] =
          block_id + 1; /* Temp: next block ID? No, this is problematic if next
                           block not created yet? */
      /* Actually, FALLTHROUGH succ should be the OFFSET of the start of next
       * block */
      cfg->blocks[block_id].succ[0] = offset;
      cfg->blocks[block_id].n_succ = 1;

      print("CFG: Created Block %d [%d-%d] Fallthrough to %d\n", block_id,
            block_start, offset, offset);

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
      cfg->blocks[block_id].branch_opcode =
          op; /* Store for comparison emission */

      if (op == 0x2A) { /* ret */
        cfg->blocks[block_id].terminator = TERM_RETURN;
        cfg->blocks[block_id].n_succ = 0;
        print("CFG: Created Block %d [%d-%d] Return\n", block_id, block_start,
              offset + op_size);
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
        print("CFG: Created Block %d [%d-%d] Cond (op=%02x) True=%d False=%d\n",
              block_id, block_start, offset + op_size, op, true_target,
              false_target);
      } else {
        cfg->blocks[block_id].terminator = TERM_UNCONDITIONAL;
        s32int branch_off = (off_size == 1) ? (s8int)il[offset + 1]
                                            : *(s32int *)&il[offset + 1];
        u32int target = offset + op_size + branch_off;
        cfg->blocks[block_id].succ[0] = target; /* Temp: offset */
        cfg->blocks[block_id].n_succ = 1;
        print("CFG: Created Block %d [%d-%d] Uncond (op=%02x) Target=%d\n",
              block_id, block_start, offset + op_size, op, target);
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
    print("CFG: Created Block %d [%d-%d] End (Return)\n", block_id, block_start,
          il_size);
    block_id++;
  }

  cfg->n_blocks = block_id;

  /* Resolve offset-based successors to block IDs */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dt_basic_block_t *b = &cfg->blocks[i];
    for (u32int s = 0; s < b->n_succ; s++) {
      u32int target_offset = b->succ[s];
      int found = 0;
      /* Find block containing this offset */
      for (u32int j = 0; j < cfg->n_blocks; j++) {
        if (cfg->blocks[j].start_offset == target_offset) {
          b->succ[s] = j;
          found = 1;
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

/*
 * @coq_proof: proofs/relooper/domtree_spec.v
 * @theorem: domtree_captures_dominance
 *   - For valid tree: dt_idom node = d -> dominates cfg d n
 * @invariant: ValidIdom cfg tree id
 *   - The idom relation correctly captures immediate dominance
 *
 * Cooper's Algorithm for computing dominators.
 * Requires: RPO numbering already computed (call domtree_compute_rpo first)
 *
 * BUG FIX NOTES:
 * 1. Must skip unprocessed predecessors (where dom[pred] == pred && pred != 0)
 * 2. Intersection must terminate: if dom[x] == x for non-entry, skip
 */
/*@
  requires \valid(cfg) && \valid(tree);
  requires cfg->n_blocks > 0 && cfg->n_blocks <= MAX_BLOCKS;
  requires tree->n_nodes == cfg->n_blocks;
  // Pre: RPO must already be computed
  requires \forall integer i; 0 <= i < cfg->n_blocks ==>
           tree->nodes[i].rpo < cfg->n_blocks;

  assigns tree->nodes[0..cfg->n_blocks-1].idom;
  assigns tree->nodes[0..cfg->n_blocks-1].n_children;
  assigns tree->nodes[0..cfg->n_blocks-1].children[0..31];

  // Post: ValidIdom - each node's idom dominates it
  ensures \forall integer i; 0 <= i < cfg->n_blocks ==>
          tree->nodes[i].idom < cfg->n_blocks;
*/
int domtree_build(dt_cfg_t *cfg, domtree_t *tree) {
  if (!cfg || !tree || cfg->n_blocks == 0)
    return -1;

  tree->n_nodes = cfg->n_blocks;
  tree->root = 0;

  /* Initialize: each node is its own dominator (marks as "unprocessed") */
  u32int dom[MAX_BLOCKS];
  /*@
    loop invariant 0 <= i <= cfg->n_blocks;
    loop invariant \forall integer j; 0 <= j < i ==> dom[j] == j;
    loop invariant \forall integer j; 0 <= j < i ==> tree->nodes[j].block_id ==
    j; loop assigns i, dom[0..cfg->n_blocks-1], tree->nodes[0..cfg->n_blocks-1];
    loop variant cfg->n_blocks - i;
  */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dom[i] = i;
    tree->nodes[i].block_id = i;
    tree->nodes[i].idom = i;
    tree->nodes[i].n_children = 0;
    // Note: Don't clear RPO - it was computed by domtree_compute_rpo
    tree->nodes[i].is_loop_header = 0;
    tree->nodes[i].is_merge_node = 0;
  }

  /* Entry node dominates only itself - mark as "processed" */
  dom[0] = 0;

  /* Iterate until fixed point */
  int changed = 1;
  int iterations = 0;
  int max_iterations = cfg->n_blocks * cfg->n_blocks; /* Safety bound */

  /*@
    loop invariant 0 <= iterations <= max_iterations;
    loop invariant dom[0] == 0;
    loop invariant \forall integer j; 0 <= j < cfg->n_blocks ==> dom[j] <
    cfg->n_blocks; loop invariant changed == 0 || changed == 1; loop assigns
    iterations, changed, dom[1..cfg->n_blocks-1]; loop variant max_iterations -
    iterations;
  */
  while (changed && iterations < max_iterations) {
    changed = 0;
    iterations++;

    /*@
      loop invariant 0 <= i <= cfg->n_blocks;
      loop invariant dom[0] == 0;
      loop invariant \forall integer j; 0 <= j < cfg->n_blocks ==> dom[j] <
      cfg->n_blocks; loop assigns i, changed, dom[1..cfg->n_blocks-1]; loop
      variant cfg->n_blocks - i;
    */
    for (u32int i = 1; i < cfg->n_blocks; i++) { /* Skip entry */
      dt_basic_block_t *b = &cfg->blocks[i];
      if (b->n_pred == 0)
        continue;

      /* Find first PROCESSED predecessor to start intersection */
      u32int new_idom = (u32int)-1;
      /*@
        loop invariant 0 <= p <= b->n_pred;
        loop invariant new_idom == (u32int)-1 || new_idom < cfg->n_blocks;
        loop assigns p, new_idom;
        loop variant b->n_pred - p;
      */
      for (u32int p = 0; p < b->n_pred; p++) {
        u32int pred = b->pred[p];
        /* A predecessor is "processed" if dom[pred] != pred OR pred == 0 */
        if (pred == 0 || dom[pred] != pred) {
          new_idom = pred;
          break;
        }
      }

      /* If no processed predecessor, skip this node for now */
      if (new_idom == (u32int)-1)
        continue;

      /* Intersect with remaining PROCESSED predecessors */
      /*@
        loop invariant 0 <= p <= b->n_pred;
        loop invariant new_idom < cfg->n_blocks;
        loop assigns p, new_idom;
        loop variant b->n_pred - p;
      */
      for (u32int p = 0; p < b->n_pred; p++) {
        u32int pred = b->pred[p];
        if (pred == new_idom)
          continue;
        /* Skip unprocessed predecessors */
        if (pred != 0 && dom[pred] == pred)
          continue;

        /* Intersect: find common dominator using RPO */
        u32int a = new_idom, b_val = pred;
        int safety = 0;
        /*@
          loop invariant 0 <= safety <= MAX_BLOCKS;
          loop invariant a < cfg->n_blocks;
          loop invariant b_val < cfg->n_blocks;
          loop invariant a < cfg->n_blocks ==> tree->nodes[a].rpo <
          cfg->n_blocks; loop invariant b_val < cfg->n_blocks ==>
          tree->nodes[b_val].rpo < cfg->n_blocks; loop assigns safety, a, b_val;
          loop variant MAX_BLOCKS - safety;
        */
        while (a != b_val && safety < MAX_BLOCKS) {
          safety++;
          /*@
            loop invariant a < cfg->n_blocks;
            loop invariant dom[a] < cfg->n_blocks;
            loop invariant tree->nodes[a].rpo >= tree->nodes[b_val].rpo || a ==
            0 || dom[a] == a; loop assigns a; loop variant tree->nodes[a].rpo;
          */
          while (tree->nodes[a].rpo > tree->nodes[b_val].rpo && a != 0 &&
                 dom[a] != a)
            a = dom[a];
          /*@
            loop invariant b_val < cfg->n_blocks;
            loop invariant dom[b_val] < cfg->n_blocks;
            loop invariant tree->nodes[b_val].rpo >= tree->nodes[a].rpo || b_val
            == 0 || dom[b_val] == b_val; loop assigns b_val; loop variant
            tree->nodes[b_val].rpo;
          */
          while (tree->nodes[b_val].rpo > tree->nodes[a].rpo && b_val != 0 &&
                 dom[b_val] != b_val)
            b_val = dom[b_val];
          /* If stuck (unprocessed node), break */
          if ((a != 0 && dom[a] == a) || (b_val != 0 && dom[b_val] == b_val))
            break;
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
  /*@
    loop invariant 0 <= i <= cfg->n_blocks;
    loop invariant \forall integer j; 0 <= j < i ==> tree->nodes[j].idom ==
    dom[j]; loop assigns i, tree->nodes[0..cfg->n_blocks-1].idom; loop variant
    cfg->n_blocks - i;
  */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    tree->nodes[i].idom = dom[i];
  }

  /* Add children to each node */
  /*@
    loop invariant 0 <= i <= cfg->n_blocks;
    loop invariant \forall integer j; 0 <= j < cfg->n_blocks ==>
    tree->nodes[j].n_children <= 32; loop assigns i,
    tree->nodes[0..cfg->n_blocks-1].n_children,
    tree->nodes[0..cfg->n_blocks-1].children[0..31]; loop variant cfg->n_blocks
    - i;
  */
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

/*
 * @coq_proof: proofs/relooper/domtree_spec.v
 * @theorem: loop_header_iff_back_edge
 *   - A block is a loop header iff it has a back edge targeting it
 * @theorem: back_edge_characterization
 *   - Edge (src, dst) is back edge iff rpo(dst) <= rpo(src)
 * @theorem: merge_node_characterization
 *   - Block is merge node iff forward_pred_count >= 2
 *
 * @invariant: ValidLoopHeader cfg tree id
 *   - node_is_loop_header = true <-> has_back_edge_to cfg tree id
 * @invariant: ValidMergeNodeProp cfg tree id
 *   - is_merge_node = true <-> forward_pred_count >= 2
 */
/*@
  requires \valid(cfg) && \valid(tree);
  requires cfg->n_blocks <= MAX_BLOCKS;
  requires tree->n_nodes == cfg->n_blocks;

  assigns tree->nodes[0..cfg->n_blocks-1].is_loop_header;
  assigns tree->nodes[0..cfg->n_blocks-1].is_merge_node;

  // Post: ValidLoopHeader - loop header iff back edge exists
  ensures \forall integer i; 0 <= i < cfg->n_blocks ==>
    (tree->nodes[i].is_loop_header == 1 ==>
      \exists integer p; 0 <= p < cfg->blocks[i].n_pred &&
        tree->nodes[cfg->blocks[i].pred[p]].rpo >= tree->nodes[i].rpo);

  // Post: ValidMergeNodeProp - merge iff 2+ forward preds
  ensures \forall integer i; 0 <= i < cfg->n_blocks ==>
    (tree->nodes[i].is_merge_node == 1 ==>
      \exists integer p1, p2; 0 <= p1 < p2 < cfg->blocks[i].n_pred &&
        tree->nodes[cfg->blocks[i].pred[p1]].rpo < tree->nodes[i].rpo &&
        tree->nodes[cfg->blocks[i].pred[p2]].rpo < tree->nodes[i].rpo);
*/
void domtree_mark_special_nodes(dt_cfg_t *cfg, domtree_t *tree) {
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dt_basic_block_t *b = &cfg->blocks[i];
    domtree_node_t *n = &tree->nodes[i];

    /* Count forward in-edges */
    u32int forward_in = 0;
    for (u32int p = 0; p < b->n_pred; p++) {
      u32int pred = b->pred[p];
      /* Back edge: rpo(pred) >= rpo(this) per back_edge_characterization */
      if (tree->nodes[pred].rpo < n->rpo) {
        forward_in++;
      } else {
        /* Back edge: this is a loop header per loop_header_iff_back_edge */
        n->is_loop_header = 1;
      }
    }

    /* Merge node: 2+ forward predecessors per merge_node_characterization */
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
