#include <fcall.h>
#include <libc.h>
#include <u.h>

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000
#define P9_REQUEST_OFFSET 0x000
#define P9_REPLY_OFFSET 0x800
#define P9_CONTROL_OFFSET 0xF00

#define P9_STATUS_IDLE 0
#define P9_STATUS_PENDING 1
#define P9_STATUS_COMPLETE 2

typedef struct P9Control {
  u32int doorbell;
  u32int status;
  u32int req_head;
  u32int req_tail;
  u32int rep_head;
  u32int rep_tail; /* Renamed from rep_size to match kernel */
  u32int rep_seq;
} P9Control;

extern uint convS2M(Fcall *f, uchar *ap, uint nap);
extern uint convM2S(uchar *ap, uint nap, Fcall *f);

/* Libc implementations */
void *memmove(void *dst, const void *src, ulong n) {
  char *d = dst;
  const char *s = src;
  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    const char *lasts = s + (n - 1);
    char *lastd = d + (n - 1);
    while (n--)
      *lastd-- = *lasts--;
  }
  return dst;
}

void *memset(void *s, int c, ulong n) {
  char *p = s;
  while (n--)
    *p++ = c;
  return s;
}

int strlen(const char *s) {
  int n = 0;
  while (*s++)
    n++;
  return n;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while ((*d++ = *src++))
    ;
  return dst;
}

static void ring_doorbell(void) {
  asm volatile("syscall" : : : "memory", "rcx", "r11");
}

/* Internal generic call */
static int p9_raw_call(Fcall *tx, Fcall *rx) {
  uchar *page = (uchar *)EXCHANGE_PAGE_ADDR;
  P9Control *ctl = (P9Control *)(page + P9_CONTROL_OFFSET);
  uchar *req_buf = page + P9_REQUEST_OFFSET;
  uchar *rep_buf = page + P9_REPLY_OFFSET;

  /* Ensure status is not PENDING before we start?
     It should be IDLE or COMPLETE from previous call. */

  uint n = convS2M(tx, req_buf, 2048);
  if (n == 0)
    return -1;

  ctl->req_tail = n;
  ctl->rep_seq = 0;

  // Set status PENDING? No kernel sets it atomically?
  // Wait, earlier logic: kernel sets PENDING in handle_handle logic?
  // p9_handle_doorbell: atomic_store(&ctl->status, P9_STATUS_PENDING...)
  // So we don't need to set it.
  // We just ring doorbell.

  ring_doorbell();

  // Wait for COMPLETE
  while (*(volatile u32int *)&ctl->status != P9_STATUS_COMPLETE)
    ;

  uint rep_len = ctl->rep_tail;
  if (rep_len == 0)
    return -1;

  if (convM2S(rep_buf, rep_len, rx) == 0)
    return -1;
  if (rx->type == Rerror)
    return -1;
  if (rx->type != Rsyscall)
    return -1;

  return 0;
}

int p9_open(char *path, int mode) {
  Fcall tx, rx;
  uchar buf[256];
  uchar *p = buf;
  int len = strlen(path);

  PBIT32(p, 0);
  p += 4;
  PBIT16(p, len);
  p += 2;
  memmove(p, path, len);
  p += len;
  PBIT8(p, mode);
  p += 1;

  tx.type = Tsyscall;
  tx.tag = 0;
  tx.scallnr = SYS_OPEN;
  tx.scount = p - buf;
  tx.sdata = buf;

  if (p9_raw_call(&tx, &rx) < 0)
    return -1;
  if (rx.scount < 4)
    return -1;
  return GBIT32(rx.sdata);
}

int p9_close(int fd) {
  Fcall tx, rx;
  uchar buf[4];
  PBIT32(buf, fd);

  tx.type = Tsyscall;
  tx.tag = 0;
  tx.scallnr = SYS_CLOSE;
  tx.scount = 4;
  tx.sdata = buf;

  return p9_raw_call(&tx, &rx);
}

int p9_read(int fd, void *buf, int count) {
  Fcall tx, rx;
  uchar args[16];
  uchar *p = args;

  PBIT32(p, fd);
  p += 4;
  PBIT64(p, 0);
  p += 8;
  PBIT32(p, count);
  p += 4;

  tx.type = Tsyscall;
  tx.tag = 0;
  tx.scallnr = SYS_READ;
  tx.scount = 16;
  tx.sdata = args;

  if (p9_raw_call(&tx, &rx) < 0)
    return -1;

  int n = rx.scount;
  if (n > count)
    n = count;
  memmove(buf, rx.sdata, n);
  return n;
}

int p9_write(int fd, void *buf, int count) {
  Fcall tx, rx;
  uchar tmp[4096];
  if (count > 2048)
    count = 2048;

  uchar *p = tmp;
  PBIT32(p, fd);
  p += 4;
  PBIT64(p, 0);
  p += 8;
  PBIT32(p, count);
  p += 4;
  memmove(p, buf, count);
  p += count;

  tx.type = Tsyscall;
  tx.tag = 0;
  tx.scallnr = SYS_WRITE;
  tx.scount = p - tmp;
  tx.sdata = tmp;

  if (p9_raw_call(&tx, &rx) < 0)
    return -1;
  if (rx.scount < 4)
    return -1;
  return GBIT32(rx.sdata);
}

void p9_exit(char *status) {
  Fcall tx, rx;
  uchar buf[256];
  uchar *p = buf;
  int len = 0;
  if (status)
    len = strlen(status);

  PBIT16(p, len);
  p += 2;
  if (len)
    memmove(p, status, len);
  p += len;

  tx.type = Tsyscall;
  tx.tag = 0;
  tx.scallnr = SYS_EXIT;
  tx.scount = p - buf;
  tx.sdata = buf;

  p9_raw_call(&tx, &rx);
  while (1)
    ;
}
