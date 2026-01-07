#include "libc.h"
#include "u.h"
#include "uuid.h"

static u64int hash_extract_u64(const u8int *hash) {
  u64int v = 0;
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

int main(void) {
  uuid_t parent;
  uuid_t pid2;
  u8int ns_cid[32];
  u8int code_hash[32];

  for (int i = 0; i < 16; i++)
    parent.data[i] = (u8int)(0xA0 + i);
  for (int i = 0; i < 32; i++) {
    ns_cid[i] = (u8int)(i + 1);
    code_hash[i] = (u8int)(0xFF - i);
  }

  u32int ns_val = ((u32int)ns_cid[0] << 24) | ((u32int)ns_cid[1] << 16) |
                  ((u32int)ns_cid[2] << 8) | (u32int)ns_cid[3];
  u16int data_b = (ns_val >> 20) & 0x0FFF;
  u64int ns_rem = ns_val & 0xFFFFF;
  u64int code_val = hash_extract_u64(code_hash);
  u64int code_42 = (code_val >> (64 - 42));
  u64int data_c = (ns_rem << 42) | code_42;

  u16int parent_sig =
      ((u16int)parent.data[0] << 8) | (u16int)parent.data[1];
  u64int data_a = parent_sig;

  uuid_pack_v8(&pid2, data_a, data_b, data_c);

  if (!uuid_verify_pid_lux9(&pid2, &parent, ns_cid, code_hash))
    return 1;

  const u8int *d = pid2.data;
  u16int got_b = ((d[6] & 0x0F) << 8) | d[7];
  u64int got_c = (u64int)(d[8] & 0x3F);
  for (int i = 9; i < 16; i++)
    got_c = (got_c << 8) | d[i];

  if (got_b != data_b)
    return 2;
  if (got_c != data_c)
    return 3;

  return 0;
}
