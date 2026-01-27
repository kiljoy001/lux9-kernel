/*
 * TLSH (Trend Micro Locality Sensitive Hash)
 *
 * Core implementation of the Trend Micro Locality Sensitive Hash.
 * 35-byte output encodes statistical properties of content for similarity
 * detection.
 */

#include "../dat.h"

extern void *memset(void *dst, int c, unsigned long n);

/* Pearson hash lookup table (Official TLSH Permutation) */
static const u8int pearson_table[256] = {
    1,   87,  49,  12,  176, 178, 102, 166, 121, 193, 6,   84,  249, 230,
    44,  163, 14,  197, 213, 181, 161, 85,  218, 80,  64,  239, 24,  226,
    236, 142, 38,  200, 110, 177, 104, 103, 141, 253, 255, 50,  77,  101,
    81,  18,  45,  96,  31,  222, 25,  107, 190, 70,  86,  237, 240, 34,
    72,  242, 20,  214, 244, 227, 149, 235, 97,  234, 57,  22,  60,  250,
    82,  175, 208, 5,   127, 199, 111, 62,  135, 248, 174, 169, 211, 58,
    66,  154, 106, 195, 245, 171, 17,  187, 182, 179, 0,   243, 132, 56,
    148, 75,  128, 133, 158, 100, 130, 126, 91,  13,  153, 246, 216, 219,
    119, 68,  223, 78,  83,  88,  201, 99,  122, 11,  92,  32,  136, 114,
    52,  10,  138, 30,  48,  183, 156, 35,  61,  26,  143, 74,  251, 94,
    129, 162, 63,  152, 170, 7,   115, 167, 241, 206, 3,   150, 55,  59,
    151, 220, 90,  53,  23,  131, 125, 173, 15,  238, 79,  95,  89,  16,
    105, 137, 225, 224, 217, 160, 37,  123, 118, 73,  2,   157, 46,  116,
    9,   145, 134, 228, 207, 212, 202, 215, 69,  229, 27,  188, 67,  124,
    168, 252, 42,  4,   29,  108, 21,  247, 19,  205, 39,  203, 233, 40,
    186, 147, 198, 192, 155, 33,  164, 191, 98,  204, 165, 180, 117, 76,
    140, 36,  210, 172, 41,  54,  159, 8,   185, 232, 113, 196, 231, 47,
    146, 120, 51,  65,  28,  144, 254, 221, 93,  189, 194, 139, 112, 43,
    71,  109, 184, 209};

#define TLSH_BUCKETS 128
#define TLSH_MIN_LEN 50

/* Pearson hash of data with salt */
static u8int pearson_hash_salt(u8int h, u8int salt, u8int data) {
  return pearson_table[h ^ salt ^ data];
}

/*
 * L-value mapping (Official Log-scale)
 * Map length into a 1-byte value
 */
static u8int l_value_encode(u64int len) {
  if (len <= 656) {
    return (u8int)(len / 4);
  } else if (len <= 3199) {
    return (u8int)(len / 16 + 124);
  } else if (len <= 15535) {
    return (u8int)(len / 64 + 174);
  } else {
    return (u8int)(len / 256 + 210);
  }
}

static u8int ratio_to_code(u8int ratio) {
  u8int code = (u8int)(ratio / 6);
  return code > 15 ? 15 : code;
}

/* Distance between two codes (2-bit codes: 0, 1, 2, 3) */
static int bucket_dist(u8int a, u8int b) {
  static const int dist_table[4][4] = {
      {0, 1, 3, 6}, {1, 0, 1, 3}, {3, 1, 0, 1}, {6, 3, 1, 0}};
  return dist_table[a & 0x03][b & 0x03];
}

int tlsh_hash(const u8int *data, u64int len, u8int tlsh_out[35]) {
  if (!data || !tlsh_out || len < TLSH_MIN_LEN) {
    return -1;
  }

  u32int buckets[TLSH_BUCKETS] = {0};
  u8int checksum = 0;

  /* Pearson hash over 5-byte sliding window with triplet mapping */
  for (u64int i = 0; i + 5 <= len; i++) {
    u8int s1 = data[i];
    u8int s2 = data[i + 1];
    u8int s3 = data[i + 2];
    u8int s4 = data[i + 3];
    u8int s5 = data[i + 4];

    /* Update checksum */
    checksum = pearson_table[checksum ^ s1];

    /* Triplet Mapping (Official Trigrams)
     * We generate 6 triplets from the 5-byte window.
     * Salts: 2, 3, 5, 7, 11, 13
     */
    u8int t1 = pearson_hash_salt(0, 2, s1);
    t1 = pearson_hash_salt(t1, s2, s3);
    buckets[t1 % TLSH_BUCKETS]++;

    u8int t2 = pearson_hash_salt(0, 3, s1);
    t2 = pearson_hash_salt(t2, s2, s4);
    buckets[t2 % TLSH_BUCKETS]++;

    u8int t3 = pearson_hash_salt(0, 5, s1);
    t3 = pearson_hash_salt(t3, s3, s4);
    buckets[t3 % TLSH_BUCKETS]++;

    u8int t4 = pearson_hash_salt(0, 7, s2);
    t4 = pearson_hash_salt(t4, s3, s5);
    buckets[t4 % TLSH_BUCKETS]++;

    u8int t5 = pearson_hash_salt(0, 11, s2);
    t5 = pearson_hash_salt(t5, s4, s5);
    buckets[t5 % TLSH_BUCKETS]++;

    u8int t6 = pearson_hash_salt(0, 13, s3);
    t6 = pearson_hash_salt(t6, s4, s5);
    buckets[t6 % TLSH_BUCKETS]++;
  }

  /* Header */
  tlsh_out[0] = checksum;
  tlsh_out[1] = l_value_encode(len);

  /* Quartiles */
  u32int sorted[TLSH_BUCKETS];
  u32int nonzero = 0;
  for (int i = 0; i < TLSH_BUCKETS; i++) {
    sorted[i] = buckets[i];
    if (buckets[i] > 0)
      nonzero++;
  }

  if (nonzero < 32) {
    return -1;
  }

  /* Selection sort for quartiles (median is enough for simple, but q1/q3 needed
   * for ratios) */
  for (int i = 0; i < 97; i++) { /* Only need up to 75th percentile */
    int min_idx = i;
    for (int j = i + 1; j < TLSH_BUCKETS; j++) {
      if (sorted[j] < sorted[min_idx])
        min_idx = j;
    }
    u32int tmp = sorted[i];
    sorted[i] = sorted[min_idx];
    sorted[min_idx] = tmp;
  }

  u32int q1 = sorted[32];
  u32int q2 = sorted[64];
  u32int q3 = sorted[96];

  if (q3 == 0) {
    return -1;
  }

  /* Q ratios */
  u8int q1_ratio = (u8int)((q1 * 100) / q3);
  u8int q2_ratio = (u8int)((q2 * 100) / q3);
  tlsh_out[2] = (ratio_to_code(q1_ratio) << 4) | ratio_to_code(q2_ratio);

  /* Bucket Encoding (2 bits per bucket, 128 buckets = 32 bytes) */
  memset(&tlsh_out[3], 0, 32);
  for (int i = 0; i < TLSH_BUCKETS; i++) {
    u8int code;
    if (buckets[i] <= q1)
      code = 0;
    else if (buckets[i] <= q2)
      code = 1;
    else if (buckets[i] <= q3)
      code = 2;
    else
      code = 3;

    int byte_idx = 3 + (i / 4);
    int bit_idx = (i % 4) * 2;
    tlsh_out[byte_idx] |= (code << bit_idx);
  }

  return 0;
}

int tlsh_distance(const u8int tlsh1[35], const u8int tlsh2[35]) {
  if (!tlsh1 || !tlsh2)
    return 9999;

  int dist = 0;

  /* Checksum (Binary) */
  if (tlsh1[0] != tlsh2[0])
    dist += 1;

  /* L-value diff */
  int ldiff = (int)tlsh1[1] - (int)tlsh2[1];
  if (ldiff < 0)
    ldiff = -ldiff;
  if (ldiff > 1)
    dist += ldiff * 12;

  /* Q-ratio diff */
  u8int q1_1 = (tlsh1[2] >> 4) & 0xF;
  u8int q2_1 = tlsh1[2] & 0xF;
  u8int q1_2 = (tlsh2[2] >> 4) & 0xF;
  u8int q2_2 = tlsh2[2] & 0xF;

  int q1_diff = (int)q1_1 - (int)q1_2;
  if (q1_diff < 0)
    q1_diff = -q1_diff;
  if (q1_diff > 1)
    dist += q1_diff * 12;

  int q2_diff = (int)q2_1 - (int)q2_2;
  if (q2_diff < 0)
    q2_diff = -q2_diff;
  if (q2_diff > 1)
    dist += q2_diff * 12;

  /* Bucket Encoding Distance */
  for (int i = 0; i < 128; i++) {
    u8int code1 = (tlsh1[3 + (i / 4)] >> ((i % 4) * 2)) & 0x03;
    u8int code2 = (tlsh2[3 + (i / 4)] >> ((i % 4) * 2)) & 0x03;
    dist += bucket_dist(code1, code2);
  }

  return dist;
}
