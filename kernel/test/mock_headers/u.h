#ifndef _U_H_
#define _U_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef uint64_t u64int;
typedef int8_t s8int;
typedef int16_t s16int;
typedef int32_t s32int;
typedef int64_t s64int;

typedef int64_t vlong;
typedef uint64_t uvlong;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned long usize;
typedef unsigned long uintptr;

#define nil NULL
#define USED(x) (void)(x)
#define BY2PG 4096

/* Needed for some kernel macros */
#define nelem(x) (sizeof(x) / sizeof((x)[0]))

#endif
