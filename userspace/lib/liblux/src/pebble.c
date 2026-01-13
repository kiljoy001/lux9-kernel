#include "lux_internal.h"

extern void *memset(void *dst, int c, ulong n);

/* Syscall numbers */
#define PEBBLE_BLACK_ALLOC 59
#define PEBBLE_BLACK_FREE 60

extern int lux_call(Fcall *tx, Fcall *rx);

static void put64(uchar *p, u64int v) {
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
  p[4] = v >> 32;
  p[5] = v >> 40;
  p[6] = v >> 48;
  p[7] = v >> 56;
}

int pebble_alloc(ulong size, void **addr) {
  Fcall tx, rx;
  uchar buf[16];

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0,
         sizeof(Fcall)); // Good practice though lux_call should handle it

  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = PEBBLE_BLACK_ALLOC;
  tx.sdata = buf;
  tx.scount = 16;

  put64(buf, (u64int)size);
  put64(buf + 8, (u64int)addr);

  if (lux_call(&tx, &rx) < 0)
    return -1;
  if (addr != nil)
    *addr = (void *)rx.retval;
  return 0;
}

int pebble_free(void *addr) {
  Fcall tx, rx;
  uchar buf[8];

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));

  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = PEBBLE_BLACK_FREE;
  tx.sdata = buf;
  tx.scount = 8;

  put64(buf, (u64int)addr);

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return 0;
}
