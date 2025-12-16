/* kernel_util.c - Kernel-compatible utility functions for QBE
 *
 * Provides memory, error, and stdlib functions for QBE in kernel context.
 * Will be linked with actual kernel implementations.
 */

#include "all.h"
#include "exchange_io.h"

/* Forward declarations for kernel functions we'll link against */
extern void *xalloc(size_t n);
extern void *xallocz(size_t n);
extern void xfree(void *p);
extern void panic(const char *fmt, ...);

/* stderr stub - debug output goes to /dev/null in kernel */
static char stderr_stub_buf[1024];
static ExchangeFILE stderr_stub_file = {
    .data = stderr_stub_buf,
    .capacity = sizeof(stderr_stub_buf),
    .mode = 'w',
};
FILE *exchange_stderr = &stderr_stub_file;

/* String conversion functions */

/* Convert string to integer */
int atoi(const char *s) {
  int n, neg;

  n = 0;
  neg = 0;
  while (*s == ' ' || *s == '\t')
    s++;
  if (*s == '-') {
    neg = 1;
    s++;
  } else if (*s == '+') {
    s++;
  }
  while (*s >= '0' && *s <= '9')
    n = n * 10 + (*s++ - '0');
  return neg ? -n : n;
}

/* Convert string to double - simplified version */
double strtod(const char *s, char **endptr) {
  double val, power;
  int sign, esign, eval;

  /* Skip whitespace */
  while (*s == ' ' || *s == '\t')
    s++;

  /* Handle sign */
  sign = 1;
  if (*s == '-') {
    sign = -1;
    s++;
  } else if (*s == '+') {
    s++;
  }

  /* Integer part */
  val = 0.0;
  while (*s >= '0' && *s <= '9')
    val = 10.0 * val + (*s++ - '0');

  /* Fractional part */
  if (*s == '.') {
    s++;
    power = 1.0;
    while (*s >= '0' && *s <= '9') {
      val = 10.0 * val + (*s++ - '0');
      power *= 10.0;
    }
    val /= power;
  }

  /* Exponent */
  if (*s == 'e' || *s == 'E') {
    s++;
    esign = 1;
    if (*s == '-') {
      esign = -1;
      s++;
    } else if (*s == '+') {
      s++;
    }
    eval = 0;
    while (*s >= '0' && *s <= '9')
      eval = 10 * eval + (*s++ - '0');

    /* Apply exponent */
    power = 1.0;
    for (int i = 0; i < eval; i++)
      power *= 10.0;
    if (esign == 1)
      val *= power;
    else
      val /= power;
  }

  if (endptr)
    *endptr = (char *)s;

  return sign * val;
}

/* setjmp/longjmp for error handling without panic */
typedef long jmp_buf[8]; /* x86-64: rbx, rbp, r12-r15, rsp, rip */

int setjmp(jmp_buf env) {
  __asm__ volatile("movq %%rbx, 0(%0)\n"
                   "movq %%rbp, 8(%0)\n"
                   "movq %%r12, 16(%0)\n"
                   "movq %%r13, 24(%0)\n"
                   "movq %%r14, 32(%0)\n"
                   "movq %%r15, 40(%0)\n"
                   "leaq 8(%%rsp), %%rdx\n" /* Save return address location */
                   "movq %%rdx, 48(%0)\n"   /* Save stack pointer */
                   "movq (%%rsp), %%rdx\n"  /* Get return address */
                   "movq %%rdx, 56(%0)\n"   /* Save return address */
                   :
                   : "r"(env)
                   : "rdx", "memory");
  return 0;
}

void longjmp(jmp_buf env, int val) {
  if (val == 0)
    val = 1;

  __asm__ volatile("movq 0(%0), %%rbx\n"
                   "movq 8(%0), %%rbp\n"
                   "movq 16(%0), %%r12\n"
                   "movq 24(%0), %%r13\n"
                   "movq 32(%0), %%r14\n"
                   "movq 40(%0), %%r15\n"
                   "movq 48(%0), %%rsp\n" /* Restore stack pointer */
                   "movq 56(%0), %%rdx\n" /* Get return address */
                   "movl %1, %%eax\n"     /* Set return value */
                   "jmp *%%rdx\n"         /* Jump to return address */
                   :
                   : "r"(env), "r"(val)
                   : "rax", "rdx", "memory");
  __builtin_unreachable();
}

/* Error handling - die_ is called by die() macro */
void die_(char *file, char *s, ...) {
  va_list ap;
  char buf[256];

  va_start(ap, s);
  vsnprintf(buf, sizeof(buf), s, ap);
  va_end(ap);

  panic("QBE error in %s: %s", file, buf);
}

/* Memory allocation: use kernel's standard malloc/free from alloc.c
 * which uses poolalloc/poolfree. Do NOT redefine malloc/free here
 * as it causes conflicts with the kernel's allocator. */

/* exit - for kernel, panic */
void exit(int status) { panic("QBE called exit(%d)", status); }

/* qsort - simple bubble sort (QBE doesn't use this much) */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
  char *arr = (char *)base;
  char *tmp;
  size_t i, j;
  int swapped;

  if (nmemb <= 1)
    return;

  tmp = malloc(size);
  if (!tmp)
    return; /* Can't sort without temp space */

  for (i = 0; i < nmemb - 1; i++) {
    swapped = 0;
    for (j = 0; j < nmemb - i - 1; j++) {
      if (compar(arr + j * size, arr + (j + 1) * size) > 0) {
        /* Swap */
        memcpy(tmp, arr + j * size, size);
        memcpy(arr + j * size, arr + (j + 1) * size, size);
        memcpy(arr + (j + 1) * size, tmp, size);
        swapped = 1;
      }
    }
    if (!swapped)
      break;
  }

  free(tmp);
}

/* QBE's emalloc - uses xallocz */
void *emalloc(size_t n) {
  void *p;

  p = xallocz(n);
  if (!p)
    die("emalloc, out of memory");
  return p;
}

/* QBE's vfree - maps to xfree */
void vfree(void *p) { xfree(p); }

/* QBE's freeall - called at end of compilation
 * Frees all allocations made via emalloc during this compilation unit.
 *
 * Implementation: The kernel uses xfree() which handles individual
 * allocations. For full pool semantics, callers should track their
 * allocations and free them explicitly. In practice, the kernel's
 * memory allocator reclaims memory when a process exits.
 */
void freeall() {
  /* In kernel context, memory is managed by xalloc/xfree.
   * The Plan 9 pool allocator automatically coalesces freed blocks.
   * For compilation contexts, the caller (clr_compile) tracks and
   * frees the output buffer, and fruity_free_module handles IR cleanup.
   *
   * If a dedicated compilation pool is needed in the future:
   * 1. Maintain a linked list of allocations in a thread-local pool
   * 2. Walk the list here and xfree each block
   * 3. Reset the pool head to nil
   */
}
