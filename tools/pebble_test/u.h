#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* Basic Types */
typedef uint8_t  u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef uint64_t u64int;
typedef uint8_t  uchar;
typedef uint16_t ushort;
typedef uint32_t uint;
typedef uint64_t ulong; /* 9front uses ulong as 64-bit on amd64 */
typedef uint64_t uintptr;
typedef uint64_t uvlong;
typedef int64_t  vlong;
typedef size_t   usize;

/* Constants */
#define nil ((void*)0)
#define KZERO 0
#define BY2PG 4096
#define PPN(x) (x)
#define PTEVALID 1

#define USED(x) if(x){}else{}
#define nelem(x) (sizeof(x)/sizeof((x)[0]))

/* Error Handling */
#define ERRMAX 128
