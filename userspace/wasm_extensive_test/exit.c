/* exit.c - Minimal _exit() implementation for proper process termination */

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00
#define Tsysexit 164

typedef unsigned int uint;
typedef unsigned char uchar;

struct P9Control {
  uint doorbell;
  uint status;
  uint req_head;
  uint req_tail;
  uint rep_head;
  uint rep_tail;
};

static void put_u32(uchar *p, uint val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
}

static void put_u16(uchar *p, unsigned short val) {
  p[0] = val;
  p[1] = val >> 8;
}

/* _exit: Terminate process cleanly via SYS_EXITS syscall
 * This is called automatically when main() returns.
 */
void _exit(int status) {
  volatile uchar *exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  volatile struct P9Control *ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);
  uchar *req = (uchar *)exchange;
  uint pos = 0;

  /* Build Tsysexit message:
   * size[4] type[1] tag[2] strlen[2] string[strlen]
   * Note: Tsysexit has its own format, NOT the standard Tsyscall format!
   */
  uint status_len = 1; /* Just null terminator for empty status */
  uint size = 4 + 1 + 2 + 2 + status_len;

  /* Clear message area */
  for (int i = 0; i < 256; i++)
    req[i] = 0;

  /* Build message */
  put_u32(req + pos, size); pos += 4;
  req[pos++] = Tsysexit;
  put_u16(req + pos, 1); pos += 2; /* tag */
  put_u16(req + pos, status_len); pos += 2; /* string length */
  req[pos] = 0; /* Empty status string (null terminator) */

  /* Ring doorbell */
  ctl->doorbell = 1;

  /* Trigger syscall */
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* Should never return, but if it does, loop forever */
  for (;;)
    __asm__ volatile("hlt");
}
