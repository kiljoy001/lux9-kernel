/* kernel_compat.h - Kernel compatibility layer for QBE
 *
 * Provides minimal definitions for QBE to compile in kernel.
 * Does NOT include full kernel headers to avoid conflicts.
 */

#ifndef KERNEL_COMPAT_H
#define KERNEL_COMPAT_H

/* Compiler builtins */
#include <stdarg.h>
#include <stddef.h>

/* Basic integer types */
#ifndef __cplusplus
#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;

/* inttypes.h printf format macros */
#define PRIi32 "d"
#define PRId32 "d"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIi64 "lld"
#define PRId64 "lld"
#define PRIu64 "llu"
#define PRIx64 "llx"

/* QBE type aliases (already in all.h but we redefine for clarity) */
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long bits;

/* limits.h */
#ifndef CHAR_BIT
#define CHAR_BIT 8
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

/* ctype.h replacements - simple ASCII-only versions */
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isspace(c)                                                             \
  ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == '\f' ||   \
   (c) == '\v')
#define isblank(c) ((c) == ' ' || (c) == '\t')
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define tolower(c) (isupper(c) ? (c) + ('a' - 'A') : (c))
#define toupper(c) (islower(c) ? (c) - ('a' - 'A') : (c))

/* Standard library replacements */
#ifndef assert
/* Use do-while(0) idiom to prevent dangling-else problems */
#define assert(x)                                                              \
  do {                                                                         \
    if (!(x)) {                                                                \
      print("ASSERT FAILED: %s:%d %s\n", __FILE__, __LINE__, #x);              \
      panic("assertion failed");                                               \
    }                                                                          \
  } while (0)
#endif

#define abort() die("QBE abort at %s:%d", __FILE__, __LINE__)

/* stdio replacements - use exchange page I/O */
#include "exchange_io.h"

#define FILE ExchangeFILE
#define fopen exchange_fopen
#define fclose exchange_fclose
#define feof exchange_feof
#define ferror exchange_ferror
#define fgetc exchange_fgetc
#define ungetc exchange_ungetc
#define fputc exchange_fputc
#define fgets exchange_fgets
#define fputs exchange_fputs
#define fread exchange_fread
#define fwrite exchange_fwrite
#define ftell exchange_ftell
#define fseek exchange_fseek
#define rewind exchange_rewind
#define fscanf exchange_fscanf
#define fprintf exchange_fprintf
#define vfprintf exchange_vfprintf

/* stdio constants */
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* Standard streams - stderr used for debug output, stub to /dev/null */
extern FILE *exchange_stderr;
#define stderr exchange_stderr

/* String functions - declare what we need */
/* String functions - declare what we need */
/* #ifndef _PORTLIB_H_ */
extern long strlen(char *s); /* Match lib.h signature approximately */
extern int strcmp(char *s1, char *s2);
extern int strncmp(char *s1, char *s2, long n);
extern int memcmp(void *s1, void *s2, size_t n);
extern char *strdup(const char *s);
/* #endif */

/* Map memcpy to memmove for Plan 9 kernel environment */
#define memcpy(dest, src, n) memmove(dest, src, n)
extern void *memmove(void *dest, void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
/* extern char *strcpy(char *dest, const char *src); */
/* extern char *strncpy(char *dest, const char *src, size_t n); */
/* #endif */
extern int vsnprintf(char *str, size_t size, const char *format, va_list ap);
extern int snprintf(char *str, size_t size, const char *format, ...);
extern int sprintf(char *str, const char *format, ...);

/* stdlib functions */
/* Kernel allocator declarations */
extern void *xalloc(unsigned long size);
extern void *xallocz(unsigned long size, int clear);
extern void xfree(void *ptr);
extern unsigned long msize(void *ptr);

/* Memory allocation shims */
#define malloc(n) xalloc(n)
#ifndef USE_PEBBLE_ALLOC
#define calloc(n, s) xallocz((n) * (s), 1)
#define free(p) xfree(p)
/* extern void *realloc(void *ptr, size_t size); */
/* #define realloc(p, s) realloc(p, s) */
#endif
/* #ifndef _PORTLIB_H_ */
/* extern void qsort(void *base, size_t nmemb, size_t size, */
/*                   int (*compar)(const void *, const void *)); */
/* #endif */
extern void exit(int status);

/* String conversion */
/* #ifndef _PORTLIB_H_ */
/* extern int atoi(const char *s); */
/* #endif */
extern double strtod(const char *s, char **endptr);

/* Error handling (setjmp/longjmp) */
typedef long jmp_buf[8];
extern int setjmp(jmp_buf env);
extern void longjmp(jmp_buf env, int val) __attribute__((noreturn));

/* limits.h additional */
#ifndef CHAR_MAX
#define CHAR_MAX 127
#endif
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef INT32_MIN
#define INT32_MIN (-INT32_MAX - 1)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 4294967295U
#endif

/* Memory allocation - defined in kernel_util.c */
extern void *emalloc(size_t n);
extern void vfree(void *p);
extern void freeall(void);

/* Error handling - defined in kernel_util.c */
extern void die_(char *file, char *s, ...);

#endif /* KERNEL_COMPAT_H */
