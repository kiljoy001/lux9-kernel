/* Stub tpm2b header for kernel */
#ifndef TSS2_UTIL_TPM2B_H
#define TSS2_UTIL_TPM2B_H

#include "../include/tss2_tpm2_types.h"

/* Base TPM2B type - generic buffer with size */
typedef struct {
    UINT16 size;
    BYTE buffer[1];
} TPM2B;

/* TPM2B utilities - minimal stubs for kernel */
#define TPM2B_TYPE1_2B_INIT(type, field, value) { \
    .size = sizeof(value), \
    .field = value \
}

#endif /* TSS2_UTIL_TPM2B_H */
