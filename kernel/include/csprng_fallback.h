/*
 * ChaCha20-based CSPRNG Fallback
 *
 * Software cryptographically secure PRNG for systems without hardware RNG.
 * Uses randomized multi-source entropy collection.
 *
 * SECURITY WARNING: This is a FALLBACK for development/testing only.
 * Production systems MUST use hardware RNG (TPM or RDRAND).
 */

#pragma once

/* Generate 64-bit random value using ChaCha20 CSPRNG */
u64int chacha20_csprng_u64(void);

/* Fill buffer with random bytes */
void chacha20_csprng_fill(u8 *buf, ulong len);

/* Get CSPRNG statistics (for debugging) */
void csprng_stats(void);
