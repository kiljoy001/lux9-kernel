/*
 * CBOR Kernel Integration for Plan 9
 * Provides Plan 9 kernel types for CBOR library
 */

#ifndef CBOR_KERNEL_H
#define CBOR_KERNEL_H

#include "u.h"

/* CBOR library expects stdint.h types, provide them via Plan 9 types */
typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;

typedef s8int int8_t;
typedef s16int int16_t;
typedef s32int int32_t;
typedef s64int int64_t;

/* Pointer-sized integers */
typedef uintptr uintptr_t;
typedef intptr intptr_t;

/* size_t is usize in Plan 9 */
typedef usize size_t;

/* bool type and values */
#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#endif

/* NULL define */
#ifndef NULL
#define NULL nil
#endif

#endif /* CBOR_KERNEL_H */
