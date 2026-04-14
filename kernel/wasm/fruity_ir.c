/* fruity_ir.c - Fruity IR Implementation
 *
 * Core IR manipulation functions using intrusive linked lists.
 * All allocations use xalloc() from the kernel.
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include <u.h>

#ifdef __FRAMAC__
#include "kernel_acsl.h"
#endif

#include "fruity_ir.h"

#ifdef __FRAMAC__
#define FRUITY_XFREE(ptr) ((void)(ptr))
#else
#define FRUITY_XFREE(ptr) xfree(ptr)
#endif

#ifdef __FRAMAC__
#define FRUITY_MEMMOVE(dst, src, nbytes)                                       \
  ((void)(dst), (void)(src), (void)(nbytes))
#else
#define FRUITY_MEMMOVE(dst, src, nbytes) memmove((dst), (src), (nbytes))
#endif

#ifdef __FRAMAC__
/*@
  @ terminates \true;
  @ exits \false;
  @ allocates \result;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \valid(\result);
  @ ensures \result == \null ||
  @         \fresh(\result, sizeof(fruity_module_t));
  @*/
static fruity_module_t *fruity_module_alloc(void);
/*@
  @ terminates \true;
  @ exits \false;
  @ allocates \result;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \valid(\result);
  @ ensures \result == \null ||
  @         \fresh(\result, sizeof(fruity_function_t));
  @*/
static fruity_function_t *fruity_function_alloc(void);
/*@
  @ terminates \true;
  @ exits \false;
  @ allocates \result;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \valid(\result);
  @ ensures \result == \null ||
  @         \fresh(\result, sizeof(fruity_basic_block_t));
  @*/
static fruity_basic_block_t *fruity_basic_block_alloc(void);
/*@
  @ terminates \true;
  @ exits \false;
  @ allocates \result;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \valid(\result);
  @ ensures \result == \null ||
  @         \fresh(\result, sizeof(fruity_instruction_t));
  @*/
static fruity_instruction_t *fruity_instruction_alloc(void);
/*@
  @ terminates \true;
  @ exits \false;
  @ allocates \result;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \valid(\result + (0 .. count - 1));
  @ ensures \result == \null ||
  @         \fresh(\result,
  @                count * (integer)sizeof(fruity_basic_block_t*));
  @*/
static fruity_basic_block_t **fruity_block_ptrs_alloc(ulong count);
#else
static fruity_module_t *fruity_module_alloc(void) {
  return xallocz(sizeof(fruity_module_t), 1);
}
static fruity_function_t *fruity_function_alloc(void) {
  return xallocz(sizeof(fruity_function_t), 1);
}
static fruity_basic_block_t *fruity_basic_block_alloc(void) {
  return xallocz(sizeof(fruity_basic_block_t), 1);
}
static fruity_instruction_t *fruity_instruction_alloc(void) {
  return xallocz(sizeof(fruity_instruction_t), 1);
}
static fruity_basic_block_t **fruity_block_ptrs_alloc(ulong count) {
  return xalloc(count * sizeof(fruity_basic_block_t *));
}
#endif

static void fruity_module_init(fruity_module_t *module, char *name) {
  module->name = name;
  module->version = 1;
  module->functions_head = nil;
  module->functions_tail = nil;
  module->function_count = 0;
  module->metadata = nil;
  module->constants.strings = nil;
  module->constants.string_count = 0;
  module->constants.blob_data = nil;
  module->constants.blob_size = 0;
  module->stats.total_instructions = 0;
  module->stats.total_basic_blocks = 0;
  module->stats.pebble_ops = 0;
  module->stats.exchange_ops = 0;
  module->stats.transaction_ops = 0;
}

static void fruity_function_init(fruity_function_t *func, char *name,
                                 u32int method_token) {
  func->name = name;
  func->method_token = method_token;
  func->signature = nil;
  func->blocks_head = nil;
  func->blocks_tail = nil;
  func->block_count = 0;
  func->entry_block = nil;
  func->exit_block = nil;
  func->local_count = 0;
  func->local_types = nil;
  func->arg_count = 0;
  func->arg_types = nil;
  func->return_type = 0;
  func->max_stack_depth = 0;
  func->pebble_metadata.uses_exchange = 0;
  func->pebble_metadata.is_transactional = 0;
  func->pebble_metadata.max_white_tokens = 0;
  func->pebble_metadata.has_loops = 0;
  func->opt_state.in_ssa_form = 0;
  func->opt_state.optimized = 0;
  func->next = nil;
  func->prev = nil;
}

static void fruity_basic_block_init(fruity_basic_block_t *block,
                                    u32int block_id) {
  block->block_id = block_id;
  block->start_offset = 0;
  block->instructions_head = nil;
  block->instructions_tail = nil;
  block->instruction_count = 0;
  block->successors = nil;
  block->predecessors = nil;
  block->successor_count = 0;
  block->predecessor_count = 0;
  block->successor_capacity = 0;
  block->predecessor_capacity = 0;
  block->idom = nil;
  block->dominance_frontier = nil;
  block->df_count = 0;
  block->df_capacity = 0;
  block->pebble_state.live_whites_in = 0;
  block->pebble_state.live_whites_out = 0;
  block->pebble_state.has_snapshot = 0;
  block->pebble_state.in_transaction = 0;
  block->next = nil;
  block->prev = nil;
}

static void fruity_instruction_init(fruity_instruction_t *instr,
                                    fruity_opcode_t opcode) {
  instr->opcode = opcode;
  instr->operand.type = FRUITY_OP_NONE;
  instr->pebble_effects.creates_white = fruity_opcode_creates_white(opcode);
  instr->pebble_effects.burns_white = fruity_opcode_burns_white(opcode);
  instr->pebble_effects.may_free = fruity_opcode_may_free(opcode);
  instr->pebble_effects.is_speculative = (opcode == FRUITY_CHERRY);
  instr->next = nil;
  instr->prev = nil;
}

static void fruity_module_append_function(fruity_module_t *module,
                                          fruity_function_t *func) {
  func->next = nil;
  func->prev = module->functions_tail;

  if (module->functions_tail != nil)
    module->functions_tail->next = func;
  else
    module->functions_head = func;

  module->functions_tail = func;
  module->function_count++;
}

static void fruity_function_append_block(fruity_function_t *func,
                                         fruity_basic_block_t *block) {
  block->next = nil;
  block->prev = func->blocks_tail;

  if (func->blocks_tail != nil)
    func->blocks_tail->next = block;
  else
    func->blocks_head = block;

  func->blocks_tail = block;
  func->block_count++;

  if (func->entry_block == nil)
    func->entry_block = block;
}

static void fruity_block_append_instruction(fruity_basic_block_t *block,
                                            fruity_instruction_t *instr) {
  instr->next = nil;
  instr->prev = block->instructions_tail;

  if (block->instructions_tail != nil)
    block->instructions_tail->next = instr;
  else
    block->instructions_head = instr;

  block->instructions_tail = instr;
  block->instruction_count++;
}

static int fruity_block_edges_grow(fruity_basic_block_t ***arr, ulong *capacity,
                                   ulong count) {
  if (count < *capacity)
    return 1;

  ulong new_cap = *capacity == 0 ? 2 : (*capacity * 2);
  fruity_basic_block_t **new_arr = fruity_block_ptrs_alloc(new_cap);
  if (new_arr == nil)
    return 0;
  if (*arr != nil) {
    FRUITY_MEMMOVE(new_arr, *arr, count * sizeof(fruity_basic_block_t *));
    FRUITY_XFREE(*arr);
  }
  *arr = new_arr;
  *capacity = new_cap;
  return 1;
}

/* Opcode metadata table */
const fruity_opcode_metadata_t fruity_opcode_table[] = {
    /* Standard operations */
    {FRUITY_NOP, "nop", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {FRUITY_ADD, "add", 0, 0, 0, 0, 2, 1, 0, 0, 0, 0},
    {FRUITY_SUB, "sub", 0, 0, 0, 0, 2, 1, 0, 0, 0, 0},
    {FRUITY_MUL, "mul", 0, 0, 0, 0, 2, 1, 0, 0, 0, 0},

    /* Pebble operations */
    {FRUITY_LIME, "lime", 1, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {FRUITY_VANILLA, "vanilla", 1, 0, 0, 0, 1, 2, 0, 0, 0, 0},
    {FRUITY_BURN, "burn", 0, 1, 1, 0, 1, 0, 0, 0, 0, 0},
    {FRUITY_DUP, "dup", 0, 0, 0, 0, 1, 2, 0, 0, 0, 0},
    {FRUITY_POP, "pop", 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},

    /* Transactional */
    {FRUITY_CHERRY, "cherry", 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {FRUITY_BERRY, "berry", 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {FRUITY_ROLLBACK, "rollback", 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},

    /* IPC */
    {FRUITY_GRAPE, "grape", 0, 1, 0, 0, 2, 0, 0, 0, 0, 0},
    {FRUITY_LEMON, "lemon", 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},

    /* Control flow */
    {FRUITY_CALL, "call", 0, 0, 0, 0, -1, -1, 0, 1, 0, 0},
    {FRUITY_RET, "ret", 0, 0, 0, 0, -1, 0, 0, 0, 1, 1},
    {FRUITY_JUMP, "jump", 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {FRUITY_BEQ, "beq", 0, 0, 0, 0, 2, 0, 1, 0, 0, 1},
    {FRUITY_BNE, "bne", 0, 0, 0, 0, 2, 0, 1, 0, 0, 1},

    /* Stack/locals */
    {FRUITY_LOAD_LOCAL, "ldloc", 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {FRUITY_STORE_LOCAL, "stloc", 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},

    /* Bulk Memory */
    {FRUITY_MEMCPY, "memcpy", 0, 0, 0, 0, 3, 0, 0, 0, 0, 0},
    {FRUITY_MEMSET, "memset", 0, 0, 0, 0, 3, 0, 0, 0, 0, 0},
    {FRUITY_MEMINIT, "meminit", 0, 0, 0, 0, 3, 0, 0, 0, 0, 0},
    {FRUITY_DATADROP, "datadrop", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {FRUITY_TABLEINIT, "tableinit", 0, 0, 0, 0, 3, 0, 0, 0, 0, 0},
    {FRUITY_ELEMDROP, "elemdrop", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {FRUITY_TABLECOPY, "tablecopy", 0, 0, 0, 0, 3, 0, 0, 0, 0, 0},

    /* Terminator */
    {0, nil, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

enum {
  FRUITY_OPCODE_TABLE_LEN =
      sizeof(fruity_opcode_table) / sizeof(fruity_opcode_table[0]),
};

/* ===== Opcode Queries ===== */

const char *fruity_opcode_name(fruity_opcode_t opcode) {
  /*@ loop invariant 0 <= i <= FRUITY_OPCODE_TABLE_LEN;
    @ loop assigns i;
    @ loop variant FRUITY_OPCODE_TABLE_LEN - i;
    @*/
  for (int i = 0; i < FRUITY_OPCODE_TABLE_LEN; i++) {
    if (fruity_opcode_table[i].name == nil)
      break;
    if (fruity_opcode_table[i].opcode == opcode)
      return fruity_opcode_table[i].name;
  }
  return "unknown";
}

int fruity_opcode_creates_white(fruity_opcode_t opcode) {
  /*@ loop invariant 0 <= i <= FRUITY_OPCODE_TABLE_LEN;
    @ loop assigns i;
    @ loop variant FRUITY_OPCODE_TABLE_LEN - i;
    @*/
  for (int i = 0; i < FRUITY_OPCODE_TABLE_LEN; i++) {
    if (fruity_opcode_table[i].name == nil)
      break;
    if (fruity_opcode_table[i].opcode == opcode)
      return fruity_opcode_table[i].creates_white;
  }
  return 0;
}

int fruity_opcode_burns_white(fruity_opcode_t opcode) {
  /*@ loop invariant 0 <= i <= FRUITY_OPCODE_TABLE_LEN;
    @ loop assigns i;
    @ loop variant FRUITY_OPCODE_TABLE_LEN - i;
    @*/
  for (int i = 0; i < FRUITY_OPCODE_TABLE_LEN; i++) {
    if (fruity_opcode_table[i].name == nil)
      break;
    if (fruity_opcode_table[i].opcode == opcode)
      return fruity_opcode_table[i].burns_white;
  }
  return 0;
}

int fruity_opcode_may_free(fruity_opcode_t opcode) {
  /*@ loop invariant 0 <= i <= FRUITY_OPCODE_TABLE_LEN;
    @ loop assigns i;
    @ loop variant FRUITY_OPCODE_TABLE_LEN - i;
    @*/
  for (int i = 0; i < FRUITY_OPCODE_TABLE_LEN; i++) {
    if (fruity_opcode_table[i].name == nil)
      break;
    if (fruity_opcode_table[i].opcode == opcode)
      return fruity_opcode_table[i].may_free;
  }
  return 0;
}

int fruity_opcode_is_terminator(fruity_opcode_t opcode) {
  /*@ loop invariant 0 <= i <= FRUITY_OPCODE_TABLE_LEN;
    @ loop assigns i;
    @ loop variant FRUITY_OPCODE_TABLE_LEN - i;
    @*/
  for (int i = 0; i < FRUITY_OPCODE_TABLE_LEN; i++) {
    if (fruity_opcode_table[i].name == nil)
      break;
    if (fruity_opcode_table[i].opcode == opcode)
      return fruity_opcode_table[i].is_terminator;
  }
  return 0;
}

/* ===== Module Management ===== */

fruity_module_t *fruity_module_create(const char *name) {
  ulong name_len;
  fruity_module_t *module = fruity_module_alloc();
  if (module == nil)
    return nil;

  name_len = (ulong)strlen((char *)name);
  if (name_len >= ACSL_MAXSTR) {
    FRUITY_XFREE(module);
    return nil;
  }

  module->name = xalloc(name_len + 1);
  if (module->name == nil) {
    FRUITY_XFREE(module);
    return nil;
  }
  kstrcpy(module->name, (char *)name, (int)(name_len + 1));

  fruity_module_init(module, module->name);

  return module;
}

void fruity_module_destroy(fruity_module_t *module) {
  if (module == nil)
    return;

  /* Free all functions */
  fruity_function_t *func = module->functions_head;
  /*@ loop invariant fruity_function_chain_valid(func);
    @ loop assigns func;
    @*/
  while (func != nil) {
    fruity_function_t *next = func->next;
    fruity_function_destroy(func);
    func = next;
  }

  /* Free constants */
  if (module->constants.strings != nil) {
    /*@ loop invariant 0 <= i <= module->constants.string_count;
      @ loop invariant module->constants.strings != \null ==>
      @   \valid(module->constants.strings +
      @          (0 .. module->constants.string_count - 1));
      @ loop assigns i;
      @ loop variant module->constants.string_count - i;
      @*/
    for (ulong i = 0; i < module->constants.string_count; i++)
      FRUITY_XFREE(module->constants.strings[i]);
    FRUITY_XFREE(module->constants.strings);
  }
  if (module->constants.blob_data != nil)
    FRUITY_XFREE(module->constants.blob_data);

  FRUITY_XFREE(module->name);
  FRUITY_XFREE(module);
}

fruity_function_t *fruity_module_add_function(fruity_module_t *module,
                                              const char *name,
                                              u32int method_token) {
  if (module == nil)
    return nil;

  fruity_function_t *func = fruity_function_create(name, method_token);
  if (func == nil)
    return nil;

  fruity_module_append_function(module, func);

  return func;
}

fruity_function_t *fruity_module_find_function(fruity_module_t *module,
                                               const char *name) {
  if (module == nil || name == nil)
    return nil;

  /*@ loop invariant fruity_function_chain_named_valid(func);
    @ loop assigns func;
    @*/
  for (fruity_function_t *func = module->functions_head; func != nil;
       func = func->next) {
    if (strcmp(func->name, name) == 0)
      return func;
  }

  return nil;
}

/* ===== Function Management ===== */

fruity_function_t *fruity_function_create(const char *name,
                                          u32int method_token) {
  ulong name_len;
  fruity_function_t *func = fruity_function_alloc();
  if (func == nil)
    return nil;

  name_len = (ulong)strlen((char *)name);
  if (name_len >= ACSL_MAXSTR) {
    FRUITY_XFREE(func);
    return nil;
  }

  func->name = xalloc(name_len + 1);
  if (func->name == nil) {
    FRUITY_XFREE(func);
    return nil;
  }
  kstrcpy(func->name, (char *)name, (int)(name_len + 1));

  fruity_function_init(func, func->name, method_token);

  return func;
}

void fruity_function_destroy(fruity_function_t *func) {
  if (func == nil)
    return;

  /* Free all blocks */
  fruity_basic_block_t *block = func->blocks_head;
  /*@ loop invariant fruity_basic_block_chain_valid(block);
    @ loop assigns block;
    @*/
  while (block != nil) {
    fruity_basic_block_t *next = block->next;
    fruity_basic_block_destroy(block);
    block = next;
  }

  /* Free locals and args */
  if (func->local_types != nil)
    FRUITY_XFREE(func->local_types);
  if (func->arg_types != nil)
    FRUITY_XFREE(func->arg_types);

  FRUITY_XFREE(func->name);
  if (func->signature != nil)
    FRUITY_XFREE(func->signature);
  FRUITY_XFREE(func);
}

fruity_basic_block_t *fruity_function_add_block(fruity_function_t *func) {
  if (func == nil)
    return nil;

  u32int block_id = func->block_count;
  fruity_basic_block_t *block = fruity_basic_block_create(block_id);
  if (block == nil)
    return nil;

  fruity_function_append_block(func, block);

  return block;
}

fruity_basic_block_t *fruity_function_find_block(fruity_function_t *func,
                                                 u32int block_id) {
  if (func == nil)
    return nil;

  /*@ loop assigns block; */
  for (fruity_basic_block_t *block = func->blocks_head; block != nil;
       block = block->next) {
    if (block->block_id == block_id)
      return block;
  }

  return nil;
}

/* ===== Basic Block Management ===== */

fruity_basic_block_t *fruity_basic_block_create(u32int block_id) {
  fruity_basic_block_t *block = fruity_basic_block_alloc();
  if (block == nil)
    return nil;

  fruity_basic_block_init(block, block_id);

  return block;
}

void fruity_basic_block_destroy(fruity_basic_block_t *block) {
  if (block == nil)
    return;

  /* Free all instructions */
  fruity_instruction_t *instr = block->instructions_head;
  /*@ loop invariant fruity_instruction_chain_valid(instr);
    @ loop assigns instr;
    @*/
  while (instr != nil) {
    fruity_instruction_t *next = instr->next;
    fruity_instruction_destroy(instr);
    instr = next;
  }

  /* Free CFG arrays */
  if (block->successors != nil)
    FRUITY_XFREE(block->successors);
  if (block->predecessors != nil)
    FRUITY_XFREE(block->predecessors);
  if (block->dominance_frontier != nil)
    FRUITY_XFREE(block->dominance_frontier);

  FRUITY_XFREE(block);
}

void fruity_basic_block_add_instruction(fruity_basic_block_t *block,
                                        fruity_instruction_t *instr) {
  if (block == nil || instr == nil)
    return;

  fruity_block_append_instruction(block, instr);
}

void fruity_basic_block_add_successor(fruity_basic_block_t *block,
                                      fruity_basic_block_t *successor) {
  if (block == nil || successor == nil)
    return;

  if (!fruity_block_edges_grow(&block->successors, &block->successor_capacity,
                               block->successor_count))
    return;

#ifdef __FRAMAC__
  /*@ assert block->successors != \null; */
  /*@ assert block->successor_count < block->successor_capacity; */
  /*@ assert \valid(block->successors +
    @              (0 .. block->successor_capacity - 1));
    @*/
#endif
  block->successors[block->successor_count++] = successor;
}

void fruity_basic_block_add_predecessor(fruity_basic_block_t *block,
                                        fruity_basic_block_t *predecessor) {
  if (block == nil || predecessor == nil)
    return;

  if (!fruity_block_edges_grow(&block->predecessors,
                               &block->predecessor_capacity,
                               block->predecessor_count))
    return;

#ifdef __FRAMAC__
  /*@ assert block->predecessors != \null; */
  /*@ assert block->predecessor_count < block->predecessor_capacity; */
  /*@ assert \valid(block->predecessors +
    @              (0 .. block->predecessor_capacity - 1));
    @*/
#endif
  block->predecessors[block->predecessor_count++] = predecessor;
}

/* ===== Instruction Construction ===== */

fruity_instruction_t *fruity_instruction_create(fruity_opcode_t opcode) {
  fruity_instruction_t *instr = fruity_instruction_alloc();
  if (instr == nil)
    return nil;

  fruity_instruction_init(instr, opcode);

  return instr;
}

void fruity_instruction_destroy(fruity_instruction_t *instr) {
  if (instr == nil)
    return;
  FRUITY_XFREE(instr);
}

void fruity_instruction_set_operand_i32(fruity_instruction_t *instr,
                                        s32int val) {
  if (instr == nil)
    return;
  instr->operand.type = FRUITY_OP_IMM_I32;
  instr->operand.value.i32 = val;
}

void fruity_instruction_set_operand_i64(fruity_instruction_t *instr,
                                        s64int val) {
  if (instr == nil)
    return;
  instr->operand.type = FRUITY_OP_IMM_I64;
  instr->operand.value.i64 = val;
}

void fruity_instruction_set_operand_local(fruity_instruction_t *instr,
                                          u32int idx) {
  if (instr == nil)
    return;
  instr->operand.type = FRUITY_OP_LOCAL;
  instr->operand.value.index = idx;
}

void fruity_instruction_set_operand_branch(fruity_instruction_t *instr,
                                           fruity_basic_block_t *target) {
  if (instr == nil)
    return;
  instr->operand.type = FRUITY_OP_BRANCH;
  instr->operand.value.target = target;
}

void fruity_instruction_set_operand_tasklet(fruity_instruction_t *instr,
                                            tasklet_id_t tasklet) {
  if (instr == nil)
    return;
  instr->operand.type = FRUITY_OP_TASKLET;
  instr->operand.value.tasklet = tasklet;
}

/* ===== Verification Stubs (to be implemented) ===== */

int fruity_module_verify(fruity_module_t *module) {
  ulong count = 0;
  if (module == nil || module->name == nil)
    return -1;

  for (fruity_function_t *func = module->functions_head; func != nil;
       func = func->next) {
    count++;
    if (fruity_function_verify(func) != 0)
      return -1;
  }

  if (count != module->function_count)
    return -1;

  return 0;
}

int fruity_function_verify(fruity_function_t *func) {
  ulong count = 0;
  int saw_entry = 0;

  if (func == nil || func->name == nil)
    return -1;

  for (fruity_basic_block_t *block = func->blocks_head; block != nil;
       block = block->next) {
    count++;
    if (block == func->entry_block)
      saw_entry = 1;
    if (fruity_basic_block_verify(block) != 0)
      return -1;
  }

  if (count != func->block_count)
    return -1;
  if (func->block_count > 0 && !saw_entry)
    return -1;

  return 0;
}

int fruity_basic_block_verify(fruity_basic_block_t *block) {
  ulong count = 0;
  fruity_instruction_t *prev = nil;

  if (block == nil)
    return -1;

  for (fruity_instruction_t *instr = block->instructions_head; instr != nil;
       instr = instr->next) {
    count++;
    if (instr->prev != prev)
      return -1;

    switch (instr->opcode) {
    case FRUITY_JUMP:
    case FRUITY_BEQ:
    case FRUITY_BNE:
    case FRUITY_BLT:
    case FRUITY_BLE:
    case FRUITY_BGT:
    case FRUITY_BGE:
    case FRUITY_BTRUE:
    case FRUITY_BFALSE:
      if (instr->operand.type != FRUITY_OP_BRANCH ||
          instr->operand.value.target == nil)
        return -1;
      break;
    case FRUITY_SWITCH:
      if (instr->operand.type != FRUITY_OP_SWITCH ||
          instr->operand.value.switch_targets == nil ||
          instr->operand.value.switch_targets->count == 0 ||
          instr->operand.value.switch_targets->targets == nil)
        return -1;
      break;
    default:
      break;
    }

    if (fruity_opcode_is_terminator(instr->opcode) && instr->next != nil)
      return -1;

    prev = instr;
  }

  if (count != block->instruction_count)
    return -1;
  if (block->instruction_count == 0) {
    if (block->instructions_head != nil || block->instructions_tail != nil)
      return -1;
  } else {
    if (block->instructions_head == nil || block->instructions_tail == nil)
      return -1;
    if (block->instructions_tail != prev)
      return -1;
  }

  /*
   * TODO: Implement full abstract interpretation for stack verification.
   *
   * Note for Multi-Value Support:
   * Opcodes with stack_pop/push == -1 (CALL, RET) have variable stack effects
   * determined by their signature. The verifier must look up the function
   * signature to determine the exact number of values popped/pushed.
   */

  return 0;
}

/* ===== Traversal ===== */

void fruity_module_foreach_function(fruity_module_t *module,
                                    fruity_function_visitor_t visitor,
                                    void *ctx) {
  if (module == nil || visitor == nil)
    return;

#ifndef __FRAMAC__
  for (fruity_function_t *func = module->functions_head; func != nil;
       func = func->next) {
    if (visitor(func, ctx) != 0)
      break;
  }
#else
  (void)ctx;
#endif
}

void fruity_function_foreach_block(fruity_function_t *func,
                                   fruity_basic_block_visitor_t visitor,
                                   void *ctx) {
  if (func == nil || visitor == nil)
    return;

#ifndef __FRAMAC__
  for (fruity_basic_block_t *block = func->blocks_head; block != nil;
       block = block->next) {
    if (visitor(block, ctx) != 0)
      break;
  }
#else
  (void)ctx;
#endif
}

void fruity_basic_block_foreach_instruction(
    fruity_basic_block_t *block, fruity_instruction_visitor_t visitor,
    void *ctx) {
  if (block == nil || visitor == nil)
    return;

#ifndef __FRAMAC__
  for (fruity_instruction_t *instr = block->instructions_head; instr != nil;
       instr = instr->next) {
    if (visitor(instr, ctx) != 0)
      break;
  }
#else
  (void)ctx;
#endif
}

/* ===== Debugging ===== */

void fruity_instruction_print(fruity_instruction_t *instr) {
  if (instr == nil)
    return;

  print("  %s", fruity_opcode_name(instr->opcode));

  switch (instr->operand.type) {
  case FRUITY_OP_IMM_I32:
    print(" %d", instr->operand.value.i32);
    break;
  case FRUITY_OP_IMM_I64:
    print(" %lld", instr->operand.value.i64);
    break;
  case FRUITY_OP_LOCAL:
    print(" local.%ud", instr->operand.value.index);
    break;
  case FRUITY_OP_BRANCH:
    if (instr->operand.value.target != nil)
      print(" bb.%ud", instr->operand.value.target->block_id);
    else
      print(" bb.<nil>");
    break;
  default:
    break;
  }

  print("\n");
}

void fruity_basic_block_print(fruity_basic_block_t *block) {
  if (block == nil)
    return;

  print("bb.%ud:\n", block->block_id);

  for (fruity_instruction_t *instr = block->instructions_head; instr != nil;
       instr = instr->next) {
    fruity_instruction_print(instr);
  }
}

void fruity_function_print(fruity_function_t *func) {
  if (func == nil)
    return;

  print("function %s:\n", func->name);

  for (fruity_basic_block_t *block = func->blocks_head; block != nil;
       block = block->next) {
    fruity_basic_block_print(block);
  }
}

void fruity_module_print(fruity_module_t *module) {
  if (module == nil)
    return;

  print("module %s (version %ud):\n", module->name, module->version);
  print("  functions: %lud\n", module->function_count);

  for (fruity_function_t *func = module->functions_head; func != nil;
       func = func->next) {
    print("\n");
    fruity_function_print(func);
  }
}
