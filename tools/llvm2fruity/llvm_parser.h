/* llvm_parser.h - LLVM Bitcode Parser Header */

#ifndef LLVM_PARSER_H
#define LLVM_PARSER_H

#include "../include/u.h"

/* LLVM Type Kinds */
typedef enum {
  LLVM_VOID_TYPE = 0,
  LLVM_FLOAT_TYPE = 2,
  LLVM_DOUBLE_TYPE = 3,
  LLVM_LABEL_TYPE = 5,
  LLVM_INTEGER_TYPE = 7,
  LLVM_FUNCTION_TYPE = 9,
  LLVM_STRUCT_TYPE = 10,
  LLVM_ARRAY_TYPE = 11,
  LLVM_POINTER_TYPE = 12,
  LLVM_VECTOR_TYPE = 13,
} llvm_type_kind_t;

/* LLVM Instruction Opcodes */
typedef enum {
  LLVM_RET = 1,
  LLVM_BR = 2,
  LLVM_SWITCH = 3,
  LLVM_ADD = 8,
  LLVM_SUB = 10,
  LLVM_MUL = 12,
  LLVM_UDIV = 14,
  LLVM_SDIV = 15,
  LLVM_ALLOCA = 26,
  LLVM_LOAD = 27,
  LLVM_STORE = 28,
  LLVM_ICMP = 53,
  LLVM_PHI = 55,
  LLVM_CALL = 56,
} llvm_opcode_t;

/* Forward declarations */
typedef struct llvm_value llvm_value_t;
typedef struct llvm_instruction llvm_instruction_t;
typedef struct llvm_basic_block llvm_basic_block_t;
typedef struct llvm_function llvm_function_t;
typedef struct llvm_module llvm_module_t;

/* LLVM Value */
struct llvm_value {
  u32int id;
  llvm_type_kind_t type;
  union {
    s64int int_val;
    double float_val;
    char *string_val;
    u32int ref_id;
  } data;
  llvm_value_t *next;
};

/* LLVM Instruction */
struct llvm_instruction {
  llvm_opcode_t opcode;
  u32int result_id;
  u32int operand_count;
  u32int *operand_ids;
  union {
    struct {
      u32int cmp_predicate;
    } cmp;
    struct {
      u32int callee_id;
    } call;
    struct {
      u32int true_bb;
      u32int false_bb;
    } branch;
  } extra;
  llvm_instruction_t *next;
};

/* LLVM Basic Block */
struct llvm_basic_block {
  u32int id;
  char *name;
  llvm_instruction_t *instructions_head;
  llvm_instruction_t *instructions_tail;
  u32int instruction_count;
  llvm_basic_block_t *next;
};

/* LLVM Function */
struct llvm_function {
  u32int id;
  char *name;
  u32int return_type;
  u32int param_count;
  u32int *param_types;
  llvm_basic_block_t *blocks_head;
  llvm_basic_block_t *blocks_tail;
  u32int block_count;
  u32int local_count;
  llvm_function_t *next;
};

/* LLVM Module */
struct llvm_module {
  char *name;
  char *target_triple;
  char *data_layout;
  llvm_function_t *functions_head;
  llvm_function_t *functions_tail;
  u32int function_count;
  llvm_value_t *constants_head;
  u32int constant_count;
};

/* ========== API ========== */

llvm_module_t *llvm_module_create(void);
void llvm_module_destroy(llvm_module_t *mod);

int llvm_parse_bitcode(u8int *data, ulong size, llvm_module_t **out_module,
                       char *errbuf, ulong errbuf_size);

void llvm_module_dump(llvm_module_t *mod);

#endif /* LLVM_PARSER_H */
