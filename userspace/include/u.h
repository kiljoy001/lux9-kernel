/* Plan 9 universal header */
#ifndef _U_H_
#define _U_H_

#ifndef static_assert
# if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define static_assert _Static_assert
# elif defined(__cplusplus) && __cplusplus >= 201103L
#  define static_assert(x, msg) static_assert(x, msg)
# else
#  define static_assert(x, msg) typedef char static_assert_##msg[(x)?1:-1]
# endif
#endif

#ifndef _Noreturn
#define _Noreturn __attribute__((noreturn))
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;

typedef unsigned long usize;
typedef long ssize;
typedef unsigned long uintptr;
typedef long intptr;

/* Plan 9 fixed-width types */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;

typedef u32int Rune;	/* UTF-8 code point */

#define nelem(x) (sizeof(x)/sizeof((x)[0]))
#ifndef offsetof
#define offsetof(s, m) (ulong)(&(((s*)0)->m))
#endif

#define nil ((void*)0)

/* USED macro to suppress unused warnings */
#define USED(...) if(__VA_ARGS__){}

typedef long jmp_buf[10];

#endif /* _U_H_ */
