/* fruity_to_qbe.c - Fruity IR to QBE IL Translator
 *
 * Simplified translator: just emit QBE stubs for now
 */

#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/mem.h"
#include "../../include/portlib.h"
#include "../../include/u.h"
#include <stdint.h> // Required for uint32_t

#include "../../include/blind_ledger.h"
#include "../../include/exchange.h"
#include "../../include/hhdm.h"
#include "fruity_to_qbe.h"
#include "qbe_buffer.h"

#define Q_EMIT(buf, ...)                                                       \
  do {                                                                         \
    if (qbe_buffer_printf(buf, __VA_ARGS__) < 0)                               \
      return -1;                                                               \
  } while (0)

/* Emit QBE IL header */
static int emit_header(QBEBuffer *buf) {
  Q_EMIT(buf, "# QBE IL generated from Fruity IR\n\n");
  Q_EMIT(buf, "# Pebble Runtime ABI\n");
  /* QBE requires labels on their own lines - proper multi-line format */
  Q_EMIT(buf, "export function l $lux_alloc(w %%size, w %%type) {\n");
  Q_EMIT(buf, "@start\n");
  Q_EMIT(buf, "    ret 0\n");
  Q_EMIT(buf, "}\n\n");
  Q_EMIT(buf, "export function $lux_token_mint(l %%ptr) {\n");
  Q_EMIT(buf, "@start\n");
  Q_EMIT(buf, "    ret\n");
  Q_EMIT(buf, "}\n\n");
  Q_EMIT(buf, "export function $lux_token_burn(l %%ptr) {\n");
  Q_EMIT(buf, "@start\n");
  Q_EMIT(buf, "    ret\n");
  Q_EMIT(buf, "}\n\n");
  return 0;
}

/* Emit a function as QBE IL */
static int emit_function(QBEBuffer *buf, fruity_function_t *func) {
  fruity_basic_block_t *bb;
  fruity_instruction_t *instr;
  int tmp_counter = 0;

  /* Emit function */
  Q_EMIT(buf, "export function w $%s(", func->name);
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

      case FRUITY_CALL: {
        extern void uartputs(char *, int);
        char debug_buf[128];
        uint32_t token_val = instr->operand.value.token;
        snprint(
            debug_buf, sizeof(debug_buf),
            "DEBUG: FRUITY_CALL token bytes: %02x %02x %02x %02x (value=%lu)\n",
            (unsigned int)((token_val >> 24) & 0xFF),
            (unsigned int)((token_val >> 16) & 0xFF),
            (unsigned int)((token_val >> 8) & 0xFF),
            (unsigned int)(token_val & 0xFF), (long unsigned int)token_val);
        uartputs(debug_buf, strlen(debug_buf));
        /* Emit call with result */
        /* For now, generate calls without explicit args - QBE handles variadics
         */
        int result_reg = ++tmp_counter;
        Q_EMIT(buf, "    %%t%d =l call $method_%ud()\n", result_reg,
               instr->operand.value.token);
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
int fruity_to_qbe(fruity_module_t *module, uintptr out_handle, char *errorbuf,
                  usize errorbuf_size) {
  extern void uartputs(char *, int);
  char debug_buf[128];
  QBEBuffer buf;
  fruity_function_t *func;
  void *vaddr;
  usize copy_len;

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: fruity_to_qbe ENTER module=%p\n", module);
  uartputs(debug_buf, strlen(debug_buf));

  if (module == nil) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "null module");
    return -1;
  }

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: fruity_to_qbe module->function_count=%d\n",
          module->function_count);
  uartputs(debug_buf, strlen(debug_buf));

  /* Initialize buffer */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: fruity_to_qbe calling qbe_buffer_init\n");
  uartputs(debug_buf, strlen(debug_buf));
  if (qbe_buffer_init(&buf) < 0) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "Failed to initialize buffer");
    return -1;
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: fruity_to_qbe buffer initialized\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Emit header */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: fruity_to_qbe calling emit_header\n");
  uartputs(debug_buf, strlen(debug_buf));
  if (emit_header(&buf) < 0) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: fruity_to_qbe emit_header FAILED\n");
    uartputs(debug_buf, strlen(debug_buf));
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "Failed to emit header");
    qbe_buffer_free(&buf);
    return -1;
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: fruity_to_qbe emit_header done, buf_len=%d\n",
          (int)qbe_buffer_len(&buf));
  uartputs(debug_buf, strlen(debug_buf));

  /* Emit all functions */
  int func_count = 0;
  for (func = module->functions_head; func != nil; func = func->next) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: fruity_to_qbe emitting function %d\n", func_count);
    uartputs(debug_buf, strlen(debug_buf));
    if (emit_function(&buf, func) < 0) {
      snprint(debug_buf, sizeof(debug_buf),
              "DEBUG: fruity_to_qbe emit_function FAILED\n");
      uartputs(debug_buf, strlen(debug_buf));
      if (errorbuf && errorbuf_size > 0)
        snprint(errorbuf, (int)errorbuf_size, "Failed to emit function");
      qbe_buffer_free(&buf);
      return -1;
    }
    func_count++;
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: fruity_to_qbe function %d done, buf_len=%d\n", func_count,
            (int)qbe_buffer_len(&buf));
    uartputs(debug_buf, strlen(debug_buf));
  }

  /* out_handle is a physical address - convert to kernel virtual */
  vaddr = KADDR(out_handle);
  copy_len = qbe_buffer_len(&buf);
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: fruity_to_qbe final buf_len=%d\n", (int)copy_len);
  uartputs(debug_buf, strlen(debug_buf));
  if (copy_len > 4096) {
    if (errorbuf && errorbuf_size > 0)
      snprint(errorbuf, (int)errorbuf_size, "Output too large: %lud bytes",
              copy_len);
    qbe_buffer_free(&buf);
    return -1;
  }

  /* Copy to page */
  memmove(vaddr, qbe_buffer_data(&buf), copy_len);
  ((char *)vaddr)[copy_len] = 0; // Null-terminate the string

  // DEBUG: Print the generated QBE IL to UART
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: Generated QBE IL (len=%ld):\n",
          copy_len);
  uartputs(debug_buf, strlen(debug_buf));
  uartputs(qbe_buffer_data(&buf), copy_len);
  uartputs("\nDEBUG: End of QBE IL\n", strlen("DEBUG: End of QBE IL\n"));

  qbe_buffer_free(&buf);
  return 0;
}
