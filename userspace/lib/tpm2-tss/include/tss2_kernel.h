/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Kernel compatibility layer for TPM2-TSS
 * Bridges TSS2 to Lux9 kernel types and functions
 */

#ifndef TSS2_KERNEL_H
#define TSS2_KERNEL_H

/* Kernel headers */
#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* Map standard C types to kernel types */
typedef u8int   uint8_t;
typedef u16int  uint16_t;
typedef u32int  uint32_t;
typedef u64int  uint64_t;
typedef s8int   int8_t;
typedef s16int  int16_t;
typedef s32int  int32_t;
typedef s64int  int64_t;

typedef usize   size_t;
typedef ssize   ssize_t;

/* Standard defines */
#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef bool
#define bool int
#define true 1
#define false 0
#endif

/* String operations - map to kernel functions */
#define memcpy(dst, src, n)  memmove(dst, src, n)
#define memset(s, c, n)      memset(s, c, n)
#define strlen(s)            strlen(s)
#define strcmp(s1, s2)       strcmp(s1, s2)
#define strncmp(s1, s2, n)   strncmp(s1, s2, n)

/* Logging - map to kernel print */
#define LOG_ERROR(fmt, ...)   print("TPM ERROR: " fmt "\n", ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) print("TPM WARN: " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    print("TPM INFO: " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   /* Disabled in kernel */
#define LOG_TRACE(fmt, ...)   /* Disabled in kernel */

/* Logging macros from util/log.h */
#define LOG_PERR(func, rc) print("TPM: %s failed with code 0x%x\n", #func, rc)
#define LOGMODULE /* Ignored */

/* Attribute macros */
#define UNUSED(x) (void)(x)

/* Format specifiers */
#ifndef PRIx32
#define PRIx32 "x"
#define PRIx64 "llx"
#define PRIxPTR "p"
#endif

/* No config.h in kernel */
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H 1
#endif

/* Endian conversion (already defined in tss2_endian.h but include path) */
#include "../util/tss2_endian.h"

#endif /* TSS2_KERNEL_H */
