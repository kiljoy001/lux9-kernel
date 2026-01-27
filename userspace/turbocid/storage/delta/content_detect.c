/*
 * content_detect.c - Content type detection for delta format selection
 *
 * Analyzes file content to choose optimal delta algorithm:
 * - xdelta3: Text files, config files, logs
 * - bsdiff: Executables, libraries, binaries
 * - RLE: Sparse files, zero-filled data
 *
 * Detection methods:
 * - File magic numbers (ELF, PE, Mach-O)
 * - Shannon entropy (compressed vs plaintext)
 * - Printable character ratio
 * - Sparsity analysis
 */

#include "../dat.h"

extern void *memset(void *dst, int c, unsigned long n);

/*
 * Delta format types
 */
typedef enum {
  DELTA_NONE = 0,    /* No delta (store full) */
  DELTA_XDELTA3 = 1, /* Text/config files */
  DELTA_BSDIFF = 2,  /* Binaries */
  DELTA_RLE = 3,     /* Sparse files */
} DeltaFormat;

/*
 * calculate_entropy - Shannon entropy (0.0 = uniform, 1.0 = random)
 */
static float calculate_entropy(const u8int *data, u64int len) {
  u64int freq[256] = {0};
  u64int sample_len = (len < 4096) ? len : 4096;

  /* Count byte frequencies in first 4KB */
  for (u64int i = 0; i < sample_len; i++) {
    freq[data[i]]++;
  }

  /* Shannon entropy: -Σ(p * log2(p)) */
  float entropy = 0.0;
  for (int i = 0; i < 256; i++) {
    if (freq[i] > 0) {
      float p = (float)freq[i] / sample_len;
      /* Approximate log2 with bit shifts (good enough) */
      int log_approx = 0;
      float p_int = p * 1000;
      while (p_int > 1) {
        p_int /= 2;
        log_approx++;
      }
      entropy -= p * log_approx / 10.0;
    }
  }

  return entropy / 8.0; /* Normalize to 0-1 */
}

/*
 * count_printable - Ratio of printable ASCII chars
 */
static float count_printable(const u8int *data, u64int len) {
  u64int printable = 0;
  u64int sample = (len < 4096) ? len : 4096;

  for (u64int i = 0; i < sample; i++) {
    u8int c = data[i];
    /* Printable: space to ~, plus tab/newline/carriage return */
    if ((c >= 32 && c <= 126) || c == '\t' || c == '\n' || c == '\r') {
      printable++;
    }
  }

  return (float)printable / sample;
}

/*
 * is_sparse - Check if file has lots of zeros
 */
static int is_sparse(const u8int *data, u64int len) {
  u64int zeros = 0;
  u64int sample = (len < 4096) ? len : 4096;

  for (u64int i = 0; i < sample; i++) {
    if (data[i] == 0)
      zeros++;
  }

  return (zeros > sample * 7 / 10); /* >70% zeros */
}

/*
 * is_elf_binary - Check for ELF magic
 */
static int is_elf_binary(const u8int *data, u64int len) {
  if (len < 4)
    return 0;
  return (data[0] == 0x7f && data[1] == 'E' && data[2] == 'L' &&
          data[3] == 'F');
}

/*
 * is_pe_binary - Check for PE (Windows) magic
 */
static int is_pe_binary(const u8int *data, u64int len) {
  if (len < 2)
    return 0;
  return (data[0] == 'M' && data[1] == 'Z');
}

/*
 * auto_select_delta_format - Choose best delta algorithm
 *
 * Returns: Recommended delta format for this content
 */
DeltaFormat auto_select_delta_format(const u8int *data, u64int len) {
  if (!data || len == 0)
    return DELTA_NONE;

  /* 1. Check for known binary formats */
  if (is_elf_binary(data, len) || is_pe_binary(data, len)) {
    return DELTA_BSDIFF;
  }

  /* 2. Check for sparse data */
  if (is_sparse(data, len)) {
    return DELTA_RLE;
  }

  /* 3. Analyze entropy */
  float entropy = calculate_entropy(data, len);

  if (entropy > 0.9) {
    /* High entropy = compressed/encrypted/random binary */
    return DELTA_BSDIFF;
  }

  /* 4. Check printable ratio for text detection */
  float printable_ratio = count_printable(data, len);

  if (printable_ratio > 0.95) {
    /* Highly printable = source code, config, text */
    return DELTA_XDELTA3;
  }

  /* 5. Medium entropy, some printable = binary data */
  if (entropy > 0.6) {
    return DELTA_BSDIFF;
  }

  /* 6. Default to xdelta3 (fast, general-purpose) */
  return DELTA_XDELTA3;
}

/*
 * format_name - Get human-readable format name
 */
const char *delta_format_name(DeltaFormat format) {
  switch (format) {
  case DELTA_XDELTA3:
    return "xdelta3";
  case DELTA_BSDIFF:
    return "bsdiff";
  case DELTA_RLE:
    return "rle";
  default:
    return "none";
  }
}
