#ifndef _UUID_H_
#define _UUID_H_

typedef struct {
  unsigned char data[16];
} uuid_t;

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

#endif /* _UUID_H_ */
