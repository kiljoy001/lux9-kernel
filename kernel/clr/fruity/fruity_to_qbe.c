/* fruity_to_qbe.c - Fruity IR to QBE IL Translator
 *
 * Simplified translator: just emit QBE stubs for now
 */

/* #include "../../include/u.h" - Removed to avoid redefinitions */
#define _U_H_
#define nil ((void *)0)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
typedef __builtin_va_list va_list;

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;

struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
};
struct Dir {
  ushort type;
  uint dev;
  Qid qid;
  ulong mode;
  ulong atime;
  ulong mtime;
  vlong length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
};
#define ERRMAX 128
struct Waitmsg {
  int pid;
  ulong time[3];
  char msg[ERRMAX];
};

#include <string.h>
/* #include "../../include/portlib.h" - Removed to avoid redefinitions */
extern int snprint(char *, int, char *, ...);

#include "../../9front-pc64/mem.h"
#include "../../include/dat.h"
#include "../../include/fns.h"

#include "blind_ledger.h"
#include "exchange.h"
#include "fruity_to_qbe.h"
#include "hhdm.h"
#include "qbe_buffer.h"

/* Emit QBE IL header */
static void emit_header(QBEBuffer *buf) {
  qbe_buffer_printf(buf, "# QBE IL generated from Fruity IR\n\n");
  qbe_buffer_printf(buf, "# Pebble Runtime ABI\n");
  qbe_buffer_printf(
      buf,
      "export function l $lux_alloc(w %%size, w %%type) { @start ret 0 }\n");
  qbe_buffer_printf(
      buf, "export function $lux_token_mint(l %%ptr) { @start ret }\n");
  qbe_buffer_printf(
      buf, "export function $lux_token_burn(l %%ptr) { @start ret }\n\n");
}

/* Emit a function as QBE IL */
static int emit_function(QBEBuffer *buf, fruity_function_t *func) {
  fruity_basic_block_t *bb;
  fruity_instruction_t *instr;
  int tmp_counter = 0;

  qbe_buffer_printf(buf, "export function w $%s(", func->name);
  for (int i = 0; i < func->arg_count; i++) {
    qbe_buffer_printf(buf, "%s %%arg%d", i == 0 ? "l" : ", l", i);
  }
  qbe_buffer_printf(buf, ") {\n");

  /* Allocate storage for locals (simplified: all 8 bytes) */
  for (int i = 0; i < func->local_count; i++) {
    qbe_buffer_printf(buf, "    %%loc%d =l alloc8 8\n", i);
  }

  qbe_buffer_printf(buf, "@start\n");

  for (bb = func->blocks_head; bb != nil; bb = bb->next) {
    if (bb != func->blocks_head)
      qbe_buffer_printf(buf, "@bb%d\n", bb->block_id);

    for (instr = bb->instructions_head; instr != nil; instr = instr->next) {
      /* Translate each Fruity opcode to QBE */
      switch (instr->opcode) {
      case FRUITY_LDC_I4:
        qbe_buffer_printf(buf, "    %%t%d =w copy %d\n", ++tmp_counter,
                          (int)instr->operand.value.i64);
        break;

      case FRUITY_ADD:
        qbe_buffer_printf(buf, "    %%t%d =w add %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      case FRUITY_SUB:
        qbe_buffer_printf(buf, "    %%t%d =w sub %%t%d, %%t%d\n",
                          tmp_counter - 1, tmp_counter - 1, tmp_counter);
        tmp_counter--;
        break;

      /* Variable Access */
      case FRUITY_LOAD_LOCAL:
        qbe_buffer_printf(buf, "    %%t%d =l loadl %%loc%d\n", ++tmp_counter,
                          instr->operand.value.index);
        break;
      case FRUITY_STORE_LOCAL:
        qbe_buffer_printf(buf, "    storel %%t%d, %%loc%d\n", tmp_counter--,
                          instr->operand.value.index);
        break;
      case FRUITY_LOAD_ARG:
        qbe_buffer_printf(buf, "    %%t%d =l copy %%arg%d\n", ++tmp_counter,
                          instr->operand.value.index);
        break;

      /* Control Flow */
      case FRUITY_JUMP:
        if (instr->operand.value.target)
          qbe_buffer_printf(buf, "    jmp @bb%d\n",
                            instr->operand.value.target->block_id);
        break;

      case FRUITY_BTRUE:
      case FRUITY_BFALSE: {
        int cond_temp = tmp_counter--;
        int target_id = instr->operand.value.target
                            ? instr->operand.value.target->block_id
                            : 0;
        int next_id = bb->next ? bb->next->block_id : 0; // Fallthrough

        /* QBE jnz: jnz %val, @true, @false */
        if (instr->opcode == FRUITY_BTRUE)
          qbe_buffer_printf(buf, "    jnz %%t%d, @bb%d, @bb%d\n", cond_temp,
                            target_id, next_id);
        else
          qbe_buffer_printf(buf, "    jnz %%t%d, @bb%d, @bb%d\n", cond_temp,
                            next_id, target_id); /* Swap for FALSE */
      } break;

      case FRUITY_LIME:
        /* call l $lux_alloc(w %size, w %type) */
        /* Assume size is on top of stack */
        qbe_buffer_printf(buf, "    %%ptr%d =l call $lux_alloc(w %%t%d, w 0)\n",
                          tmp_counter, tmp_counter);
        /* Result replaces stack top */
        break;

      case FRUITY_RET:
        /* Return top of stack */
        qbe_buffer_printf(buf, "    ret %%t%d\n", tmp_counter);
        goto done_block;

      default:
        qbe_buffer_printf(buf, "    # opcode %d (unimplemented)\n",
                          instr->opcode);
        break;
      }
    }
  done_block:;
  }

  qbe_buffer_printf(buf, "}\n\n");
  return 0;
}

/* Main translation function */
int fruity_to_qbe(fruity_module_t *module, uintptr out_handle, char *errorbuf,
                  usize errorbuf_size) {
  QBEBuffer buf;
  fruity_function_t *func;
  void *vaddr;
  usize copy_len;

  if (module == nil) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, errorbuf_size, "null module");
    return -1;
  }

  /* Initialize buffer */
  qbe_buffer_init(&buf);

  /* Emit header */
  emit_header(&buf);

  /* Emit all functions */
  for (func = module->functions_head; func != nil; func = func->next) {
    if (emit_function(&buf, func) < 0) {
      if (errorbuf && errorbuf_size > 0)
        snprint(errorbuf, errorbuf_size, "Failed to emit function");
      qbe_buffer_free(&buf);
      return -1;
    }
  }

  /* out_handle is a physical address - convert to kernel virtual */
  vaddr = KADDR(out_handle);
  copy_len = qbe_buffer_len(&buf);
  if (copy_len > 4096) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, errorbuf_size, "Output too large: %lud bytes",
              copy_len);
    qbe_buffer_free(&buf);
    return -1;
  }

  /* Copy to page */
  memmove(vaddr, qbe_buffer_data(&buf), copy_len);
  ((char *)vaddr)[copy_len] = 0;

  qbe_buffer_free(&buf);
  return 0;
}
