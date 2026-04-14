/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/* Copyright (C) 2016-2022 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights
 * Reserved. Adapted for Lux9 kernel by removing Linux-specific dependencies.
 *
 * SipHash: a fast short-input PRF
 * https://131002.net/siphash/
 *
 * This implementation is specifically for SipHash2-4 for a secure PRF
 * and HalfSipHash1-3/SipHash1-3 for an insecure PRF only suitable for
 * hashtables.
 */

#ifndef _SIPHASH_H
#define _SIPHASH_H

#include "u.h"

/* SipHash key structure */
typedef struct {
  u64int key[2];
} siphash_key_t;

/* HalfSipHash key structure (for hash tables) */
typedef struct hsiphash_key_t {
  u64int
      key[2]; /* On 64-bit, HalfSipHash is actually SipHash for performance */
} hsiphash_key_t;
/*@ lemma hsiphash_key_size: sizeof(hsiphash_key_t) == 16; */

/* Check if key is zero */
static inline int siphash_key_is_zero(const siphash_key_t *key) {
  return !(key->key[0] | key->key[1]);
}

/* Core SipHash functions - 64-bit output */
u64int siphash_1u64(u64int a, const siphash_key_t *key);
u64int siphash_2u64(u64int a, u64int b, const siphash_key_t *key);
u64int siphash(const void *data, usize len, const siphash_key_t *key);

/* HalfSipHash functions - 32-bit output (for hash tables) */
/* HalfSipHash functions - 32-bit output (for hash tables) */
/*@
  @ requires \valid_read(key);
  @ terminates \true;
  @ assigns \nothing;
  @*/
u32int hsiphash_1u32(u32int a, const hsiphash_key_t *key);

/*@
  @ requires \valid_read(key);
  @ terminates \true;
  @ assigns \nothing;
  @*/
u32int hsiphash_2u32(u32int a, u32int b, const hsiphash_key_t *key);

/* HalfSipHash: bounded data for verification (8KB max for hash table keys)
 * Implementation verified separately (3/3 goals in siphash.c). */
/*@
  @ requires len <= 8192;
  @ requires len > 0 ==> \valid_read(((unsigned char*)data) + (0 .. len - 1));
  @ requires \valid_read(key);
  @ terminates \true;
  @ assigns \nothing;
  @*/
u32int hsiphash(const void *data, usize len, const hsiphash_key_t *key);

/* Secure RNG functions for key generation */
/*@ requires len >= 0 && \valid(((unsigned char*)buffer) + (0 .. len-1));
  @ terminates \true;
  @ assigns ((unsigned char*)buffer)[0 .. len-1];
  @*/
extern int tpm_get_random(unsigned char *buffer, int len);

/*@ terminates \true; assigns \nothing; */
extern u64int rdrand_u64(void);

/*@ terminates \true; assigns \nothing; */
extern int crypto_hw_rdrand_available(void);

/*@ terminates \true; assigns \nothing; */
extern u64int chacha20_csprng_u64(void);

/*
 * SipHash permutation macros
 */

/* Rotate left for 64-bit */
#define ROL64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))

/* SipHash permutation (2-4 variant) */
#define SIPHASH_PERMUTATION(a, b, c, d)                                        \
  ((a) += (b), (b) = ROL64((b), 13), (b) ^= (a), (a) = ROL64((a), 32),         \
   (c) += (d), (d) = ROL64((d), 16), (d) ^= (c), (a) += (d),                   \
   (d) = ROL64((d), 21), (d) ^= (a), (c) += (b), (b) = ROL64((b), 17),         \
   (b) ^= (c), (c) = ROL64((c), 32))

/* SipHash initialization constants */
#define SIPHASH_CONST_0 0x736f6d6570736575ULL
#define SIPHASH_CONST_1 0x646f72616e646f6dULL
#define SIPHASH_CONST_2 0x6c7967656e657261ULL
#define SIPHASH_CONST_3 0x7465646279746573ULL

/* HalfSipHash permutation (1-3 variant for hash tables) */
#define HSIPHASH_PERMUTATION(a, b, c, d) SIPHASH_PERMUTATION(a, b, c, d)
#define HSIPHASH_CONST_0 SIPHASH_CONST_0
#define HSIPHASH_CONST_1 SIPHASH_CONST_1
#define HSIPHASH_CONST_2 SIPHASH_CONST_2
#define HSIPHASH_CONST_3 SIPHASH_CONST_3

#endif /* _SIPHASH_H */
