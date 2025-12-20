/* fruity_ir.h - Fruity Intermediate Representation Core Data Structures
 *
 * The Fruity IR represents compiled CLR code with explicit Pebble operations.
 * All data structures use intrusive doubly-linked lists (kernel pattern).
 *
 * Key Components:
 *   - fruity_instruction_t: Individual IR instruction
 *   - fruity_basic_block_t: CFG node containing instructions
 *   - fruity_function_t: Complete function with CFG
 *   - fruity_module_t: Collection of functions (compiled assembly)
 */

#ifndef FRUITY_IR_H
#define FRUITY_IR_H

/* Fruity IR assumes kernel headers are already included:
 *   u.h, portlib.h, mem.h, dat.h, fns.h
 * These must be included by the caller before including fruity_ir.h
 */

#include "fruity_opcodes.h"
#include "fruity_types.h"

/* Forward declarations */
typedef struct fruity_instruction fruity_instruction_t;
typedef struct fruity_basic_block fruity_basic_block_t;
typedef struct fruity_function fruity_function_t;
typedef struct fruity_module fruity_module_t;

/* Switch targets structure */
typedef struct {
  u32int count;
  fruity_basic_block_t **targets;
} fruity_switch_targets_t;

/* ===== Instruction Operand ===== */

typedef enum {
  FRUITY_OP_NONE,
  FRUITY_OP_IMM_I32, /* int32 immediate */
  FRUITY_OP_IMM_I64, /* int64 immediate */
  FRUITY_OP_IMM_R32, /* float32 immediate */
  FRUITY_OP_IMM_R64, /* float64 immediate */
  FRUITY_OP_LOCAL,   /* Local variable index */
  FRUITY_OP_ARG,     /* Argument index */
  FRUITY_OP_FIELD,   /* Field offset */
  FRUITY_OP_TYPE,    /* Type token (ECMA-335 metadata) */
  FRUITY_OP_METHOD,  /* Method token */
  FRUITY_OP_BRANCH,  /* Branch target (basic block) */
  FRUITY_OP_SWITCH,  /* Switch targets */
  FRUITY_OP_TASKLET, /* Tasklet ID (for GRAPE) */
} fruity_operand_type_t;

typedef union {
  s32int i32;
  s64int i64;
  float r32;
  double r64;
  u32int index;                 /* Local/arg/field index */
  u32int token;                 /* Type/method token */
  fruity_basic_block_t *target; /* Branch target */
  fruity_switch_targets_t *switch_targets; /* Switch targets */
  tasklet_id_t tasklet;         /* For GRAPE opcode */
} fruity_operand_value_t;

typedef struct {
  fruity_operand_type_t type;
  fruity_operand_value_t value;
} fruity_operand_t;

/* ===== Instruction ===== */

struct fruity_instruction {
  fruity_opcode_t opcode;
  fruity_operand_t operand;

  /* Pebble effect annotations (for optimization) */
  struct {
    int creates_white;
    int burns_white;
    int may_free;
    int is_speculative;
  } pebble_effects;

  /* Debug information */
  u32int msil_offset;      /* Original MSIL offset */
  u32int source_line;      /* Source code line number */
  const char *source_file; /* Source file name */

  /* Intrusive doubly-linked list */
  fruity_instruction_t *next;
  fruity_instruction_t *prev;
};

/* ===== Basic Block (CFG Node) ===== */

struct fruity_basic_block {
  u32int block_id;
  u32int start_offset; /* IL offset where this block starts */

  /* Instructions (intrusive doubly-linked list) */
  fruity_instruction_t *instructions_head;
  fruity_instruction_t *instructions_tail;
  ulong instruction_count;

  /* Control Flow Graph edges */
  fruity_basic_block_t **successors;
  fruity_basic_block_t **predecessors;
  ulong successor_count;
  ulong predecessor_count;
  ulong successor_capacity;
  ulong predecessor_capacity;

  /* Dominance information (for SSA optimization) */
  fruity_basic_block_t *idom; /* Immediate dominator */
  fruity_basic_block_t **dominance_frontier;
  ulong df_count;
  ulong df_capacity;

  /* Dataflow analysis state */
  struct {
    ulong live_whites_in;  /* White tokens live on entry */
    ulong live_whites_out; /* White tokens live on exit */
    int has_snapshot;      /* Has CHERRY (Red snapshot)? */
    int in_transaction;    /* Inside CHERRY...BERRY block? */
  } pebble_state;

  /* Intrusive doubly-linked list */
  fruity_basic_block_t *next;
  fruity_basic_block_t *prev;
};

/* ===== Function ===== */

struct fruity_function {
  u32int method_token; /* ECMA-335 metadata token */
  char *name;          /* Function name */
  char *signature;     /* Type signature */

  /* Basic blocks (intrusive doubly-linked list) */
  fruity_basic_block_t *blocks_head;
  fruity_basic_block_t *blocks_tail;
  ulong block_count;

  fruity_basic_block_t *entry_block;
  fruity_basic_block_t *exit_block;

  /* Locals and arguments */
  ulong local_count;
  clr_value_type_t *local_types;

  ulong arg_count;
  clr_value_type_t *arg_types;
  clr_value_type_t return_type;

  /* Stack analysis */
  ulong max_stack_depth;

  /* Pebble metadata (computed during analysis) */
  struct {
    int uses_exchange;      /* Has GRAPE? */
    int is_transactional;   /* Has CHERRY/BERRY? */
    ulong max_white_tokens; /* Max white tokens alive */
    int has_loops;          /* Has back edges? */
  } pebble_metadata;

  /* Optimization state */
  struct {
    int in_ssa_form; /* Is in SSA form? */
    int optimized;   /* Has been optimized? */
  } opt_state;

  /* Intrusive doubly-linked list */
  fruity_function_t *next;
  fruity_function_t *prev;
};

/* ===== Module (Compilation Unit) ===== */

struct fruity_module {
  char *name;     /* Module/assembly name */
  u32int version; /* Version number */

  /* Functions (intrusive doubly-linked list) */
  fruity_function_t *functions_head;
  fruity_function_t *functions_tail;
  ulong function_count;

  /* Type information (ECMA-335 metadata) */
  void *metadata; /* Opaque metadata handle */

  /* Constants pool */
  struct {
    char **strings;
    ulong string_count;
    void *blob_data;
    ulong blob_size;
  } constants;

  /* Module-level statistics */
  struct {
    ulong total_instructions;
    ulong total_basic_blocks;
    ulong pebble_ops;      /* LIME/VANILLA/BURN count */
    ulong exchange_ops;    /* GRAPE count */
    ulong transaction_ops; /* CHERRY/BERRY count */
  } stats;
};

/* ===== IR Construction Functions ===== */

/* Module management */
fruity_module_t *fruity_module_create(const char *name);
void fruity_module_destroy(fruity_module_t *module);
fruity_function_t *fruity_module_add_function(fruity_module_t *module,
                                              const char *name,
                                              u32int method_token);
fruity_function_t *fruity_module_find_function(fruity_module_t *module,
                                               const char *name);

/* Function management */
fruity_function_t *fruity_function_create(const char *name,
                                          u32int method_token);
void fruity_function_destroy(fruity_function_t *func);
fruity_basic_block_t *fruity_function_add_block(fruity_function_t *func);
fruity_basic_block_t *fruity_function_find_block(fruity_function_t *func,
                                                 u32int block_id);

/* Basic block management */
fruity_basic_block_t *fruity_basic_block_create(u32int block_id);
void fruity_basic_block_destroy(fruity_basic_block_t *block);
void fruity_basic_block_add_instruction(fruity_basic_block_t *block,
                                        fruity_instruction_t *instr);
void fruity_basic_block_add_successor(fruity_basic_block_t *block,
                                      fruity_basic_block_t *successor);
void fruity_basic_block_add_predecessor(fruity_basic_block_t *block,
                                        fruity_basic_block_t *predecessor);

/* Instruction construction */
fruity_instruction_t *fruity_instruction_create(fruity_opcode_t opcode);
void fruity_instruction_destroy(fruity_instruction_t *instr);
void fruity_instruction_set_operand_i32(fruity_instruction_t *instr,
                                        s32int val);
void fruity_instruction_set_operand_i64(fruity_instruction_t *instr,
                                        s64int val);
void fruity_instruction_set_operand_local(fruity_instruction_t *instr,
                                          u32int idx);
void fruity_instruction_set_operand_branch(fruity_instruction_t *instr,
                                           fruity_basic_block_t *target);
void fruity_instruction_set_operand_tasklet(fruity_instruction_t *instr,
                                            tasklet_id_t tasklet);

/* ===== IR Analysis and Verification ===== */

/* CFG analysis */
int fruity_function_build_cfg(fruity_function_t *func);
int fruity_function_compute_dominators(fruity_function_t *func);
int fruity_function_compute_dominance_frontier(fruity_function_t *func);

/* Pebble analysis */
int fruity_function_analyze_white_tokens(fruity_function_t *func);
int fruity_function_verify_white_balance(fruity_function_t *func);
int fruity_function_verify_transaction_nesting(fruity_function_t *func);

/* Verification */
int fruity_module_verify(fruity_module_t *module);
int fruity_function_verify(fruity_function_t *func);
int fruity_basic_block_verify(fruity_basic_block_t *block);

/* ===== IR Traversal ===== */

/* Iterator callbacks */
typedef int (*fruity_instruction_visitor_t)(fruity_instruction_t *instr,
                                            void *ctx);
typedef int (*fruity_basic_block_visitor_t)(fruity_basic_block_t *block,
                                            void *ctx);
typedef int (*fruity_function_visitor_t)(fruity_function_t *func, void *ctx);

/* Traversal functions */
void fruity_module_foreach_function(fruity_module_t *module,
                                    fruity_function_visitor_t visitor,
                                    void *ctx);
void fruity_function_foreach_block(fruity_function_t *func,
                                   fruity_basic_block_visitor_t visitor,
                                   void *ctx);
void fruity_basic_block_foreach_instruction(
    fruity_basic_block_t *block, fruity_instruction_visitor_t visitor,
    void *ctx);

/* ===== IR Utilities ===== */

/* Statistics */
void fruity_module_compute_stats(fruity_module_t *module);
void fruity_module_print_stats(fruity_module_t *module);

/* Debugging */
void fruity_instruction_print(fruity_instruction_t *instr);
void fruity_basic_block_print(fruity_basic_block_t *block);
void fruity_function_print(fruity_function_t *func);
void fruity_module_print(fruity_module_t *module);

/* ===== CBOR Serialization ===== */

/* Encode Fruity IR module to CBOR format */
ulong fruity_module_to_cbor(fruity_module_t *module, u8int *buf, ulong bufsize,
                            char *errbuf, ulong errbuf_size);

/* Decode CBOR data into Fruity IR module */
fruity_module_t *fruity_module_from_cbor(u8int *data, ulong datalen,
                                         char *errbuf, ulong errbuf_size);

#endif /* FRUITY_IR_H */
