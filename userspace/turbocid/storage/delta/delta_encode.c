/*
 * delta_encode.c - Multi-format delta encoder/decoder dispatcher
 *
 * Provides unified interface for different delta algorithms:
 * - xdelta3: Fast, general-purpose (text/config)
 * - bsdiff: Binary-optimized (executables) [stub for now]
 * - RLE: Sparse file compression [stub for now]
 *
 * Auto-selects best format based on content type.
 */

#include "../dat.h"

/* Import delta format types */
typedef enum {
  DELTA_NONE = 0,
  DELTA_XDELTA3 = 1,
  DELTA_BSDIFF = 2,
  DELTA_RLE = 3,
} DeltaFormat;

/* External functions */
extern int xdelta3_encode(u8int *base, u64int base_len, u8int *target,
                          u64int target_len, u8int *delta_out,
                          u32int *delta_len);

extern int xdelta3_decode(u8int *base, u64int base_len, u8int *delta,
                          u32int delta_len, u8int *target_out,
                          u64int target_len);

extern DeltaFormat auto_select_delta_format(const u8int *data, u64int len);

/*
 * encode_delta - Encode delta using specified format
 *
 * Returns: 0 on success, -1 on error
 */
int encode_delta(u8int *base, u64int base_len, u8int *target, u64int target_len,
                 DeltaFormat format, u8int *delta_out, u32int *delta_len_out) {

  if (!base || !target || !delta_out || !delta_len_out)
    return -1;

  switch (format) {
  case DELTA_XDELTA3:
    return xdelta3_encode(base, base_len, target, target_len, delta_out,
                          delta_len_out);

  case DELTA_BSDIFF:
    /* TODO: Phase 4 - implement bsdiff */
    return -1;

  case DELTA_RLE:
    /* TODO: Phase 4 - implement RLE delta */
    return -1;

  default:
    return -1;
  }
}

/*
 * decode_delta - Decode delta using specified format
 *
 * Returns: 0 on success, -1 on error
 */
int decode_delta(u8int *base, u64int base_len, u8int *delta, u32int delta_len,
                 DeltaFormat format, u8int *target_out, u64int target_len) {

  if (!base || !delta || !target_out)
    return -1;

  switch (format) {
  case DELTA_XDELTA3:
    return xdelta3_decode(base, base_len, delta, delta_len, target_out,
                          target_len);

  case DELTA_BSDIFF:
    /* TODO: Phase 4 - implement bsdiff decode */
    return -1;

  case DELTA_RLE:
    /* TODO: Phase 4 - implement RLE decode */
    return -1;

  default:
    return -1;
  }
}

/*
 * encode_delta_auto - Auto-select format and encode
 *
 * Automatically chooses the best delta format based on content.
 * Returns: 0 on success, -1 on error
 * Sets format_out to the chosen format
 */
int encode_delta_auto(u8int *base, u64int base_len, u8int *target,
                      u64int target_len, u8int *delta_out,
                      u32int *delta_len_out, DeltaFormat *format_out) {

  /* Auto-select format */
  DeltaFormat format = auto_select_delta_format(target, target_len);

  /* Try encoding */
  int rc = encode_delta(base, base_len, target, target_len, format, delta_out,
                        delta_len_out);

  if (rc == 0 && format_out) {
    *format_out = format;
  }

  return rc;
}

/*
 * encode_delta_with_fallback - Try primary format, fall back if needed
 *
 * Tries the auto-selected format first.
 * If delta is >80% of original size, returns -1 (caller should store FULL).
 */
int encode_delta_with_fallback(u8int *base, u64int base_len, u8int *target,
                               u64int target_len, u8int *delta_out,
                               u32int *delta_len_out, DeltaFormat *format_out) {

  DeltaFormat format;
  int rc = encode_delta_auto(base, base_len, target, target_len, delta_out,
                             delta_len_out, &format);

  if (rc != 0) {
    return -1;
  }

  /* Check if delta is worth it */
  if (*delta_len_out >= (target_len * 8 / 10)) {
    /* Delta is >= 80% of original size - not efficient */
    return -1;
  }

  if (format_out) {
    *format_out = format;
  }

  return 0;
}
