/* fruity_to_qbe.c - Fruity IR to QBE IL Translator
 *
 * Simplified translator: just emit QBE stubs for now
 */

#ifdef USERSPACE_TEST
#include "fruity_to_qbe.h"
#include "qbe_buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mocks for userspace */
#define snprint snprintf
#define nil NULL
typedef unsigned long ulong;
typedef uint32_t u32int;
typedef unsigned long usize;
typedef unsigned long uintptr;

int qbe_buffer_init(QBEBuffer *buf);
int qbe_buffer_printf(QBEBuffer *buf, const char *fmt, ...);
void qbe_buffer_free(QBEBuffer *buf);
size_t qbe_buffer_len(QBEBuffer *buf);
char *qbe_buffer_data(QBEBuffer *buf);

#define KADDR(x) ((void *)(uintptr)(x))

#else
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/mem.h"
#include "../../include/portlib.h"
#include "../../include/u.h"

#include "../../include/blind_ledger.h"
#include "../../include/exchange.h"
#include "../../include/hhdm.h"
#include "../il_parser.h"
#include "fruity_to_qbe.h"
#include "qbe_buffer.h"
#endif

/* Forward declaration for qbe_buffer if not included */
#ifndef QBE_BUFFER_H
#ifdef USERSPACE_TEST
typedef struct {
  char *data;
  size_t size;
  size_t capacity;
} QBEBuffer;
#endif
#endif

#define Q_EMIT(buf, ...)                                                       \
  do {                                                                         \
    if (qbe_buffer_printf(buf, __VA_ARGS__) < 0)                               \
      return -1;                                                               \
  } while (0)

static ulong label_uniq = 0;

/* Get element size from instruction operand */
extern ulong clr_get_type_size(u32int token);

static int get_element_size_from_instr(fruity_instruction_t *instr) {
  /* Check operand type */
  if (instr->operand.type == FRUITY_OP_TYPE) {
    /* Has type token - resolve actual size */
    u32int elem_type = instr->operand.value.token;
    ulong size = clr_get_type_size(elem_type);
    if (size > 64)
      return 8; /* Large structs use references */
    if (size == 0)
      return 8; /* Default to pointer size */
    return (int)size;
  } else if (instr->operand.type == FRUITY_OP_IMM_I32) {
    /* Typed variant - infer from IL opcode */
    u32int opcode = instr->operand.value.i32;
    /* IL opcodes: LDELEM_I1=0x90, LDELEM_I2=0x92, LDELEM_I4=0x94, etc. */
    switch (opcode) {
    case 0x90: /* IL_LDELEM_I1 */
    case 0x91: /* IL_STELEM_I1 */
      return 1;
    case 0x92: /* IL_LDELEM_I2 */
    case 0x93: /* IL_STELEM_I2 */
      return 2;
    case 0x94: /* IL_LDELEM_I4 */
    case 0x95: /* IL_STELEM_I4 */
    case 0x96: /* IL_LDELEM_R4 */
    case 0x9C: /* IL_STELEM_R4 */
      return 4;
    case 0x97: /* IL_LDELEM_I8 */
    case 0x98: /* IL_STELEM_I8 */
    case 0x99: /* IL_LDELEM_R8 */
    case 0x9D: /* IL_STELEM_R8 */
    case 0x9A: /* IL_LDELEM_I */
    case 0x9B: /* IL_LDELEM_REF */
    case 0x9E: /* IL_STELEM_I */
    case 0x9F: /* IL_STELEM_REF */
      return 8;
    default:
      return 4; /* Default fallback */
    }
  }
  return 4; /* Final fallback */
}

/* Emit static field usage as global data */
static int emit_static_fields(QBEBuffer *buf, il_assembly_t *assembly) {
  /* Iterate Field table */
  size_t count = assembly->tables_header.row_counts[TABLE_FIELD];
  extern int print(char *, ...);
  print("DEBUG: emit_static_fields count=%d\n", (int)count);

  for (uint32_t i = 1; i <= count; i++) {
    field_row_t *row = il_get_field(assembly, i);
    if (!row) {
      print("DEBUG: il_get_field(%d) returned NULL\n", i);
      continue;
    }

    /* Dump first field raw flags for verification */
    if (i == 1) {
      print("DEBUG: Field[1] flags=%04x\n", row->flags);
    }

    /* fdStatic = 0x0010 */
    if (row->flags & 0x0010) {
      uint32_t token = (TABLE_FIELD << 24) | i;
      print("DEBUG: Emitting static token %08x (flags=%04x)\n", token,
            row->flags);
      Q_EMIT(buf, "data $static_%u = { z 8 }\n", token);
    } else {
      /* Print non-static fields too for now to debug */
      print("DEBUG: Field[%d] skipped (flags=%04x)\n", i, row->flags);
    }
    xfree(row);
  }
  return 0;
}

/* Emit QBE IL header */
static int emit_header(QBEBuffer *buf) {
  extern void uartputs(char *, int);
  Q_EMIT(buf, "# QBE IL generated from Fruity IR\n\n");
  /*
   * NOTE: We intentionally do NOT emit lux_alloc, lux_token_mint,
   * lux_token_burn stubs here. The JIT compiles all functions in order, and the
   * entry point (main) must be at offset 0 so that calling the code page
   * directly works. Runtime functions are resolved via resolve_kernel_symbol
   * instead.
   */
  return 0;
}

/* Emit a function as QBE IL */
static int emit_function(QBEBuffer *buf, fruity_function_t *func,
                         fruity_module_t *module) {
  fruity_basic_block_t *bb;
  fruity_instruction_t *instr;
  int tmp_counter = 0;
  extern int print(char *, ...);

  print("DEBUG: emit_function func=%p name=%s blocks_head=%p\n", func,
        func->name ? func->name : "(null)", func->blocks_head);

  /* Emit function */
  Q_EMIT(buf, "export function w $%s(", func->name ? func->name : "unnamed");
  for (ulong i = 0; i < func->arg_count; i++) {
    Q_EMIT(buf, "%s %%arg%lu", i == 0 ? "l" : ", l", i);
  }
  Q_EMIT(buf, ") {\n");

  /* Allocate storage for locals (simplified: all 8 bytes) */
  for (ulong i = 0; i < func->local_count; i++) {
    Q_EMIT(buf, "    %%loc%lu =l alloc8 8\n", i);
  }

  Q_EMIT(buf, "@start\n");

  for (bb = func->blocks_head; bb != nil; bb = bb->next) {
    if (bb != func->blocks_head)
      Q_EMIT(buf, "@bb%d\n", bb->block_id);

    for (instr = bb->instructions_head; instr != nil; instr = instr->next) {
      /* Translate each Fruity opcode to QBE */
      switch (instr->opcode) {
      case FRUITY_LDC_I4:
        Q_EMIT(buf, "    %%t%d =w copy %d\n", ++tmp_counter,
               (int)instr->operand.value.i64);
        break;

      case FRUITY_ADD:
        Q_EMIT(buf, "    %%t%d =w add %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_SUB:
        Q_EMIT(buf, "    %%t%d =w sub %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_MUL:
        Q_EMIT(buf, "    %%t%d =w mul %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_DIV:
        Q_EMIT(buf, "    %%t%d =w div %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_REM:
        Q_EMIT(buf, "    %%t%d =w rem %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_NEG:
        Q_EMIT(buf, "    %%t%d =w sub 0, %%t%d\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_DIV_UN:
        Q_EMIT(buf, "    %%t%d =w udiv %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_REM_UN:
        Q_EMIT(buf, "    %%t%d =w urem %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      /* Overflow arithmetic */
      case FRUITY_ADD_OVF: {
        int r = tmp_counter;
        int l = tmp_counter - 1;
        int res = tmp_counter - 1; /* Result overwrites left */
        int uniq = ++label_uniq;

        Q_EMIT(buf, "    %%t%d =w add %%t%d, %%t%d\n", res, l, r);

        /* Check: (~(l ^ r) & (l ^ res)) < 0 */
        int t_xor_lr = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w xor %%t%d, %%t%d\n", t_xor_lr, l, r);

        int t_not_xor_lr = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w xor %%t%d, -1\n", t_not_xor_lr, t_xor_lr);

        int t_xor_lres = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w xor %%t%d, %%t%d\n", t_xor_lres, l, res);

        int t_and = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w and %%t%d, %%t%d\n", t_and, t_not_xor_lr,
               t_xor_lres);

        int t_cond = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w csltw %%t%d, 0\n", t_cond, t_and);

        Q_EMIT(buf, "    jnz %%t%d, @ovf_fail_%ld, @ovf_ok_%ld\n", t_cond, uniq,
               uniq);
        Q_EMIT(buf, "@ovf_fail_%ld\n", uniq);
        Q_EMIT(buf, "    call $System_Environment_FailFast(l 0)\n");
        Q_EMIT(buf, "@ovf_ok_%ld\n", uniq);

        tmp_counter = res; /* Reset stack to result */
        break;
      }

      case FRUITY_ADD_OVF_UN: {
        int r = tmp_counter;
        int l = tmp_counter - 1;
        int res = tmp_counter - 1;
        int uniq = ++label_uniq;

        Q_EMIT(buf, "    %%t%d =w add %%t%d, %%t%d\n", res, l, r);

        /* Check: res < l (unsigned) */
        int t_cond = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w cultw %%t%d, %%t%d\n", t_cond, res, l);

        Q_EMIT(buf, "    jnz %%t%d, @ovf_fail_%ld, @ovf_ok_%ld\n", t_cond, uniq,
               uniq);
        Q_EMIT(buf, "@ovf_fail_%ld\n", uniq);
        Q_EMIT(buf, "    call $System_Environment_FailFast(l 0)\n");
        Q_EMIT(buf, "@ovf_ok_%ld\n", uniq);

        tmp_counter = res;
        break;
      }

      case FRUITY_SUB_OVF: {
        int r = tmp_counter;
        int l = tmp_counter - 1;
        int res = tmp_counter - 1;
        int uniq = ++label_uniq;

        Q_EMIT(buf, "    %%t%d =w sub %%t%d, %%t%d\n", res, l, r);

        /* Check: ((l ^ r) & (l ^ res)) < 0 */
        int t_xor_lr = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w xor %%t%d, %%t%d\n", t_xor_lr, l, r);

        int t_xor_lres = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w xor %%t%d, %%t%d\n", t_xor_lres, l, res);

        int t_and = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w and %%t%d, %%t%d\n", t_and, t_xor_lr,
               t_xor_lres);

        int t_cond = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w csltw %%t%d, 0\n", t_cond, t_and);

        Q_EMIT(buf, "    jnz %%t%d, @ovf_fail_%ld, @ovf_ok_%ld\n", t_cond, uniq,
               uniq);
        Q_EMIT(buf, "@ovf_fail_%ld\n", uniq);
        Q_EMIT(buf, "    call $System_Environment_FailFast(l 0)\n");
        Q_EMIT(buf, "@ovf_ok_%ld\n", uniq);

        tmp_counter = res;
        break;
      }

      case FRUITY_SUB_OVF_UN: {
        int r = tmp_counter;
        int l = tmp_counter - 1;
        int res = tmp_counter - 1;
        int uniq = ++label_uniq;

        /* Check BEFORE sub: l < r (unsigned) -> overflow if borrowing */
        int t_cond = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =w cultw %%t%d, %%t%d\n", t_cond, l, r);

        Q_EMIT(buf, "    jnz %%t%d, @ovf_fail_%ld, @ovf_sub_%ld\n", t_cond,
               uniq, uniq);
        Q_EMIT(buf, "@ovf_fail_%ld\n", uniq);
        Q_EMIT(buf, "    call $System_Environment_FailFast(l 0)\n");
        Q_EMIT(buf, "@ovf_sub_%ld\n", uniq);

        Q_EMIT(buf, "    %%t%d =w sub %%t%d, %%t%d\n", res, l, r);

        Q_EMIT(buf, "@ovf_ok_%ld\n", uniq);

        tmp_counter = res;
        break;
      }

      case FRUITY_MUL_OVF: {
        /* Signed Mul: widen to 64-bit, check bounds */
        int r = tmp_counter;
        int l = tmp_counter - 1;
        int res = tmp_counter - 1;
        int uniq = ++label_uniq;

        int l64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l extsw %%t%d\n", l64, l);

        int r64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l extsw %%t%d\n", r64, r);

        int res64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l mul %%t%d, %%t%d\n", res64, l64, r64);

        /* Truncate to 32 */
        Q_EMIT(buf, "    %%t%d =w copy %%t%d\n", res, res64);

        /* Sign extend back to 64 to check */
        int check64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l extsw %%t%d\n", check64, res);

        int t_eq = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l ceql %%t%d, %%t%d\n", t_eq, res64, check64);

        /* if equal, ok (jump to ok), else fail */
        Q_EMIT(buf, "    jnz %%t%d, @ovf_ok_%ld, @ovf_fail_%ld\n", t_eq, uniq,
               uniq);
        Q_EMIT(buf, "@ovf_fail_%ld\n", uniq);
        Q_EMIT(buf, "    call $System_Environment_FailFast(l 0)\n");
        Q_EMIT(buf, "@ovf_ok_%ld\n", uniq);

        tmp_counter = res;
        break;
      }

      case FRUITY_MUL_OVF_UN: {
        /* Unsigned Mul: widen, check bounds */
        int r = tmp_counter;
        int l = tmp_counter - 1;
        int res = tmp_counter - 1;
        int uniq = ++label_uniq;

        int l64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l extuw %%t%d\n", l64, l);

        int r64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l extuw %%t%d\n", r64, r);

        int res64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l mul %%t%d, %%t%d\n", res64, l64, r64);

        /* Truncate to 32 */
        Q_EMIT(buf, "    %%t%d =w copy %%t%d\n", res, res64);

        /* Zero extend back to 64 to check */
        int check64 = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l extuw %%t%d\n", check64, res);

        int t_eq = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l ceql %%t%d, %%t%d\n", t_eq, res64, check64);

        /* if equal, ok */
        Q_EMIT(buf, "    jnz %%t%d, @ovf_ok_%ld, @ovf_fail_%ld\n", t_eq, uniq,
               uniq);
        Q_EMIT(buf, "@ovf_fail_%ld\n", uniq);
        Q_EMIT(buf, "    call $System_Environment_FailFast(l 0)\n");
        Q_EMIT(buf, "@ovf_ok_%ld\n", uniq);

        tmp_counter = res;
        break;
      }

      /* Bitwise operations */
      case FRUITY_AND:
        Q_EMIT(buf, "    %%t%d =w and %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_OR:
        Q_EMIT(buf, "    %%t%d =w or %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_XOR:
        Q_EMIT(buf, "    %%t%d =w xor %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_NOT:
        Q_EMIT(buf, "    %%t%d =w xor %%t%d, -1\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_SHL:
        Q_EMIT(buf, "    %%t%d =w shl %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_SHR:
        Q_EMIT(buf, "    %%t%d =w sar %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_SHR_UN:
        Q_EMIT(buf, "    %%t%d =w shr %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      /* Comparison operations */
      case FRUITY_CEQ:
        Q_EMIT(buf, "    %%t%d =w ceqw %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_CNE:
        Q_EMIT(buf, "    %%t%d =w cnew %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_CLT:
        Q_EMIT(buf, "    %%t%d =w csltw %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_CLE:
        Q_EMIT(buf, "    %%t%d =w cslew %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_CGT:
        Q_EMIT(buf, "    %%t%d =w csgtw %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_CGE:
        Q_EMIT(buf, "    %%t%d =w csgew %%t%d, %%t%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      /* Stack operations */
      case FRUITY_DUP:
        Q_EMIT(buf, "    %%t%d =w copy %%t%d\n", tmp_counter + 1, tmp_counter);
        tmp_counter++;
        break;

      case FRUITY_DUP_REF:
        /* Duplicate reference: copy pointer + addref via white token */
        Q_EMIT(buf, "    %%t%d =l copy %%t%d\n", tmp_counter + 1, tmp_counter);
        Q_EMIT(buf, "    call $clr_object_addref(l %%t%d)\n", tmp_counter + 1);
        tmp_counter++;
        break;

      case FRUITY_POP:
        tmp_counter--; /* Just decrement stack pointer */
        break;

      case FRUITY_POP_REF:
        /* Pop reference: release via white token burn */
        Q_EMIT(buf, "    call $clr_object_release(l %%t%d)\n", tmp_counter);
        tmp_counter--;
        break;

      /* Additional constants */
      case FRUITY_LDC_I8:
        Q_EMIT(buf, "    %%t%d =l copy %lld\n", ++tmp_counter,
               (long long)instr->operand.value.i64);
        break;

      /* Commented out R4/R8 to avoid SSE errors on kernel build */
      /*case FRUITY_LDC_R4:
        Q_EMIT(buf, "    %%t%d =s copy s_%f\n", ++tmp_counter,
               instr->operand.value.r32);
        break;

      case FRUITY_LDC_R8:
        Q_EMIT(buf, "    %%t%d =d copy d_%f\n", ++tmp_counter,
               instr->operand.value.r64);
        break; */

      /* Type conversions */
      case FRUITY_CONV_I4:
        Q_EMIT(buf, "    %%t%d =w extsw %%t%d\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_CONV_I8:
        Q_EMIT(buf, "    %%t%d =l extsw %%t%d\n", tmp_counter, tmp_counter);
        break;

      /* Commented out R4/R8 conversions to avoid SSE errors */
      /*case FRUITY_CONV_R4:
        Q_EMIT(buf, "    %%t%d =s swtof %%t%d\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_CONV_R8:
        Q_EMIT(buf, "    %%t%d =d swtof %%t%d\n", tmp_counter, tmp_counter);
        break; */

      /* Local/Arg address operations */
      case FRUITY_LOAD_LOCAL_ADDR:
        Q_EMIT(buf, "    %%t%d =l copy %%loc%lu\n", ++tmp_counter,
               (unsigned long)instr->operand.value.index);
        break;

      case FRUITY_LOAD_ARG_ADDR:
        Q_EMIT(buf, "    %%t%d =l copy %%arg%lu\n", ++tmp_counter,
               (unsigned long)instr->operand.value.index);
        break;

      case FRUITY_STORE_ARG:
        Q_EMIT(buf, "    storel %%arg%lu, %%t%d\n",
               (unsigned long)instr->operand.value.index, tmp_counter--);
        break;

      /* Field/Memory operations */
      case FRUITY_LOAD_FIELD:
        /* obj_ref on stack, add field offset and load */
        Q_EMIT(buf, "    %%addr%d =l add %%t%dL, %d\n", tmp_counter,
               tmp_counter, instr->operand.value.i32);
        Q_EMIT(buf, "    %%t%d =w loadw %%addr%d\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_STORE_FIELD:
        /* obj_ref, value on stack */
        Q_EMIT(buf, "    %%addr%d =l add %%t%d, %d\n", tmp_counter - 1,
               tmp_counter - 1, instr->operand.value.i32);
        Q_EMIT(buf, "    storew %%addr%d, %%t%d\n", tmp_counter - 1,
               tmp_counter);
        tmp_counter -= 2;
        break;

      case FRUITY_LOAD_STATIC:
        /* Load from static field (global address) */
        Q_EMIT(buf, "    %%t%d =w loadw $static_%u\n", ++tmp_counter,
               instr->operand.value.token);
        break;

      case FRUITY_STORE_STATIC:
        Q_EMIT(buf, "    storew $static_%u, %%t%d\n",
               instr->operand.value.token, tmp_counter--);
        break;

      case FRUITY_LOAD_IND:
        Q_EMIT(buf, "    %%t%d =w loadw %%t%d\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_STORE_IND:
        Q_EMIT(buf, "    storew %%t%d, %%t%d\n", tmp_counter - 1, tmp_counter);
        tmp_counter -= 2;
        break;

      case FRUITY_LDFLDA:
        /* Load field address */
        Q_EMIT(buf, "    %%t%d =l add %%t%d, %d\n", tmp_counter, tmp_counter,
               instr->operand.value.i32);
        break;

      case FRUITY_MEMCPY:
        /* dest, src, size on stack */
        Q_EMIT(buf, "    call $memcpy(l %%t%d, l %%t%d, l %%t%d)\n",
               tmp_counter - 2, tmp_counter - 1, tmp_counter);
        tmp_counter -= 3;
        break;

      case FRUITY_MEMSET:
        /* dest, val, size on stack */
        Q_EMIT(buf, "    call $memset(l %%t%d, w %%t%d, l %%t%d)\n",
               tmp_counter - 2, tmp_counter - 1, tmp_counter);
        tmp_counter -= 3;
        break;

      /* Object model operations */
      case FRUITY_CASTCLASS:
        Q_EMIT(buf, "    %%t%d =l call $lux_castclass(l %%t%d, w %u)\n",
               tmp_counter, tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_ISINST:
        Q_EMIT(buf, "    %%t%d =l call $lux_isinst(l %%t%d, w %u)\n",
               tmp_counter, tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_BOX:
        Q_EMIT(buf, "    %%t%d =l call $lux_box(w %%t%d, w %u)\n", tmp_counter,
               tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_UNBOX:
        Q_EMIT(buf, "    %%t%d =l call $lux_unbox(l %%t%d, w %u)\n",
               tmp_counter, tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_UNBOX_ANY:
        Q_EMIT(buf, "    %%t%d =w call $lux_unbox_any(l %%t%d, w %u)\n",
               tmp_counter, tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_INITOBJ:
        Q_EMIT(buf, "    call $memset(l %%t%d, w 0, w %u)\n", tmp_counter--,
               instr->operand.value.token);
        break;

      case FRUITY_CPOBJ:
        /* dest, src on stack */
        Q_EMIT(buf, "    call $memcpy(l %%t%d, l %%t%d, w %u)\n",
               tmp_counter - 1, tmp_counter, instr->operand.value.token);
        tmp_counter -= 2;
        break;

      case FRUITY_LDOBJ:
        Q_EMIT(buf, "    %%t%d =w loadw %%t%d\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_STOBJ:
        Q_EMIT(buf, "    storew %%t%d, %%t%d\n", tmp_counter - 1, tmp_counter);
        tmp_counter -= 2;
        break;

      /* Array operations */
      case FRUITY_LDLEN:
        /* Array length at offset 0 */
        Q_EMIT(buf, "    %%t%d =w loadw %%t%d\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_LDELEM: {
        /* array, index on stack → value */
        int elem_size = get_element_size_from_instr(instr);
        Q_EMIT(buf, "    %%idx%d =l mul %%t%d, %d\n", tmp_counter, tmp_counter,
               elem_size);
        Q_EMIT(buf, "    %%addr%d =l add %%t%d, %%idx%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        Q_EMIT(buf, "    %%t%d =w loadw %%addr%d\n", tmp_counter - 1,
               tmp_counter - 1);
        tmp_counter--;
        break;
      }

      case FRUITY_STELEM: {
        /* array, index, value on stack */
        int elem_size = get_element_size_from_instr(instr);
        Q_EMIT(buf, "    %%idx%d =l mul %%t%d, %d\n", tmp_counter - 1,
               tmp_counter - 1, elem_size);
        Q_EMIT(buf, "    %%addr%d =l add %%t%d, %%idx%d\n", tmp_counter - 2,
               tmp_counter - 2, tmp_counter - 1);
        Q_EMIT(buf, "    storew %%addr%d, %%t%d\n", tmp_counter - 2,
               tmp_counter);
        tmp_counter -= 3;
        break;
      }

      case FRUITY_LDELEMA: {
        /* array, index → address */
        int elem_size = get_element_size_from_instr(instr);
        Q_EMIT(buf, "    %%idx%d =l mul %%t%d, %d\n", tmp_counter, tmp_counter,
               elem_size);
        Q_EMIT(buf, "    %%t%d =l add %%t%d, %%idx%d\n", tmp_counter - 1,
               tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;
      }

      /* Advanced function operations */
      case FRUITY_CALLI:
        /* Indirect call: fn_ptr on stack */
        Q_EMIT(buf, "    %%t%d =l call %%t%d()\n", tmp_counter, tmp_counter);
        break;

      case FRUITY_LDFTN:
        Q_EMIT(buf, "    %%t%d =l copy $method_%u\n", ++tmp_counter,
               instr->operand.value.token);
        break;

      case FRUITY_LDVIRTFTN:
        Q_EMIT(buf, "    %%t%d =l call $lux_ldvirtftn(l %%t%d, w %u)\n",
               tmp_counter, tmp_counter, instr->operand.value.token);
        break;

      /* Exception handling */
      case FRUITY_THROW:
        Q_EMIT(buf, "    call $lux_throw(l %%t%d)\n", tmp_counter--);
        break;

      case FRUITY_RETHROW:
        Q_EMIT(buf, "    call $lux_rethrow()\n");
        break;

      case FRUITY_LEAVE:
        Q_EMIT(buf, "    jmp @bb%u\n", instr->operand.value.target->block_id);
        break;

      case FRUITY_ENDFINALLY:
        Q_EMIT(buf, "    call $lux_endfinally()\n");
        break;

      /* Metadata operations */
      case FRUITY_SIZEOF:
        Q_EMIT(buf, "    %%t%d =w copy %u\n", ++tmp_counter,
               instr->operand.value.token); /* TODO: actual size lookup */
        break;

      case FRUITY_LDTOKEN:
        Q_EMIT(buf, "    %%t%d =w copy %u\n", ++tmp_counter,
               instr->operand.value.token);
        break;

      /* Typed reference operations */
      case FRUITY_MKREFANY:
        Q_EMIT(buf, "    %%t%d =l call $lux_mkrefany(l %%t%d, w %u)\n",
               tmp_counter, tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_REFANYVAL:
        Q_EMIT(buf, "    %%t%d =l call $lux_refanyval(l %%t%d, w %u)\n",
               tmp_counter, tmp_counter, instr->operand.value.token);
        break;

      case FRUITY_REFANYTYPE:
        Q_EMIT(buf, "    %%t%d =w call $lux_refanytype(l %%t%d)\n", tmp_counter,
               tmp_counter);
        break;

      case FRUITY_CALL: {
        extern int print(char *, ...);
        uint32_t token_val = instr->operand.value.token;
        int resolved = 0;

        /* Attempt to resolve MemberRef */
        if (module && module->metadata) {
          char type_name[128] = {0};
          char method_name[128] = {0};
          il_assembly_t *asm_ptr = (il_assembly_t *)module->metadata;

          /* Try resolving via MemberRef */
          if (il_resolve_memberref(asm_ptr, token_val, type_name,
                                   sizeof(type_name), method_name,
                                   sizeof(method_name)) == 0) {
            /* Success (MemberRef resolved) */
          } else if (((token_val >> 24) & 0xFF) == 0x2B) {
            print("DEBUG: Attempting to resolve MethodSpec token 0x%x\n",
                  token_val);
            /* Try resolving MethodSpec */
            if (il_resolve_methodspec(asm_ptr, token_val, type_name,
                                      sizeof(type_name), method_name,
                                      sizeof(method_name)) == 0) {
              print("DEBUG: Resolved MethodSpec token 0x%08x -> %s::%s\n",
                    token_val, type_name, method_name);
            } else {
              /* Resolution failed */
              type_name[0] = 0;
            }
          }

          if (type_name[0] != 0) {
            print("DEBUG: Resolved Call token 0x%08x -> %s::%s\n", token_val,
                  type_name, method_name);

            /* Map to native InternalCalls */
            if (strcmp(type_name, "System.Console") == 0) {
              if (strcmp(method_name, "WriteLine") == 0) {
                int arg = tmp_counter;
                tmp_counter++;
                Q_EMIT(buf,
                       "    %%t%d =l call $clr_console_writeline(l %%t%d)\n",
                       tmp_counter, arg);
                resolved = 1;
              } else if (strcmp(method_name, "Write") == 0) {
                int arg = tmp_counter;
                tmp_counter++;
                Q_EMIT(buf, "    %%t%d =l call $clr_console_write(l %%t%d)\n",
                       tmp_counter, arg);
                resolved = 1;
              }
            }
          }
        }

        if (!resolved) {
          /* Fallback to raw token if not resolved or not mapped */
          print("DEBUG: FRUITY_CALL unresolved token: %02x %02x %02x %02x "
                "(value=%lu)\n",
                (unsigned int)((token_val >> 24) & 0xFF),
                (unsigned int)((token_val >> 16) & 0xFF),
                (unsigned int)((token_val >> 8) & 0xFF),
                (unsigned int)(token_val & 0xFF), (long unsigned int)token_val);

          int result_reg = ++tmp_counter;
          Q_EMIT(buf, "    %%t%d =l call $method_%ud()\n", result_reg,
                 token_val);
        }
        /* Note: Proper arg handling requires tracking call signature metadata
         */
        /* This is a simplified version that works for nullary functions */
      } break;

      /* Variable Access */
      case FRUITY_LOAD_LOCAL:
        Q_EMIT(buf, "    %%t%d =l loadl %%loc%d\n", ++tmp_counter,
               instr->operand.value.index);
        break;
      case FRUITY_STORE_LOCAL:
        Q_EMIT(buf, "    storel %%t%d, %%loc%d\n", tmp_counter--,
               instr->operand.value.index);
        break;
      case FRUITY_LOAD_ARG:
        Q_EMIT(buf, "    %%t%d =l copy %%arg%d\n", ++tmp_counter,
               instr->operand.value.index);
        break;

      /* Control Flow */
      case FRUITY_SWITCH: {
        fruity_switch_targets_t *targets = instr->operand.value.switch_targets;
        int val_reg = tmp_counter--; /* Value to switch on */

        if (!targets)
          break;

        for (u32int i = 0; i < targets->count; i++) {
          fruity_basic_block_t *target = targets->targets[i];
          u32int target_id = target ? target->block_id : 0;
          int cmp_reg = ++tmp_counter;

          /* Check if val == i */
          Q_EMIT(buf, "    %%t%d =w ceqw %%t%d, %d\n", cmp_reg, val_reg, i);

          /* If match, jump to target. Else jump to next check (local label) */
          if (i < targets->count - 1) {
            Q_EMIT(buf, "    jnz %%t%d, @bb%ud, @sw_%ud_%ud\n", cmp_reg,
                   target_id, bb->block_id, i + 1);
            Q_EMIT(buf, "@sw_%ud_%ud\n", bb->block_id, i + 1);
          } else {
            /* Last check. If match, jump target. Else fallthrough (default) */
            u32int next_id = bb->next ? bb->next->block_id : 0;
            Q_EMIT(buf, "    jnz %%t%d, @bb%ud, @bb%ud\n", cmp_reg, target_id,
                   next_id);
          }
          tmp_counter--; /* Consume cmp_reg */
        }
      } break;

      case FRUITY_JUMP:
        if (instr->operand.value.target)
          Q_EMIT(buf, "    jmp @bb%d\n", instr->operand.value.target->block_id);
        break;

      /* Comparison Branches */
      case FRUITY_BEQ:
      case FRUITY_BNE:
      case FRUITY_BLT:
      case FRUITY_BLE:
      case FRUITY_BGT:
      case FRUITY_BGE: {
        int right = tmp_counter--;
        int left = tmp_counter--;
        int cond = ++tmp_counter; // Result of comparison
        char *op = "";

        switch (instr->opcode) {
        case FRUITY_BEQ:
          op = "ceqw";
          break;
        case FRUITY_BNE:
          op = "cnew";
          break; // QBE has cnew? Check. cneww?
        case FRUITY_BLT:
          op = "csltw";
          break;
        case FRUITY_BLE:
          op = "cslew";
          break;
        case FRUITY_BGT:
          op = "csgtw";
          break;
        case FRUITY_BGE:
          op = "csgew";
          break;
        }
        /* Fix cne logic: if no cnew, use ceqw and invert? QBE has cnew for
         * long? cneww for word? */
        /* QBE doc: ceqw, ceql, cnew, cnel... */
        /* Actually QBE IR usually uses type suffix only for memory/moves? No
         * logic ops too */
        /* QBE: ceqw (Compare Equal Word). cnew (Compare Not Equal Word). Yes.
         */
        /* QBE: ceqw (Compare Equal Word). cnew (Compare Not Equal Word). Yes.
         */
        Q_EMIT(buf, "    %%t%d =w %s %%t%d, %%t%d\n", cond, op, left, right);

        /* Branch */
        u32int target_id = instr->operand.value.target
                               ? instr->operand.value.target->block_id
                               : 0;
        u32int next_id = bb->next ? bb->next->block_id : 0;

        if (target_id > 0 && next_id > 0) {
          Q_EMIT(buf, "    jnz %%t%d, @bb%u, @bb%u\n", cond, target_id,
                 next_id);
        } else if (target_id > 0) {
          /* Implicit fallthrough fallback - use next_id=0 which is invalid if
           * not patched */
          Q_EMIT(buf, "    jnz %%t%d, @bb%u, @bb%u_impl\n", cond, target_id,
                 bb->block_id);
        } else {
          /* No target? */
          Q_EMIT(buf, "    # Missing branch target\n");
        }

        tmp_counter--; // Consume cond
      } break;

      case FRUITY_BTRUE:
      case FRUITY_BFALSE: {
        int cond_temp = tmp_counter--;
        u32int target_id = instr->operand.value.target
                               ? instr->operand.value.target->block_id
                               : 0;
        u32int next_id = bb->next ? bb->next->block_id : 0; // Fallthrough

        /* QBE jnz: jnz %val, @true, @false */
        if (instr->opcode == FRUITY_BTRUE)
          Q_EMIT(buf, "    jnz %%t%d, @bb%ud, @bb%ud\n", cond_temp, target_id,
                 next_id);
        else
          Q_EMIT(buf, "    jnz %%t%d, @bb%ud, @bb%ud\n", cond_temp, next_id,
                 target_id); /* Swap for FALSE */
      } break;

      case FRUITY_LOAD_STRING:
        Q_EMIT(buf, "    %%t%d =l call $clr_string_from_literal(w %d)\n",
               ++tmp_counter, instr->operand.value.i32);
        break;

      case FRUITY_NEWOBJ:
        /* Object allocation - simplified without args */
        Q_EMIT(buf, "    %%t%d =l call $clr_newobj(w %d)\n", ++tmp_counter,
               instr->operand.value.token);
        break;

      case FRUITY_NEWARR:
        /* Array allocation - size on stack */
        Q_EMIT(buf, "    %%t%d =l call $clr_newarr(w %d, w %%t%d)\n",
               tmp_counter, instr->operand.value.token, tmp_counter);
        break;

      case FRUITY_LIME:
        /* call l $lux_alloc(w %size, w %type) */
        /* Assume size is on top of stack */
        Q_EMIT(buf, "    %%ptr%d =l call $lux_alloc(w %%t%d, w 0)\n",
               tmp_counter, tmp_counter);
        /* Result replaces stack top */
        break;

      case FRUITY_VANILLA:
        /* call l $lux_addref(l %ptr) */
        /* Duplicate reference by issuing new WHITE token */
        Q_EMIT(buf, "    %%ptr%d =l call $lux_addref(l %%t%d)\n", tmp_counter,
               tmp_counter);
        /* Result is same pointer (for stack convenience) */
        break;

      case FRUITY_BURN:
        /* call $lux_release(l %ptr) */
        /* Release WHITE token, free if last reference */
        Q_EMIT(buf, "    call $lux_release(l %%t%d)\n", tmp_counter);
        tmp_counter--; /* Pop from stack */
        break;

      case FRUITY_RET:
        /* Return top of stack */
        Q_EMIT(buf, "    ret %%t%d\n", tmp_counter);
        goto done_block;

      default:
        Q_EMIT(buf, "    # opcode %d (unimplemented)\n", instr->opcode);
        break;
      }
    }
  done_block:;
  }

  Q_EMIT(buf, "}\n\n");
  return 0;
}

/* Main translation function */
int fruity_to_qbe(fruity_module_t *module, uintptr out_handle, ulong *out_size,
                  char *errorbuf, usize errorbuf_size) {
  extern void uartputs(char *, int);
  extern int jitdebug;
  extern int print(char *, ...);
  char debug_buf[128];
  QBEBuffer buf;
  fruity_function_t *func;
  void *vaddr;
  usize copy_len;

  print("DEBUG: fruity_to_qbe ENTER module=%p\n", module);

  if (module == nil) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "null module");
    return -1;
  }

  print("DEBUG: fruity_to_qbe module->function_count=%d\n",
        module->function_count);

  /* Initialize buffer */
  print("DEBUG: fruity_to_qbe calling qbe_buffer_init\n");
  if (qbe_buffer_init(&buf) < 0) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "Failed to initialize buffer");
    return -1;
  }
  print("DEBUG: fruity_to_qbe buffer initialized\n");

  /* Emit header */
  print("DEBUG: fruity_to_qbe calling emit_header\n");
  if (emit_header(&buf) < 0) {
    print("DEBUG: fruity_to_qbe emit_header FAILED\n");
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "Failed to emit header");
    qbe_buffer_free(&buf);
    return -1;
  }

  /* Emit static fields */
  emit_static_fields(&buf, (il_assembly_t *)module->metadata);

  print("DEBUG: fruity_to_qbe emit_header done, buf_len=%d\n",
        (int)qbe_buffer_len(&buf));

  /* Emit all functions */
  int func_count = 0;
  for (func = module->functions_head; func != nil; func = func->next) {
    if (emit_function(&buf, func, module) < 0) {
      print("DEBUG: fruity_to_qbe emit_function FAILED\n");
      if (errorbuf && errorbuf_size > 0)
        snprint(errorbuf, (int)errorbuf_size, "Failed to emit function");
      qbe_buffer_free(&buf);
      return -1;
    }
    func_count++;
    print("DEBUG: fruity_to_qbe function %d done, buf_len=%d\n", func_count,
          (int)qbe_buffer_len(&buf));
  }

  /* Get buffer length */
  copy_len = qbe_buffer_len(&buf);
  print("DEBUG: fruity_to_qbe final buf_len=%d\n", (int)copy_len);

  /* Two-pass API: if out_handle == 0, this is a size query */
  if (out_handle == 0) {
    if (out_size)
      *out_size = copy_len + 1; /* +1 for null terminator */
    print("DEBUG: fruity_to_qbe size query: returning size=%lud\n",
          copy_len + 1);
    qbe_buffer_free(&buf);
    return 0;
  }

  /* out_handle is a physical address - convert to kernel virtual */
  vaddr = KADDR(out_handle);

  /* Flatten buffer to contiguous memory and copy to output page */
  {
    usize flat_len;
    PebbleBlue *flat_token = qbe_buffer_flatten(&buf, &flat_len);
    if (flat_token == nil) {
      if (errorbuf && errorbuf_size > 0)
        snprint(errorbuf, (int)errorbuf_size, "Failed to flatten buffer");
      qbe_buffer_free(&buf);
      return -1;
    }

    char *flat_data = (char *)flat_token->blue_data;

    /* Copy to output page */
    memmove(vaddr, flat_data, flat_len);
    ((char *)vaddr)[flat_len] = 0; /* Null-terminate */

    /* DEBUG: Print the generated QBE IL to UART */
    // if (jitdebug) {
    print("DEBUG: Generated QBE IL (len=%ld):\n", flat_len);
    print("%.*s\n", (int)flat_len, flat_data);
    print("DEBUG: End of QBE IL\n");
    // }

    /* Free the Blue token now that we've copied the data */
    pebble_blue_free(flat_token);
  }

  qbe_buffer_free(&buf);
  return 0;
}
