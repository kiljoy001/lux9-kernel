/* Stubs for missing functionality */
#include "dat.h"
#include "error.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

/* WASM Stubs - REMOVED: Real implementations exist in kernel/wasm/ */
/* The following functions are now implemented in:
 * - wasm_9p_extract_cap_uuid: kernel/wasm/wasm_9p_integration.c
 * - wasm_9p_validate_capability: kernel/wasm/wasm_9p_integration.c
 * - wasm_fs_submit: kernel/wasm/wasm_fileserver.c
 */

/* UUID Stubs */
/* uuid_pack_pebble is in uuid.c */

/* Compiler Builtins */
/* __popcountdi2 - count bits set in 64-bit integer */
/*@
  @ assigns \nothing;
  @*/
int __popcountdi2(long long a) {
  unsigned long long x = (unsigned long long)a;
  x -= (x >> 1) & 0x5555555555555555ULL;
  x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
  x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
  return (x * 0x0101010101010101ULL) >> 56;
}

/* PCI Family Stubs */
/* Map pci_config_* to 9front pcicfgrw* function pointers */
/* Tbdf is int: (bus<<16)|(dev<<11)|(func<<8) */
#define MKBUS(b, d, f) (((b) << 16) | ((d) << 11) | ((f) << 8))

extern int (*pcicfgrw32)(int tbdf, int rno, int data, int read);
extern int (*pcicfgrw16)(int tbdf, int rno, int data, int read);
extern int (*pcicfgrw8)(int tbdf, int rno, int data, int read);

/*@
  @ assigns \nothing;
  @*/
u32int pci_config_read32(int b, int d, int f, int r) {
  if (pcicfgrw32)
    return pcicfgrw32(MKBUS(b, d, f), r, 0, 1);
  return 0xFFFFFFFF;
}
/*@
  @ assigns \nothing;
  @*/
u16int pci_config_read16(int b, int d, int f, int r) {
  if (pcicfgrw16)
    return pcicfgrw16(MKBUS(b, d, f), r, 0, 1);
  return 0xFFFF;
}
/*@
  @ assigns \nothing;
  @*/
u8int pci_config_read8(int b, int d, int f, int r) {
  if (pcicfgrw8)
    return pcicfgrw8(MKBUS(b, d, f), r, 0, 1);
  return 0xFF;
}
/*@
  @ assigns \nothing;
  @*/
void pci_config_write32(int b, int d, int f, int r, u32int v) {
  if (pcicfgrw32)
    pcicfgrw32(MKBUS(b, d, f), r, v, 0);
}
/*@
  @ assigns \nothing;
  @*/
void pci_config_write16(int b, int d, int f, int r, u16int v) {
  if (pcicfgrw16)
    pcicfgrw16(MKBUS(b, d, f), r, v, 0);
}
/*@
  @ assigns \nothing;
  @*/
void pci_config_write8(int b, int d, int f, int r, u8int v) {
  if (pcicfgrw8)
    pcicfgrw8(MKBUS(b, d, f), r, v, 0);
}

/* Family Internal Stubs (missing implementations) */
void setup_pci_event_system(void *f) {}
void setup_pci_transaction_manager(void *f) {}
void update_pci_family_stats(void *f) {}
void notify_pci_device_removed(void *f) {}

/* Process Wrappers */
Proc *current_process(void) { return up; /* up is defined in dat.h/macro */ }

/* Lock Stubs */
void lock_init(Lock *l) { memset(l, 0, sizeof(Lock)); }

/*@
  @ requires \valid(chan);
  @ assigns \nothing;
  @*/int validate_channel_operation_permission(void *chan, int op) {
  return 1; // Allow for now
}

/* nsec - return nanoseconds since boot */
vlong nsec(void) { return fastticks2ns(fastticks(nil)); }

/* randombytes - fill buffer with random bytes */
/*@
  @ requires \valid(buf);
  @ assigns \nothing;
  @*/void randombytes(u8int *buf, usize len) {
  extern void chacha20_csprng_fill(u8int *, ulong);
  chacha20_csprng_fill(buf, len);
}
