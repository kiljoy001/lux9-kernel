/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Endianness conversion macros for TPM2-TSS
 * Adapted for Lux9 kernel from tpm2-tss project
 */

#ifndef TSS2_ENDIAN_H
#define TSS2_ENDIAN_H

#include "u.h"

/* Big-endian to host conversion */
#define BE_TO_HOST_16(value) ((u16int)( \
    (((u16int)(value) & 0x00FF) << 8) | \
    (((u16int)(value) & 0xFF00) >> 8)))

#define BE_TO_HOST_32(value) ((u32int)( \
    (((u32int)(value) & 0x000000FF) << 24) | \
    (((u32int)(value) & 0x0000FF00) << 8) | \
    (((u32int)(value) & 0x00FF0000) >> 8) | \
    (((u32int)(value) & 0xFF000000) >> 24)))

#define BE_TO_HOST_64(value) ((u64int)( \
    (((u64int)(value) & 0x00000000000000FFULL) << 56) | \
    (((u64int)(value) & 0x000000000000FF00ULL) << 40) | \
    (((u64int)(value) & 0x0000000000FF0000ULL) << 24) | \
    (((u64int)(value) & 0x00000000FF000000ULL) << 8) | \
    (((u64int)(value) & 0x000000FF00000000ULL) >> 8) | \
    (((u64int)(value) & 0x0000FF0000000000ULL) >> 24) | \
    (((u64int)(value) & 0x00FF000000000000ULL) >> 40) | \
    (((u64int)(value) & 0xFF00000000000000ULL) >> 56)))

/* Host to big-endian conversion */
#define HOST_TO_BE_16 BE_TO_HOST_16
#define HOST_TO_BE_32 BE_TO_HOST_32
#define HOST_TO_BE_64 BE_TO_HOST_64

#endif /* TSS2_ENDIAN_H */
