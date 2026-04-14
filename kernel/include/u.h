/* Plan 9 universal header */
#ifndef _U_H_
#define _U_H_

#ifdef __FRAMAC__
#include "framac_stubs.h"
#endif

/* Add static_assert support for compile-time checks */
#ifndef static_assert
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define static_assert _Static_assert
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define static_assert(x, msg) static_assert(x, msg)
#else
#define static_assert(x, msg) typedef char static_assert_##msg[(x) ? 1 : -1]
#endif
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

typedef u32int Rune; /* UTF-8 code point */

typedef struct {
  u8int data[16];
} uuid_t;

/* UTF-8 constants (standard Plan 9) */
enum {
  UTFmax = 4,         /* maximum bytes per rune */
  Runesync = 0x80,    /* cannot represent part of a UTF sequence */
  Runeself = 0x80,    /* rune and UTF sequences are the same (<) */
  Runeerror = 0xFFFD, /* decoding error in UTF */
  Runemax = 0x10FFFF, /* 21 bit rune */
};

#ifndef nelem
#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef offsetof
#define offsetof(s, m) (ulong)(&(((struct s *)0)->m))
#endif

#ifndef nil
#define nil ((void *)0)
#endif

/* USED macro to suppress unused warnings */
#define USED(...)                                                              \
  if (__VA_ARGS__) {                                                           \
  }

/* Compile-time type size assertions */
static_assert(sizeof(ulong) == sizeof(void *), "ulong must match pointer size");
static_assert(sizeof(uintptr) == sizeof(void *),
              "uintptr must match pointer size");
static_assert(sizeof(usize) == sizeof(void *), "usize must match pointer size");
static_assert(sizeof(ssize) == sizeof(void *), "ssize must match pointer size");

#endif /* _U_H_ */
