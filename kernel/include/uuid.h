#ifndef _UUID_H_
#define _UUID_H_

#include "u.h"

/* UUID Manipulation Functions */
void uuid_clear(uuid_t *u);
int uuid_compare(const uuid_t *a, const uuid_t *b);
void uuid_copy(uuid_t *dst, const uuid_t *src);
int uuid_parse(const char *in, uuid_t *uu);
void uuid_unparse(const uuid_t *uu, char *out);
int uuid_is_null(const uuid_t *uu);

/* UUID Generation (RFC 9562 v8) */
void uuid_new_v8(uuid_t *u);
/* Pack custom data into UUIDv8 payload (122 bits) */
void uuid_pack_v8(uuid_t *u, unsigned long long data_a, unsigned short data_b,
                  unsigned long long data_c);

/* Pebble Token Packing (Token ID + Generation + Index) */
typedef struct {
  unsigned int token;
  unsigned int generation;
  unsigned short index;
} pebble_uuid_data_t;

void uuid_pack_pebble(uuid_t *u, unsigned int token, unsigned int generation,
                      unsigned short index);
int uuid_unpack_pebble(const uuid_t *u, unsigned int *token,
                       unsigned int *generation, unsigned short *index);

/* Capability UUID Packing (PA hash + Type + Perms + Epoch)
 * UUIDv8 Layout for capabilities:
 * Bits 0-47:   PA hash high (48 bits)
 * Bits 48-51:  Version = 8 (0b1000)
 * Bits 52-59:  Type (8 bits: CAP_TYPE_MEMORY, etc.)
 * Bits 60-63:  Permissions (4 bits: R/W/X/T)
 * Bits 64-65:  Variant = 0b10
 * Bits 66-81:  Epoch (16 bits)
 * Bits 82-127: PA hash low (46 bits)
 * Total PA hash: 94 bits from 32-byte BLAKE2b
 */
void uuid_pack_capability(uuid_t *u, const unsigned char *pa_hash,
                          unsigned short epoch, unsigned char type,
                          unsigned char perms);

int uuid_unpack_capability(const uuid_t *u, unsigned short *epoch,
                           unsigned char *type, unsigned char *perms);

/* Extract PA hash bits from capability UUID (94 bits total) */
void uuid_get_pa_hash_bits(const uuid_t *u, unsigned char *pa_hash_out);

/* Lux9 Secure PID2 Packing */
void uuid_pack_pid_lux9(uuid_t *u, const uuid_t *parent_uuid,
                        const u8int *namespace_cid, const u8int *code_hash);

/* Helper to verify PID2 components */
int uuid_verify_pid_lux9(const uuid_t *pid2, const uuid_t *parent_uuid,
                         const u8int *namespace_cid, const u8int *code_hash);

#endif /* _UUID_H_ */
