/*
 * xdelta3_simple.c - Simplified xdelta3 implementation for Sophia
 *
 * A lightweight delta encoder/decoder using sliding window matching.
 * Optimized for text files and source code.
 *
 * Algorithm:
 *   1. Compute rolling checksums (Adler32) of source blocks
 *   2. Scan target for matching blocks
 *   3. Emit COPY/INSERT operations
 *
 * Format:
 *   [op_type:1] [length:varint] [data/offset:varint]
 *   - COPY: copy from base at offset
 *   - INSERT: insert literal data
 */

#include "dat.h"

extern void *memset(void *dst, int c, unsigned long n);
extern void *memmove(void *dst, const void *src, unsigned long n);
extern void *malloc(unsigned long size);
extern void free(void *ptr);

/*
 * Delta operation types
 */
typedef enum {
  XDELTA_INSERT = 0, /* Insert literal bytes */
  XDELTA_COPY = 1,   /* Copy from base */
} XDeltaOp;

/*
 * Delta header
 */
typedef struct {
  u32int magic;      /* 'XD3' + version */
  u32int base_len;   /* Length of base (for verification) */
  u32int target_len; /* Length of target (reconstructed) */
} XDeltaHeader;

#define XDELTA_MAGIC 0x58443301 /* 'XD3' + version 1 */
#define XDELTA_WINDOW 16        /* Window size for matching */
#define XDELTA_MAX_MATCH 255    /* Max match length */

/*
 * Adler32 rolling checksum
 */
typedef struct {
  u32int a;
  u32int b;
} Adler32;

static u32int adler32_sum(Adler32 *ctx) { return (ctx->b << 16) | ctx->a; }

static void adler32_init(Adler32 *ctx) {
  ctx->a = 1;
  ctx->b = 0;
}

static void adler32_update(Adler32 *ctx, u8int byte_in, u8int byte_out,
                           int first) {
  if (first) {
    ctx->a = 1;
    ctx->b = 0;
  }

  /* Remove old byte */
  ctx->a = (ctx->a - byte_out + 256) & 0xFFFF;
  ctx->b = (ctx->b - XDELTA_WINDOW * byte_out + 256 * XDELTA_WINDOW) & 0xFFFF;

  /* Add new byte */
  ctx->a = (ctx->a + byte_in) & 0xFFFF;
  ctx->b = (ctx->b + ctx->a) & 0xFFFF;
}

/*
 * Varint encoding (saves space in delta)
 */
static int encode_varint(u8int *buf, u32int value) {
  int len = 0;
  while (value >= 0x80) {
    buf[len++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  buf[len++] = value & 0x7F;
  return len;
}

static int decode_varint(u8int *buf, u32int *value) {
  *value = 0;
  int shift = 0;
  int len = 0;

  while (len < 5) { /* Max 5 bytes for u32 */
    u8int byte = buf[len++];
    *value |= ((u32int)(byte & 0x7F)) << shift;
    if (!(byte & 0x80))
      break;
    shift += 7;
  }

  return len;
}

/*
 * xdelta3_encode - Create delta from base to target
 *
 * Returns: 0 on success, -1 on error
 * delta_len: bytes written to delta_out
 */
int xdelta3_encode(u8int *base, u64int base_len, u8int *target,
                   u64int target_len, u8int *delta_out, u32int *delta_len) {

  if (!base || !target || !delta_out || !delta_len)
    return -1;

  /* Write header */
  XDeltaHeader *hdr = (XDeltaHeader *)delta_out;
  hdr->magic = XDELTA_MAGIC;
  hdr->base_len = base_len;
  hdr->target_len = target_len;

  u32int delta_pos = sizeof(XDeltaHeader);
  u32int target_pos = 0;

  /* Build checksum table for base */
  /* TODO: For phase 1, use simple byte-by-byte matching */
  /* Phase 2: Add proper rolling checksum hash table */

  while (target_pos < target_len) {
    /* Try to find match in base */
    u32int best_match_offset = 0;
    u32int best_match_len = 0;

    /* Simple O(n²) matching for now - optimize later */
    for (u32int base_pos = 0; base_pos < base_len; base_pos++) {
      u32int match_len = 0;
      while (match_len < XDELTA_MAX_MATCH &&
             target_pos + match_len < target_len &&
             base_pos + match_len < base_len &&
             target[target_pos + match_len] == base[base_pos + match_len]) {
        match_len++;
      }

      if (match_len > best_match_len && match_len >= 4) { /* Min 4 bytes */
        best_match_len = match_len;
        best_match_offset = base_pos;
      }
    }

    if (best_match_len >= 4) {
      /* Emit COPY operation */
      delta_out[delta_pos++] = XDELTA_COPY;
      delta_pos += encode_varint(&delta_out[delta_pos], best_match_len);
      delta_pos += encode_varint(&delta_out[delta_pos], best_match_offset);
      target_pos += best_match_len;
    } else {
      /* Emit INSERT operation */
      /* Collect literal bytes */
      u32int literal_start = target_pos;
      while (target_pos < target_len && (target_pos - literal_start) < 127) {
        /* Check if next byte would match */
        int found_match = 0;
        for (u32int base_pos = 0; base_pos < base_len - 4; base_pos++) {
          if (target[target_pos] == base[base_pos] &&
              target[target_pos + 1] == base[base_pos + 1] &&
              target[target_pos + 2] == base[base_pos + 2] &&
              target[target_pos + 3] == base[base_pos + 3]) {
            found_match = 1;
            break;
          }
        }
        if (found_match)
          break;
        target_pos++;
      }

      u32int literal_len = target_pos - literal_start;
      delta_out[delta_pos++] = XDELTA_INSERT;
      delta_pos += encode_varint(&delta_out[delta_pos], literal_len);
      memmove(&delta_out[delta_pos], &target[literal_start], literal_len);
      delta_pos += literal_len;
    }
  }

  *delta_len = delta_pos;
  return 0;
}

/*
 * xdelta3_decode - Reconstruct target from base + delta
 *
 * Returns: 0 on success, -1 on error
 */
int xdelta3_decode(u8int *base, u64int base_len, u8int *delta, u32int delta_len,
                   u8int *target_out, u64int target_len) {

  if (!base || !delta || !target_out)
    return -1;

  /* Verify header */
  XDeltaHeader *hdr = (XDeltaHeader *)delta;
  if (hdr->magic != XDELTA_MAGIC)
    return -1;
  if (hdr->base_len != base_len)
    return -1;
  if (hdr->target_len != target_len)
    return -1;

  u32int delta_pos = sizeof(XDeltaHeader);
  u32int target_pos = 0;

  while (delta_pos < delta_len && target_pos < target_len) {
    u8int op = delta[delta_pos++];
    u32int length, offset;

    switch (op) {
    case XDELTA_COPY:
      delta_pos += decode_varint(&delta[delta_pos], &length);
      delta_pos += decode_varint(&delta[delta_pos], &offset);

      /* Copy from base */
      if (offset + length > base_len || target_pos + length > target_len)
        return -1;

      memmove(&target_out[target_pos], &base[offset], length);
      target_pos += length;
      break;

    case XDELTA_INSERT:
      delta_pos += decode_varint(&delta[delta_pos], &length);

      /* Insert literal */
      if (delta_pos + length > delta_len || target_pos + length > target_len)
        return -1;

      memmove(&target_out[target_pos], &delta[delta_pos], length);
      delta_pos += length;
      target_pos += length;
      break;

    default:
      return -1; /* Invalid operation */
    }
  }

  if (target_pos != target_len)
    return -1;

  return 0;
}
