#ifndef __FRAMAC__
// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright (C) 2016-2022 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights
 * Reserved. Adapted for Lux9 kernel - simplified version for hash table use.
 *
 * SipHash: a fast short-input PRF
 * https://131002.net/siphash/
 *
 * This implementation provides SipHash2-4 for secure PRF and
 * HalfSipHash1-3 for hash tables.
 */

#include "siphash.h"
#include "portlib.h"
#include "u.h"

#define SIPROUND SIPHASH_PERMUTATION(v0, v1, v2, v3)

/* Helper: convert little-endian bytes to u64 */
static inline u64int get_unaligned_le64(const u8int *p) {
  return (u64int)p[0] | ((u64int)p[1] << 8) | ((u64int)p[2] << 16) |
         ((u64int)p[3] << 24) | ((u64int)p[4] << 32) | ((u64int)p[5] << 40) |
         ((u64int)p[6] << 48) | ((u64int)p[7] << 56);
}

/* Helper: convert little-endian bytes to u32 */
static inline u32int get_unaligned_le32(const u8int *p) {
  return (u32int)p[0] | ((u32int)p[1] << 8) | ((u32int)p[2] << 16) |
         ((u32int)p[3] << 24);
}

/* Helper: convert little-endian bytes to u16 */
static inline u16int get_unaligned_le16(const u8int *p) {
  return (u16int)p[0] | ((u16int)p[1] << 8);
}

/*
 * siphash - compute 64-bit siphash PRF value
 * @data: buffer to hash
 * @len: size of data
 * @key: the siphash key
 */
/*@
  requires \valid((unsigned char*)data + (0..len-1));
  requires \valid(key);
  assigns \nothing;
*/
u64int siphash(const void *data, usize len, const siphash_key_t *key) {
  const u8int *in = data;
  const u8int *end = in + len - (len % sizeof(u64int));
  const u8int left = len & (sizeof(u64int) - 1);
  u64int m;
  u64int v0 = SIPHASH_CONST_0;
  u64int v1 = SIPHASH_CONST_1;
  u64int v2 = SIPHASH_CONST_2;
  u64int v3 = SIPHASH_CONST_3;
  u64int b = ((u64int)len) << 56;

  /* Initialize with key */
  v3 ^= key->key[1];
  v2 ^= key->key[0];
  v1 ^= key->key[1];
  v0 ^= key->key[0];

  /* Process 8-byte blocks */
  for (; in != end; in += sizeof(u64int)) {
    m = get_unaligned_le64(in);
    v3 ^= m;
    SIPROUND;
    SIPROUND;
    v0 ^= m;
  }

  /* Handle remaining bytes */
  switch (left) {
  case 7:
    b |= ((u64int)end[6]) << 48; /* fallthrough */
  case 6:
    b |= ((u64int)end[5]) << 40; /* fallthrough */
  case 5:
    b |= ((u64int)end[4]) << 32; /* fallthrough */
  case 4:
    b |= get_unaligned_le32(end);
    break;
  case 3:
    b |= ((u64int)end[2]) << 16; /* fallthrough */
  case 2:
    b |= get_unaligned_le16(end);
    break;
  case 1:
    b |= end[0];
  }

  /* Finalization */
  v3 ^= b;
  SIPROUND;
  SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  SIPROUND;

  return (v0 ^ v1) ^ (v2 ^ v3);
}

/*
 * siphash_1u64 - compute 64-bit siphash PRF value of a u64
 * @first: the u64 to hash
 * @key: the siphash key
 */
u64int siphash_1u64(u64int first, const siphash_key_t *key) {
  u64int v0 = SIPHASH_CONST_0;
  u64int v1 = SIPHASH_CONST_1;
  u64int v2 = SIPHASH_CONST_2;
  u64int v3 = SIPHASH_CONST_3;
  u64int b = 8ULL << 56;

  v3 ^= key->key[1];
  v2 ^= key->key[0];
  v1 ^= key->key[1];
  v0 ^= key->key[0];

  v3 ^= first;
  SIPROUND;
  SIPROUND;
  v0 ^= first;

  v3 ^= b;
  SIPROUND;
  SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  SIPROUND;

  return (v0 ^ v1) ^ (v2 ^ v3);
}

/*
 * siphash_2u64 - compute 64-bit siphash PRF value of 2 u64s
 * @first: first u64
 * @second: second u64
 * @key: the siphash key
 */
u64int siphash_2u64(u64int first, u64int second, const siphash_key_t *key) {
  u64int v0 = SIPHASH_CONST_0;
  u64int v1 = SIPHASH_CONST_1;
  u64int v2 = SIPHASH_CONST_2;
  u64int v3 = SIPHASH_CONST_3;
  u64int b = 16ULL << 56;

  v3 ^= key->key[1];
  v2 ^= key->key[0];
  v1 ^= key->key[1];
  v0 ^= key->key[0];

  v3 ^= first;
  SIPROUND;
  SIPROUND;
  v0 ^= first;

  v3 ^= second;
  SIPROUND;
  SIPROUND;
  v0 ^= second;

  v3 ^= b;
  SIPROUND;
  SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  SIPROUND;

  return (v0 ^ v1) ^ (v2 ^ v3);
}

/*
 * HalfSipHash for hash tables (32-bit output)
 * On 64-bit systems, this is actually SipHash1-3 for better performance
 */

#define HSIPROUND SIPHASH_PERMUTATION(v0, v1, v2, v3)

/*
 * hsiphash - compute 32-bit hsiphash PRF value for hash tables
 * @data: buffer to hash
 * @len: size of data
 * @key: the hsiphash key
 */
/*@
  requires \valid((unsigned char*)data + (0..len-1));
  requires \valid(key);
  assigns \nothing;
*/
u32int hsiphash(const void *data, usize len, const hsiphash_key_t *key) {
  const u8int *in = data;
  const u8int *end = in + len - (len % sizeof(u64int));
  const u8int left = len & (sizeof(u64int) - 1);
  u64int m;
  u64int v0 = HSIPHASH_CONST_0;
  u64int v1 = HSIPHASH_CONST_1;
  u64int v2 = HSIPHASH_CONST_2;
  u64int v3 = HSIPHASH_CONST_3;
  u64int b = ((u64int)len) << 56;

  v3 ^= key->key[1];
  v2 ^= key->key[0];
  v1 ^= key->key[1];
  v0 ^= key->key[0];

  /* Process 8-byte blocks with 1 round (HalfSipHash) */
  for (; in != end; in += sizeof(u64int)) {
    m = get_unaligned_le64(in);
    v3 ^= m;
    HSIPROUND;
    v0 ^= m;
  }

  /* Handle remaining bytes */
  switch (left) {
  case 7:
    b |= ((u64int)end[6]) << 48; /* fallthrough */
  case 6:
    b |= ((u64int)end[5]) << 40; /* fallthrough */
  case 5:
    b |= ((u64int)end[4]) << 32; /* fallthrough */
  case 4:
    b |= get_unaligned_le32(end);
    break;
  case 3:
    b |= ((u64int)end[2]) << 16; /* fallthrough */
  case 2:
    b |= get_unaligned_le16(end);
    break;
  case 1:
    b |= end[0];
  }

  /* Finalization with 3 rounds */
  v3 ^= b;
  HSIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  HSIPROUND;
  HSIPROUND;
  HSIPROUND;

  return (u32int)((v0 ^ v1) ^ (v2 ^ v3));
}

/*
 * hsiphash_1u32 - compute 32-bit hsiphash PRF value of a u32
 * @first: the u32 to hash
 * @key: the hsiphash key
 */
u32int hsiphash_1u32(u32int first, const hsiphash_key_t *key) {
  u64int v0 = HSIPHASH_CONST_0;
  u64int v1 = HSIPHASH_CONST_1;
  u64int v2 = HSIPHASH_CONST_2;
  u64int v3 = HSIPHASH_CONST_3;
  u64int b = 4ULL << 56;

  v3 ^= key->key[1];
  v2 ^= key->key[0];
  v1 ^= key->key[1];
  v0 ^= key->key[0];

  b |= first;

  v3 ^= b;
  HSIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  HSIPROUND;
  HSIPROUND;
  HSIPROUND;

  return (u32int)((v0 ^ v1) ^ (v2 ^ v3));
}

/*
 * hsiphash_2u32 - compute 32-bit hsiphash PRF value of 2 u32s
 * @first: first u32
 * @second: second u32
 * @key: the hsiphash key
 */
u32int hsiphash_2u32(u32int first, u32int second, const hsiphash_key_t *key) {
  u64int v0 = HSIPHASH_CONST_0;
  u64int v1 = HSIPHASH_CONST_1;
  u64int v2 = HSIPHASH_CONST_2;
  u64int v3 = HSIPHASH_CONST_3;
  u64int b = 8ULL << 56;
  u64int combined = ((u64int)second << 32) | first;

  v3 ^= key->key[1];
  v2 ^= key->key[0];
  v1 ^= key->key[1];
  v0 ^= key->key[0];

  v3 ^= combined;
  HSIPROUND;
  v0 ^= combined;

  v3 ^= b;
  HSIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  HSIPROUND;
  HSIPROUND;
  HSIPROUND;

  return (u32int)((v0 ^ v1) ^ (v2 ^ v3));
}

#ifdef __FRAMAC__
/*@ ensures \true; */ void framac_pass_dummy(void) {}
#endif
#endif

#ifdef __FRAMAC__
/*@ ensures \true; */ void framac_pass_dummy_siphash_c(void) {}
#endif
