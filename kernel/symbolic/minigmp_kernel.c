/*
 * mini-gmp kernel wrapper
 *
 * Provides libc stubs and initializes mini-GMP to use kernel allocator.
 * This file #includes mini-gmp.c directly to intercept libc dependencies.
 */

/* Tell mini-gmp.c to skip standard headers */
#define MINI_GMP_KERNEL 1

/* Block standard headers that might leak through */
#define _ASSERT_H 1
#define _CTYPE_H 1
#define _LIMITS_H 1
#define _STDIO_H 1
#define _STDLIB_H 1
#define _STRING_H 1
#define _FLOAT_H 1
#define __ASSERT_H 1

/* Kernel headers - use minimal set */
#include "libc.h"
#include "u.h"

/* Forward declarations we need from kernel */
extern void *malloc(ulong size);
extern void free(void *p);
extern void panic(char *fmt, ...);

/* size_t for mini-gmp */
typedef ulong size_t;

/*
 * Stubs for libc functions used by mini-gmp
 */

/* mini-gmp uses fprintf(stderr, ...) for errors - redirect to print */
#define fprintf(f, fmt, ...) print(fmt, ##__VA_ARGS__)
#define stderr ((void *)0)
#define abort() panic("mini-gmp abort")

/* assert - use kernel panic */
#undef assert
#define assert(x)                                                              \
  do {                                                                         \
    if (!(x))                                                                  \
      panic("gmp: " #x);                                                       \
  } while (0)

/* ctype.h functions */
static inline int gmp_isspace(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
static inline int gmp_isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int gmp_isalpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static inline int gmp_isalnum(int c) {
  return gmp_isdigit(c) || gmp_isalpha(c);
}
#define isspace gmp_isspace
#define isdigit gmp_isdigit
#define isalpha gmp_isalpha
#define isalnum gmp_isalnum

/* limits.h values */
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif
#ifndef LONG_MAX
#define LONG_MAX 0x7FFFFFFFFFFFFFFFL
#endif
#ifndef ULONG_MAX
#define ULONG_MAX 0xFFFFFFFFFFFFFFFFUL
#endif
#ifndef LONG_MIN
#define LONG_MIN (-LONG_MAX - 1)
#endif
#ifndef INT_MAX
#define INT_MAX 2147483647
#endif
#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif
#ifndef UINT_MAX
#define UINT_MAX 4294967295U
#endif
#ifndef SHRT_MAX
#define SHRT_MAX 32767
#endif
#ifndef SHRT_MIN
#define SHRT_MIN (-SHRT_MAX - 1)
#endif
#ifndef USHRT_MAX
#define USHRT_MAX 65535
#endif

/* float.h - disable float support */
#define MINI_GMP_DONT_USE_FLOAT_H 1
#define DBL_MANT_DIG 53
#define FLT_RADIX 2

/* stdio.h stubs - mini-gmp uses these for mpz_out_str etc */
typedef void FILE;

/* string.h - kernel provides strlen, strcmp, memcpy, memset, memcmp */
/* mini-gmp uses these, kernel has them */

/* stdio file I/O - stub since we don't use mpz_out_str */
#define fwrite(buf, sz, cnt, f) (0)
#define fputc(c, f) (0)

/* stdlib.h - use kernel allocator */
static void *gmp_kernel_alloc(size_t size) { return malloc(size); }

static void *gmp_kernel_realloc(void *old, size_t old_size, size_t new_size) {
  void *p = malloc(new_size);
  if (p && old) {
    memmove(p, old, old_size < new_size ? old_size : new_size);
    free(old);
  }
  return p;
}

static void gmp_kernel_free(void *p, size_t size) {
  USED(size);
  free(p);
}

/* Now include mini-gmp implementation */
#include "mini-gmp.c"

/* Initialize mini-GMP with kernel allocator */
void minigmp_init(void) {
  mp_set_memory_functions(gmp_kernel_alloc, gmp_kernel_realloc,
                          gmp_kernel_free);
  print("mini-GMP initialized with kernel allocator\n");
}
