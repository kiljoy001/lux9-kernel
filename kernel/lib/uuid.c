/* uuid.c - UUID Library */
#include "../include/uuid.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/u.h"

/* Helper for hex conversion (Plan 9 / Kernel env usually has library functions,
 * but implementing minimal self-contained) */
static int hex_val(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static char hex_char(int v) {
  if (v >= 0 && v <= 9)
    return '0' + v;
  if (v >= 10 && v <= 15)
    return 'a' + (v - 10);
  return '0';
}

void uuid_clear(uuid_t *u) {
  if (!u)
    return;
  for (int i = 0; i < 16; i++)
    u->data[i] = 0;
}

int uuid_compare(const uuid_t *a, const uuid_t *b) {
  if (!a || !b)
    return 0; // Undefined safe
  for (int i = 0; i < 16; i++) {
    if (a->data[i] < b->data[i])
      return -1;
    if (a->data[i] > b->data[i])
      return 1;
  }
  return 0;
}

void uuid_copy(uuid_t *dst, const uuid_t *src) {
  if (!dst || !src)
    return;
  for (int i = 0; i < 16; i++)
    dst->data[i] = src->data[i];
}

/* Parse standard UUID string: 8-4-4-4-12 */
int uuid_parse(const char *in, uuid_t *uu) {
  int i = 0;
  const char *p = in;

  if (!uu || !in)
    return -1;

  for (i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      if (*p == '-')
        p++;
    }
    int h1 = hex_val(*p++);
    int h2 = hex_val(*p++);
    if (h1 < 0 || h2 < 0)
      return -1;
    uu->data[i] = (h1 << 4) | h2;
  }
  return 0;
}

/* Format: 36 bytes + null */
void uuid_unparse(const uuid_t *uu, char *out) {
  if (!uu || !out)
    return;
  const unsigned char *d = uu->data;
  char *p = out;

  for (int i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      *p++ = '-';
    }
    *p++ = hex_char(d[i] >> 4);
    *p++ = hex_char(d[i] & 0x0F);
  }
  *p = 0;
}

int uuid_is_null(const uuid_t *uu) {
  if (!uu)
    return 1;
  for (int i = 0; i < 16; i++) {
    if (uu->data[i] != 0)
      return 0;
  }
  return 1;
}

/* Generate UUIDv8 (Custom/Experimental) per RFC 9562 */
void uuid_new_v8(uuid_t *u) {
  /* Use non-blocking ChaCha20 CSPRNG to avoid qlock hang in early exec */
  extern void chacha20_csprng_fill(u8int * buf, ulong len);

  if (!u)
    return;

  /* 60 bits of timestamp (nanoseconds), 62 bits of randomness */

  /* Get time: fastticks to ns */
  uvlong ns = fastticks2ns(fastticks(nil));

  /* Fill with random first using non-blocking CSPRNG */
  chacha20_csprng_fill(u->data, 16);

  /* Overlay timestamp into first 60 bits (custom_a and custom_b) */
  /* custom_a: 48 bits (Bytes 0-5) */
  /* custom_b: 12 bits (Bytes 6-7 high) */

  /* We use the lower 60 bits of the nanosecond timestamp */
  u->data[0] = (ns >> 52) & 0xFF;
  u->data[1] = (ns >> 44) & 0xFF;
  u->data[2] = (ns >> 36) & 0xFF;
  u->data[3] = (ns >> 28) & 0xFF;
  u->data[4] = (ns >> 20) & 0xFF;
  u->data[5] = (ns >> 12) & 0xFF;

  /* custom_b (12 bits) + ver (4 bits) */
  /* Byte 6: custom_b high 4 bits + ver (bits 48-51=ver, 52-55=custom_b_high) */
  /* Actually RFC says: | custom_a | ver | custom_b |
     Bytes: 0-5 (48) | 6 (hi nibble ver) | ...

     Byte 6:
     hi nibble: ver
     lo nibble: custom_b (top 4 bits)

     Byte 7:
     custom_b (low 8 bits)
  */

  uvlong ts_high = (ns >> 12) & 0xFFFFFFFFFFFFULL;
  u16int ts_low = ns & 0xFFF;

  u->data[0] = (ts_high >> 40) & 0xFF;
  u->data[1] = (ts_high >> 32) & 0xFF;
  u->data[2] = (ts_high >> 24) & 0xFF;
  u->data[3] = (ts_high >> 16) & 0xFF;
  u->data[4] = (ts_high >> 8) & 0xFF;
  u->data[5] = ts_high & 0xFF;

  /* Version 8: 1000 */
  /* Byte 6: Ver(4) | Custom_B_Hi(4) */
  u->data[6] = (0x08 << 4) | ((ts_low >> 8) & 0x0F);
  u->data[7] = ts_low & 0xFF;

  /* Variant: 10xx */
  /* Byte 8 (bits 64-71): var (2 bits) | custom_c (6 bits) */
  u->data[8] = (u->data[8] & 0x3F) | 0x80;
}

/* Pack custom data into UUIDv8 payload (122 bits)
   data_a (48 bits): Bytes 0-5
   data_b (12 bits): Bytes 6-7 (shared with ver)
   data_c (62 bits): Bytes 8-15 (shared with var)
*/
void uuid_pack_v8(uuid_t *u, unsigned long long data_a, unsigned short data_b,
                  unsigned long long data_c) {
  if (!u)
    return;

  /* custom_a: 48 bits */
  u->data[0] = (data_a >> 40) & 0xFF;
  u->data[1] = (data_a >> 32) & 0xFF;
  u->data[2] = (data_a >> 24) & 0xFF;
  u->data[3] = (data_a >> 16) & 0xFF;
  u->data[4] = (data_a >> 8) & 0xFF;
  u->data[5] = data_a & 0xFF;

  /* custom_b: 12 bits */
  /* Byte 6: Ver(4) | custom_b_hi(4) */
  /* Version 8: 1000 */
  u->data[6] = (0x08 << 4) | ((data_b >> 8) & 0x0F);
  u->data[7] = data_b & 0xFF;

  /* custom_c: 62 bits */
  /* Byte 8: Var(2) | custom_c_hi(6) */
  /* Variant: 10xx (0x80) */
  u->data[8] = 0x80 | ((data_c >> 56) & 0x3F);
  u->data[9] = (data_c >> 48) & 0xFF;
  u->data[10] = (data_c >> 40) & 0xFF;
  u->data[11] = (data_c >> 32) & 0xFF;
  u->data[12] = (data_c >> 24) & 0xFF;
  u->data[13] = (data_c >> 16) & 0xFF;
  u->data[14] = (data_c >> 8) & 0xFF;
  u->data[15] = data_c & 0xFF;
}

/* Pebble Token Packing
   data_a (48 bits): Token ID (32) | Index (16)
   data_b (12 bits): Generation (Low 12)
   data_c (62 bits): Generation (High 20) | Reserved (42)
*/
void uuid_pack_pebble(uuid_t *u, unsigned int token, unsigned int generation,
                      unsigned short index) {
  if (!u)
    return;

  /* Prepare data_a: Token ID (32) << 16 | Index (16) */
  unsigned long long data_a = ((unsigned long long)token << 16) | index;

  /* Prepare data_b: Generation Low 12 bits */
  unsigned short data_b = generation & 0xFFF;

  /* Prepare data_c: Generation High 20 bits */
  /* We place the high 20 bits of generation at the TOP of data_c */
  /* data_c is 62 bits. So GenHigh << (62 - 20) = GenHigh << 42 */
  unsigned long long gen_high = (generation >> 12) & 0xFFFFF;
  unsigned long long data_c = gen_high << 42;

  /* We can use the magic/reserved space later if needed */
  /* Currently reserved bits are 0 */

  uuid_pack_v8(u, data_a, data_b, data_c);
}

int uuid_unpack_pebble(const uuid_t *u, unsigned int *token,
                       unsigned int *generation, unsigned short *index) {
  if (!u)
    return -1;

  /* We need to reverse uuid_pack_v8 manually or access bytes directly */
  /* Accessing bytes directly is safer as uuid_pack_v8 is write-only logic
   * usually */

  const unsigned char *d = u->data;

  /* Version Check: Byte 6 high encoded as 1000 (8) */
  if ((d[6] >> 4) != 8)
    return -1; /* Not UUIDv8 */

  /* Variant Check: Byte 8 high encoded as 10xx (8, 9, A, B) */
  if ((d[8] >> 6) != 2)
    return -1; /* Not Variant 1 */

  /* Data A: Bytes 0-5 (48 bits) */
  unsigned long long data_a = 0;
  for (int i = 0; i < 6; i++) {
    data_a = (data_a << 8) | d[i];
  }

  /* Token = Upper 32, Index = Lower 16 */
  if (token)
    *token = (unsigned int)(data_a >> 16);
  if (index)
    *index = (unsigned short)(data_a & 0xFFFF);

  /* Data B: Byte 6 (low 4) | Byte 7 (8) = 12 bits */
  unsigned int data_b = ((d[6] & 0x0F) << 8) | d[7];

  /* Data C: Byte 8 (low 6) | Bytes 9-15 (56) = 62 bits */
  unsigned long long data_c = (unsigned long long)(d[8] & 0x3F);
  for (int i = 9; i < 16; i++) {
    data_c = (data_c << 8) | d[i];
  }

  /* Generation Construction */
  /* Gen Low 12 = data_b */
  /* Gen High 20 = Top 20 bits of data_c */
  unsigned int gen_low = data_b;
  unsigned int gen_high = (unsigned int)(data_c >> 42);

  if (generation)
    *generation = (gen_high << 12) | gen_low;

  return 0;
}

/* Capability UUID Packing
   Layout:
   data_a (48 bits): PA hash high 48 bits
   data_b (12 bits): Type (8) | Perms (4)
   data_c (62 bits): Epoch (16) | PA hash low 46 bits
*/
void uuid_pack_capability(uuid_t *u, const unsigned char *pa_hash,
                          unsigned short epoch, unsigned char type,
                          unsigned char perms) {
  if (!u || !pa_hash)
    return;

  /* Extract 94 bits from PA hash (32 bytes = 256 bits)
   * We take the first 94 bits (11.75 bytes)
   * High 48 bits: bytes 0-5
   * Low 46 bits: bytes 6-11, upper 6 bits of byte 12
   */

  /* data_a: PA hash high 48 bits (bytes 0-5) */
  unsigned long long data_a = 0;
  for (int i = 0; i < 6; i++) {
    data_a = (data_a << 8) | pa_hash[i];
  }

  /* data_b: Type (8 bits) | Perms (4 bits) = 12 bits */
  unsigned short data_b = ((unsigned short)type << 4) | (perms & 0x0F);

  /* data_c: Epoch (16 bits) | PA hash low (46 bits) = 62 bits
   * PA hash low: bytes 6-11 (48 bits) but we only use 46 bits
   * So: bytes 6-11 = 48 bits, take upper 46 bits (shift right 2)
   */
  unsigned long long pa_low = 0;
  for (int i = 6; i < 12; i++) {
    pa_low = (pa_low << 8) | pa_hash[i];
  }
  /* Take upper 46 bits of pa_low (48 bits → 46 bits) */
  pa_low >>= 2;

  /* Pack: Epoch (16 bits) in high, PA low (46 bits) in low */
  unsigned long long data_c = ((unsigned long long)epoch << 46) | pa_low;

  uuid_pack_v8(u, data_a, data_b, data_c);
}

int uuid_unpack_capability(const uuid_t *u, unsigned short *epoch,
                           unsigned char *type, unsigned char *perms) {
  if (!u)
    return -1;

  const unsigned char *d = u->data;

  /* Version Check: Byte 6 high should be 8 */
  if ((d[6] >> 4) != 8)
    return -1;

  /* Variant Check: Byte 8 high should be 10 */
  if ((d[8] >> 6) != 2)
    return -1;

  /* data_b: Byte 6 (low 4) | Byte 7 (8) = Type (8) | Perms (4) */
  unsigned int data_b = ((d[6] & 0x0F) << 8) | d[7];

  if (type)
    *type = (unsigned char)(data_b >> 4);
  if (perms)
    *perms = (unsigned char)(data_b & 0x0F);

  /* data_c: Byte 8 (low 6) | Bytes 9-15 (56) = 62 bits
   * Epoch (16 bits) in high, PA low (46 bits) in low
   */
  unsigned long long data_c = (unsigned long long)(d[8] & 0x3F);
  for (int i = 9; i < 16; i++) {
    data_c = (data_c << 8) | d[i];
  }

  if (epoch)
    *epoch = (unsigned short)(data_c >> 46);

  return 0;
}

void uuid_get_pa_hash_bits(const uuid_t *u, unsigned char *pa_hash_out) {
  if (!u || !pa_hash_out)
    return;

  const unsigned char *d = u->data;

  /* Extract PA hash (94 bits total):
   * High 48 bits: Bytes 0-5 (data_a)
   * Low 46 bits: From data_c (after epoch)
   */

  /* High 48 bits: bytes 0-5 → pa_hash_out[0-5] */
  for (int i = 0; i < 6; i++) {
    pa_hash_out[i] = d[i];
  }

  /* Low 46 bits: Extract from data_c
   * data_c is in bytes 8-15 (62 bits total)
   * Epoch is high 16 bits, PA low is low 46 bits
   */
  unsigned long long data_c = (unsigned long long)(d[8] & 0x3F);
  for (int i = 9; i < 16; i++) {
    data_c = (data_c << 8) | d[i];
  }

  /* Extract low 46 bits */
  unsigned long long pa_low = data_c & 0x3FFFFFFFFFFFULL; /* 46 bits mask */

  /* Shift back to 48 bits (pad with 2 zero bits on right) */
  pa_low <<= 2;

  /* Store in pa_hash_out[6-11] */
  for (int i = 11; i >= 6; i--) {
    pa_hash_out[i] = (unsigned char)(pa_low & 0xFF);
    pa_low >>= 8;
  }

  /* Zero out remaining bytes (94 bits = 11.75 bytes, so bytes 12-31 are zero)
   */
  for (int i = 12; i < 32; i++) {
    pa_hash_out[i] = 0;
  }
}

/*
 * Lux9 Secure PID2 Implementation
 * Pattern: Cryptographic Passport
 *
 * Layout (122 bits payload):
 * - data_a (48 bits): Ancestry (Timestamp high + Parent Hash high)
 * - data_b (32 bits): Namespace CID (Hash of mount/cap config)
 * - data_c (42 bits): Code Integrity (Hash of .text segment)
 */

/* Helper: extract high 64-bits of a hash array */
static u64int hash_extract_u64(const u8int *hash) {
  u64int v = 0;
  if (!hash)
    return 0;
  /* Big-endian extraction for stability */
  v |= (u64int)hash[0] << 56;
  v |= (u64int)hash[1] << 48;
  v |= (u64int)hash[2] << 40;
  v |= (u64int)hash[3] << 32;
  v |= (u64int)hash[4] << 24;
  v |= (u64int)hash[5] << 16;
  v |= (u64int)hash[6] << 8;
  v |= (u64int)hash[7];
  return v;
}

void uuid_pack_pid_lux9(uuid_t *u, const uuid_t *parent_uuid,
                        const u8int *namespace_cid, const u8int *code_hash) {
  if (!u)
    return;

  /* data_a (48 bits): Ancestry
   * Composition:
   * - Top 32 bits: Monotonic Timestamp (fastticks) for uniqueness
   * - Bottom 16 bits: Top 16 bits of Parent UUID Hash (Provenance)
   */
  extern uvlong fastticks(uvlong * hz); // From kernel
  uvlong now = fastticks(nil);
  u64int parent_sig = 0;

  if (parent_uuid) {
    /* Use the parent's UUID bytes directly as a "hash" since it's already
     * random */
    parent_sig = ((u64int)parent_uuid->data[0] << 8) | parent_uuid->data[1];
  }

  u64int data_a = ((now & 0xFFFFFFFF) << 16) | (parent_sig & 0xFFFF);

  /* data_b (32 bits): Namespace CID
   * Full 32 bits of the Namespace BLAKE2b/SHA256 hash.
   * This is the "Container ID".
   */
  u64int data_b_val = 0;
  if (namespace_cid) {
    /* Take first 4 bytes of CID */
    data_b_val = ((u64int)namespace_cid[0] << 24) |
                 ((u64int)namespace_cid[1] << 16) |
                 ((u64int)namespace_cid[2] << 8) | (u64int)namespace_cid[3];
  }
  unsigned short data_b_parts =
      (unsigned short)(data_b_val >>
                       16); /* High 16? No data_b is 12 bits in std v8?
                               Wait, standard v8 is 12 bits data_b.
                               We are OVERLOADING layout.
                               uuid_pack_v8 takes:
                                 data_a (48)
                                 data_b (12)
                                 data_c (62)
                               Total 122 bits.
                               Our Plan:
                                 data_a: 48 (Ancestry)
                                 data_b: 32 (Namespace) -> Doesn't fit in 12!
                                 data_c: 42 (Code)

                               Adjustment:
                               We must manually map our 32+42 bits into the
                               12+62 slots. Total available: 74 bits (12+62).
                               Proposed: 32 (NS) + 42 (Code) = 74. PERFECT FIT.
                             */

  /*
   * Mapping Strategy:
   * UUID Field "data_b" (12 bits) <- Top 12 bits of Namespace CID
   * UUID Field "data_c" (62 bits) <- Bottom 20 bits of Namespace CID | 42 bits
   * of Code Hash
   */

  u32int ns_val = (u32int)data_b_val; // 32 bits

  // 1. Fill data_b (12 bits) with top 12 bits of NS
  unsigned short data_b_pack = (ns_val >> 20) & 0xFFF;

  // 2. Prepare data_c (62 bits)
  // Top 20 bits = Bottom 20 bits of NS
  // Bottom 42 bits = Top 42 bits of Code Hash
  u64int ns_rem = ns_val & 0xFFFFF; // 20 bits

  u64int code_val = hash_extract_u64(code_hash); // 64 bits
  u64int code_42 = (code_val >> (64 - 42));      // Top 42 bits

  u64int data_c_pack = (ns_rem << 42) | code_42;

  uuid_pack_v8(u, data_a, data_b_pack, data_c_pack);
}

int uuid_verify_pid_lux9(const uuid_t *pid2, const uuid_t *parent_uuid,
                         const u8int *namespace_cid, const u8int *code_hash) {
  /* Requires unpacking logic - for now, we leave as a stub or implement
   * unpacker first. To verify, we would regenerate the expected UUID (ignoring
   * timestamp) and compare components. But wait, timestamp is in data_a. We
   * can't verify data_a equality. We verify data_b and data_c components.
   *
   * Reconstruct expected data_b and data_c from inputs, then compare against
   * UUID fields.
   */
  if (!pid2 || !namespace_cid || !code_hash)
    return 0;

  // 1. Extract raw fields from UUID
  // Reverse of uuid_pack_v8 logic
  const u8int *d = pid2->data;
  u16int data_b_pack = ((d[6] & 0x0F) << 8) | d[7];
  u64int data_c_pack = (u64int)(d[8] & 0x3F);
  for (int i = 9; i < 16; i++)
    data_c_pack = (data_c_pack << 8) | d[i];

  // 2. Reconstruct expected values
  // Namespace CID (32 bits)
  u64int data_b_val =
      ((u64int)namespace_cid[0] << 24) | ((u64int)namespace_cid[1] << 16) |
      ((u64int)namespace_cid[2] << 8) | (u64int)namespace_cid[3];
  u32int ns_val = (u32int)data_b_val;

  // Code Hash (42 bits)
  u64int code_val = hash_extract_u64(code_hash);
  u64int code_42 = (code_val >> (64 - 42));

  // 3. Compare data_b (Top 12 bits of NS)
  u16int expected_b = (ns_val >> 20) & 0xFFF;
  if (data_b_pack != expected_b)
    return 0; // Namespace (Top) mismatch

  // 4. Compare data_c (Bottom 20 bits of NS | Code 42)
  u64int ns_rem = ns_val & 0xFFFFF;
  u64int expected_c = (ns_rem << 42) | code_42;
  if (data_c_pack != expected_c)
    return 0; // Namespace (Bottom) or Code mismatch

  // 5. Compare Ancestry (Optional / Best Effort)
  // data_a[16..0] should match parent_uuid[0..1]
  if (parent_uuid) {
    u64int data_a = 0;
    for (int i = 0; i < 6; i++)
      data_a = (data_a << 8) | d[i];

    u16int stored_parent_sig = data_a & 0xFFFF;
    u16int actual_parent_sig =
        ((u16int)parent_uuid->data[0] << 8) | parent_uuid->data[1];

    if (stored_parent_sig != actual_parent_sig)
      return 0; // Parent mismatch
  }

  return 1;
}
