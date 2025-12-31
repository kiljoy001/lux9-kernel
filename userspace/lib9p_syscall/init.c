/* Init - Process 1
 * Starts the resurrection server (Layer 2 service monitor)
 * Uses 9P syscalls via exchange page
 */

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00
#define Tsyscall 130
#define Rsyscall 131
#define Rerror 107
#define Texec 128

#define SYS_WRITE 4
#define SYS_FORK 9

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long long uvlong;

/* P9 Control structure */
struct P9Control {
  uint doorbell;
  uint status;
  uint req_head;
  uint req_tail;
  uint rep_head;
  uint rep_tail;
};

/* Global exchange page pointers */
static volatile uchar *exchange;
static volatile struct P9Control *ctl;

/* Simple memory operations */
static void *memcpy(void *dst, const void *src, unsigned long n) {
  uchar *d = dst;
  const uchar *s = src;
  while (n--)
    *d++ = *s++;
  return dst;
}

static void *memset(void *dst, int c, unsigned long n) {
  uchar *d = dst;
  while (n--)
    *d++ = (uchar)c;
  return dst;
}

static int strlen(const char *s) {
  int n = 0;
  while (*s++)
    n++;
  return n;
}

/* Write little-endian integers */
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

static uint get_u32(const uchar *p) {
  return (uint)p[0] | ((uint)p[1] << 8) | ((uint)p[2] << 16) |
         ((uint)p[3] << 24);
}

static uvlong get_u64(const uchar *p) {
  return (uvlong)p[0] | ((uvlong)p[1] << 8) | ((uvlong)p[2] << 16) |
         ((uvlong)p[3] << 24) | ((uvlong)p[4] << 32) | ((uvlong)p[5] << 40) |
         ((uvlong)p[6] << 48) | ((uvlong)p[7] << 56);
}

/* Print a message to console via SYS_WRITE */
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
  memcpy(req + pos, msg, msg_len);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
}

/* Fork a new process - returns PID (0 in child, >0 in parent, -1 on error) */
static int do_fork(void) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;

  memset(req, 0, 256);

  uint size = 4 + 1 + 2 + 4 + 4 + 4; /* header + flags */
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_FORK);
  pos += 4;
  put_u32(req + pos, 4);
  pos += 4; /* scount */
  put_u32(req + pos, 16);
  pos += 4; /* flags = RFPROC (1<<4 = 16) */

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* Parse reply */
  pos = 0;
  get_u32(req + pos);
  pos += 4; /* size */
  uchar reply_type = req[pos++];
  pos += 2; /* tag */

  if (reply_type == Rerror) {
    return -1;
  }

  uvlong retval = get_u64(req + pos);
  return (int)retval;
}

/* Execute a program (replaces current process) */
static void do_exec(const char *path) {
  uchar *req = (uchar *)exchange;
  int pathlen = strlen(path);
  uint pos = 0;

  memset(req, 0, 512);

  /* Texec: [size][type=128][tag][pathlen:2][path:n] */
  uint size = 4 + 1 + 2 + 2 + pathlen;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Texec;
  put_u16(req + pos, 1);
  pos += 2;
  put_u16(req + pos, pathlen);
  pos += 2;
  memcpy(req + pos, path, pathlen);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* If we get here, exec failed */
  print("init: exec failed\n");
}

void main(void) {
  exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  print("=== Lux9 Init Starting ===\n");

  /* Fork and exec the resurrection server */
  print("init: Starting resurrection server...\n");

  int pid = do_fork();

  if (pid < 0) {
    print("init: fork failed!\n");
    for (;;)
      ;
  }

  if (pid == 0) {
    /* Child - exec resurrection server */
    do_exec("/boot/resurrection");
    /* If exec fails, hang */
    for (;;)
      ;
  }

  /* Parent (init) continues */
  print("init: Resurrection server started, init entering idle loop\n");

  /* Init stays alive as PID 1 - required for process tree */
  for (;;) {
    /* Could handle signals/events here in future */
    for (volatile int i = 0; i < 10000000; i++)
      ;
  }
}
