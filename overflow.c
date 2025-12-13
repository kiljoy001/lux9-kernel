/*
 * ECMA-335 Overflow Detection Functions
 * Copyright (c) 2025 GHOSTDAG IPC Consensus System
 * 
 * Implements overflow detection for arithmetic and conversion operations
 * as specified in ECMA-335 Common Language Infrastructure specification.
 */

#include "overflow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/*==============================================================================
 * Arithmetic Operations with Overflow Detection
 * ECMA-335 Sections: III.3.2 (add.ovf), III.3.49 (mul.ovf), III.3.65 (sub.ovf)
 *============================================================================*/

/* 32-bit Addition */
bool add_ovf_i32(int32_t a, int32_t b, int32_t *result) {
    if ((a > 0 && b > 0 && a > INT32_MAX - b) || 
        (a < 0 && b < 0 && a < INT32_MIN - b)) {
        return true;  /* Overflow */
    }
    *result = a + b;
    return false;  /* No overflow */
}

bool add_ovf_u32(uint32_t a, uint32_t b, uint32_t *result) {
    if (b > 0 && a > UINT32_MAX - b) {
        return true;  /* Overflow */
    }
    *result = a + b;
    return false;  /* No overflow */
}

/* 64-bit Addition */
bool add_ovf_i64(int64_t a, int64_t b, int64_t *result) {
    if ((a > 0 && b > 0 && a > INT64_MAX - b) || 
        (a < 0 && b < 0 && a < INT64_MIN - b)) {
        return true;  /* Overflow */
    }
    *result = a + b;
    return false;  /* No overflow */
}

bool add_ovf_u64(uint64_t a, uint64_t b, uint64_t *result) {
    if (b > 0 && a > UINT64_MAX - b) {
        return true;  /* Overflow */
    }
    *result = a + b;
    return false;  /* No overflow */
}

/* 32-bit Subtraction */
bool sub_ovf_i32(int32_t a, int32_t b, int32_t *result) {
    if ((a >= 0 && b < 0 && a - INT32_MAX > b) || 
        (a < 0 && b > 0 && INT32_MIN - a > -b)) {
        return true;  /* Overflow */
    }
    *result = a - b;
    return false;  /* No overflow */
}

bool sub_ovf_u32(uint32_t a, uint32_t b, uint32_t *result) {
    if (a < b) {
        return true;  /* Overflow (would wrap around) */
    }
    *result = a - b;
    return false;  /* No overflow */
}

/* 64-bit Subtraction */
bool sub_ovf_i64(int64_t a, int64_t b, int64_t *result) {
    if ((a >= 0 && b < 0 && a - INT64_MAX > b) || 
        (a < 0 && b > 0 && INT64_MIN - a > -b)) {
        return true;  /* Overflow */
    }
    *result = a - b;
    return false;  /* No overflow */
}

bool sub_ovf_u64(uint64_t a, uint64_t b, uint64_t *result) {
    if (a < b) {
        return true;  /* Overflow (would wrap around) */
    }
    *result = a - b;
    return false;  /* No overflow */
}

/* 32-bit Multiplication */
bool mul_ovf_i32(int32_t a, int32_t b, int32_t *result) {
    /* Special cases for multiplication */
    if (a == 0 || b == 0) {
        *result = 0;
        return false;
    }
    
    if (a == -1 && b == INT32_MIN) {
        return true;  /* -1 * INT32_MIN would overflow */
    }
    
    if (b == -1 && a == INT32_MIN) {
        return true;  /* INT32_MIN * -1 would overflow */
    }
    
    /* Check magnitude overflow */
    int64_t temp = (int64_t)a * (int64_t)b;
    if (temp > INT32_MAX || temp < INT32_MIN) {
        return true;  /* Overflow */
    }
    
    *result = (int32_t)temp;
    return false;
}

bool mul_ovf_u32(uint32_t a, uint32_t b, uint32_t *result) {
    /* Special cases */
    if (a == 0 || b == 0) {
        *result = 0;
        return false;
    }
    
    /* Check overflow */
    uint64_t temp = (uint64_t)a * (uint64_t)b;
    if (temp > UINT32_MAX) {
        return true;  /* Overflow */
    }
    
    *result = (uint32_t)temp;
    return false;
}

/* 64-bit Multiplication */
bool mul_ovf_i64(int64_t a, int64_t b, int64_t *result) {
    /* Special cases for multiplication */
    if (a == 0 || b == 0) {
        *result = 0;
        return false;
    }
    
    if (a == -1 && b == INT64_MIN) {
        return true;  /* -1 * INT64_MIN would overflow */
    }
    
    if (b == -1 && a == INT64_MIN) {
        return true;  /* INT64_MIN * -1 would overflow */
    }
    
    /* Check magnitude overflow using 128-bit if available, otherwise use division */
    if (a > 0) {
        if (b > 0) {
            if (a > INT64_MAX / b) return true;
        } else {
            if (b < INT64_MIN / a) return true;
        }
    } else {
        if (b > 0) {
            if (a < INT64_MIN / b) return true;
        } else {
            if (b != 0 && a < INT64_MAX / b) return true;
        }
    }
    
    *result = a * b;
    return false;
}

bool mul_ovf_u64(uint64_t a, uint64_t b, uint64_t *result) {
    /* Special cases */
    if (a == 0 || b == 0) {
        *result = 0;
        return false;
    }
    
    /* Check overflow */
    if (a > UINT64_MAX / b) {
        return true;  /* Overflow */
    }
    
    *result = a * b;
    return false;
}

/*==============================================================================
 * Conversion Operations with Overflow Detection
 * ECMA-335 Section: III.3.28 (conv.ovf.<to type>)
 *============================================================================*/

/* Convert from int32 to smaller types */
bool conv_ovf_i1_from_i32(int32_t value, int8_t *result) {
    if (value < INT8_MIN || value > INT8_MAX) {
        return true;  /* Overflow */
    }
    *result = (int8_t)value;
    return false;
}

bool conv_ovf_u1_from_i32(int32_t value, uint8_t *result) {
    if (value < 0 || value > UINT8_MAX) {
        return true;  /* Overflow (negative or too large) */
    }
    *result = (uint8_t)value;
    return false;
}

bool conv_ovf_i2_from_i32(int32_t value, int16_t *result) {
    if (value < INT16_MIN || value > INT16_MAX) {
        return true;  /* Overflow */
    }
    *result = (int16_t)value;
    return false;
}

bool conv_ovf_u2_from_i32(int32_t value, uint16_t *result) {
    if (value < 0 || value > UINT16_MAX) {
        return true;  /* Overflow (negative or too large) */
    }
    *result = (uint16_t)value;
    return false;
}

/* Convert from int32 to int32/uint32 (no overflow possible) */
bool conv_ovf_i4_from_i32(int32_t value, int32_t *result) {
    *result = value;  /* No overflow possible */
    return false;
}

bool conv_ovf_u4_from_i32(uint32_t value, uint32_t *result) {
    *result = value;  /* No overflow possible */
    return false;
}

/* Convert from int32 to larger types (always safe) */
bool conv_ovf_i8_from_i32(int32_t value, int64_t *result) {
    *result = (int64_t)value;  /* Always safe */
    return false;
}

bool conv_ovf_u8_from_i32(uint32_t value, uint64_t *result) {
    *result = (uint64_t)value;  /* Always safe */
    return false;
}

bool conv_ovf_i_from_i32(int32_t value, intptr_t *result) {
    *result = (intptr_t)value;  /* Always safe */
    return false;
}

bool conv_ovf_u_from_i32(uint32_t value, uintptr_t *result) {
    *result = (uintptr_t)value;  /* Always safe */
    return false;
}

/* Convert from int32 to floating point */
bool conv_ovf_r4_from_i32(int32_t value, float *result) {
    *result = (float)value;  /* May lose precision but won't overflow */
    return false;
}

bool conv_ovf_r8_from_i32(int32_t value, double *result) {
    *result = (double)value;  /* No overflow */
    return false;
}

/* Convert from uint32 to smaller types */
bool conv_ovf_i1_from_u32(uint32_t value, int8_t *result) {
    if (value > (uint32_t)INT8_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (int8_t)value;
    return false;
}

bool conv_ovf_u1_from_u32(uint32_t value, uint8_t *result) {
    if (value > UINT8_MAX) {
        return true;  /* Overflow */
    }
    *result = (uint8_t)value;
    return false;
}

bool conv_ovf_i2_from_u32(uint32_t value, int16_t *result) {
    if (value > (uint32_t)INT16_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (int16_t)value;
    return false;
}

bool conv_ovf_u2_from_u32(uint32_t value, uint16_t *result) {
    if (value > UINT16_MAX) {
        return true;  /* Overflow */
    }
    *result = (uint16_t)value;
    return false;
}

/* Convert from uint32 to uint32/uint32 (no overflow possible) */
bool conv_ovf_i4_from_u32(uint32_t value, int32_t *result) {
    if (value > (uint32_t)INT32_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (int32_t)value;
    return false;
}

bool conv_ovf_u4_from_u32(uint32_t value, uint32_t *result) {
    *result = value;  /* No overflow possible */
    return false;
}

/* Convert from uint32 to larger types (always safe) */
bool conv_ovf_i8_from_u32(uint32_t value, int64_t *result) {
    *result = (int64_t)value;  /* Always safe */
    return false;
}

bool conv_ovf_u8_from_u32(uint32_t value, uint64_t *result) {
    *result = (uint64_t)value;  /* Always safe */
    return false;
}

bool conv_ovf_i_from_u32(uint32_t value, intptr_t *result) {
    *result = (intptr_t)value;  /* Always safe */
    return false;
}

bool conv_ovf_u_from_u32(uint32_t value, uintptr_t *result) {
    *result = (uintptr_t)value;  /* Always safe */
    return false;
}

/* Convert from uint32 to floating point */
bool conv_ovf_r4_from_u32(uint32_t value, float *result) {
    *result = (float)value;  /* May lose precision but won't overflow */
    return false;
}

bool conv_ovf_r8_from_u32(uint32_t value, double *result) {
    *result = (double)value;  /* No overflow */
    return false;
}

/* Convert from int64 to smaller types */
bool conv_ovf_i1_from_i64(int64_t value, int8_t *result) {
    if (value < INT8_MIN || value > INT8_MAX) {
        return true;  /* Overflow */
    }
    *result = (int8_t)value;
    return false;
}

bool conv_ovf_u1_from_i64(int64_t value, uint8_t *result) {
    if (value < 0 || value > UINT8_MAX) {
        return true;  /* Overflow (negative or too large) */
    }
    *result = (uint8_t)value;
    return false;
}

bool conv_ovf_i2_from_i64(int64_t value, int16_t *result) {
    if (value < INT16_MIN || value > INT16_MAX) {
        return true;  /* Overflow */
    }
    *result = (int16_t)value;
    return false;
}

bool conv_ovf_u2_from_i64(int64_t value, uint16_t *result) {
    if (value < 0 || value > UINT16_MAX) {
        return true;  /* Overflow (negative or too large) */
    }
    *result = (uint16_t)value;
    return false;
}

bool conv_ovf_i4_from_i64(int64_t value, int32_t *result) {
    if (value < INT32_MIN || value > INT32_MAX) {
        return true;  /* Overflow */
    }
    *result = (int32_t)value;
    return false;
}

bool conv_ovf_u4_from_i64(int64_t value, uint32_t *result) {
    if (value < 0 || value > (int64_t)UINT32_MAX) {
        return true;  /* Overflow (negative or too large) */
    }
    *result = (uint32_t)value;
    return false;
}

/* Convert from int64 to int64/uint64 (no overflow possible) */
bool conv_ovf_i8_from_i64(int64_t value, int64_t *result) {
    *result = value;  /* No overflow possible */
    return false;
}

bool conv_ovf_u8_from_i64(int64_t value, uint64_t *result) {
    if (value < 0) {
        return true;  /* Overflow (negative) */
    }
    *result = (uint64_t)value;
    return false;
}

/* Convert from int64 to intptr/uintptr */
bool conv_ovf_i_from_i64(int64_t value, intptr_t *result) {
    *result = (intptr_t)value;  /* Assume same size or larger */
    return false;
}

bool conv_ovf_u_from_i64(int64_t value, uintptr_t *result) {
    if (value < 0) {
        return true;  /* Overflow (negative) */
    }
    *result = (uintptr_t)value;
    return false;
}

/* Convert from int64 to floating point */
bool conv_ovf_r4_from_i64(int64_t value, float *result) {
    /* Check if value is too large for float representation */
    if (value > FLT_MAX || value < -FLT_MAX) {
        return true;  /* Overflow */
    }
    *result = (float)value;
    return false;
}

bool conv_ovf_r8_from_i64(int64_t value, double *result) {
    *result = (double)value;  /* No overflow for int64 to double */
    return false;
}

/* Convert from uint64 to smaller types */
bool conv_ovf_i1_from_u64(uint64_t value, int8_t *result) {
    if (value > (uint64_t)INT8_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (int8_t)value;
    return false;
}

bool conv_ovf_u1_from_u64(uint64_t value, uint8_t *result) {
    if (value > UINT8_MAX) {
        return true;  /* Overflow */
    }
    *result = (uint8_t)value;
    return false;
}

bool conv_ovf_i2_from_u64(uint64_t value, int16_t *result) {
    if (value > (uint64_t)INT16_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (int16_t)value;
    return false;
}

bool conv_ovf_u2_from_u64(uint64_t value, uint16_t *result) {
    if (value > UINT16_MAX) {
        return true;  /* Overflow */
    }
    *result = (uint16_t)value;
    return false;
}

bool conv_ovf_i4_from_u64(uint64_t value, int32_t *result) {
    if (value > (uint64_t)INT32_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (int32_t)value;
    return false;
}

bool conv_ovf_u4_from_u64(uint64_t value, uint32_t *result) {
    if (value > UINT32_MAX) {
        return true;  /* Overflow */
    }
    *result = (uint32_t)value;
    return false;
}

/* Convert from uint64 to int64 (check for overflow) */
bool conv_ovf_i8_from_u64(uint64_t value, int64_t *result) {
    if (value > (uint64_t)INT64_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (int64_t)value;
    return false;
}

/* Convert from uint64 to uint64 (no overflow possible) */
bool conv_ovf_u8_from_u64(uint64_t value, uint64_t *result) {
    *result = value;  /* No overflow possible */
    return false;
}

/* Convert from uint64 to intptr/uintptr */
bool conv_ovf_i_from_u64(uint64_t value, intptr_t *result) {
    if (value > (uint64_t)INTPTR_MAX) {
        return true;  /* Overflow (too large for signed) */
    }
    *result = (intptr_t)value;
    return false;
}

bool conv_ovf_u_from_u64(uint64_t value, uintptr_t *result) {
    *result = (uintptr_t)value;  /* Assume same size or larger */
    return false;
}

/* Convert from uint64 to floating point */
bool conv_ovf_r4_from_u64(uint64_t value, float *result) {
    /* Check if value is too large for float representation */
    if (value > (uint64_t)FLT_MAX) {
        return true;  /* Overflow */
    }
    *result = (float)value;
    return false;
}

bool conv_ovf_r8_from_u64(uint64_t value, double *result) {
    *result = (double)value;  /* No overflow for uint64 to double */
    return false;
}

/* Convert from floating point to integer types */
bool conv_ovf_i1_from_r4(float value, int8_t *result) {
    if (value < INT8_MIN || value > INT8_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int8_t)value;
    return false;
}

bool conv_ovf_u1_from_r4(float value, uint8_t *result) {
    if (value < 0 || value > UINT8_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint8_t)value;
    return false;
}

bool conv_ovf_i2_from_r4(float value, int16_t *result) {
    if (value < INT16_MIN || value > INT16_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int16_t)value;
    return false;
}

bool conv_ovf_u2_from_r4(float value, uint16_t *result) {
    if (value < 0 || value > UINT16_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint16_t)value;
    return false;
}

bool conv_ovf_i4_from_r4(float value, int32_t *result) {
    if (value < (float)INT32_MIN || value > (float)INT32_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int32_t)value;
    return false;
}

bool conv_ovf_u4_from_r4(float value, uint32_t *result) {
    if (value < 0 || value > (float)UINT32_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint32_t)value;
    return false;
}

bool conv_ovf_i8_from_r4(float value, int64_t *result) {
    if (value < INT64_MIN || value > INT64_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int64_t)value;
    return false;
}

bool conv_ovf_u8_from_r4(float value, uint64_t *result) {
    if (value < 0 || value > UINT64_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint64_t)value;
    return false;
}

/* Convert from double to integer types */
bool conv_ovf_i1_from_r8(double value, int8_t *result) {
    if (value < INT8_MIN || value > INT8_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int8_t)value;
    return false;
}

bool conv_ovf_u1_from_r8(double value, uint8_t *result) {
    if (value < 0 || value > UINT8_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint8_t)value;
    return false;
}

bool conv_ovf_i2_from_r8(double value, int16_t *result) {
    if (value < INT16_MIN || value > INT16_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int16_t)value;
    return false;
}

bool conv_ovf_u2_from_r8(double value, uint16_t *result) {
    if (value < 0 || value > UINT16_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint16_t)value;
    return false;
}

bool conv_ovf_i4_from_r8(double value, int32_t *result) {
    if (value < INT32_MIN || value > INT32_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int32_t)value;
    return false;
}

bool conv_ovf_u4_from_r8(double value, uint32_t *result) {
    if (value < 0 || value > UINT32_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint32_t)value;
    return false;
}

bool conv_ovf_i8_from_r8(double value, int64_t *result) {
    if (value < INT64_MIN || value > INT64_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (int64_t)value;
    return false;
}

bool conv_ovf_u8_from_r8(double value, uint64_t *result) {
    if (value < 0 || value > UINT64_MAX || isnan(value) || isinf(value)) {
        return true;  /* Overflow or invalid */
    }
    *result = (uint64_t)value;
    return false;
}

/* Convert between integer and native int */