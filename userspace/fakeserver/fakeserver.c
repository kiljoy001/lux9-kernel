/*
 * Fake Server for Resurrection Testing
 * Prints message and loops.
 */

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00

/* 9P Message types */
#define Tsyscall 130
#define SYS_WRITE 4
#define SYS_EXIT 8

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long long uvlong;
typedef unsigned long long u64int;

extern unsigned long long lux_exchange_base;
static inline unsigned long long exchange_base(void) {
  if (lux_exchange_base != 0)
    return lux_exchange_base;
  return EXCHANGE_PAGE_ADDR;
}

struct P9Control {
  uint doorbell;
  uint status;
  uint req_head;
  uint req_tail;
  uint rep_head;
  uint rep_tail;
};

static volatile uchar *exchange;
static volatile struct P9Control *ctl;

static void put_u32(uchar *p, uint val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
}

static void put_u64(uchar *p, uvlong val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
  p[4] = val >> 32;
  p[5] = val >> 40;
  p[6] = val >> 48;
  p[7] = val >> 56;
}

static void put_u16(uchar *p, unsigned short val) {
  p[0] = val;
  p[1] = val >> 8;
}

static void *memset(void *dst, int c, unsigned long n) {
  uchar *d = dst;
  while (n--)
    *d++ = (uchar)c;
  return dst;
}

static int strlen(const char *s) {
  int len = 0;
  while (*s++)
    len++;
  return len;
}

static void print(const char *msg) {
  int msg_len = strlen(msg);
  uchar *req = (uchar *)exchange;
  uint sdata_size = 4 + 8 + 4 + msg_len;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_size;
  uint pos = 0;

  memset(req, 0, 256);
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_WRITE);
  pos += 4;
  put_u32(req + pos, sdata_size);
  pos += 4;
  put_u32(req + pos, 1);
  pos += 4; /* fd = 1 (stdout) */
  put_u64(req + pos, 0);
  pos += 8; /* offset */
  put_u32(req + pos, msg_len);
  pos += 4;

  const char *s = msg;
  while (msg_len--)
    req[pos++] = *s++;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
}

static void sys_exit(void) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;

  memset(req, 0, 128);
  /* Tsyscall header: size, type, tag, sys_exit, scount, status_len */
  uint size = 4 + 1 + 2 + 4 + 4 + 2;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_EXIT);
  pos += 4;
  put_u32(req + pos, 2);
  pos += 4; /* scount */
  put_u16(req + pos, 0);
  pos += 2; /* status len 0 */

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
  for (;;)
    ;
}

void main(void) {
  exchange = (volatile uchar *)exchange_base();
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  print("FakeServer: I am alive! Will crash in 5 ticks.\n");

  /* Busy loop with output */
  for (int j = 0; j < 5; j++) {
    for (volatile int i = 0; i < 50000000; i++)
      ;
    print("FakeServer: Tick...\n");
  }

  print("FakeServer: Simulating crash (exit)...\n");
  sys_exit();
}
