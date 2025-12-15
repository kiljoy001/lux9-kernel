/* qbe_compile.c - Minimal x86-64 Code Generator
 *
 * Implements a simple stack-based JIT for QBE IL.
 * All QBE temporaries (%tN) are mapped to stack slots [rbp - N*8].
 */

#include "../9front-pc64/mem.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/portlib.h"
#include "../include/u.h"

#include "qbe_compile.h"

/* Symbol table for labels */
typedef struct Symbol {
  char name[32];
  uintptr offset; /* Offset from start of page */
  struct Symbol *next;
} Symbol;

static Symbol *symbols = nil;

static void add_symbol(char *name, uintptr offset) {
  Symbol *s = (Symbol *)0; /* Using malloc or static array? Kernel env... */
  /* For this constrained environment, we'll use a static array or simple bump
     allocator if possible. But wait, we can't easily malloc. Let's use a small
     static array for the prototype. */
}

/* Actually, let's use a fixed size array for now inside the compile function or
 * static */
#define MAX_SYMBOLS 64
typedef struct {
  char name[32];
  uintptr offset;
} LabelSym;

static int find_label(LabelSym *syms, int count, char *name) {
  for (int i = 0; i < count; i++) {
    if (strcmp(syms[i].name, name) == 0)
      return i;
  }
  return -1;
}

/* x86-64 encoding helpers */
static void emit_byte(u8int **p, u8int b) { *(*p)++ = b; }
static void emit_dword(u8int **p, u32int dw) {
  emit_byte(p, dw & 0xFF);
  emit_byte(p, (dw >> 8) & 0xFF);
  emit_byte(p, (dw >> 16) & 0xFF);
  emit_byte(p, (dw >> 24) & 0xFF);
}
static void emit_int32(u8int **p, s32int dw) { emit_dword(p, (u32int)dw); }

/* Parse helper: skip whitespace */
static char *skip_ws(char *s) {
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
    s++;
  return s;
}

/* Parse helper: parse temporary ID (%t123 -> 123) */
static int parse_temp(char **s) {
  *s = skip_ws(*s);
  char *p = *s;
  if (*p != '%')
    return -1;
  p++; /* skip % */
  if (*p == 't' || *p == 'r' || *p == 'p' ||
      (*p == 'p' && *(p + 1) == 't' && *(p + 2) == 'r')) {
    /* Handle %t, %r, %ptr prefixes roughly same mapping */
    while (*p >= 'a' && *p <= 'z')
      p++;
  }

  int id = strtol(p, &p, 10);
  *s = p;
  return id;
}

/* Parse helper: parse immediate */
static int parse_imm(char **s) {
  int val = strtol(*s, s, 10);
  return val;
}

/* Simple tokenizer to find next word */
static int next_token(char **s, char *buf, int len) {
  *s = skip_ws(*s);
  if (**s == 0)
    return 0;

  int i = 0;
  while (**s && **s != ' ' && **s != '\t' && **s != '\n' && **s != ',' &&
         **s != '=' && **s != '(' && **s != ')') {
    if (i < len - 1)
      buf[i++] = *(*s)++;
    else
      (*s)++;
  }
  buf[i] = 0;

  /* Consume separators if needed */
  while (**s == ',' || **s == '=' || **s == '(' || **s == ')')
    (*s)++;

  return 1;
}

/* ... existing helper functions ... */

/* External kernel symbols for runtime linking */
extern void clr_console_write(void *);
extern void clr_console_writeline(void *);

/* CLR Memory Allocation
 *
 * TODO: Integrate with Pebble system once CLR has proper white token
 * verification infrastructure. Currently uses xalloc directly.
 *
 * To properly integrate with Pebble:
 * 1. CLR needs to request white tokens via pebble_issue_white()
 * 2. Verify white tokens via pebble_white_verify()
 * 3. Only then call pebble_black_alloc() with proper UserCapability
 */
static void *pebble_alloc_clr(ulong size, void *type_hint) {
  USED(type_hint);
  /* Use xalloc until CLR has proper Pebble integration */
  return xalloc(size);
}
#define pebble_alloc pebble_alloc_clr

/* System.Object / System.String Internals */
extern int clr_object_gethashcode(void *);
extern void *clr_object_gettype(void *);
extern void *clr_string_concat2(void *, void *);
extern void *clr_string_concat3(void *, void *, void *);
extern int clr_string_equals(void *, void *);
extern int clr_string_get_length(void *);
extern ushort clr_string_get_chars(void *, int);

/* System.Array Internals */
extern int clr_array_get_length(void *);
extern void *clr_array_getvalue(void *, int);
extern void clr_array_setvalue(void *, void *, int);

extern int clr_environment_tickcount(void);
extern void clr_environment_exit(int);
extern void clr_environment_failfast(void *);

extern void clr_monitor_enter(void *);
extern void clr_monitor_exit(void *);

/* System.P9.P9Internal - 9P operations */
extern unsigned int clr_p9_attach(const char *path);
extern int clr_p9_read(unsigned int fid, void *buffer, int offset, int count,
                       long long position);
extern int clr_p9_write(unsigned int fid, void *buffer, int offset, int count,
                        long long position);
extern void clr_p9_clunk(unsigned int fid);
extern long long clr_p9_stat(unsigned int fid);

/* BCL Helpers (clr_bcl_helpers.c) */
extern long long clr_datetime_now(void);
extern int clr_datetime_tickcount(void);
extern int clr_bigint_add(u32int *left, int leftLen, u32int *right,
                          int rightLen, u32int *result);
extern int clr_bigint_sub(u32int *left, int leftLen, u32int *right,
                          int rightLen, u32int *result);
extern int clr_bigint_mul(u32int *left, int leftLen, u32int *right,
                          int rightLen, u32int *result);
extern int clr_bigint_div_small(u32int *dividend, int dividendLen,
                                u32int divisor, u32int *quotient,
                                u32int *remainder);
extern void clr_span_clear(void *ptr, int elementSize, int count);
extern void clr_span_copy(void *dst, void *src, int elementSize, int count);
extern int clr_decimal_add96(u32int *a, u32int *b, u32int *result);
extern u32int clr_decimal_mul32(u32int *a, u32int multiplier, u32int *result);

/* CLR Runtime Internals */
extern void *clr_string_from_literal(u32int);
extern void *clr_newobj(u32int);
extern void *clr_newarr(u32int, u32int);

/* Simple symbol resolver */
static void *resolve_kernel_symbol(char *name) {
  /* Runtime Helpers */
  if (strcmp(name, "clr_string_from_literal") == 0)
    return (void *)clr_string_from_literal;
  if (strcmp(name, "clr_newobj") == 0)
    return (void *)clr_newobj;
  if (strcmp(name, "clr_newarr") == 0)
    return (void *)clr_newarr;

  /* Console */
  if (strcmp(name, "System_Console_Internal_Write") == 0)
    return (void *)clr_console_write;
  if (strcmp(name, "System_Console_Internal_WriteLine") == 0)
    return (void *)clr_console_writeline;

  /* Environment */
  if (strcmp(name, "System_Environment_get_TickCount") == 0)
    return (void *)clr_environment_tickcount;
  if (strcmp(name, "System_Environment_Exit") == 0)
    return (void *)clr_environment_exit;
  if (strcmp(name, "System_Environment_FailFast") == 0)
    return (void *)clr_environment_failfast;

  /* Threading */
  if (strcmp(name, "System_Threading_Monitor_Enter") == 0)
    return (void *)clr_monitor_enter;
  if (strcmp(name, "System_Threading_Monitor_Exit") == 0)
    return (void *)clr_monitor_exit;

  /* Allocation */
  if (strcmp(name, "lux_alloc") == 0)
    return (void *)pebble_alloc;

  /* System.Object */
  if (strcmp(name, "System_Object_GetHashCode") == 0)
    return (void *)clr_object_gethashcode;
  if (strcmp(name, "System_Object_GetType") == 0)
    return (void *)clr_object_gettype;

  /* System.String */
  if (strcmp(name, "System_String_Internal_Concat2") == 0)
    return (void *)clr_string_concat2;
  if (strcmp(name, "System_String_Internal_Concat3") == 0)
    return (void *)clr_string_concat3;
  if (strcmp(name, "System_String_Equals") == 0)
    return (void *)clr_string_equals;
  if (strcmp(name, "System_String_get_Length") == 0)
    return (void *)clr_string_get_length;
  if (strcmp(name, "System_String_get_Chars") == 0)
    return (void *)clr_string_get_chars;

  /* System.Array */
  if (strcmp(name, "System_Array_get_Length") == 0)
    return (void *)clr_array_get_length;
  if (strcmp(name, "System_Array_GetValue") == 0)
    return (void *)clr_array_getvalue;
  if (strcmp(name, "System_Array_SetValue") == 0)
    return (void *)clr_array_setvalue;

  /* System.P9.P9Internal - 9P filesystem operations */
  if (strcmp(name, "System_P9_P9Internal_Attach") == 0)
    return (void *)clr_p9_attach;
  if (strcmp(name, "System_P9_P9Internal_Read") == 0)
    return (void *)clr_p9_read;
  if (strcmp(name, "System_P9_P9Internal_Write") == 0)
    return (void *)clr_p9_write;
  if (strcmp(name, "System_P9_P9Internal_Clunk") == 0)
    return (void *)clr_p9_clunk;
  if (strcmp(name, "System_P9_P9Internal_Stat") == 0)
    return (void *)clr_p9_stat;

  /* BCL Helpers */
  if (strcmp(name, "System_DateTime_Internal_GetNow") == 0)
    return (void *)clr_datetime_now;
  if (strcmp(name, "System_Environment_get_TickCount_Internal") == 0)
    return (void *)clr_datetime_tickcount;
  if (strcmp(name, "System_Numerics_BigInteger_Internal_Add") == 0)
    return (void *)clr_bigint_add;
  if (strcmp(name, "System_Numerics_BigInteger_Internal_Sub") == 0)
    return (void *)clr_bigint_sub;
  if (strcmp(name, "System_Numerics_BigInteger_Internal_Mul") == 0)
    return (void *)clr_bigint_mul;
  if (strcmp(name, "System_Numerics_BigInteger_Internal_DivSmall") == 0)
    return (void *)clr_bigint_div_small;
  if (strcmp(name, "System_Span_Internal_Clear") == 0)
    return (void *)clr_span_clear;
  if (strcmp(name, "System_Span_Internal_Copy") == 0)
    return (void *)clr_span_copy;
  if (strcmp(name, "System_Decimal_Internal_Add96") == 0)
    return (void *)clr_decimal_add96;
  if (strcmp(name, "System_Decimal_Internal_Mul32") == 0)
    return (void *)clr_decimal_mul32;

  return nil;
}

/*
 * Compile QBE IL to native x86-64 code
 */
int qbe_compile_page(uintptr qbe_page, uintptr asm_page, char *errorbuf,
                     usize errorbuf_size) {
  void *qbe_vaddr, *asm_vaddr;
  u8int *code;
  char *p;
  char token[64];

  /* Temp storage for finding max temp usage */
  int max_temp = 0;

  USED(errorbuf);
  USED(errorbuf_size);

  /* Convert physical addresses to kernel virtual */
  qbe_vaddr = KADDR(qbe_page);
  asm_vaddr = KADDR(asm_page);

  p = (char *)qbe_vaddr;
  code = (u8int *)asm_vaddr;

  /* Zero output */
  memset(asm_vaddr, 0, BY2PG);

  /* PROLOGUE */
  /* We don't know stack size yet, but let's assume max 64 temps = 512 bytes for
   * now */
  /* push rbp */
  emit_byte(&code, 0x55);
  /* mov rbp, rsp */
  emit_byte(&code, 0x48);
  emit_byte(&code, 0x89);
  emit_byte(&code, 0xE5);
  /* sub rsp, 512 */
  emit_byte(&code, 0x48);
  emit_byte(&code, 0x81);
  emit_byte(&code, 0xEC);
  emit_dword(&code, 512);

  /* Label table */
  LabelSym labels[MAX_SYMBOLS];
  int label_count = 0;

  /* Fixup table for forward jumps */
  struct Fixup {
    char name[32];
    u8int *patch_addr; /* Where to write the relative offset */
    u8int *next_inst;  /* Address of instruction AFTER the jump */
  } fixups[MAX_SYMBOLS];
  int fixup_count = 0;

  /* First Pass: Generate code, record labels, record fixups */
  while (*p) {
    char *line_start = p;
    p = skip_ws(p);
    if (*p == 0)
      break;

    if (*p == '#') {
      while (*p && *p != '\n')
        p++;
      continue;
    }

    /* Label definition: @bb1 */
    if (*p == '@') {
      int i = 0;
      char name[32];
      while (*p && *p != '\n' && *p != ' ' && i < 31)
        name[i++] = *p++;
      name[i] = 0;

      if (label_count < MAX_SYMBOLS) {
        strncpy(labels[label_count].name, name, 31);
        labels[label_count].offset =
            (uintptr)(code - (u8int *)asm_vaddr); /* Current offset */
        label_count++;
      }
      continue;
    }

    if (strncmp(p, "jmp", 3) == 0) {
      /* jmp @target */
      p += 3;
      p = skip_ws(p);
      char target[32];
      int i = 0;
      while (*p && *p != '\n' && *p != ' ' && i < 31)
        target[i++] = *p++;
      target[i] = 0;

      /* Emit jmp rel32 (E9 md) */
      emit_byte(&code, 0xE9);

      /* Record fixup */
      if (fixup_count < MAX_SYMBOLS) {
        strncpy(fixups[fixup_count].name, target, 31);
        fixups[fixup_count].patch_addr = code;
        /* Placeholder */
        emit_dword(&code, 0x00000000);
        fixups[fixup_count].next_inst = code;
        fixup_count++;
      }
    } else if (strncmp(p, "jnz", 3) == 0) {
      /* jnz %val, @true, @false -- Simplified: just handle first target for now
       * or assume structure */
      /* For this task, let's parse: jnz %val, @target, @fallthrough */
      p += 3;
      p = skip_ws(p);

      /* Parse condition var (ignored for now, assuming flags set by previous
       * cmp/test... wait QBE doesn't have explicit cmp for this? */
      /* Actually QBE `jnz` takes a value. We need to `test` it. */
      if (*p == '%') {
        int val_id = parse_temp(&p);
        /* test val, val */
        /* mov rax, [rbp-...] */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(val_id * 8));
        /* test rax, rax */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x85);
        emit_byte(&code, 0xC0);
      }

      p = skip_ws(p);
      if (*p == ',')
        p++;
      p = skip_ws(p);

      char target[32];
      int i = 0;
      while (*p && *p != ',' && *p != ' ' && i < 31)
        target[i++] = *p++;
      target[i] = 0;

      /* jne rel32 (0F 85 md) */
      emit_byte(&code, 0x0F);
      emit_byte(&code, 0x85);

      if (fixup_count < MAX_SYMBOLS) {
        strncpy(fixups[fixup_count].name, target, 31);
        fixups[fixup_count].patch_addr = code;
        emit_dword(&code, 0x00000000);
        fixups[fixup_count].next_inst = code;
        fixup_count++;
      }

      /* Consume @false target (fallthrough assumption or explicit jump?) */
      /* Simplification: ignore second arg, assume fallthrough logic handled by
       * IR structure */
      while (*p && *p != '\n')
        p++;

    } else if (*p == 'e' && strncmp(p, "export", 6) == 0) {
      while (*p && *p != '{')
        p++;
      if (*p == '{')
        p++;
      continue;
    } else if (*p == '}') {
      p++;
      continue;
    } else

      /* ... Existing instruction parsing ... */
      if (*p == '%') {
        int dest_id = parse_temp(&p);
        p = skip_ws(p);
        if (*p == '=')
          p++; /* consume = */
        p = skip_ws(p);

        next_token(&p, token, sizeof(token));
        if (strcmp(token, "w") == 0 || strcmp(token, "l") == 0) {
          next_token(&p, token, sizeof(token));
        }

        if (strcmp(token, "copy") == 0) {
          /* ... existing copy ... */
          if (*p == '%') {
            int src_id = parse_temp(&p);
            emit_byte(&code, 0x48);
            emit_byte(&code, 0x8B);
            emit_byte(&code, 0x85);
            emit_dword(&code, -(src_id * 8));
            emit_byte(&code, 0x48);
            emit_byte(&code, 0x89);
            emit_byte(&code, 0x85);
            emit_dword(&code, -(dest_id * 8));
          } else {
            int imm = parse_imm(&p);
            emit_byte(&code, 0x48);
            emit_byte(&code, 0xC7);
            emit_byte(&code, 0x85);
            emit_dword(&code, -(dest_id * 8));
            emit_dword(&code, imm);
          }
        } else if (strcmp(token, "add") == 0) {
          /* ... existing add ... */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x03);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "sub") == 0) { /* NEW: sub support */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x2B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "mul") == 0) {
          /* mul: imul rax, [rbp-src2*8] */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* imul rax, [rbp-src2*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xAF);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "and") == 0) {
          /* and %a, %b */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* and rax, [rbp-src2*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x23);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "or") == 0) {
          /* or %a, %b */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* or rax, [rbp-src2*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x0B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "xor") == 0) {
          /* xor %a, %b */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* xor rax, [rbp-src2*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x33);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "rem") == 0 || strcmp(token, "urem") == 0) {
          /* rem - modulo using idiv, result in rdx */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* xor rdx, rdx (for unsigned) or cqo (for signed) - use cqo */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x99); /* cqo */
          /* idiv [rbp-src2*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xF7);
          emit_byte(&code, 0xBD);
          emit_dword(&code, -(src2 * 8));
          /* mov [rbp-dest*8], rdx (remainder) */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x95);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "div") == 0) {
          /* signed division */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x99); /* cqo */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xF7);
          emit_byte(&code, 0xBD);
          emit_dword(&code, -(src2 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "udiv") == 0) {
          /* div: idiv (signed) or div (unsigned) - for now alias to idiv logic
           */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x99);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xF7);
          emit_byte(&code, 0xBD);
          emit_dword(&code, -(src2 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));

          /* alloc8 N - already handled by 512-byte frame, just use slot */
          /* The dest temp IS the allocated storage (rbp - dest*8) */
          /* lea rax, [rbp - dest*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8D);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
          /* Store the address in the slot itself */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
          /* Skip the size argument */
          while (*p && *p != '\n' && *p != ' ')
            p++;
        } else if (strcmp(token, "ceqw") == 0 || strcmp(token, "ceql") == 0) {
          /* ceqw %a, %b - compare equal */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* cmp rax, [rbp-src2*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* sete al */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x94);
          emit_byte(&code, 0xC0);
          /* movzx eax, al */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "cnew") == 0 || strcmp(token, "cnel") == 0) {
          /* cnew %a, %b - compare not equal */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* setne al */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x95);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "loadl") == 0) {
          /* loadl %dest, %ptr - load 64-bit from memory */
          int ptr_id = parse_temp(&p);
          /* mov rax, [rbp-ptr*8] ; get pointer */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(ptr_id * 8));
          /* mov rax, [rax] ; load from pointer */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x00);
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "loadw") == 0) {
          /* loadw %dest, %ptr - load 32-bit from memory */
          int ptr_id = parse_temp(&p);
          /* mov rax, [rbp-ptr*8] ; get pointer */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(ptr_id * 8));
          /* mov eax, [rax] ; load 32-bit from pointer */
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x00);
          /* mov [rbp-dest*8], rax (zero-extended) */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "csltw") == 0 || strcmp(token, "csltl") == 0) {
          /* csltw - signed less than */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* setl al (signed less) */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x9C);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "csgtw") == 0 || strcmp(token, "csgtl") == 0) {
          /* csgtw - signed greater than */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* setg al (signed greater) */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x9F);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "cslew") == 0 || strcmp(token, "cslel") == 0) {
          /* cslew - signed less or equal */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* setle al */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x9E);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "csgew") == 0 || strcmp(token, "csgel") == 0) {
          /* csgew - signed greater or equal */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* setge al */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x9D);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "cultw") == 0 || strcmp(token, "cultl") == 0) {
          /* cultw - unsigned less than */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* setb al (unsigned below) */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x92);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "cugtw") == 0 || strcmp(token, "cugtl") == 0) {
          /* cugtw - unsigned greater than */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x3B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src2 * 8));
          /* seta al (unsigned above) */
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0x97);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x0F);
          emit_byte(&code, 0xB6);
          emit_byte(&code, 0xC0);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "shl") == 0) {
          /* shl %a, %b - shift left */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* mov rcx, [rbp-src2*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x8D);
          emit_dword(&code, -(src2 * 8));
          /* shl rax, cl */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xD3);
          emit_byte(&code, 0xE0);
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "shr") == 0) {
          /* shr %a, %b - logical shift right */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x8D);
          emit_dword(&code, -(src2 * 8));
          /* shr rax, cl */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xD3);
          emit_byte(&code, 0xE8);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "sar") == 0) {
          /* sar %a, %b - arithmetic shift right */
          int src1 = parse_temp(&p);
          while (*p == ',' || *p == ' ')
            p++;
          int src2 = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x8D);
          emit_dword(&code, -(src2 * 8));
          /* sar rax, cl */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xD3);
          emit_byte(&code, 0xF8);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "neg") == 0) {
          /* neg %a - negate */
          int src1 = parse_temp(&p);
          /* mov rax, [rbp-src1*8] */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* neg rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xF7);
          emit_byte(&code, 0xD8);
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "extsw") == 0) {
          /* extsw - extend signed word to long */
          int src1 = parse_temp(&p);
          /* mov eax, [rbp-src1*8] ; load 32-bit */
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* cdqe ; sign-extend eax to rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x98);
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "extuw") == 0) {
          /* extuw - extend unsigned word to long (zero-extend) */
          int src1 = parse_temp(&p);
          /* mov eax, [rbp-src1*8] ; automatically zero-extended */
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(src1 * 8));
          /* mov [rbp-dest*8], rax */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else if (strcmp(token, "call") == 0) {
          /* call $func(arg1, arg2, ...) with AMD64 ABI */
          /* AMD64 ABI: first 6 integer args in rdi, rsi, rdx, rcx, r8, r9 */
          p = skip_ws(p);
          if (*p == '$')
            p++; /* Skip $ */

          char func_name[64];
          int i = 0;
          while (*p && *p != '(' && *p != ' ' && i < 63) {
            func_name[i++] = *p++;
          }
          func_name[i] = 0;

          /* Parse arguments inside (...) */
          int arg_temps[6] = {0};
          int arg_count = 0;

          p = skip_ws(p);
          if (*p == '(') {
            p++; /* Skip ( */
            while (*p && *p != ')' && arg_count < 6) {
              p = skip_ws(p);
              if (*p == '%') {
                arg_temps[arg_count++] = parse_temp(&p);
              } else if (*p >= '0' && *p <= '9') {
                /* Immediate - store as negative temp ID (special marker) */
                arg_temps[arg_count++] =
                    -parse_imm(&p) - 1; /* -1 to distinguish from temp 0 */
              }
              p = skip_ws(p);
              if (*p == ',')
                p++;
            }
            if (*p == ')')
              p++;
          }

          /* Emit argument moves per AMD64 ABI */
          /* rdi=arg0, rsi=arg1, rdx=arg2, rcx=arg3, r8=arg4, r9=arg5 */
          for (int a = 0; a < arg_count; a++) {
            int temp_id = arg_temps[a];
            if (temp_id < 0) {
              /* Immediate value: mov reg, imm */
              int imm = -(temp_id + 1);
              switch (a) {
              case 0: /* mov rdi, imm */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0xBF);
                emit_dword(&code, imm);
                emit_dword(&code, 0);
                break;
              case 1: /* mov rsi, imm */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0xBE);
                emit_dword(&code, imm);
                emit_dword(&code, 0);
                break;
              case 2: /* mov rdx, imm */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0xBA);
                emit_dword(&code, imm);
                emit_dword(&code, 0);
                break;
              case 3: /* mov rcx, imm */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0xB9);
                emit_dword(&code, imm);
                emit_dword(&code, 0);
                break;
              case 4: /* mov r8, imm */
                emit_byte(&code, 0x49);
                emit_byte(&code, 0xB8);
                emit_dword(&code, imm);
                emit_dword(&code, 0);
                break;
              case 5: /* mov r9, imm */
                emit_byte(&code, 0x49);
                emit_byte(&code, 0xB9);
                emit_dword(&code, imm);
                emit_dword(&code, 0);
                break;
              }
            } else {
              /* Load from stack slot, then move to arg register */
              /* First load to rax: mov rax, [rbp-temp*8] */
              emit_byte(&code, 0x48);
              emit_byte(&code, 0x8B);
              emit_byte(&code, 0x85);
              emit_dword(&code, -(temp_id * 8));

              /* Then move rax to appropriate register */
              switch (a) {
              case 0: /* mov rdi, rax */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0x89);
                emit_byte(&code, 0xC7);
                break;
              case 1: /* mov rsi, rax */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0x89);
                emit_byte(&code, 0xC6);
                break;
              case 2: /* mov rdx, rax */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0x89);
                emit_byte(&code, 0xC2);
                break;
              case 3: /* mov rcx, rax */
                emit_byte(&code, 0x48);
                emit_byte(&code, 0x89);
                emit_byte(&code, 0xC1);
                break;
              case 4: /* mov r8, rax */
                emit_byte(&code, 0x49);
                emit_byte(&code, 0x89);
                emit_byte(&code, 0xC0);
                break;
              case 5: /* mov r9, rax */
                emit_byte(&code, 0x49);
                emit_byte(&code, 0x89);
                emit_byte(&code, 0xC1);
                break;
              }
            }
          }

          void *addr = resolve_kernel_symbol(func_name);
          if (addr) {
            /* mov rax, addr (64-bit immediate) */
            emit_byte(&code, 0x48);
            emit_byte(&code, 0xB8);
            emit_dword(&code, (uintptr)addr & 0xFFFFFFFF); /* Low 32 */
            emit_dword(&code, (uintptr)addr >> 32);        /* High 32 */

            /* call rax */
            emit_byte(&code, 0xFF);
            emit_byte(&code, 0xD0);

            /* Store return value (in rax) to dest temp */
            emit_byte(&code, 0x48);
            emit_byte(&code, 0x89);
            emit_byte(&code, 0x85);
            emit_dword(&code, -(dest_id * 8));
          }

          /* Consume rest of line */
          while (*p && *p != '\n')
            p++;
        }
      } else if (strncmp(p, "storel", 6) == 0 &&
                 (p[6] == ' ' || p[6] == '\t')) {
        p += 6;
        p = skip_ws(p);
        /* storel %val, %ptr */
        int val_id = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int ptr_id = parse_temp(&p);

        /* mov rax, [rbp-ptr*8] ; get pointer */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(ptr_id * 8));
        /* mov rcx, [rbp-val*8] ; get value */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x8D);
        emit_dword(&code, -(val_id * 8));
        /* mov [rax], rcx ; store */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x08);
      } else if (strncmp(p, "storew", 6) == 0 &&
                 (p[6] == ' ' || p[6] == '\t')) {
        p += 6;
        p = skip_ws(p);
        /* storew %val, %ptr */
        int val_id = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int ptr_id = parse_temp(&p);

        /* mov rax, [rbp-ptr*8] ; get pointer */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(ptr_id * 8));
        /* mov ecx, [rbp-val*8] ; get value (32-bit) */
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x8D);
        emit_dword(&code, -(val_id * 8));
        /* mov [rax], ecx ; store 32-bit */
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x08);
      } else if (strncmp(p, "ret", 3) == 0) {
        /* ... existing ret ... */
        p += 3;
        p = skip_ws(p);
        if (*p == '%') {
          int ret_id = parse_temp(&p);
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x8B);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(ret_id * 8));
        } else {
          int imm = parse_imm(&p);
          emit_byte(&code, 0xB8);
          emit_dword(&code, imm);
        }
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0xEC);
        emit_byte(&code, 0x5D);
        emit_byte(&code, 0xC3);
      } else {
        while (*p && *p != '\n')
          p++;
      }
  }

  /* Second Pass: Resolve Fixups */
  for (int i = 0; i < fixup_count; i++) {
    int idx = find_label(labels, label_count, fixups[i].name);
    if (idx >= 0) {
      uintptr target_offset = labels[idx].offset;
      uintptr instr_end_offset =
          (uintptr)(fixups[i].next_inst - (u8int *)asm_vaddr);
      s32int rel = (s32int)(target_offset - instr_end_offset);

      /* Write rel32 to patch_addr */
      u8int *patch = fixups[i].patch_addr;
      *patch++ = rel & 0xFF;
      *patch++ = (rel >> 8) & 0xFF;
      *patch++ = (rel >> 16) & 0xFF;
      *patch++ = (rel >> 24) & 0xFF;
    }
  }

  return 0;
}
