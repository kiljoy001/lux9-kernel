/* Init - Process 1
 * Starts the resurrection server (Layer 2 service monitor)
 * Uses 9P syscalls via exchange page
 */

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00
#define Tsyscall 130
#define Rsyscall 131
#define Rerror 107
#define Tcreate 114
#define Texec 128

#define SYS_CLOSE 4
#define SYS_WRITE 20
#define SYS_CREATE 22
#define SYS_RFORK 19

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

  /* Tsyscall header */
  /* SYS_RFORK(flags) */
  uint size = 4 + 1 + 2 + 4 + 4 + 4; /* header + flags */
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_RFORK);
  pos += 4;
  put_u32(req + pos, 4);
  pos += 4; /* sdata size */
  put_u32(req + pos, 1);
  pos += 4;               /* scount */
  put_u32(req + pos, 16); /* flags = RFPROC (16) */

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

  uvlong retval =
      get_u64(req + pos + 3); /* RFORK returns pid in retval (offset 3 u64) No
wait. Rerror or Rsyscall. Wait, struct is: [size:4][type:1][tag:2][retval:8] pos
points to after tag (offset 7). So retval is at pos.
*/
  retval = get_u64(req + pos);
  return (int)retval;
}

/* Execute a program (replaces current process) */
static void do_exec(const char *path) {
  uchar *req = (uchar *)exchange;
  int pathlen = strlen(path);
  uint pos = 0;

  memset(req, 0, 512);

  /* Texec format per convM2S: [size 4][type 1][tag 2][count 4][data count]
   * The 'count' field contains the length of the data that follows (pathlen + 2
   * for path string) The data is: [pathlen 2][path n] */
  uint data_size = 2 + pathlen;          /* 2-byte pathlen + path bytes */
  uint size = 4 + 1 + 2 + 4 + data_size; /* size + type + tag + count + data */
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Texec;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, data_size); /* 4-byte count (size of following data) */
  pos += 4;
  put_u16(req + pos, pathlen); /* 2-byte pathlen within data */
  pos += 2;
  memcpy(req + pos, path, pathlen);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* If we get here, exec failed */
  print("init: exec failed\n");
}

/* Create a file - returns FD */
static int do_create(const char *path, int mode) {
  uchar *req = (uchar *)exchange;
  int pathlen = strlen(path);
  uint pos = 0;

  memset(req, 0, 512);

  /* SYS_CREATE(path, mode, perm) ? No.
     Plan 9 create(2): create(char *name, int omode, ulong perm).
     Translates to Tcreate on the parent directory fid.
     This implies walking.

     But we are using Lux9 syscalls which might be path-based (like Linux)?
     If sys_create in kernel is path-based, we are good.
     If it's fid-based, we can't easily use it from init without tracking fids.

     Let's check sys.h again. It has OPEN, CLOSE, READ, WRITE, CREATE.
     Lux9 kernel seems to map these directly to internal functions.
     Given this is `init.c` (userspace), and it successfully calls SYS_EXEC
     (with path), the kernel probably handles path resolution.

     So we assume SYS_CREATE(path, omode, perm) exists.
     Mode 2 (O_RDWR). Perm 0644.
  */

  uint size = 4 + 1 + 2 + 4 + 4 + 4 + 2 + pathlen + 4 + 4;
  /* syscall args: path(ptr,len), mode, perm */

  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_CREATE);
  pos += 4;

  uint sdata_size = 4 + 2 + pathlen + 4 + 4;
  put_u32(req + pos, sdata_size);
  pos += 4; /* sdata size */
  put_u32(req + pos, 3);
  pos += 4; /* scount */

  put_u16(req + pos, pathlen);
  pos += 2;
  memcpy(req + pos, path, pathlen);
  pos += pathlen;

  put_u32(req + pos, 2); /* O_RDWR */
  pos += 4;
  put_u32(req + pos, mode); /* perm */
  pos += 4;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* Return FD */
  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos] == Rerror)
    return -1;

  return (int)get_u64(req + pos + 3);
}

/* Write to FD */
static int do_write(int fd, const char *buf, int count) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;

  uint size = 4 + 1 + 2 + 4 + 4 + 4 + 8 + 4 + count;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_WRITE);
  pos += 4;
  put_u32(req + pos, 4 + 8 + 4 + count);
  pos += 4; /* sdata size */
  put_u32(req + pos, 3);
  pos += 4; /* scount */

  put_u32(req + pos, fd);
  pos += 4;
  put_u64(req + pos, 0);
  pos += 8; /* offset (ignored for pipes/append?) */
  put_u32(req + pos, count);
  pos += 4;
  memcpy(req + pos, buf, count);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos] == Rerror)
    return -1;

  return (int)get_u64(req + pos + 3);
}

static void do_close(int fd) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;
  uint size = 4 + 1 + 2 + 4 + 4 + 4;
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_CLOSE);
  pos += 4;
  put_u32(req + pos, 4);
  pos += 4; /* sdata size */
  put_u32(req + pos, 1);
  pos += 4; /* scount */
  put_u32(req + pos, fd);
  pos += 4;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
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
    do_exec("#/./boot/resurrection");
    /* If exec fails, hang */
    for (;;)
      ;
  }

  /* Parent (init) continues */
  print("init: Resurrection server started, registering services...\n");

  /* Give resurrection server a specific amount of time to come up and mount
   * /srv */
  for (volatile int i = 0; i < 5000000; i++)
    ;

  /* Register services */
  const char *services[] = {
      "rump_server", "/boot/rump_server", "turbocid", "/boot/turbocid",
      "wasm_test",   "/boot/wasm_test",   0};

  for (int i = 0; services[i]; i += 2) {
    /* Construct path: /srv/service_name */
    char path[128];
    int idx = 0;
    path[idx++] = '/';
    path[idx++] = 's';
    path[idx++] = 'r';
    path[idx++] = 'v';
    path[idx++] = '/';
    const char *sname = services[i];
    while (*sname)
      path[idx++] = *sname++;
    path[idx] = 0;

    print("init: Creating ");
    print(path);
    print("\n");

    /* 432 = 0660 octal? 420 = 0644. */
    int fd = do_create(path, 420);

    if (fd < 0) {
      print("init: Failed to create service entry: ");
      print(path);
      print("\n");
      continue;
    }

    /* Write config */
    char buf[128];
    int j = 0;
    const char *keys = "exec=";
    while (*keys)
      buf[j++] = *keys++;
    const char *execpath = services[i + 1];
    while (*execpath)
      buf[j++] = *execpath++;
    buf[j] = 0;

    if (do_write(fd, buf, j) < 0) {
      print("init: Failed to write config for ");
      print(services[i]);
      print("\n");
    } else {
      print("init: Registered ");
      print(services[i]);
      print("\n");
    }

    do_close(fd);
  }

  print("init: Resurrection server started, init entering idle loop\n");

  /* Init stays alive as PID 1 - required for process tree */
  for (;;) {
    /* Could handle signals/events here in future */
    for (volatile int i = 0; i < 10000000; i++)
      ;
  }
}
