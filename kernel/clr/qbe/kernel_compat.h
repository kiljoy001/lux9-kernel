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
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

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

/* Standard library replacements */
#define assert(x)                                                              \
  do {                                                                         \
    if (!(x)) {                                                                \
      die("assertion failed: %s at %s:%d", #x, __FILE__, __LINE__);            \
    }                                                                          \
  } while (0)

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
/* extern size_t strlen(const char *s); */
/* extern int strcmp(const char *s1, const char *s2); */
/* extern int strncmp(const char *s1, const char *s2, size_t n); */
/* extern int memcmp(const void *s1, const void *s2, size_t n); */
/* #endif */
extern void *memcpy(void *dest, const void *src, size_t n);
/* #ifndef _PORTLIB_H_ */
/* extern void *memmove(void *dest, const void *src, size_t n); */
extern void *memset(void *s, int c, size_t n);
/* extern char *strcpy(char *dest, const char *src); */
/* extern char *strncpy(char *dest, const char *src, size_t n); */
/* #endif */
extern int vsnprintf(char *str, size_t size, const char *format, va_list ap);
extern int snprintf(char *str, size_t size, const char *format, ...);
extern int sprintf(char *str, const char *format, ...);

/* stdlib functions */
extern void *malloc(size_t size);
extern void *calloc(size_t nmemb, size_t size);
extern void *realloc(void *ptr, size_t size);
extern void free(void *ptr);
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
