/* cil_domtree.h - Dominator Tree for CIL Control Flow Analysis
 *
 * Implements Ramsey's "Beyond Relooper" algorithm for translating
 * unstructured CIL control flow to structured WebAssembly.
 *
 * Key concepts:
 * - Dominator tree: X dominates Y if all paths to Y go through X
 * - Reverse postorder (RPO): numbering where lower = earlier in execution
 * - Back edge: edge to a node with lower/equal RPO (loop back)
 * - Merge node: node with 2+ forward in-edges
 * - Loop header: target of a back edge
 */

#ifndef CIL_DOMTREE_H
#define CIL_DOMTREE_H

#include "cil_opcodes.h"

/* Maximum basic blocks per method */
#define MAX_BLOCKS 256

/* Maximum children per dominator tree node */
#define MAX_DOM_CHILDREN 32

/* Maximum context depth for nested blocks/loops */
#define MAX_CTX_DEPTH 64

/* ===== Basic Block (dt_ prefix to avoid collision with cil_relooper.c) =====
 */
typedef struct dt_basic_block {
  u32int id;           /* Block ID (0 = entry) */
  u32int start_offset; /* CIL start offset */
  u32int end_offset;   /* CIL end offset (exclusive) */

  /* Successors (0-2 for most blocks) */
  u32int succ[2];
  u32int n_succ;

  /* Predecessors (built during CFG construction) */
  u32int pred[MAX_DOM_CHILDREN];
  u32int n_pred;

  /* Control flow terminator type */
  enum {
    TERM_FALLTHROUGH,   /* Falls through to next block */
    TERM_UNCONDITIONAL, /* Unconditional branch */
    TERM_CONDITIONAL,   /* Conditional branch (2 successors) */
    TERM_RETURN,        /* Return from function */
    TERM_SWITCH         /* Switch statement */
  } terminator;

  /* For conditional: the condition is the last opcode before branch */
  u32int branch_offset; /* Offset of branch instruction */
} dt_basic_block_t;

/* ===== Control Flow Graph (dt_ prefix to avoid collision) ===== */
typedef struct dt_cfg {
  dt_basic_block_t blocks[MAX_BLOCKS];
  u32int n_blocks;
  u32int entry_block; /* Usually 0 */

  /* CIL bytecode reference */
  u8int *il;
  u32int il_size;
} dt_cfg_t;

/* ===== Dominator Tree Node ===== */
typedef struct domtree_node {
  u32int block_id; /* Which basic block this represents */
  u32int idom;     /* Immediate dominator's block_id */

  /* Children (nodes immediately dominated by this one) */
  u32int children[MAX_DOM_CHILDREN];
  u32int n_children;

  /* Reverse postorder number (lower = earlier) */
  u32int rpo;

  /* Flags computed from CFG/RPO */
  u8int is_loop_header; /* Target of back edge */
  u8int is_merge_node;  /* Has 2+ forward in-edges */
} domtree_node_t;

/* ===== Dominator Tree ===== */
typedef struct domtree {
  domtree_node_t nodes[MAX_BLOCKS];
  u32int n_nodes;
  u32int root; /* Entry block (RPO = 0) */
} domtree_t;

/* ===== Translation Context (for br index computation) ===== */
typedef enum {
  CTX_IF_THEN_ELSE,     /* Inside if...else...end */
  CTX_LOOP_HEADED_BY,   /* Inside loop headed by label */
  CTX_BLOCK_FOLLOWED_BY /* Inside block followed by label */
} ctx_type_t;

typedef struct ctx_frame {
  ctx_type_t type;
  u32int label; /* Block ID for LOOP/BLOCK contexts */
} ctx_frame_t;

typedef struct translation_ctx {
  ctx_frame_t frames[MAX_CTX_DEPTH];
  u32int depth;

  /* Fallthrough label (for br elimination optimization) */
  u32int fallthrough_label;
  u8int has_fallthrough;
} translation_ctx_t;

/* ===== API Functions ===== */

/* Build CFG from CIL bytecode */
int domtree_build_cfg(u8int *il, u32int il_size, dt_cfg_t *cfg);

/* Build dominator tree from CFG */
int domtree_build(dt_cfg_t *cfg, domtree_t *tree);

/* Compute reverse postorder numbering */
void domtree_compute_rpo(dt_cfg_t *cfg, domtree_t *tree);

/* Mark loop headers and merge nodes */
void domtree_mark_special_nodes(dt_cfg_t *cfg, domtree_t *tree);

/* Get domtree node by block ID */
domtree_node_t *domtree_get_node(domtree_t *tree, u32int block_id);

/* Translation context helpers */
void ctx_init(translation_ctx_t *ctx);
void ctx_push(translation_ctx_t *ctx, ctx_type_t type, u32int label);
void ctx_pop(translation_ctx_t *ctx);
int ctx_index_of(translation_ctx_t *ctx, u32int label);

/* Debug */
void domtree_dump(domtree_t *tree, dt_cfg_t *cfg);

#endif /* CIL_DOMTREE_H */
