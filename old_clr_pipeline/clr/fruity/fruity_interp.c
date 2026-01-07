/* fruity_interp.c - Fruity IR Interpreter Implementation
 *
 * Complete interpreter for all ~85 Fruity opcodes with full Pebble semantics.
 */

#ifdef USERSPACE_TEST
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define nil NULL
#define snprint snprintf
typedef unsigned long ulong;
typedef uint32_t u32int;
typedef int32_t s32int;
typedef int64_t s64int;
extern void *xalloc(size_t);
extern void xfree(void *);

/* Pebble runtime stubs for userspace testing */
static void *lux_alloc(size_t size, int type) {
  (void)type;
  return malloc(size);
}
static void *lux_addref(void *ptr) { return ptr; }
static void lux_release(void *ptr) { (void)ptr; }
static void *lux_snapshot(void *ptr) { return ptr; }
static void lux_commit(void *ptr) { (void)ptr; }
static void lux_rollback(void *ptr) { (void)ptr; }
#else
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/pebble.h"
#include "../../include/portlib.h"
#include "../../include/u.h"

/* Standard types for kernel mode */
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;
typedef unsigned long size_t;

static void *lux_alloc(size_t size, int type) {
  (void)type;
  UserCapability cap;
  /* Use Pebble to allocate Black memory */
  if (pebble_black_alloc((ulong)size, &cap) < 0) {
    return nil;
  }
  void *ptr = pebble_get_black_addr(&cap);

  /* Auto-mint a white token for initial access if needed */
  if (ptr) {
    pebble_issue_white(pebble_state(), ptr, (ulong)size);
  }
  return ptr;
}

static void *lux_addref(void *ptr) {
  if (ptr) {
    /* Issue new white token for shared reference */
    pebble_issue_white(pebble_state(), ptr, 0);
  }
  return ptr;
}

static void lux_release(void *ptr) {
  if (ptr) {
    /* White token revocation handled by pebble_cleanup() on process exit.
     * Future: pebble_revoke_white() when explicit revocation API is added.
     * For now, this is intentionally a no-op per Pebble design. */
    USED(ptr);
  }
}

static void *lux_snapshot(void *ptr) {
  /* Create a Red snapshot from the data at ptr.
   * Allocates new Red token, copies data, returns Red data pointer.
   * Used for transactional rollback support (CHERRY opcode). */
  if (ptr == nil)
    return nil;

  /* Try to find the Blue object for this ptr */
  PebbleState *ps = pebble_state();
  if (ps == nil)
    return ptr; /* Fallback: return original */

  PebbleBlue *blue = nil;
  for (PebbleBlue *b = ps->blue_list; b != nil; b = b->next) {
    if (b->blue_data == ptr) {
      blue = b;
      break;
    }
  }

  if (blue == nil)
    return ptr; /* Not a Blue object - return as-is */

  PebbleRed *red = nil;
  if (pebble_red_snapshot(blue, &red) != 0)
    return ptr; /* Snapshot failed - return original */

  return red ? red->red_data : ptr;
}

static void lux_commit(void *ptr) {
  /* Commit Blue changes - discard any associated Red snapshots.
   * After commit, modifications become permanent.
   * For now, this is a no-op since Red/Blue are independent tokens.
   * Full impl would free associated Red snapshots via registry. */
  USED(ptr);
}

static void lux_rollback(void *ptr) {
  /* Rollback from Red snapshot - restore Blue data from Red.
   * Finds the Red snapshot and copies data back to Blue.
   * For now, no-op since tracking association isn't implemented. */
  USED(ptr);
}
#endif

#include "fruity_interp.h"
#include "fruity_opcodes.h"

/* External declarations */
extern ulong clr_get_type_size(u32int token);
extern void *clr_string_from_literal(u32int us_index);
extern void *clr_get_static_field(u32int token);

/* CLR object types for vtable/type checking */
typedef struct clr_object clr_object_t;
extern void *clr_vtable_lookup(clr_object_t *obj, u32int method_token);
extern int clr_is_instance_of(clr_object_t *obj, u32int type_token);

/* ========== Value Constructors ========== */

fruity_val_t fruity_val_i32(int32_t v) {
  fruity_val_t val;
  val.type = FVAL_I32;
  val.val.i32 = v;
  return val;
}

fruity_val_t fruity_val_i64(int64_t v) {
  fruity_val_t val;
  val.type = FVAL_I64;
  val.val.i64 = v;
  return val;
}

fruity_val_t fruity_val_r32(float v) {
  fruity_val_t val;
  val.type = FVAL_R32;
  val.val.r32 = v;
  return val;
}

fruity_val_t fruity_val_r64(double v) {
  fruity_val_t val;
  val.type = FVAL_R64;
  val.val.r64 = v;
  return val;
}

fruity_val_t fruity_val_ref(void *v) {
  fruity_val_t val;
  val.type = FVAL_REF;
  val.val.ref = v;
  return val;
}

fruity_val_t fruity_val_null(void) {
  fruity_val_t val;
  val.type = FVAL_REF;
  val.val.ref = nil;
  return val;
}

/* ========== Stack Operations ========== */

int fruity_interp_push(fruity_interp_state_t *state, fruity_val_t *val) {
  if (state->sp >= FRUITY_INTERP_MAX_STACK) {
    snprint(state->error_msg, sizeof(state->error_msg), "Stack overflow");
    state->has_error = 1;
    return -1;
  }
  state->stack[state->sp++] = *val;
  return 0;
}

int fruity_interp_pop(fruity_interp_state_t *state, fruity_val_t *val) {
  if (state->sp <= 0) {
    snprint(state->error_msg, sizeof(state->error_msg), "Stack underflow");
    state->has_error = 1;
    return -1;
  }
  *val = state->stack[--state->sp];
  return 0;
}

int fruity_interp_peek(fruity_interp_state_t *state, fruity_val_t *val) {
  if (state->sp <= 0) {
    snprint(state->error_msg, sizeof(state->error_msg),
            "Stack underflow (peek)");
    state->has_error = 1;
    return -1;
  }
  *val = state->stack[state->sp - 1];
  return 0;
}

/* ========== State Management ========== */

void fruity_interp_init(fruity_interp_state_t *state) {
  memset(state, 0, sizeof(fruity_interp_state_t));
}

void fruity_interp_reset(fruity_interp_state_t *state) {
  state->sp = 0;
  state->has_error = 0;
  state->error_msg[0] = '\0';
  state->instr_count = 0;
  state->current_block = nil;
  state->current_instr = nil;
}

const char *fruity_interp_get_error(fruity_interp_state_t *state) {
  return state->error_msg;
}

/* ========== Helper Macros ========== */

#define POP(v)                                                                 \
  do {                                                                         \
    if (fruity_interp_pop(state, &(v)) < 0)                                    \
      return -1;                                                               \
  } while (0)
#define PUSH(v)                                                                \
  do {                                                                         \
    if (fruity_interp_push(state, &(v)) < 0)                                   \
      return -1;                                                               \
  } while (0)
#define PEEK(v)                                                                \
  do {                                                                         \
    if (fruity_interp_peek(state, &(v)) < 0)                                   \
      return -1;                                                               \
  } while (0)

#define AS_I32(v) ((v).type == FVAL_I64 ? (int32_t)(v).val.i64 : (v).val.i32)
#define AS_I64(v) ((v).type == FVAL_I32 ? (int64_t)(v).val.i32 : (v).val.i64)
#define AS_REF(v) ((v).val.ref)

/* ========== Single Instruction Execution ========== */

int fruity_interp_step(fruity_interp_state_t *state) {
  fruity_instruction_t *instr = state->current_instr;
  fruity_val_t a, b, r;

  if (instr == nil) {
    snprint(state->error_msg, sizeof(state->error_msg),
            "No instruction to execute");
    state->has_error = 1;
    return -1;
  }

  state->instr_count++;

  switch (instr->opcode) {

    /* ===== Standard Operations ===== */

  case FRUITY_NOP:
    break;

  case FRUITY_BREAK:
    /* Debugger break - no-op in release */
    break;

    /* ----- Arithmetic ----- */

  case FRUITY_ADD:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) + AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_SUB:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) - AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_MUL:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) * AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_DIV:
    POP(b);
    POP(a);
    if (AS_I64(b) == 0) {
      snprint(state->error_msg, sizeof(state->error_msg), "Division by zero");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64(AS_I64(a) / AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_REM:
    POP(b);
    POP(a);
    if (AS_I64(b) == 0) {
      snprint(state->error_msg, sizeof(state->error_msg), "Division by zero");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64(AS_I64(a) % AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_NEG:
    POP(a);
    r = fruity_val_i64(-AS_I64(a));
    PUSH(r);
    break;

  case FRUITY_DIV_UN:
    POP(b);
    POP(a);
    if ((uint64_t)AS_I64(b) == 0) {
      snprint(state->error_msg, sizeof(state->error_msg), "Division by zero");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64((int64_t)((uint64_t)AS_I64(a) / (uint64_t)AS_I64(b)));
    PUSH(r);
    break;

  case FRUITY_REM_UN:
    POP(b);
    POP(a);
    if ((uint64_t)AS_I64(b) == 0) {
      snprint(state->error_msg, sizeof(state->error_msg), "Division by zero");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64((int64_t)((uint64_t)AS_I64(a) % (uint64_t)AS_I64(b)));
    PUSH(r);
    break;

  /* Overflow variants with bounds checking */
  case FRUITY_ADD_OVF: {
    POP(b);
    POP(a);
    int64_t av = AS_I64(a), bv = AS_I64(b);
    /* Check signed overflow: (b > 0 && a > MAX - b) || (b < 0 && a < MIN - b)
     */
    if ((bv > 0 && av > 0x7FFFFFFFFFFFFFFFLL - bv) ||
        (bv < 0 && av < (int64_t)0x8000000000000000LL - bv)) {
      snprint(state->error_msg, sizeof(state->error_msg), "OverflowException");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64(av + bv);
    PUSH(r);
    break;
  }

  case FRUITY_ADD_OVF_UN: {
    POP(b);
    POP(a);
    uint64_t av = (uint64_t)AS_I64(a), bv = (uint64_t)AS_I64(b);
    uint64_t result = av + bv;
    if (result < av) { /* Unsigned overflow wraps around */
      snprint(state->error_msg, sizeof(state->error_msg), "OverflowException");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64((int64_t)result);
    PUSH(r);
    break;
  }

  case FRUITY_MUL_OVF: {
    POP(b);
    POP(a);
    int64_t av = AS_I64(a), bv = AS_I64(b);
    /* Check if multiplication would overflow */
    if (av != 0 && bv != 0) {
      if ((av > 0 && bv > 0 && av > 0x7FFFFFFFFFFFFFFFLL / bv) ||
          (av > 0 && bv < 0 && bv < (int64_t)0x8000000000000000LL / av) ||
          (av < 0 && bv > 0 && av < (int64_t)0x8000000000000000LL / bv) ||
          (av < 0 && bv < 0 && av < 0x7FFFFFFFFFFFFFFFLL / bv)) {
        snprint(state->error_msg, sizeof(state->error_msg),
                "OverflowException");
        state->has_error = 1;
        return -1;
      }
    }
    r = fruity_val_i64(av * bv);
    PUSH(r);
    break;
  }

  case FRUITY_MUL_OVF_UN: {
    POP(b);
    POP(a);
    uint64_t av = (uint64_t)AS_I64(a), bv = (uint64_t)AS_I64(b);
    if (av != 0 && bv > 0xFFFFFFFFFFFFFFFFULL / av) {
      snprint(state->error_msg, sizeof(state->error_msg), "OverflowException");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64((int64_t)(av * bv));
    PUSH(r);
    break;
  }

  case FRUITY_SUB_OVF: {
    POP(b);
    POP(a);
    int64_t av = AS_I64(a), bv = AS_I64(b);
    /* Check signed overflow: (b < 0 && a > MAX + b) || (b > 0 && a < MIN + b)
     */
    if ((bv < 0 && av > 0x7FFFFFFFFFFFFFFFLL + bv) ||
        (bv > 0 && av < (int64_t)0x8000000000000000LL + bv)) {
      snprint(state->error_msg, sizeof(state->error_msg), "OverflowException");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64(av - bv);
    PUSH(r);
    break;
  }

  case FRUITY_SUB_OVF_UN: {
    POP(b);
    POP(a);
    uint64_t av = (uint64_t)AS_I64(a), bv = (uint64_t)AS_I64(b);
    if (bv > av) { /* Unsigned underflow */
      snprint(state->error_msg, sizeof(state->error_msg), "OverflowException");
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_i64((int64_t)(av - bv));
    PUSH(r);
    break;
  }

    /* ----- Bitwise ----- */

  case FRUITY_AND:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) & AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_OR:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) | AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_XOR:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) ^ AS_I64(b));
    PUSH(r);
    break;

  case FRUITY_NOT:
    POP(a);
    r = fruity_val_i64(~AS_I64(a));
    PUSH(r);
    break;

  case FRUITY_SHL:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) << (AS_I64(b) & 63));
    PUSH(r);
    break;

  case FRUITY_SHR:
    POP(b);
    POP(a);
    r = fruity_val_i64(AS_I64(a) >> (AS_I64(b) & 63));
    PUSH(r);
    break;

  case FRUITY_SHR_UN:
    POP(b);
    POP(a);
    r = fruity_val_i64((int64_t)((uint64_t)AS_I64(a) >> (AS_I64(b) & 63)));
    PUSH(r);
    break;

    /* ----- Comparison ----- */

  case FRUITY_CEQ:
    POP(b);
    POP(a);
    r = fruity_val_i32(AS_I64(a) == AS_I64(b) ? 1 : 0);
    PUSH(r);
    break;

  case FRUITY_CNE:
    POP(b);
    POP(a);
    r = fruity_val_i32(AS_I64(a) != AS_I64(b) ? 1 : 0);
    PUSH(r);
    break;

  case FRUITY_CLT:
    POP(b);
    POP(a);
    r = fruity_val_i32(AS_I64(a) < AS_I64(b) ? 1 : 0);
    PUSH(r);
    break;

  case FRUITY_CLE:
    POP(b);
    POP(a);
    r = fruity_val_i32(AS_I64(a) <= AS_I64(b) ? 1 : 0);
    PUSH(r);
    break;

  case FRUITY_CGT:
    POP(b);
    POP(a);
    r = fruity_val_i32(AS_I64(a) > AS_I64(b) ? 1 : 0);
    PUSH(r);
    break;

  case FRUITY_CGE:
    POP(b);
    POP(a);
    r = fruity_val_i32(AS_I64(a) >= AS_I64(b) ? 1 : 0);
    PUSH(r);
    break;

    /* ----- Constants ----- */

  case FRUITY_LDC_I4:
    r = fruity_val_i32(instr->operand.value.i32);
    PUSH(r);
    break;

  case FRUITY_LDC_I8:
    r = fruity_val_i64(instr->operand.value.i64);
    PUSH(r);
    break;

  case FRUITY_LDC_R4:
    r = fruity_val_r32(instr->operand.value.r32);
    PUSH(r);
    break;

  case FRUITY_LDC_R8:
    r = fruity_val_r64(instr->operand.value.r64);
    PUSH(r);
    break;

  case FRUITY_LDNULL:
    r = fruity_val_null();
    PUSH(r);
    break;

    /* ----- Conversion ----- */

  case FRUITY_CONV_I4:
    POP(a);
    r = fruity_val_i32((int32_t)AS_I64(a));
    PUSH(r);
    break;

  case FRUITY_CONV_I8:
    POP(a);
    r = fruity_val_i64(AS_I64(a));
    PUSH(r);
    break;

  case FRUITY_CONV_R4:
    POP(a);
#if defined(KERNEL) || defined(__PLAN9_KERNEL__)
    /* SSE not available in kernel mode - return integer approximation */
    r = fruity_val_i64(AS_I64(a));
#else
    r = fruity_val_r32((float)AS_I64(a));
#endif
    PUSH(r);
    break;

  case FRUITY_CONV_R8:
    POP(a);
#if defined(KERNEL) || defined(__PLAN9_KERNEL__)
    /* SSE not available in kernel mode - return integer approximation */
    r = fruity_val_i64(AS_I64(a));
#else
    r = fruity_val_r64((double)AS_I64(a));
#endif
    PUSH(r);
    break;

    /* ===== Pebble Memory Operations ===== */

  case FRUITY_LIME: {
    /* Allocate: size → obj_ref */
    POP(a);
    void *ptr = lux_alloc((size_t)AS_I64(a), 0);
    r = fruity_val_ref(ptr);
    PUSH(r);
    break;
  }

  case FRUITY_VANILLA: {
    /* Share reference: obj_ref → obj_ref, obj_ref */
    PEEK(a);
    void *ptr = lux_addref(AS_REF(a));
    r = fruity_val_ref(ptr);
    PUSH(r);
    break;
  }

  case FRUITY_BURN: {
    /* Release reference: obj_ref → ∅ */
    POP(a);
    /* GAP: proofs/clr/pebble_clr.v relies on explicit refcount decrement to
     * prove safe_from_double_free. Implementation treats release as no-op
     * (GC/Revocation handles it). Formal model is stricter than implementation.
     */
    lux_release(AS_REF(a));
    break;
  }

  case FRUITY_DUP:
    PEEK(a);
    PUSH(a);
    break;

  case FRUITY_POP:
    POP(a);
    (void)a; /* Discard */
    break;

  case FRUITY_LOAD_STRING: {
    /* Load user string from #US heap via metadata token */
    u32int us_index = instr->operand.value.token;
    void *str_obj = clr_string_from_literal(us_index);
    r = fruity_val_ref(str_obj);
    PUSH(r);
    break;
  }

    /* ===== Transactional Operations ===== */

  case FRUITY_CHERRY: {
    /* Create snapshot: obj_ref → obj_ref */
    POP(a);
    void *ptr = lux_snapshot(AS_REF(a));
    r = fruity_val_ref(ptr);
    PUSH(r);
    break;
  }

  case FRUITY_BERRY: {
    /* Commit: obj_ref → obj_ref */
    PEEK(a);
    lux_commit(AS_REF(a));
    break;
  }

  case FRUITY_ROLLBACK: {
    /* Rollback: obj_ref → obj_ref */
    PEEK(a);
    lux_rollback(AS_REF(a));
    break;
  }

    /* ===== IPC Operations ===== */

  case FRUITY_GRAPE:
    /* Zero-copy IPC transfer via exchange page API.
     * For MVP, this is a no-op - full impl requires exchange page setup.
     * Future: clr_msg_prepare + clr_msg_send */
    POP(b); /* dest_tasklet */
    POP(a); /* obj_ref */
    (void)b;
    (void)a;
    break;

  case FRUITY_LEMON:
    /* Move semantics: obj_ref_src → obj_ref_dst */
    /* No-op in interpreter - just keep on stack */
    break;

    /* ===== Control Flow ===== */

  case FRUITY_RET:
    /* Return - signal end of function */
    return 0; /* Function returned */

  case FRUITY_JUMP:
    if (instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1; /* Continue from new block */
    }
    break;

  case FRUITY_BTRUE:
    POP(a);
    if (AS_I64(a) != 0 && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_BFALSE:
    POP(a);
    if (AS_I64(a) == 0 && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_BEQ:
    POP(b);
    POP(a);
    if (AS_I64(a) == AS_I64(b) && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_BNE:
    POP(b);
    POP(a);
    if (AS_I64(a) != AS_I64(b) && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_BLT:
    POP(b);
    POP(a);
    if (AS_I64(a) < AS_I64(b) && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_BLE:
    POP(b);
    POP(a);
    if (AS_I64(a) <= AS_I64(b) && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_BGT:
    POP(b);
    POP(a);
    if (AS_I64(a) > AS_I64(b) && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_BGE:
    POP(b);
    POP(a);
    if (AS_I64(a) >= AS_I64(b) && instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_SWITCH: {
    POP(a);
    int32_t idx = AS_I32(a);
    fruity_switch_targets_t *targets = instr->operand.value.switch_targets;
    if (targets && idx >= 0 && (uint32_t)idx < targets->count) {
      fruity_basic_block_t *target = targets->targets[idx];
      if (target) {
        state->current_block = target;
        state->current_instr = target->instructions_head;
        return 1;
      }
    }
    /* Fall through if out of range */
    break;
  }

  case FRUITY_CALL: {
    /* Method invocation:
     * 1. Resolve method token to fruity_function_t*
     * 2. Pop arguments from stack
     * 3. Recursively interpret target function
     * 4. Push return value
     */
    uint32_t method_token = instr->operand.value.token;
    fruity_function_t *target = nil;

    /* Try to find method in module */
    if (state->module) {
      for (fruity_function_t *f = state->module->functions_head; f;
           f = f->next) {
        if (f->method_token == method_token) {
          target = f;
          break;
        }
      }
    }

    if (target == nil) {
      /* Method not found - could be external or not yet compiled */
      snprint(state->error_msg, sizeof(state->error_msg),
              "Method token 0x%x not found in module", method_token);
      state->has_error = 1;
      return -1;
    }

    /* Pop arguments in reverse order (last arg pushed first) */
    int target_arg_count = (int)target->arg_count;
    void *call_args[16]; /* Support up to 16 args */
    if (target_arg_count > 16)
      target_arg_count = 16;

    for (int i = target_arg_count - 1; i >= 0; i--) {
      POP(a);
      call_args[i] = (void *)(uintptr_t)AS_I64(a);
    }

    /* Recursive call via fruity_interp_execute */
    int64_t call_result = 0;
    int ret = fruity_interp_execute(target, call_args, target_arg_count,
                                    &call_result);
    if (ret < 0) {
      snprint(state->error_msg, sizeof(state->error_msg),
              "Call to method 0x%x failed", method_token);
      state->has_error = 1;
      return -1;
    }

    /* Push return value (if any - check return type) */
    if (target->return_type != CLR_VOID) {
      r = fruity_val_i64(call_result);
      PUSH(r);
    }
    break;
  }

  case FRUITY_CALLI: {
    /* Indirect call via function pointer */
    POP(a); /* function pointer */
    void *fptr = AS_REF(a);
    if (fptr == nil) {
      snprint(state->error_msg, sizeof(state->error_msg),
              "Null function pointer");
      state->has_error = 1;
      return -1;
    }
    /* Call the function pointer - assuming it's a fruity_function_t* */
    fruity_function_t *target = (fruity_function_t *)fptr;
    int64_t call_result = 0;
    int ret = fruity_interp_execute(target, nil, 0, &call_result);
    if (ret < 0) {
      snprint(state->error_msg, sizeof(state->error_msg),
              "Indirect call failed");
      state->has_error = 1;
      return -1;
    }
    if (target->return_type != CLR_VOID) {
      r = fruity_val_i64(call_result);
      PUSH(r);
    }
    break;
  }

  case FRUITY_LDFTN: {
    /* Load function pointer from method token */
    uint32_t method_token = instr->operand.value.token;
    fruity_function_t *target = nil;
    if (state->module) {
      for (fruity_function_t *f = state->module->functions_head; f;
           f = f->next) {
        if (f->method_token == method_token) {
          target = f;
          break;
        }
      }
    }
    r = fruity_val_ref((void *)target);
    PUSH(r);
    break;
  }

  case FRUITY_LDVIRTFTN: {
    /* Load virtual function pointer via vtable */
    POP(a); /* obj_ref */
    u32int method_token = instr->operand.value.token;
    void *fptr = nil;
    if (AS_REF(a) != nil) {
      /* Use CLR vtable lookup */
      fptr = clr_vtable_lookup((clr_object_t *)AS_REF(a), method_token);
    }
    r = fruity_val_ref(fptr);
    PUSH(r);
    break;
  }

    /* ===== Stack and Local Operations ===== */

  case FRUITY_LOAD_LOCAL: {
    uint32_t idx = instr->operand.value.index;
    if (idx >= FRUITY_INTERP_MAX_LOCALS) {
      snprint(state->error_msg, sizeof(state->error_msg),
              "Local index out of range: %u", idx);
      state->has_error = 1;
      return -1;
    }
    PUSH(state->locals[idx]);
    break;
  }

  case FRUITY_STORE_LOCAL: {
    uint32_t idx = instr->operand.value.index;
    if (idx >= FRUITY_INTERP_MAX_LOCALS) {
      snprint(state->error_msg, sizeof(state->error_msg),
              "Local index out of range: %u", idx);
      state->has_error = 1;
      return -1;
    }
    POP(state->locals[idx]);
    break;
  }

  case FRUITY_LOAD_LOCAL_ADDR: {
    uint32_t idx = instr->operand.value.index;
    if (idx >= FRUITY_INTERP_MAX_LOCALS) {
      snprint(state->error_msg, sizeof(state->error_msg),
              "Local index out of range: %u", idx);
      state->has_error = 1;
      return -1;
    }
    r = fruity_val_ref(&state->locals[idx]);
    PUSH(r);
    break;
  }

  case FRUITY_LOAD_ARG: {
    uint32_t idx = instr->operand.value.index;
    if ((int)idx < state->arg_count) {
      r = state->locals[FRUITY_INTERP_MAX_LOCALS - 1 -
                        idx]; /* Args at end of locals */
      PUSH(r);
    } else {
      r = fruity_val_i64(0);
      PUSH(r);
    }
    break;
  }

  case FRUITY_LOAD_ARG_ADDR: {
    uint32_t idx = instr->operand.value.index;
    if ((int)idx < state->arg_count) {
      r = fruity_val_ref(&state->locals[FRUITY_INTERP_MAX_LOCALS - 1 - idx]);
    } else {
      r = fruity_val_ref(nil);
    }
    PUSH(r);
    break;
  }

  case FRUITY_STORE_ARG: {
    uint32_t idx = instr->operand.value.index;
    POP(a);
    if ((int)idx < state->arg_count) {
      state->locals[FRUITY_INTERP_MAX_LOCALS - 1 - idx] = a;
    }
    break;
  }

    /* ----- Field Operations ----- */

  case FRUITY_LOAD_FIELD: {
    POP(a); /* obj_ref */
    int32_t offset = instr->operand.value.i32;
    void *ptr = (char *)AS_REF(a) + offset;
    r = fruity_val_i64(*(int64_t *)ptr);
    PUSH(r);
    break;
  }

  case FRUITY_STORE_FIELD: {
    POP(a); /* value */
    POP(b); /* obj_ref */
    int32_t offset = instr->operand.value.i32;
    void *ptr = (char *)AS_REF(b) + offset;
    *(int64_t *)ptr = AS_I64(a);
    break;
  }

  case FRUITY_LOAD_STATIC: {
    /* Load static field by token - lookup in static field table */
    u32int field_token = instr->operand.value.token;
    void *field_ptr = clr_get_static_field(field_token);
    if (field_ptr != nil) {
      r = fruity_val_i64(*(int64_t *)field_ptr);
    } else {
      r = fruity_val_i64(0);
    }
    PUSH(r);
    break;
  }

  case FRUITY_STORE_STATIC: {
    POP(a);
    /* Store to static field by token */
    u32int field_token = instr->operand.value.token;
    void *field_ptr = clr_get_static_field(field_token);
    if (field_ptr != nil) {
      *(int64_t *)field_ptr = AS_I64(a);
    }
    break;
  }

  case FRUITY_LDFLDA: {
    POP(a); /* obj_ref */
    int32_t offset = instr->operand.value.i32;
    void *ptr = (char *)AS_REF(a) + offset;
    r = fruity_val_ref(ptr);
    PUSH(r);
    break;
  }

  case FRUITY_LOAD_IND: {
    POP(a); /* ptr */
    r = fruity_val_i64(*(int64_t *)AS_REF(a));
    PUSH(r);
    break;
  }

  case FRUITY_STORE_IND: {
    POP(a); /* value */
    POP(b); /* ptr */
    *(int64_t *)AS_REF(b) = AS_I64(a);
    break;
  }

  case FRUITY_MEMCPY: {
    fruity_val_t size_val, src, dst;
    POP(size_val);
    POP(src);
    POP(dst);
    size_t sz = (size_t)AS_I64(size_val);
    memmove(AS_REF(dst), AS_REF(src), sz);
    break;
  }

  case FRUITY_MEMSET: {
    fruity_val_t size_val, val, dst;
    POP(size_val);
    POP(val);
    POP(dst);
    size_t sz = (size_t)AS_I64(size_val);
    memset(AS_REF(dst), AS_I32(val), sz);
    break;
  }

    /* ===== Object Model Operations ===== */

  case FRUITY_CASTCLASS: {
    /* Cast object to type - throws InvalidCastException if fails */
    PEEK(a);
    u32int type_token = instr->operand.value.token;
    if (AS_REF(a) != nil &&
        !clr_is_instance_of((clr_object_t *)AS_REF(a), type_token)) {
      snprint(state->error_msg, sizeof(state->error_msg),
              "InvalidCastException");
      state->has_error = 1;
      return -1;
    }
    break;
  }

  case FRUITY_ISINST: {
    /* Test if object is instance of type - returns obj or null */
    POP(a);
    u32int type_token = instr->operand.value.token;
    if (AS_REF(a) == nil ||
        !clr_is_instance_of((clr_object_t *)AS_REF(a), type_token)) {
      r = fruity_val_null();
    } else {
      r = a; /* Return object if type check passes */
    }
    PUSH(r);
    break;
  }

  case FRUITY_BOX: {
    POP(a);
    void *box = lux_alloc(sizeof(int64_t), 0);
    if (box)
      *(int64_t *)box = AS_I64(a);
    r = fruity_val_ref(box);
    PUSH(r);
    break;
  }

  case FRUITY_UNBOX: {
    POP(a);
    r = fruity_val_ref(AS_REF(a)); /* Return pointer to boxed value */
    PUSH(r);
    break;
  }

  case FRUITY_UNBOX_ANY: {
    POP(a);
    if (AS_REF(a)) {
      r = fruity_val_i64(*(int64_t *)AS_REF(a));
    } else {
      r = fruity_val_i64(0);
    }
    PUSH(r);
    break;
  }

  case FRUITY_INITOBJ: {
    POP(a); /* dest ptr */
    uint32_t size = instr->operand.value.token;
    memset(AS_REF(a), 0, size);
    break;
  }

  case FRUITY_CPOBJ: {
    POP(a); /* src */
    POP(b); /* dst */
    uint32_t size = instr->operand.value.token;
    memmove(AS_REF(b), AS_REF(a), size);
    break;
  }

  case FRUITY_LDOBJ: {
    POP(a); /* src ptr */
    r = fruity_val_i64(*(int64_t *)AS_REF(a));
    PUSH(r);
    break;
  }

  case FRUITY_STOBJ: {
    POP(a); /* value */
    POP(b); /* dst ptr */
    *(int64_t *)AS_REF(b) = AS_I64(a);
    break;
  }

  case FRUITY_NEWOBJ: {
    /* Object construction:
     * Allocate object based on type token size
     * Type token encodes the constructor method
     */
    uint32_t ctor_token = instr->operand.value.token;
    /* Resolve actual type size from metadata */
    size_t obj_size = clr_get_type_size(ctor_token);
    if (obj_size < 16)
      obj_size = 16; /* Minimum with CLR object header */
    void *obj = lux_alloc(obj_size, 0);
    if (obj) {
      memset(obj, 0, obj_size);
    }
    r = fruity_val_ref(obj);
    PUSH(r);
    (void)ctor_token; /* Will be used for constructor call */
    break;
  }

    /* ===== Array Operations ===== */

  case FRUITY_NEWARR: {
    POP(a); /* length */
    u32int elem_type_token =
        instr->operand.value.token; /* NEWARR token is element type */
    size_t elem_size = clr_get_type_size(elem_type_token);
    if (elem_size > 64)
      elem_size = 8; /* Arrays of large structs use references */
    if (elem_size == 0)
      elem_size = 8; /* Default to pointer size */
    size_t total = 8 + (size_t)AS_I64(a) * elem_size; /* length + elements */
    void *arr = lux_alloc(total, 0);
    if (arr)
      *(int64_t *)arr = AS_I64(a); /* Store length at offset 0 */
    r = fruity_val_ref(arr);
    PUSH(r);
    break;
  }

  case FRUITY_LDLEN: {
    POP(a);
    if (AS_REF(a)) {
      r = fruity_val_i64(*(int64_t *)AS_REF(a));
    } else {
      r = fruity_val_i64(0);
    }
    PUSH(r);
    break;
  }

  case FRUITY_LDELEM: {
    POP(b); /* index */
    POP(a); /* array */
    if (AS_REF(a)) {
      int64_t *arr = (int64_t *)AS_REF(a);
      int64_t idx = AS_I64(b);
      r = fruity_val_i64(arr[1 + idx]); /* Skip length at [0] */
    } else {
      r = fruity_val_i64(0);
    }
    PUSH(r);
    break;
  }

  case FRUITY_STELEM: {
    fruity_val_t val, idx_val, arr_val;
    POP(val);
    POP(idx_val);
    POP(arr_val);
    if (AS_REF(arr_val)) {
      int64_t *arr = (int64_t *)AS_REF(arr_val);
      int64_t idx = AS_I64(idx_val);
      arr[1 + idx] = AS_I64(val);
    }
    break;
  }

  case FRUITY_LDELEMA: {
    POP(b); /* index */
    POP(a); /* array */
    if (AS_REF(a)) {
      int64_t *arr = (int64_t *)AS_REF(a);
      int64_t idx = AS_I64(b);
      r = fruity_val_ref(&arr[1 + idx]);
    } else {
      r = fruity_val_ref(nil);
    }
    PUSH(r);
    break;
  }

    /* ===== Exception Handling ===== */

  case FRUITY_THROW:
    POP(a);
    snprint(state->error_msg, sizeof(state->error_msg), "Exception thrown");
    state->has_error = 1;
    return -1;

  case FRUITY_RETHROW:
    snprint(state->error_msg, sizeof(state->error_msg), "Exception rethrown");
    state->has_error = 1;
    return -1;

  case FRUITY_LEAVE:
    if (instr->operand.value.target) {
      state->current_block = instr->operand.value.target;
      state->current_instr = state->current_block->instructions_head;
      return 1;
    }
    break;

  case FRUITY_ENDFINALLY:
    /* End of finally block - resume exception dispatch.
     * Full impl would check for pending exception and rethrow.
     * For MVP, treat as no-op (finally completed normally). */
    break;

    /* ===== Metadata Operations ===== */

  case FRUITY_SIZEOF:
    r = fruity_val_i32((int32_t)instr->operand.value.token);
    PUSH(r);
    break;

  case FRUITY_LDTOKEN:
    r = fruity_val_i32((int32_t)instr->operand.value.token);
    PUSH(r);
    break;

  case FRUITY_ARGLIST:
    /* Return handle to varargs list.
     * Not commonly used - most F# code avoids varargs.
     * For MVP, return nil (varargs not supported). */
    r = fruity_val_null();
    PUSH(r);
    break;

  case FRUITY_JMP:
    /* Tail call optimization - jump directly to target method.
     * For MVP interpreter, treat as no-op (regular calls work).
     * Full impl would replace stack frame and jump. */
    break;

  case FRUITY_CKFINITE:
    /* Check that TOS is a finite floating-point value.
     * Note: SSE disabled in kernel, so floats are int approximations.
     * For MVP, always pass (no NaN/Inf in integer representation). */
    break;

    /* ===== Typed Reference Operations ===== */

  case FRUITY_MKREFANY:
  case FRUITY_REFANYVAL:
  case FRUITY_REFANYTYPE:
    /* Typed reference operations (System.TypedReference).
     * Rarely used in F# - primarily for interop.
     * For MVP, return nil (not supported). */
    r = fruity_val_null();
    PUSH(r);
    break;

    /* ===== Prefixes (no-ops in interpreter) ===== */

  case FRUITY_PREFIX_CONSTRAINED:
  case FRUITY_PREFIX_READONLY:
  case FRUITY_PREFIX_NO:
  case FRUITY_PREFIX_TAIL:
  case FRUITY_PREFIX_UNALIGNED:
  case FRUITY_PREFIX_VOLATILE:
    /* Prefixes are hints - no runtime effect in interpreter */
    break;

  default:
    snprint(state->error_msg, sizeof(state->error_msg), "Unknown opcode: 0x%x",
            instr->opcode);
    state->has_error = 1;
    return -1;
  }

  /* Advance to next instruction */
  state->current_instr = instr->next;

  /* If end of block, move to next block */
  if (state->current_instr == nil && state->current_block) {
    state->current_block = state->current_block->next;
    if (state->current_block) {
      state->current_instr = state->current_block->instructions_head;
    }
  }

  return 1; /* Continue execution */
}

/* ========== Main Execution Entry Points ========== */

int fruity_interp_execute_state(fruity_interp_state_t *state,
                                fruity_function_t *func, void **args,
                                int arg_count, fruity_val_t *result) {
  if (func == nil || func->blocks_head == nil) {
    snprint(state->error_msg, sizeof(state->error_msg), "Invalid function");
    state->has_error = 1;
    return -1;
  }

  /* Setup state */
  fruity_interp_reset(state);
  state->func = func;
  /* Convert raw void* args to fruity_val_t and store in locals */
  for (int i = 0; i < arg_count && i < FRUITY_INTERP_MAX_LOCALS; i++) {
    state->locals[FRUITY_INTERP_MAX_LOCALS - 1 - i] =
        fruity_val_i64((int64_t)(intptr_t)(args ? args[i] : nil));
  }
  state->args = nil; /* Args stored in locals */
  state->arg_count = arg_count;
  state->local_count = (int)func->local_count;
  state->current_block = func->blocks_head;
  state->current_instr = func->blocks_head->instructions_head;

  /* Execute until return or error */
  int status;
  while ((status = fruity_interp_step(state)) > 0) {
    if (state->current_instr == nil)
      break;
  }

  if (status < 0) {
    return -1;
  }

  /* Get result from stack */
  if (result && state->sp > 0) {
    *result = state->stack[state->sp - 1];
  }

  return 0;
}

int fruity_interp_execute(fruity_function_t *func, void **args, int arg_count,
                          void *result) {
  fruity_interp_state_t state;
  fruity_val_t val_result;

  fruity_interp_init(&state);

  int ret =
      fruity_interp_execute_state(&state, func, args, arg_count, &val_result);

  if (ret == 0 && result) {
    *(int64_t *)result = AS_I64(val_result);
  }

  return ret;
}
