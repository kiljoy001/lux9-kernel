/* qbe_compile.c - Minimal x86-64 Code Generator
 *
 * Implements a simple stack-based JIT for QBE IL.
 * All QBE temporaries (%tN) are mapped to stack slots [rbp - N*8].
 */

/* #include "u.h" removed */
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
extern long strtol(char *, char **, int);
extern int snprint(char *, int, char *, ...);

#include "../9front-pc64/mem.h"
#include "../include/dat.h"
#include "../include/fns.h"
#define USED(x) (void)(x)

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

/* Stub pebble_alloc for CLR runtime - TODO: full Pebble integration */
static void *pebble_alloc_stub(ulong size, void *type_hint) {
  USED(type_hint);
  return xalloc(size);
}
#define pebble_alloc pebble_alloc_stub

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

/* Simple symbol resolver */
static void *resolve_kernel_symbol(char *name) {
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
        } else if (strcmp(token, "call") == 0) {
          /* call $func(...) */
          /* Parse function name */
          p = skip_ws(p);
          if (*p == '$')
            p++; /* Skip $ */

          char func_name[64];
          int i = 0;
          while (*p && *p != '(' && *p != ' ' && i < 63) {
            func_name[i++] = *p++;
          }
          func_name[i] = 0;

          void *addr = resolve_kernel_symbol(func_name);
          if (addr) {
            /* mov rax, addr */
            emit_byte(&code, 0x48);
            emit_byte(&code, 0xB8);
            emit_dword(&code, (uintptr)addr & 0xFFFFFFFF); /* Low 32 */
            emit_dword(&code, (uintptr)addr >> 32);        /* High 32 */

            /* call rax */
            emit_byte(&code, 0xFF);
            emit_byte(&code, 0xD0);
          }

          /* Consume rest of line (args, etc.) */
          while (*p && *p != '\n')
            p++;
        }
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
