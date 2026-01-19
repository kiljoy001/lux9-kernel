#include "u.h"
#include "dat.h"
#include "fns.h"
#include "uuid.h"

/*@
  @ requires pid2 == \null || \valid(pid2);
  @ assigns \nothing;
  @*/
static u16int pid2_data_b(const uuid_t *pid2) {
  return (u16int)(((pid2->data[6] & 0x0F) << 8) | pid2->data[7]);
}

/*@
  @ requires hash == \null || \valid(hash);
  @ assigns \nothing;
  @*/
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

/*@
  @ assigns \nothing;
  @*/
void pid2_selftest(void) {
  uuid_t parent;
  uuid_t pid2;
  u8int ns_cid[32];
  u8int code_hash[32];
  u32int ns_val;
  u16int data_b;
  u64int ns_rem;
  u64int code_val;
  u64int code_42;
  u64int data_c;
  u16int parent_sig;
  u64int data_a;

    /*@ loop invariant 0 <= i <= 16;
    @ loop assigns i;
    @ loop variant 16 - i;
    @*/
  for (int i = 0; i < 16; i++)
    parent.data[i] = (u8int)(0xA0 + i);
    /*@ loop invariant 0 <= i <= 32;
    @ loop assigns i;
    @ loop variant 32 - i;
    @*/
  for (int i = 0; i < 32; i++) {
    ns_cid[i] = (u8int)(i + 1);
    code_hash[i] = (u8int)(0xFF - i);
  }

  ns_val = ((u32int)ns_cid[0] << 24) | ((u32int)ns_cid[1] << 16) |
           ((u32int)ns_cid[2] << 8) | (u32int)ns_cid[3];
  data_b = (ns_val >> 20) & 0x0FFF;
  ns_rem = ns_val & 0xFFFFF;
  code_val = hash_extract_u64(code_hash);
  code_42 = (code_val >> (64 - 42));
  data_c = (ns_rem << 42) | code_42;

  parent_sig = ((u16int)parent.data[0] << 8) | (u16int)parent.data[1];
  data_a = parent_sig;

  uuid_pack_v8(&pid2, data_a, data_b, data_c);

  if (!uuid_verify_pid_lux9(&pid2, &parent, ns_cid, code_hash)) {
    print("PID2 selftest: verify failed\n");
  } else {
    u16int got_b = pid2_data_b(&pid2);
    u64int got_c = (u64int)(pid2.data[8] & 0x3F);
      /*@ loop invariant 0 <= i <= 16;
    @ loop assigns i;
    @ loop variant 16 - i;
    @*/
  for (int i = 9; i < 16; i++)
      got_c = (got_c << 8) | pid2.data[i];
    if (got_b != data_b || got_c != data_c)
      print("PID2 selftest: pack mismatch\n");
    else
      print("PID2 selftest: pack/verify ok\n");
  }

  if (up != nil && up->pgrp != nil) {
    Pgrp *pg = up->pgrp;
    u64int saved_notallowed[nelem(pg->notallowed)];
    u8int saved_cid[32];
    uuid_t saved_pid2;
    u16int before;
    u16int after;

    memmove(saved_notallowed, pg->notallowed, sizeof(saved_notallowed));
    memmove(saved_cid, pg->namespace_cid, sizeof(saved_cid));
    memmove(&saved_pid2, &up->pid2, sizeof(saved_pid2));

    before = pid2_data_b(&up->pid2);
    devmask(pg, 0, "M");
    after = pid2_data_b(&up->pid2);

    if (before == after)
      print("PID2 selftest: namespace CID unchanged\n");
    else
      print("PID2 selftest: namespace CID updated\n");

    wlock(&pg->ns);
    memmove(pg->notallowed, saved_notallowed, sizeof(saved_notallowed));
    memmove(pg->namespace_cid, saved_cid, sizeof(saved_cid));
    wunlock(&pg->ns);
    memmove(&up->pid2, &saved_pid2, sizeof(saved_pid2));
  }
}
