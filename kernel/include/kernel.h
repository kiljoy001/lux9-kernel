/* kernel.h - Unified Kernel Header
 *
 * Include this single header to get all kernel types and declarations
 * in the correct order. This prevents include ordering issues.
 *
 * Usage: #include "kernel.h"
 */

#pragma once

/* Base types first */
#include "u.h"

/* C99 compatibility for WASM3 and other libs */
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
typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef s8int int8_t;
typedef s16int int16_t;
typedef s32int int32_t;
typedef s64int int64_t;
typedef usize size_t;
typedef ssize ssize_t;
#endif

/* Core kernel data structures */
#include "dat.h"

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
