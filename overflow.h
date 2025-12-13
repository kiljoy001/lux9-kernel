/*
 * ECMA-335 Overflow Detection Functions
 * Copyright (c) 2025 GHOSTDAG IPC Consensus System
 * 
 * Implements overflow detection for arithmetic and conversion operations
 * as specified in ECMA-335 Common Language Infrastructure specification.
 */

#ifndef OVERFLOW_H
#define OVERFLOW_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>

/*==============================================================================
 * Arithmetic Operations with Overflow Detection
 * ECMA-335 Sections: III.3.2 (add.ovf), III.3.49 (mul.ovf), III.3.65 (sub.ovf)
 *============================================================================*/

/* 32-bit Addition */
bool add_ovf_i32(int32_t a, int32_t b, int32_t *result);
bool add_ovf_u32(uint32_t a, uint32_t b, uint32_t *result);

/* 64-bit Addition */
bool add_ovf_i64(int64_t a, int64_t b, int64_t *result);
bool add_ovf_u64(uint64_t a, uint64_t b, uint64_t *result);

/* 32-bit Subtraction */
bool sub_ovf_i32(int32_t a, int32_t b, int32_t *result);
bool sub_ovf_u32(uint32_t a, uint32_t b, uint32_t *result);

/* 64-bit Subtraction */
bool sub_ovf_i64(int64_t a, int64_t b, int64_t *result);
bool sub_ovf_u64(uint64_t a, uint64_t b, uint64_t *result);

/* 32-bit Multiplication */
bool mul_ovf_i32(int32_t a, int32_t b, int32_t *result);
bool mul_ovf_u32(uint32_t a, uint32_t b, uint32_t *result);

/* 64-bit Multiplication */
bool mul_ovf_i64(int64_t a, int64_t b, int64_t *result);
bool mul_ovf_u64(uint64_t a, uint64_t b, uint64_t *result);

/*==============================================================================
 * Conversion Operations with Overflow Detection
 * ECMA-335 Section: III.3.28 (conv.ovf.<to type>)
 *============================================================================*/

/* Convert from int32 to smaller types */
bool conv_ovf_i1_from_i32(int32_t value, int8_t *result);
bool conv_ovf_u1_from_i32(int32_t value, uint8_t *result);
bool conv_ovf_i2_from_i32(int32_t value, int16_t *result);
bool conv_ovf_u2_from_i32(int32_t value, uint16_t *result);

/* Convert from int32 to int32/uint32 (no overflow possible) */
bool conv_ovf_i4_from_i32(int32_t value, int32_t *result);
bool conv_ovf_u4_from_i32(uint32_t value, uint32_t *result);

/* Convert from int32 to larger types (always safe) */
bool conv_ovf_i8_from_i32(int32_t value, int64_t *result);
bool conv_ovf_u8_from_i32(uint32_t value, uint64_t *result);
bool conv_ovf_i_from_i32(int32_t value, intptr_t *result);
bool conv_ovf_u_from_i32(uint32_t value, uintptr_t *result);

/* Convert from int32 to floating point */
bool conv_ovf_r4_from_i32(int32_t value, float *result);
bool conv_ovf_r8_from_i32(int32_t value, double *result);

/* Convert from uint32 to smaller types */
bool conv_ovf_i1_from_u32(uint32_t value, int8_t *result);
bool conv_ovf_u1_from_u32(uint32_t value, uint8_t *result);
bool conv_ovf_i2_from_u32(uint32_t value, int16_t *result);
bool conv_ovf_u2_from_u32(uint32_t value, uint16_t *result);

/* Convert from uint32 to uint32/uint32 (no overflow possible) */
bool conv_ovf_i4_from_u32(uint32_t value, int32_t *result);
bool conv_ovf_u4_from_u32(uint32_t value, uint32_t *result);

/* Convert from uint32 to larger types (always safe) */
bool conv_ovf_i8_from_u32(uint32_t value, int64_t *result);
bool conv_ovf_u8_from_u32(uint32_t value, uint64_t *result);
bool conv_ovf_i_from_u32(uint32_t value, intptr_t *result);
bool conv_ovf_u_from_u32(uint32_t value, uintptr_t *result);

/* Convert from uint32 to floating point */
bool conv_ovf_r4_from_u32(uint32_t value, float *result);
bool conv_ovf_r8_from_u32(uint32_t value, double *result);

/* Convert from int64 to smaller types */
bool conv_ovf_i1_from_i64(int64_t value, int8_t *result);
bool conv_ovf_u1_from_i64(int64_t value, uint8_t *result);
bool conv_ovf_i2_from_i64(int64_t value, int16_t *result);
bool conv_ovf_u2_from_i64(int64_t value, uint16_t *result);
bool conv_ovf_i4_from_i64(int64_t value, int32_t *result);
bool conv_ovf_u4_from_i64(int64_t value, uint32_t *result);

/* Convert from int64 to int64/uint64 (no overflow possible) */
bool conv_ovf_i8_from_i64(int64_t value, int64_t *result);
bool conv_ovf_u8_from_i64(int64_t value, uint64_t *result);

/* Convert from int64 to intptr/uintptr */
bool conv_ovf_i_from_i64(int64_t value, intptr_t *result);
bool conv_ovf_u_from_i64(int64_t value, uintptr_t *result);

/* Convert from int64 to floating point */
bool conv_ovf_r4_from_i64(int64_t value, float *result);
bool conv_ovf_r8_from_i64(int64_t value, double *result);

/* Convert from uint64 to smaller types */
bool conv_ovf_i1_from_u64(uint64_t value, int8_t *result);
bool conv_ovf_u1_from_u64(uint64_t value, uint8_t *result);
bool conv_ovf_i2_from_u64(uint64_t value, int16_t *result);
bool conv_ovf_u2_from_u64(uint64_t value, uint16_t *result);
bool conv_ovf_i4_from_u64(uint64_t value, int32_t *result);
bool conv_ovf_u4_from_u64(uint64_t value, uint32_t *result);

/* Convert from uint64 to int64 (check for overflow) */
bool conv_ovf_i8_from_u64(uint64_t value, int64_t *result);

/* Convert from uint64 to uint64 (no overflow possible) */
bool conv_ovf_u8_from_u64(uint64_t value, uint64_t *result);

/* Convert from uint64 to intptr/uintptr */
bool conv_ovf_i_from_u64(uint64_t value, intptr_t *result);
bool conv_ovf_u_from_u64(uint64_t value, uintptr_t *result);

/* Convert from uint64 to floating point */
bool conv_ovf_r4_from_u64(uint64_t value, float *result);
bool conv_ovf_r8_from_u64(uint64_t value, double *result);

/* Convert from floating point to integer types */
bool conv_ovf_i1_from_r4(float value, int8_t *result);
bool conv_ovf_u1_from_r4(float value, uint8_t *result);
bool conv_ovf_i2_from_r4(float value, int16_t *result);
bool conv_ovf_u2_from_r4(float value, uint16_t *result);
bool conv_ovf_i4_from_r4(float value, int32_t *result);
bool conv_ovf_u4_from_r4(float value, uint32_t *result);
bool conv_ovf_i8_from_r4(float value, int64_t *result);
bool conv_ovf_u8_from_r4(float value, uint64_t *result);

bool conv_ovf_i1_from_r8(double value, int8_t *result);
bool conv_ovf_u1_from_r8(double value, uint8_t *result);
bool conv_ovf_i2_from_r8(double value, int16_t *result);
bool conv_ovf_u2_from_r8(double value, uint16_t *result);
bool conv_ovf_i4_from_r8(double value, int32_t *result);
bool conv_ovf_u4_from_r8(double value, uint32_t *result);
bool conv_ovf_i8_from_r8(double value, int64_t *result);
bool conv_ovf_u8_from_r8(double value, uint64_t *result);

/* Convert between integer and native int */
bool conv_ovf_i_from_i32(int32_t value, intptr_t *result);
bool conv_ovf_u_from_i32(uint32_t value, uintptr_t *result);
bool conv_ovf_i_from_u32(uint32_t value, intptr_t *result);
bool conv_ovf_u_from_u32(uint32_t value, uintptr_t *result);
bool conv_ovf_i_from_i64(int64_t value, intptr_t *result);
bool conv_ovf_u_from_i64(int64_t value, uintptr_t *result);
bool conv_ovf_i_from_u64(uint64_t value, intptr_t *result);
bool conv_ovf_u_from_u64(uint64_t value, uintptr_t *result);

/* Utility macros for type limits */
#define INT8_MIN_VAL  ((int8_t)0x80)
#define INT8_MAX_VAL  ((int8_t)0x7F)
#define UINT8_MAX_VAL ((uint8_t)0xFF)
#define INT16_MIN_VAL ((int16_t)0x8000)
#define INT16_MAX_VAL ((int16_t)0x7FFF)
#define UINT16_MAX_VAL ((uint16_t)0xFFFF)

#endif /* OVERFLOW_H */