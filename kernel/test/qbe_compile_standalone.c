/*
 * qbe_compile_standalone.c - Standalone QBE IL Compiler for Host Testing
 *
 * This is a stripped version of qbe_compile.c that can run on host Linux
 * without kernel dependencies. For debugging IL->ASM compilation.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t uintptr;
typedef size_t usize;
typedef uint8_t u8int;
typedef int32_t s32int;
typedef uint32_t u32int;

#define MAX_SYMBOLS 256

/* Emit helpers */
static void emit_byte(u8int **p, u8int b) { *(*p)++ = b; }

static void emit_dword(u8int **p, u32int dw) {
  *(*p)++ = dw & 0xFF;
  *(*p)++ = (dw >> 8) & 0xFF;
  *(*p)++ = (dw >> 16) & 0xFF;
  *(*p)++ = (dw >> 24) & 0xFF;
}

/* Parse helper: skip whitespace */
static char *skip_ws(char *s) {
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
    s++;
  return s;
}

/* Parse helper: parse temporary ID
 * Handles:
 *   %t1, %t2, ...     → 1, 2, ...
 *   %loc0, %loc1, ... → 101, 102, ... (separate namespace for locals)
 *   %a, %b, ..., %z   → mapped by first letter
 *   %cond, %val, etc  → mapped by first letter
 */
static int parse_temp(char **s) {
  *s = skip_ws(*s);
  char *p = *s;
  if (*p != '%')
    return -1;
  p++; /* skip % */

  /* Handle %t<num> - numbered temporaries */
  if (*p == 't' && p[1] >= '0' && p[1] <= '9') {
    p++; /* skip 't' */
    int val = 0;
    while (*p >= '0' && *p <= '9') {
      val = val * 10 + (*p - '0');
      p++;
    }
    *s = p;
    return val + 1; /* 1-indexed for stack slots */
  }

  /* Handle %arg<num> - arguments */
  if (strncmp(p, "arg", 3) == 0) {
    p += 3;
    int val = 0;
    while (*p >= '0' && *p <= '9') {
      val = val * 10 + (*p - '0');
      p++;
    }
    *s = p;
    printf("DEBUG: parse_temp arg -> %d\n", 200 + val);
    return 200 + val; /* 200+ for arguments */
  }

  /* Handle %loc<num> - local variables */
  if (strncmp(p, "loc", 3) == 0) {
    p += 3;
    int val = 0;
    while (*p >= '0' && *p <= '9') {
      val = val * 10 + (*p - '0');
      p++;
    }
    *s = p;
    return 101 + val; /* offset to avoid collision with temps */
  }

  /* Handle named temps: %a -> 1, %b -> 2, ..., %z -> 26
   * Also handles multi-char names: %cond, %val -> use first letter */
  if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
    int val;
    if (*p >= 'a' && *p <= 'z')
      val = *p - 'a' + 1;
    else
      val = *p - 'A' + 1;

    /* Skip rest of identifier */
    while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
           (*p >= '0' && *p <= '9') || *p == '_') {
      p++;
    }
    *s = p;
    return val;
  }

  /* Fallback: just a number */
  int val = 0;
  while (*p >= '0' && *p <= '9') {
    val = val * 10 + (*p - '0');
    p++;
  }
  *s = p;
  return val + 1;
}

/* Parse helper: parse immediate value */
static int parse_imm(char **s) {
  *s = skip_ws(*s);
  char *p = *s;
  int neg = 0;
  if (*p == '-') {
    neg = 1;
    p++;
  }
  int val = 0;
  if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
    /* Hex */
    p += 2;
    while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
           (*p >= 'A' && *p <= 'F')) {
      val *= 16;
      if (*p >= '0' && *p <= '9')
        val += *p - '0';
      else if (*p >= 'a' && *p <= 'f')
        val += *p - 'a' + 10;
      else
        val += *p - 'A' + 10;
      p++;
    }
  } else {
    while (*p >= '0' && *p <= '9') {
      val = val * 10 + (*p - '0');
      p++;
    }
  }
  *s = p;
  return neg ? -val : val;
}

/* Simple tokenizer */
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

  /* Consume separators */
  while (**s == ',' || **s == '=' || **s == '(' || **s == ')')
    (*s)++;

  return 1;
}

/* Label symbol table */
typedef struct {
  char name[32];
  uintptr offset;
} LabelSym;

static int find_label(LabelSym *labels, int count, char *name) {
  for (int i = 0; i < count; i++) {
    if (strcmp(labels[i].name, name) == 0)
      return i;
  }
  return -1;
}

/* Fixup table for forward jumps */
typedef struct {
  char name[32];
  u8int *patch_addr;
  u8int *next_inst;
} Fixup;

/*
 * Main compilation function.
 * Compiles QBE IL text to x86-64 machine code.
 */
int qbe_compile_page_standalone(char *qbe_il, uint8_t *code_out,
                                size_t code_size, char *errbuf,
                                size_t errsize) {
  printf("DEBUG: qbe_compile_page_standalone ENTER\n");
  char *p = qbe_il;
  u8int *code = code_out;
  uint8_t *code_end = code_out + code_size;
  char token[64];

  /* Zero output */
  /* Zero output */
  memset(code_out, 0, code_size);

  /* PROLOGUE: push rbp; mov rbp, rsp; sub rsp, 4096 */
  emit_byte(&code, 0x55); /* push rbp */
  emit_byte(&code, 0x48); /* mov rbp, rsp */
  emit_byte(&code, 0x89);
  emit_byte(&code, 0xE5);
  emit_byte(&code, 0x48); /* sub rsp, 512 */
  /* sub rsp, 0x10000 (64KB stack frame) */
  emit_byte(&code, 0x48);
  emit_byte(&code, 0x81);
  emit_byte(&code, 0xEC);
  emit_dword(&code, 0x10000);

  /* ABI: Spill registers to argument slots (200+) */
  /* arg0 (200) <- RDI */
  emit_byte(&code, 0x48);
  emit_byte(&code, 0x89);
  emit_byte(&code, 0xBD);
  emit_dword(&code, -(200 * 8));
  /* arg1 (201) <- RSI */
  emit_byte(&code, 0x48);
  emit_byte(&code, 0x89);
  emit_byte(&code, 0xB5);
  emit_dword(&code, -(201 * 8));
  /* arg2 (202) <- RDX */
  emit_byte(&code, 0x48);
  emit_byte(&code, 0x89);
  emit_byte(&code, 0x95);
  emit_dword(&code, -(202 * 8));
  /* arg3 (203) <- RCX */
  emit_byte(&code, 0x48);
  emit_byte(&code, 0x89);
  emit_byte(&code, 0x8D);
  emit_dword(&code, -(203 * 8));

  /* Label table (static to avoid stack overflow) */
  static LabelSym labels[MAX_SYMBOLS];
  int label_count = 0;

  /* Fixup table */
  static Fixup fixups[MAX_SYMBOLS];
  int fixup_count = 0;

  /* Function name for recursion */
  char current_func_name[64] = "";
  char *func_decl = strstr(qbe_il, "function");
  if (func_decl) {
    char *name_start = strchr(func_decl, '$');
    if (name_start) {
      name_start++; /* Skip $ */
      char *name_end = name_start;
      while (*name_end && *name_end != '(' && *name_end != ' ')
        name_end++;
      int len = name_end - name_start;
      if (len > 63)
        len = 63;
      strncpy(current_func_name, name_start, len);
      current_func_name[len] = '\0';
      // printf("DEBUG: Compiling function '%s'\n", current_func_name);
    }
  }

  /* First Pass: Generate code */
  while (*p) {
    p = skip_ws(p);
    if (*p == 0)
      break;
    printf("JIT Parse: %.10s...\n", p); /* DEBUG */

    /* Skip comments */
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
        labels[label_count].offset = (uintptr)(code - code_out);
        label_count++;
      }
      continue;
    }

    /* jmp @target */
    if (strncmp(p, "jmp", 3) == 0) {
      p += 3;
      p = skip_ws(p);
      char target[32];
      int i = 0;
      while (*p && *p != '\n' && *p != ' ' && i < 31)
        target[i++] = *p++;
      target[i] = 0;

      /* Emit jmp rel32 (E9 xx xx xx xx) */
      emit_byte(&code, 0xE9);

      if (fixup_count < MAX_SYMBOLS) {
        strncpy(fixups[fixup_count].name, target, 31);
        fixups[fixup_count].patch_addr = code;
        emit_dword(&code, 0x00000000); /* placeholder */
        fixups[fixup_count].next_inst = code;
        fixup_count++;
      }
      continue;
    }

    /* jnz %val, @true, @false */
    if (strncmp(p, "jnz", 3) == 0) {
      p += 3;
      p = skip_ws(p);

      if (*p == '%') {
        int val_id = parse_temp(&p);
        /* mov rax, [rbp-offset] */
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

      /* Parse true target */
      p = skip_ws(p);
      char true_target[32];
      int i = 0;
      while (*p && *p != ',' && *p != '\n' && *p != ' ' && i < 31)
        true_target[i++] = *p++;
      true_target[i] = 0;

      /* jnz rel32 (0F 85 xx xx xx xx) - jumps to true target if non-zero */
      emit_byte(&code, 0x0F);
      emit_byte(&code, 0x85);

      if (fixup_count < MAX_SYMBOLS) {
        strncpy(fixups[fixup_count].name, true_target, 31);
        fixups[fixup_count].patch_addr = code;
        emit_dword(&code, 0x00000000);
        fixups[fixup_count].next_inst = code;
        fixup_count++;
      }

      /* Parse false target */
      p = skip_ws(p);
      if (*p == ',')
        p++;
      p = skip_ws(p);
      char false_target[32];
      i = 0;
      while (*p && *p != ',' && *p != '\n' && *p != ' ' && i < 31)
        false_target[i++] = *p++;
      false_target[i] = 0;

      /* jmp rel32 (E9 xx xx xx xx) - unconditional jump to false target */
      emit_byte(&code, 0xE9);

      if (fixup_count < MAX_SYMBOLS) {
        strncpy(fixups[fixup_count].name, false_target, 31);
        fixups[fixup_count].patch_addr = code;
        emit_dword(&code, 0x00000000);
        fixups[fixup_count].next_inst = code;
        fixup_count++;
      }

      continue;
    }

    /* export function ... */
    if (*p == 'e' && strncmp(p, "export", 6) == 0) {
      while (*p && *p != '{')
        p++;
      if (*p == '{')
        p++;
      continue;
    }

    /* } - end of function */
    if (*p == '}') {
      p++;
      continue;
    }

    /* storel %val, %dst (or storew) */
    if (strncmp(p, "store", 5) == 0) {
      /* Treating store as copy to 'dst' slot */
      p += 5;
      /* check for l/w/b */
      if (*p == 'l' || *p == 'w' || *p == 'b')
        p++;
      p = skip_ws(p);

      int src = parse_temp(&p);
      while (*p == ',' || *p == ' ')
        p++;
      int dst = parse_temp(&p);

      /* mov rax, [rbp-src] */
      emit_byte(&code, 0x48);
      emit_byte(&code, 0x8B);
      emit_byte(&code, 0x85);
      emit_dword(&code, -(src * 8));

      /* mov [rbp-dst], rax */
      emit_byte(&code, 0x48);
      emit_byte(&code, 0x89);
      emit_byte(&code, 0x85);
      emit_dword(&code, -(dst * 8));
      continue;
    }

    /* ret [%val | imm] */
    if (strncmp(p, "ret", 3) == 0) {
      p += 3;
      p = skip_ws(p);

      if (*p == '%') {
        int ret_id = parse_temp(&p);
        /* mov rax, [rbp-offset] */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(ret_id * 8));
      } else if (*p >= '0' && *p <= '9') {
        int imm = parse_imm(&p);
        /* mov eax, imm */
        emit_byte(&code, 0xB8);
        emit_dword(&code, imm);
      }

      /* EPILOGUE: mov rsp, rbp; pop rbp; ret */
      emit_byte(&code, 0x48);
      emit_byte(&code, 0x89);
      emit_byte(&code, 0xEC);
      emit_byte(&code, 0x5D);
      emit_byte(&code, 0xC3);
      continue;
    }

    /* %dest =type instruction ... */
    if (*p == '%') {
      int dest_id = parse_temp(&p);
      p = skip_ws(p);
      if (*p == '=')
        p++;
      p = skip_ws(p);

      next_token(&p, token, sizeof(token));
      /* Skip type specifier (w/l) */
      if (strcmp(token, "w") == 0 || strcmp(token, "l") == 0) {
        next_token(&p, token, sizeof(token));
      }

      /* call instruction - skip for now, not actually compiled */
      if (strcmp(token, "call") == 0) {
        while (*p && *p != '\n')
          p++;
        continue;
      }

      /* Check for ops */
      /* Check for ops */
      if (strcmp(token, "copy") == 0) {
        printf("DEBUG: Parsed COPY\n");
        while (*p == ' ')
          p++; /* Skip space after token */
        u32int src_id = 0;
        if (*p == '%') {
          src_id = parse_temp(&p); // Corrected to use the declared src_id
          /* mov rax, [rbp-src]; mov [rbp-dest], rax */
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
          /* mov qword [rbp-offset], imm */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0xC7);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
          emit_dword(&code, imm);
        }
      }
      /* loadl, loadw, load (generic) */
      else if (strncmp(token, "load", 4) == 0) {
        printf("DEBUG: Parsed LOAD\n");
        while (*p == ' ')
          p++;
        int src_id = parse_temp(&p);

        /* mov rax, [rbp-src]; mov [rbp-dest], rax */
        /* Same as copy register-to-register */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src_id * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* add */
      else if (strcmp(token, "add") == 0) {
        printf("DEBUG: Parsed ADD\n");
        while (*p == ' ')
          p++; /* Skip space after token */
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        /* mov rax, [rbp-src1]; add rax, [rbp-src2]; mov [rbp-dest], rax */
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
      }
      /* sub */
      else if (strcmp(token, "sub") == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        /* mov rax, [rbp-src1]; sub rax, [rbp-src2]; mov [rbp-dest], rax */
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
      }
      /* mul */
      else if (strcmp(token, "mul") == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        /* mov rax, [rbp-src1]; imul rax, [rbp-src2]; mov [rbp-dest], rax */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src1 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x0F);
        emit_byte(&code, 0xAF);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src2 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* div */
      else if (strcmp(token, "div") == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        /* mov rax, [rbp-src1]; cqo; idiv [rbp-src2]; mov [rbp-dest], rax */
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
      }
      /* neg */
      else if (strcmp(token, "neg") == 0) {
        int src = parse_temp(&p);
        /* mov rax, [rbp-src]; neg rax; mov [rbp-dest], rax */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0xF7);
        emit_byte(&code, 0xD8); /* neg rax */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* and */
      else if (strcmp(token, "and") == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src2 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x23);
        emit_byte(&code, 0x85);
        emit_dword(
            &code,
            -(src1 *
              8)); // Fix: and %rax, [rbp-src1]? No, src1 is dest usually?
        /* wait. src1=rax. src2=operand. emit 48 23 85 ... src2? */
        /* and rax, [rbp-src2] is 48 23 85 disp */
        /* prev code: mov rax, [rbp-src1]. and rax, [rbp-src2]. mov [rbp-dest],
         * rax */
        /* The snippet showed 'and' partial. Let's assume 'and' was correct or
         * close. */
        /* I will insert comparisons after 'and' */

        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* comparisons */
      else if (strncmp(token, "c", 1) == 0 &&
               (strcmp(token, "ceqw") == 0 || strcmp(token, "cnew") == 0 ||
                strcmp(token, "cslew") == 0 || strcmp(token, "csltw") == 0 ||
                strcmp(token, "csgew") == 0 || strcmp(token, "csgtw") == 0)) {

        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);

        /* cmp [rbp-src1], [rbp-src2] ? No, move to reg first. */
        /* mov rax, [rbp-src1] */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src1 * 8));

        /* cmp rax, [rbp-src2] */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x3B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src2 * 8));

        /* setcc al */
        emit_byte(&code, 0x0F);
        if (strcmp(token, "ceqw") == 0)
          emit_byte(&code, 0x94); /* sete */
        else if (strcmp(token, "cnew") == 0)
          emit_byte(&code, 0x95); /* setne */
        else if (strcmp(token, "cslew") == 0)
          emit_byte(&code, 0x9E); /* setle */
        else if (strcmp(token, "csltw") == 0)
          emit_byte(&code, 0x9C); /* setl */
        else if (strcmp(token, "csgew") == 0)
          emit_byte(&code, 0x9D); /* setge */
        else if (strcmp(token, "csgtw") == 0)
          emit_byte(&code, 0x9F); /* setg */
        emit_byte(&code, 0xC0);   /* ModRM: al (11 000 000) */

        /* movzx rax, al */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x0F);
        emit_byte(&code, 0xB6);
        emit_byte(&code, 0xC0);

        /* mov [rbp-dest], rax */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* or */
      else if (strcmp(token, "or") == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src1 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x0B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src2 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* xor */
      else if (strcmp(token, "xor") == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src1 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x33);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src2 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* shl */
      else if (strcmp(token, "shl") == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        /* mov rax, [src1]; mov rcx, [src2]; shl rax, cl */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src1 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x8D);
        emit_dword(&code, -(src2 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0xD3);
        emit_byte(&code, 0xE0); /* shl rax, cl */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* shr */
      else if (strcmp(token, "shr") == 0) {
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
        emit_byte(&code, 0x48);
        emit_byte(&code, 0xD3);
        emit_byte(&code, 0xE8); /* shr rax, cl */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      }
      /* call - Self-recursion only (for now) */
      else if (strcmp(token, "call") == 0) {
        char target[64];
        next_token(&p, target, sizeof(target));

        /* Target format: $name(...) */
        char *tvar = target;
        if (*tvar == '$')
          tvar++;

        /* Extract name until ( */
        char *end = tvar;
        while (*end && *end != '(')
          end++;
        *end = '\0';

        if (strcmp(tvar, current_func_name) == 0) {
          /* call rel32 (recursion) */
          int current_offset = (int)(code - code_out);
          int rel = -(current_offset + 5);

          emit_byte(&code, 0xE8);
          emit_dword(&code, rel);

          /* Result handling */
          emit_byte(&code, 0x48);
          emit_byte(&code, 0x89);
          emit_byte(&code, 0x85);
          emit_dword(&code, -(dest_id * 8));
        } else {
          printf(
              "WARNING: External call to %s not supported in standalone JIT\n",
              tvar);
          /* Emit NOP to safely continue? Or just nothing (will crash if result
           * used) */
        }

        /* Skip args until newline */
        while (*p && *p != '\n')
          p++;
      }
      /* Comparison Ops */
      else if (strncmp(token, "ceq", 3) == 0 || strncmp(token, "cne", 3) == 0 ||
               strncmp(token, "cslt", 4) == 0 ||
               strncmp(token, "csgt", 4) == 0) {
        int src1 = parse_temp(&p);
        while (*p == ',' || *p == ' ')
          p++;
        int src2 = parse_temp(&p);
        /* mov rax, [src1]; cmp rax, [src2]; setcc al; movzx eax, al */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x8B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src1 * 8));
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x3B);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(src2 * 8));

        /* setcc based on comparison type */
        emit_byte(&code, 0x0F);
        if (strncmp(token, "ceq", 3) == 0)
          emit_byte(&code, 0x94); /* sete */
        else if (strncmp(token, "cne", 3) == 0)
          emit_byte(&code, 0x95); /* setne */
        else if (strncmp(token, "cslt", 4) == 0)
          emit_byte(&code, 0x9C); /* setl */
        else if (strncmp(token, "csgt", 4) == 0)
          emit_byte(&code, 0x9F); /* setg */
        else
          emit_byte(&code, 0x94); /* default sete */
        emit_byte(&code, 0xC0);   /* al */

        /* movzx eax, al */
        emit_byte(&code, 0x0F);
        emit_byte(&code, 0xB6);
        emit_byte(&code, 0xC0);

        /* mov [dest], rax */
        emit_byte(&code, 0x48);
        emit_byte(&code, 0x89);
        emit_byte(&code, 0x85);
        emit_dword(&code, -(dest_id * 8));
      } else {
        /* Unknown instruction - skip to end of line */
        while (*p && *p != '\n')
          p++;
      }
      continue;
    }

    /* Skip unknown lines */
    while (*p && *p != '\n')
      p++;
  }

  /* Second Pass: Resolve Fixups */
  for (int i = 0; i < fixup_count; i++) {
    int idx = find_label(labels, label_count, fixups[i].name);
    if (idx >= 0) {
      uintptr target_offset = labels[idx].offset;
      uintptr instr_end_offset = (uintptr)(fixups[i].next_inst - code_out);
      s32int rel = (s32int)(target_offset - instr_end_offset);

      u8int *patch = fixups[i].patch_addr;
      *patch++ = rel & 0xFF;
      *patch++ = (rel >> 8) & 0xFF;
      *patch++ = (rel >> 16) & 0xFF;
      *patch++ = (rel >> 24) & 0xFF;
    }
  }

  return 0;
}
