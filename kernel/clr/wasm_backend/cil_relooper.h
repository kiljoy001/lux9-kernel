/* cil_relooper.h - Relooper algorithm for CIL→WASM control flow
 *
 * Implements the relooper algorithm to convert arbitrary CIL control flow
 * (including loops and irreducible control flow) into structured WASM
 * control flow constructs (block, loop, if).
 *
 * Based on Emscripten's relooper by Alon Zakai (2011).
 */

#ifndef CIL_RELOOPER_H
#define CIL_RELOOPER_H

#include "../il_parser.h"
#include "wasm_buffer.h"

/* Maximum blocks and edges we can handle */
#define RELOOP_MAX_BLOCKS 256
#define RELOOP_MAX_EDGES 512
#define RELOOP_MAX_DEPTH 32

/* ========== Basic Block ========== */

typedef struct cil_block {
  u32int id;           /* Block index */
  u32int start_offset; /* CIL offset of first instruction */
  u32int end_offset;   /* CIL offset after last instruction */

  /* Successors */
  u32int successors[4]; /* Max 4 successors (switch can have more) */
  u32int succ_count;

  /* Predecessors (computed) */
  u32int predecessors[8];
  u32int pred_count;

  /* Control flow properties */
  int is_loop_header;   /* Has back edge pointing here */
  int is_branch_target; /* Target of explicit branch */
  int is_entry;         /* Entry point */
  int visited;          /* For graph traversal */

  /* Branch info */
  u8int branch_opcode;  /* Terminal branch opcode (0 if fallthrough) */
  s32int branch_target; /* Target offset for branch */
} cil_block_t;

/* ========== Control Flow Graph ========== */

typedef struct cil_cfg {
  cil_block_t blocks[RELOOP_MAX_BLOCKS];
  u32int block_count;
  u32int entry_block;

  /* Offset to block mapping */
  u32int offset_to_block[4096]; /* CIL offset -> block index */

  /* Loop headers (blocks that dominate back edges) */
  u32int loop_headers[RELOOP_MAX_BLOCKS];
  u32int loop_header_count;
} cil_cfg_t;

/* ========== Shape Types ========== */

typedef enum {
  SHAPE_SIMPLE,   /* Single block, no branches */
  SHAPE_LOOP,     /* Loop with back edge */
  SHAPE_IF,       /* Conditional (if/else) */
  SHAPE_BREAK,    /* Break out of enclosing structure */
  SHAPE_CONTINUE, /* Continue to loop header */
  SHAPE_SEQUENCE, /* Sequence of shapes */
  SHAPE_MULTI,    /* Multiple entry (label dispatch) */
} shape_type_t;

/* Forward declaration */
typedef struct cil_shape cil_shape_t;

struct cil_shape {
  shape_type_t type;

  union {
    /* SHAPE_SIMPLE */
    struct {
      u32int block_id;
    } simple;

    /* SHAPE_LOOP */
    struct {
      cil_shape_t *inner;  /* Body of loop */
      cil_shape_t *next;   /* After loop */
      u32int header_block; /* Loop header block ID */
    } loop;

    /* SHAPE_IF */
    struct {
      u32int block_id; /* Block with condition */
      cil_shape_t *then_shape;
      cil_shape_t *else_shape;
      cil_shape_t *next; /* Merge point */
    } cond;

    /* SHAPE_SEQUENCE */
    struct {
      cil_shape_t *first;
      cil_shape_t *next;
    } seq;

    /* SHAPE_BREAK/CONTINUE */
    struct {
      u32int target_depth; /* How many levels to break/continue */
    } branch;

    /* SHAPE_MULTI */
    struct {
      u32int *handled; /* Block IDs in this multi */
      u32int handled_count;
      cil_shape_t *next;
    } multi;
  };
};

/* ========== Relooper Context ========== */

typedef struct {
  cil_cfg_t *cfg;
  il_method_t *method;
  il_assembly_t *assembly;
  wasm_buffer_t *output;

  /* Label stack for nested structures */
  u32int label_stack[RELOOP_MAX_DEPTH];
  u32int label_depth;

  /* Shape allocator */
  cil_shape_t shapes[RELOOP_MAX_BLOCKS * 2];
  u32int shape_count;

  /* Block set for processing */
  u8int in_set[RELOOP_MAX_BLOCKS];
  u8int processed[RELOOP_MAX_BLOCKS];
} reloop_ctx_t;

/* ========== Public API ========== */

/**
 * reloop_build_cfg - Build control flow graph from CIL method
 *
 * Parses CIL bytecode and builds a CFG with basic blocks and edges.
 * Identifies branch targets and loop headers.
 *
 * @param method: IL method to analyze
 * @param cfg: Output CFG structure
 * @return: 0 on success, negative on error
 */
int reloop_build_cfg(il_method_t *method, cil_cfg_t *cfg);

/**
 * reloop_analyze - Analyze CFG and build shape tree
 *
 * Applies the relooper algorithm to identify control flow patterns
 * and build a tree of shapes (loops, conditionals, sequences).
 *
 * @param cfg: Input control flow graph
 * @param ctx: Relooper context for shape allocation
 * @return: Root shape on success, NULL on error
 */
cil_shape_t *reloop_analyze(cil_cfg_t *cfg, reloop_ctx_t *ctx);

/**
 * reloop_emit - Emit WASM from shape tree
 *
 * Traverses the shape tree and emits structured WASM control flow.
 *
 * @param ctx: Relooper context with output buffer
 * @param shape: Root shape to emit
 * @return: 0 on success, negative on error
 */
int reloop_emit(reloop_ctx_t *ctx, cil_shape_t *shape);

/**
 * reloop_compile_method - Full relooper compilation
 *
 * Convenience function that builds CFG, analyzes, and emits WASM.
 *
 * @param method: IL method to compile
 * @param output: Output buffer for WASM bytecode
 * @return: 0 on success, negative on error
 */
int reloop_compile_method(il_method_t *method, il_assembly_t *assembly,
                          wasm_buffer_t *output);

/**
 * reloop_compile_method_ramsey - Ramsey algorithm compilation
 *
 * Uses Ramsey's "Beyond Relooper" algorithm based on dominator trees
 * and reverse postorder numbering for optimal control flow translation.
 *
 * @param il: CIL bytecode
 * @param il_size: Size of bytecode
 * @param output: Output buffer for WASM bytecode
 * @return: 0 on success, negative on error
 */
int reloop_compile_method_ramsey(il_method_t *method, il_assembly_t *assembly,
                                 wasm_buffer_t *output);

/* Helper to check if offset is a branch target */
int reloop_is_branch_target(cil_cfg_t *cfg, u32int offset);

/* Helper to get block containing offset */
u32int reloop_get_block_at(cil_cfg_t *cfg, u32int offset);

/* Debug: print CFG */
void reloop_dump_cfg(cil_cfg_t *cfg);

#endif /* CIL_RELOOPER_H */
