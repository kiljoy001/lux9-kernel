/* kernel.h - Unified Kernel Header
 *
 * Include this single header to get all kernel types and declarations
 * in the correct order. This prevents include ordering issues.
 *
 * Usage: #include "kernel.h"
 */

#pragma once

/* 1. Base types and Core definitions */
#include "core_types.h"
#include "types_fwd.h"
#include "u.h"

/* 2. C99 compatibility */
#ifndef __cplusplus
#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif
#endif

/* C99 integer types mapped to Plan 9 types */
#ifndef _STDINT_KERNEL_H
#define _STDINT_KERNEL_H
#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;
#else
typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef s8int int8_t;
typedef s16int int16_t;
typedef s32int int32_t;
typedef s64int int64_t;
#endif

#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;
#else
typedef ulong size_t;
#endif

#if defined(__PTRDIFF_TYPE__)
typedef __PTRDIFF_TYPE__ ssize_t;
#else
typedef long ssize_t;
#endif
#endif

/* Core kernel data structures */
#include "dat.h"

/* Page ownership and borrow checking (needed by fns.h) */
#include "pageown.h"

/* Function declarations */
#include "fns.h"

#ifdef __PLAN9_KERNEL__
#define vsnprintf vsnprint
#endif

/* Memory layout */
#include "mem.h"

/* Error definitions */
#include "error.h"

/* 9P protocol */
#include "fcall.h"

/* Library functions */
#include "portlib.h"

/* Pebble token system */
#include "pebble.h"
#include "pebble_kernel.h"

/* Verification (Frama-C only) */
#ifdef __FRAMAC__
#include "kernel_acsl.h"
#endif
