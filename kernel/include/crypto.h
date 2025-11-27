/*
 * Kernel Cryptographic Functions - SUPERCOP Integration
 *
 * Provides SHA256 and HMAC-SHA256 for kernel use.
 * Based on sphlib from SUPERCOP.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* SHA256 output size */
#define CRYPTO_SHA256_BYTES 32

/* HMAC-SHA256 key size (can be any size, but 32 bytes recommended) */
#define CRYPTO_HMAC_KEY_BYTES 32

/*
 * SHA256 hash function
 *
 * Computes SHA256(data).
 *
 * out: Output buffer (must be at least 32 bytes)
 * data: Input data
 * len: Length of input data
 *
 * Returns: 0 on success, -1 on error
 */
int crypto_sha256(uint8_t *out, const uint8_t *data, size_t len);

/*
 * HMAC-SHA256 message authentication
 *
 * Computes HMAC-SHA256(key, data).
 *
 * out: Output buffer (must be at least 32 bytes)
 * key: HMAC key
 * keylen: Length of key (typically 32 bytes, but can be any size)
 * data: Input data to authenticate
 * len: Length of input data
 *
 * Returns: 0 on success, -1 on error
 */
int crypto_hmac_sha256(uint8_t *out, const uint8_t *key, size_t keylen,
                       const uint8_t *data, size_t len);

/*
 * TPM-backed key storage for HMAC
 *
 * These functions use TPM PCRs and sealed keys for secure key storage.
 */

/* Initialize TPM key storage (call once at boot) */
int crypto_tpm_key_init(void);

/* Get current HMAC key (unseals from TPM if needed) */
int crypto_tpm_get_hmac_key(uint8_t *key_out, size_t *keylen);

/* Rotate HMAC key (generates new key, seals to TPM) */
int crypto_tpm_rotate_hmac_key(void);

/* TPM-backed HMAC using sealed key */
int crypto_tpm_hmac_sha256(uint8_t *out, const uint8_t *data, size_t len);
