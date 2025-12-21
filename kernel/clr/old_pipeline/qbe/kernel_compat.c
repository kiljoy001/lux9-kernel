/* kernel_compat.c - Compatibility layer for QBE in kernel
 * Maps GNU libc functions to Plan 9 equivalents
 */

#include "dat.h"
#include "exchange_io.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

/* QBE global variables - Defined weakly so QBE code can link */
/* These will be properly initialized by qbe_kernel_wrapper.c */
char debug['Z' + 1]; /* Debug flags array */

/* Map vsnprintf to Plan 9 vsnprint */
int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args) {
  return vsnprint(buf, size, (char *)fmt, args);
}

/* Map snprintf to Plan 9 snprint */
int snprintf(char *buf, unsigned long size, const char *fmt, ...) {
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprint(buf, size, (char *)fmt, args);
  va_end(args);
  return n;
}

/* Map sprintf to snprintf (unsafe but needed by QBE) */
int sprintf(char *buf, const char *fmt, ...) {
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprint(buf, 4096, (char *)fmt, args); /* Assume 4KB buffer */
  va_end(args);
  return n;
}

/* calloc - allocate and zero memory */
void *calloc(unsigned long nmemb, unsigned long size) {
  unsigned long total = nmemb * size;
  void *p = xalloc(total);
  if (p != nil)
    memset(p, 0, total);
  return p;
}

/* strtod - not implemented (kernel has no FPU support with -mno-sse)
 * If the linker complains about this, it means code is trying to use floating
 * point. Leaving undefined to catch such cases. */

/* __assert_fail - assertion failure */
void __assert_fail(const char *assertion, const char *file, unsigned int line,
                   const char *function) {
  panic("QBE assertion failed: %s at %s:%d in %s", assertion, file, line,
        function);
}

/* exchange_stderr - global stderr stream (stub, discards output) */
ExchangeFILE *exchange_stderr = nil;

/* setjmp/longjmp - minimal implementation for x86-64 error handling */
int setjmp(long env[8]) {
  __asm__ volatile("movq %%rbx, 0(%0)\n\t"   /* Save rbx */
                   "movq %%rbp, 8(%0)\n\t"   /* Save rbp */
                   "movq %%r12, 16(%0)\n\t"  /* Save r12 */
                   "movq %%r13, 24(%0)\n\t"  /* Save r13 */
                   "movq %%r14, 32(%0)\n\t"  /* Save r14 */
                   "movq %%r15, 40(%0)\n\t"  /* Save r15 */
                   "movq %%rsp, 48(%0)\n\t"  /* Save rsp */
                   "movq (%%rsp), %%rax\n\t" /* Get return address */
                   "movq %%rax, 56(%0)\n\t"  /* Save return address */
                   :                         /* no outputs */
                   : "r"(env)
                   : "rax", "memory");
  return 0;
}

void longjmp(long env[8], int val) {
  int retval = (val == 0) ? 1 : val;
  __asm__ volatile("movl %1, %%eax\n\t"     /* Set return value first */
                   "movq 0(%0), %%rbx\n\t"  /* Restore rbx */
                   "movq 8(%0), %%rbp\n\t"  /* Restore rbp */
                   "movq 16(%0), %%r12\n\t" /* Restore r12 */
                   "movq 24(%0), %%r13\n\t" /* Restore r13 */
                   "movq 32(%0), %%r14\n\t" /* Restore r14 */
                   "movq 40(%0), %%r15\n\t" /* Restore r15 */
                   "movq 56(%0), %%rdx\n\t" /* Get return address */
                   "movq 48(%0), %%rsp\n\t" /* Restore rsp */
                   "jmp *%%rdx\n\t"         /* Jump to return address */
                   :                        /* no outputs */
                   : "r"(env), "r"(retval)
                   : "rax", "rdx", "rbx", "r12", "r13", "r14", "r15", "memory");
  __builtin_unreachable();
}

/* __ctype_b_loc - character type checking for QBE's parse.c isspace() */
const unsigned short **__ctype_b_loc(void) {
/* Character type flags for isspace, isalpha, isdigit, etc.
 * Bit layout (glibc compatible):
 *   _ISupper = 0x0100, _ISlower = 0x0200, _ISalpha = 0x0400
 *   _ISdigit = 0x0800, _ISxdigit = 0x1000, _ISspace = 0x2000
 *   _ISprint = 0x4000, _ISgraph = 0x8000
 */
#define _ISspace 0x2000
#define _ISprint 0x4000
#define _ISdigit 0x0800
#define _ISxdigit 0x1000
#define _ISalpha 0x0400
#define _ISupper 0x0100
#define _ISlower 0x0200
#define _ISalnum (_ISalpha | _ISdigit)

  static unsigned short ctype_table[256] = {0};
  static unsigned short *const ctype_ptr = &ctype_table[0];
  static unsigned short *const *ctype_loc = &ctype_ptr;
  static int initialized = 0;

  if (!initialized) {
    int i;
    /* Space characters */
    ctype_table[' '] = _ISspace | _ISprint;
    ctype_table['\t'] = _ISspace;
    ctype_table['\n'] = _ISspace;
    ctype_table['\r'] = _ISspace;
    ctype_table['\f'] = _ISspace;
    ctype_table['\v'] = _ISspace;

    /* Digits */
    for (i = '0'; i <= '9'; i++)
      ctype_table[i] = _ISdigit | _ISxdigit | _ISprint;

    /* Uppercase letters */
    for (i = 'A'; i <= 'Z'; i++)
      ctype_table[i] = _ISupper | _ISalpha | _ISprint;
    for (i = 'A'; i <= 'F'; i++)
      ctype_table[i] |= _ISxdigit;

    /* Lowercase letters */
    for (i = 'a'; i <= 'z'; i++)
      ctype_table[i] = _ISlower | _ISalpha | _ISprint;
    for (i = 'a'; i <= 'f'; i++)
      ctype_table[i] |= _ISxdigit;

    /* Printable ASCII */
    for (i = 0x21; i <= 0x7E; i++)
      ctype_table[i] |= _ISprint;

    initialized = 1;
  }

  return ctype_loc;
}

/* Phase 7: __stack_chk_fail now defined in kernel/9front-pc64/main.c */
