/* llvm_parser.c - LLVM Bitcode Parser
 *
 * Parses LLVM .bc (bitcode) files to extract modules, functions, and
 * instructions for translation to Fruity IR.
 *
 * Based on LLVM Bitcode Format: https://llvm.org/docs/BitCodeFormat.html
 */

#include "../include/portlib.h"
#include "../include/u.h"

/* LLVM Bitcode Magic */
#define LLVM_BITCODE_MAGIC 0x4243C0DE /* "BC" + 0xC0DE */

/* Block IDs */
#define BLOCKINFO_BLOCK_ID 0
#define MODULE_BLOCK_ID 8
#define PARAMATTR_BLOCK_ID 9
#define PARAMATTR_GROUP_BLOCK_ID 10
#define CONSTANTS_BLOCK_ID 11
#define FUNCTION_BLOCK_ID 12
#define VALUE_SYMTAB_BLOCK_ID 14
#define METADATA_BLOCK_ID 15
#define TYPE_BLOCK_ID 17

/* Abbreviation IDs */
#define END_BLOCK 0
#define ENTER_SUBBLOCK 1
#define DEFINE_ABBREV 2
#define UNABBREV_RECORD 3

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
  LLVM_INVOKE = 5,
  LLVM_UNREACHABLE = 7,
  LLVM_ADD = 8,
  LLVM_SUB = 10,
  LLVM_MUL = 12,
  LLVM_UDIV = 14,
  LLVM_SDIV = 15,
  LLVM_UREM = 17,
  LLVM_SREM = 18,
  LLVM_SHL = 20,
  LLVM_LSHR = 21,
  LLVM_ASHR = 22,
  LLVM_AND = 23,
  LLVM_OR = 24,
  LLVM_XOR = 25,
  LLVM_ALLOCA = 26,
  LLVM_LOAD = 27,
  LLVM_STORE = 28,
  LLVM_GEP = 29,
  LLVM_TRUNC = 33,
  LLVM_ZEXT = 34,
  LLVM_SEXT = 35,
  LLVM_FPTOUI = 36,
  LLVM_FPTOSI = 37,
  LLVM_UITOFP = 38,
  LLVM_SITOFP = 39,
  LLVM_PTRTOINT = 44,
  LLVM_INTTOPTR = 45,
  LLVM_BITCAST = 46,
  LLVM_ICMP = 53,
  LLVM_FCMP = 54,
  LLVM_PHI = 55,
  LLVM_CALL = 56,
  LLVM_SELECT = 57,
} llvm_opcode_t;

/* LLVM Value */
typedef struct llvm_value {
  u32int id;
  llvm_type_kind_t type;
  union {
    s64int int_val;
    double float_val;
    char *string_val;
    u32int ref_id; /* Reference to another value */
  } data;
  struct llvm_value *next;
} llvm_value_t;

/* LLVM Instruction */
typedef struct llvm_instruction {
  llvm_opcode_t opcode;
  u32int result_id; /* SSA value ID of result */
  u32int operand_count;
  u32int *operand_ids; /* SSA value IDs of operands */

  /* Extra info for specific instructions */
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

  struct llvm_instruction *next;
} llvm_instruction_t;

/* LLVM Basic Block */
typedef struct llvm_basic_block {
  u32int id;
  char *name;
  llvm_instruction_t *instructions_head;
  llvm_instruction_t *instructions_tail;
  u32int instruction_count;
  struct llvm_basic_block *next;
} llvm_basic_block_t;

/* LLVM Function */
typedef struct llvm_function {
  u32int id;
  char *name;
  u32int return_type;
  u32int param_count;
  u32int *param_types;
  llvm_basic_block_t *blocks_head;
  llvm_basic_block_t *blocks_tail;
  u32int block_count;
  u32int local_count; /* SSA value count */
  struct llvm_function *next;
} llvm_function_t;

/* LLVM Module */
typedef struct llvm_module {
  char *name;
  char *target_triple;
  char *data_layout;
  llvm_function_t *functions_head;
  llvm_function_t *functions_tail;
  u32int function_count;
  llvm_value_t *constants_head;
  u32int constant_count;
} llvm_module_t;

/* Bitstream Reader */
typedef struct {
  u8int *data;
  ulong size;
  ulong bit_pos;
} bitstream_t;

/* ========== BITSTREAM READING ========== */

static u32int read_bits(bitstream_t *bs, u32int n) {
  u32int result = 0;
  for (u32int i = 0; i < n; i++) {
    ulong byte_idx = bs->bit_pos / 8;
    u32int bit_idx = bs->bit_pos % 8;
    if (byte_idx < bs->size) {
      u32int bit = (bs->data[byte_idx] >> bit_idx) & 1;
      result |= (bit << i);
    }
    bs->bit_pos++;
  }
  return result;
}

static u64int read_vbr(bitstream_t *bs, u32int n) {
  /* Variable-bit-rate encoding */
  u64int result = 0;
  u32int shift = 0;
  u32int piece;

  do {
    piece = read_bits(bs, n);
    result |= ((u64int)(piece & ((1u << (n - 1)) - 1))) << shift;
    shift += n - 1;
  } while (piece & (1u << (n - 1)));

  return result;
}

static void align_32(bitstream_t *bs) {
  bs->bit_pos = (bs->bit_pos + 31) & ~31UL;
}

/* ========== MODULE CREATION ========== */

llvm_module_t *llvm_module_create(void) {
  llvm_module_t *mod = smalloc(sizeof(llvm_module_t));
  if (!mod)
    return nil;

  memset(mod, 0, sizeof(llvm_module_t));
  return mod;
}

void llvm_module_destroy(llvm_module_t *mod) {
  if (!mod)
    return;

  /* Free functions */
  llvm_function_t *func = mod->functions_head;
  while (func) {
    llvm_function_t *next = func->next;

    /* Free blocks */
    llvm_basic_block_t *bb = func->blocks_head;
    while (bb) {
      llvm_basic_block_t *bb_next = bb->next;

      /* Free instructions */
      llvm_instruction_t *instr = bb->instructions_head;
      while (instr) {
        llvm_instruction_t *instr_next = instr->next;
        if (instr->operand_ids)
          free(instr->operand_ids);
        free(instr);
        instr = instr_next;
      }

      if (bb->name)
        free(bb->name);
      free(bb);
      bb = bb_next;
    }

    if (func->name)
      free(func->name);
    if (func->param_types)
      free(func->param_types);
    free(func);
    func = next;
  }

  /* Free constants */
  llvm_value_t *val = mod->constants_head;
  while (val) {
    llvm_value_t *next = val->next;
    free(val);
    val = next;
  }

  if (mod->name)
    free(mod->name);
  if (mod->target_triple)
    free(mod->target_triple);
  if (mod->data_layout)
    free(mod->data_layout);
  free(mod);
}

/* ========== BITCODE PARSING ========== */

static int parse_module_block(bitstream_t *bs, llvm_module_t *mod);
static int parse_function_block(bitstream_t *bs, llvm_function_t *func);

int llvm_parse_bitcode(u8int *data, ulong size, llvm_module_t **out_module,
                       char *errbuf, ulong errbuf_size) {
  if (size < 4) {
    if (errbuf)
      snprint(errbuf, errbuf_size, "File too small");
    return -1;
  }

  /* Check magic */
  u32int magic = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  if (magic != LLVM_BITCODE_MAGIC) {
    if (errbuf)
      snprint(errbuf, errbuf_size, "Invalid LLVM bitcode magic");
    return -1;
  }

  bitstream_t bs = {
      .data = data, .size = size, .bit_pos = 32 /* Skip magic */
  };

  llvm_module_t *mod = llvm_module_create();
  if (!mod) {
    if (errbuf)
      snprint(errbuf, errbuf_size, "Out of memory");
    return -1;
  }

  /* Parse top-level blocks */
  while (bs.bit_pos < size * 8) {
    u32int code = read_bits(&bs, 2);

    if (code == ENTER_SUBBLOCK) {
      u32int block_id = read_vbr(&bs, 8);
      u32int new_abbrev_len = read_vbr(&bs, 4);
      align_32(&bs);
      u32int block_len = read_bits(&bs, 32); /* Block length in 32-bit words */

      if (block_id == MODULE_BLOCK_ID) {
        if (parse_module_block(&bs, mod) < 0) {
          llvm_module_destroy(mod);
          if (errbuf)
            snprint(errbuf, errbuf_size, "Failed to parse module");
          return -1;
        }
      } else {
        /* Skip unknown block */
        bs.bit_pos += block_len * 32;
      }
    } else if (code == END_BLOCK) {
      align_32(&bs);
      break;
    } else {
      /* Skip record */
      bs.bit_pos += 32;
    }
  }

  *out_module = mod;
  return 0;
}

static int parse_module_block(bitstream_t *bs, llvm_module_t *mod) {
  /* Simplified: just locate function blocks */
  /* Real implementation would parse type tables, value symbol table, etc. */

  while (1) {
    u32int code = read_bits(bs, 2);

    if (code == END_BLOCK) {
      align_32(bs);
      return 0;
    } else if (code == ENTER_SUBBLOCK) {
      u32int block_id = read_vbr(bs, 8);
      read_vbr(bs, 4); /* abbrev len */
      align_32(bs);
      u32int block_len = read_bits(bs, 32);

      if (block_id == FUNCTION_BLOCK_ID) {
        llvm_function_t *func = smalloc(sizeof(llvm_function_t));
        if (!func)
          return -1;
        memset(func, 0, sizeof(llvm_function_t));
        func->id = mod->function_count++;

        if (parse_function_block(bs, func) < 0) {
          free(func);
          return -1;
        }

        /* Add to module */
        if (mod->functions_tail) {
          mod->functions_tail->next = func;
        } else {
          mod->functions_head = func;
        }
        mod->functions_tail = func;
      } else {
        /* Skip block */
        bs->bit_pos += block_len * 32;
      }
    } else {
      /* Skip record */
      read_vbr(bs, 6); /* Record code */
      u32int num_ops = read_vbr(bs, 6);
      for (u32int i = 0; i < num_ops; i++) {
        read_vbr(bs, 6);
      }
    }
  }
}

static int parse_function_block(bitstream_t *bs, llvm_function_t *func) {
  /* Simplified function parsing */
  llvm_basic_block_t *current_bb = nil;

  while (1) {
    u32int code = read_bits(bs, 2);

    if (code == END_BLOCK) {
      align_32(bs);
      return 0;
    } else if (code == ENTER_SUBBLOCK) {
      /* Skip nested blocks */
      read_vbr(bs, 8);
      read_vbr(bs, 4);
      align_32(bs);
      u32int len = read_bits(bs, 32);
      bs->bit_pos += len * 32;
    } else {
      /* Parse instruction record */
      u32int rec_code = read_vbr(bs, 6);
      u32int num_ops = read_vbr(bs, 6);

      /* Create basic block if needed */
      if (!current_bb) {
        current_bb = smalloc(sizeof(llvm_basic_block_t));
        if (!current_bb)
          return -1;
        memset(current_bb, 0, sizeof(llvm_basic_block_t));
        current_bb->id = func->block_count++;

        if (func->blocks_tail) {
          func->blocks_tail->next = current_bb;
        } else {
          func->blocks_head = current_bb;
        }
        func->blocks_tail = current_bb;
      }

      /* Create instruction */
      llvm_instruction_t *instr = smalloc(sizeof(llvm_instruction_t));
      if (!instr)
        return -1;
      memset(instr, 0, sizeof(llvm_instruction_t));

      instr->opcode = rec_code;
      instr->result_id = func->local_count++;
      instr->operand_count = num_ops;

      if (num_ops > 0) {
        instr->operand_ids = smalloc(num_ops * sizeof(u32int));
        if (!instr->operand_ids) {
          free(instr);
          return -1;
        }
        for (u32int i = 0; i < num_ops; i++) {
          instr->operand_ids[i] = read_vbr(bs, 6);
        }
      }

      /* Add to block */
      if (current_bb->instructions_tail) {
        current_bb->instructions_tail->next = instr;
      } else {
        current_bb->instructions_head = instr;
      }
      current_bb->instructions_tail = instr;
      current_bb->instruction_count++;

      /* Terminators start new basic block */
      if (rec_code == LLVM_RET || rec_code == LLVM_BR ||
          rec_code == LLVM_SWITCH || rec_code == LLVM_UNREACHABLE) {
        current_bb = nil;
      }
    }
  }
}

/* ========== DEBUG OUTPUT ========== */

void llvm_module_dump(llvm_module_t *mod) {
  if (!mod)
    return;

  print("LLVM Module: %s\n", mod->name ? mod->name : "(unnamed)");
  print("  Functions: %ud\n", mod->function_count);

  for (llvm_function_t *func = mod->functions_head; func; func = func->next) {
    print("  Function %ud: %s\n", func->id,
          func->name ? func->name : "(unnamed)");
    print("    Blocks: %ud, Locals: %ud\n", func->block_count,
          func->local_count);

    for (llvm_basic_block_t *bb = func->blocks_head; bb; bb = bb->next) {
      print("    Block %ud: %ud instructions\n", bb->id, bb->instruction_count);
    }
  }
}
