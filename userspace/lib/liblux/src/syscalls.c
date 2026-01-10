#include "lux_internal.h"

extern uint convS2M(Fcall *f, uchar *ap, uint n);
extern uint convM2S(uchar *ap, uint n, Fcall *f);
extern void *memmove(void *dst, const void *src, ulong n);
extern void *memset(void *dst, int c, ulong n);
/* SECURITY: No malloc/free - all allocations via pebble system */
extern int pebble_alloc(ulong size, void **addr);
extern int pebble_free(void *addr);

/* Packing Helpers */
static void pack8(uchar *p, int v) { p[0] = v; }
static void pack16(uchar *p, int v) {
  p[0] = v;
  p[1] = v >> 8;
}
static void pack32(uchar *p, int v) {
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
}

/* Exchange Pool Syscalls */
#define SYS_EXCHANGE_ALLOC 67
#define SYS_EXCHANGE_FREE 68

static void pack64(uchar *p, uvlong v) {
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
  p[4] = v >> 32;
  p[5] = v >> 40;
  p[6] = v >> 48;
  p[7] = v >> 56;
}
static int packstr(uchar *p, char *s) {
  int n = 0;
  while (s[n])
    n++;
  pack16(p, n);
  memmove(p + 2, s, n);
  return 2 + n;
}

int lux_call(Fcall *tx, Fcall *rx) {
  uchar *page = (uchar *)EXCHANGE_PAGE_ADDR;

  /* 1. Marshal Request */
  int n = convS2M(tx, page + P9_MSG_OFFSET, P9_MSG_SIZE);
  if (n <= 0)
    return -100; /* convS2M failed */

  /* 2. Ring Doorbell */
  _syscall();

  /* 3. Unmarshal Reply */
  // Debug check: verify rx is a valid user pointer
  if ((u64int)rx > 0x7FFFFFFFFFFF) {
    return -2; // Return special error for bad pointer to avoid crash
  }

  memset(rx, 0, sizeof(Fcall));
  // The original convM2S call is below, using page + P9_MSG_OFFSET and
  // P9_MSG_SIZE
  uint ret = convM2S(page + P9_MSG_OFFSET, P9_MSG_SIZE, rx);

  if ((int)ret <= 0)
    return -200; /* convM2S failed */

  if (rx->type == Rerror)
    return -1;
  return 0;
}

/* Generic syscall wrapper with sdata buffer management */
static int do_syscall(int scallnr, uchar *sdata, int scount, u64int *retval) {
  Fcall tx, rx;

  /* Clear structures to avoid any stack garbage */
  memset(&tx, 0, sizeof(Fcall));
  // memset(&rx, 0, sizeof(Fcall)); // lux_call clears this

  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = scallnr;
  tx.sflags = 0;
  tx.sdata = sdata;
  tx.scount = scount;

  int err = lux_call(&tx, &rx);
  if (err < 0)
    return err;

  if (retval)
    *retval = rx.retval;
  return 0;
}

#define SYS_BIND_RAW 2

int sys_open(char *path, int mode) {
  uchar buf[1024];
  uchar *p = buf;

  // [path s] [mode 1]
  p += packstr(p, path);
  pack8(p, mode);
  p += 1;

  u64int ret;
  if (do_syscall(SYS_OPEN, buf, p - buf, &ret) < 0)
    return -1;
  return (int)ret;
}

int sys_close(int fd) {
  uchar buf[16];
  uchar *p = buf;

  // [fd 4]
  pack32(p, fd);
  p += 4;

  return do_syscall(SYS_CLOSE, buf, p - buf, nil);
}

long sys_read(int fd, void *buf, long n) {
  uchar sbuf[32];
  uchar *p = sbuf;
  Fcall tx, rx;

  // [fd 4] [offset 8] [count 4]
  pack32(p, fd);
  p += 4;
  pack64(p, 0);
  p += 8;
  pack32(p, n);
  p += 4;

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_READ;
  tx.sdata = sbuf;
  tx.scount = p - sbuf;

  if (lux_call(&tx, &rx) < 0)
    return -1;

  // Copy data from reply
  if (rx.count > n)
    rx.count = n;
  if (rx.count > 0 && rx.sdata) {
    memmove(buf, rx.sdata, rx.count);
  }
  return rx.count;
}

long sys_write(int fd, void *buf, long n) {
  uchar sbuf[1024];
  uchar *p, *allocbuf = nil;
  uchar *data_start;

  int hdr_len = 4 + 8 + 4; // fid+off+cnt
  if (hdr_len + n <= sizeof(sbuf)) {
    p = sbuf;
  } else {
    if (pebble_alloc(hdr_len + n, (void **)&allocbuf) < 0)
      return -1;
    p = allocbuf;
  }

  data_start = p;
  // [fd 4] [offset 8] [count 4] [data]
  pack32(p, fd);
  p += 4;
  pack64(p, 0);
  p += 8;
  pack32(p, n);
  p += 4;
  memmove(p, buf, n);
  p += n;

  u64int ret;
  int res = do_syscall(SYS_WRITE, data_start, p - data_start, &ret);

  if (allocbuf)
    pebble_free(allocbuf);

  if (res < 0)
    return -1;
  return (long)ret;
}

long sys_pwrite(int fd, void *buf, long n, long offset) {
  uchar sbuf[1024];
  uchar *p, *allocbuf = nil;
  uchar *data_start;

  int hdr_len = 4 + 8 + 4; // fid+off+cnt
  if (hdr_len + n <= sizeof(sbuf)) {
    p = sbuf;
  } else {
    if (pebble_alloc(hdr_len + n, (void **)&allocbuf) < 0)
      return -1;
    p = allocbuf;
  }

  data_start = p;
  // [fd 4] [offset 8] [count 4] [data]
  pack32(p, fd);
  p += 4;
  pack64(p, offset);
  p += 8;
  pack32(p, n);
  p += 4;
  memmove(p, buf, n);
  p += n;

  u64int ret;
  int res = do_syscall(SYS_PWRITE, data_start, p - data_start, &ret);

  if (allocbuf)
    pebble_free(allocbuf);

  if (res < 0)
    return -1;
  return (long)ret;
}

void sys_exit(char *msg) {
  uchar buf[256];
  uchar *p = buf;

  // [msg s]
  p += packstr(p, msg ? msg : "");

  do_syscall(SYS_EXIT, buf, p - buf, nil);
  while (1)
    ;
}

int sys_create(char *path, int mode, uint perm) {
  uchar buf[1024];
  uchar *p = buf;

  // [path s] [mode 4] [perm 4]
  p += packstr(p, path);
  pack32(p, mode);
  p += 4;
  pack32(p, perm);
  p += 4;

  u64int ret;
  if (do_syscall(SYS_CREATE, buf, p - buf, &ret) < 0)
    return -1;
  return (int)ret;
}

extern long _syscall(void);

int sys_rfork(int flags) {
  /* Special handling for rfork:
     Parent receives Rsysfork reply with child PID in r->pid.
     Child gets a fresh exchange page (P9SEG is nil on fork, demand-paged).

     Detection strategy:
     1. Issue syscall
     2. Try to parse reply from exchange page
     3. If parse fails or reply is wrong type -> we are child (return 0)
     4. If parse succeeds with Rsysfork -> we are parent (return r->pid)
  */
  uchar buf[16];
  Fcall tx, rx;
  uchar *p = buf;

  memset(&tx, 0, sizeof(Fcall));
  tx.type = Tsysfork;
  tx.tag = 1;
  tx.flags = flags;

  uchar *page = (uchar *)EXCHANGE_PAGE_ADDR;

  /* 1. Marshal Request */
  int n = convS2M(&tx, page + P9_MSG_OFFSET, P9_MSG_SIZE);
  if (n <= 0)
    return -100;

  /* 2. Syscall */
  _syscall();

  /* 3. Parse reply to distinguish parent from child */
  memset(&rx, 0, sizeof(Fcall));
  uint parsed = convM2S(page + P9_MSG_OFFSET, P9_MSG_SIZE, &rx);

  /* Child detection:
   * - Child's exchange page is fresh/empty (P9SEG newly allocated on fault)
   * - Parse will fail OR return wrong message type
   * - Parent's page has valid Rsysfork reply
   */
  if (parsed == 0 || rx.type != Rsysfork) {
    /* We are the child - return 0 */
    return 0;
  }

  /* We are the parent - return child PID from reply */
  return (int)rx.pid;
}

int sys_bind(char *old, char *new, int flags) {
  uchar buf[1024];
  uchar *p = buf;

  p += packstr(p, old);
  p += packstr(p, new);
  pack32(p, flags);
  p += 4;

  return do_syscall(SYS_BIND_RAW, buf, p - buf, nil);
}

int sys_getpid2(void *out, ulong len) {
  uchar buf[32];
  uchar *p = buf;

  pack64(p, (uvlong)out);
  p += 8;
  pack64(p, (uvlong)len);
  p += 8;
  return do_syscall(SYS_GETPID2, buf, p - buf, nil);
}

void sys_exec(char *path) {
  /* Use Tsysexec (162) which is cleaner and verified in kernel */
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsysexec;
  tx.tag = 1;
  tx.path = path;
  tx.argc = 0; /* No additional args for now */

  int ret = lux_call(&tx, &rx);
  /* If we return, exec failed */
}

int sys_pipe(int *fds) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));

  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_PIPE;
  tx.sflags = 0;
  tx.sdata = 0;
  tx.scount = 0;

  if (lux_call(&tx, &rx) < 0)
    return -1;

  // Response contains 8 bytes: [fd0:4][fd1:4]
  if (rx.scount >= 8 && rx.sdata) {
    uchar *p = rx.sdata;
    fds[0] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    p += 4;
    fds[1] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
  } else {
    return -1;
  }
  return 0;
}

long sys_seek(int fd, long offset, int whence) {
  uchar buf[32];
  uchar *p = buf;

  // [fd 4] [offset 8] [whence 4]
  pack32(p, fd);
  p += 4;
  pack64(p, offset);
  p += 8;
  pack32(p, whence);
  p += 4;

  u64int ret;
  if (do_syscall(SYS_SEEK, buf, p - buf, &ret) < 0)
    return -1;
  return (long)ret;
}

int sys_wait(void) {
  u64int ret;
  if (do_syscall(SYS_WAIT, 0, 0, &ret) < 0)
    return -1;
  return (int)ret;
}

long sys_pread(int fd, void *buf, long n, long offset) {
  uchar sbuf[32];
  uchar *p = sbuf;
  Fcall tx, rx;

  // [fd 4] [offset 8] [count 4]
  pack32(p, fd);
  p += 4;
  pack64(p, offset);
  p += 8;
  pack32(p, n);
  p += 4;

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_PREAD;
  tx.sdata = sbuf; // Just header
  tx.scount = p - sbuf;

  if (lux_call(&tx, &rx) < 0)
    return -1;

  if (rx.count > n)
    rx.count = n;
  if (rx.count > 0 && rx.sdata) {
    memmove(buf, rx.sdata, rx.count);
  }
  return rx.count;
}

uvlong sys_nsec(void) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_NSEC;
  tx.scount = 0;
  tx.sdata = 0;

  if (lux_call(&tx, &rx) < 0)
    return 0;
  return rx.retval;
}

int sys_stat(char *path, uchar *buf, int nbuf) {
  uchar sbuf[1024];
  uchar *p = sbuf;

  // [path s]
  p += packstr(p, path);

  u64int ret;
  if (do_syscall(SYS_STAT, sbuf, p - sbuf, &ret) < 0)
    return -1;
  // TODO: Copy stat data from response if needed
  return 0;
}

int sys_wstat(char *path, uchar *buf, int nbuf) {
  uchar sbuf[1024];
  uchar *p, *allocbuf = nil;
  uchar *data_start;

  int pathlen = 0;
  while (path[pathlen])
    pathlen++;

  int hdr_len = 2 + pathlen + 2; // pathlen+path+nstat
  if (hdr_len + nbuf <= sizeof(sbuf)) {
    p = sbuf;
  } else {
    if (pebble_alloc(hdr_len + nbuf, (void **)&allocbuf) < 0)
      return -1;
    p = allocbuf;
  }

  data_start = p;
  // [path s] [nstat 2] [stat bytes]
  p += packstr(p, path);
  pack16(p, nbuf);
  p += 2;
  memmove(p, buf, nbuf);
  p += nbuf;

  u64int ret;
  int res = do_syscall(SYS_WSTAT, data_start, p - data_start, &ret);

  if (allocbuf)
    pebble_free(allocbuf);

  if (res < 0)
    return -1;
  return 0;
}

int sys_mount(int fd, int afd, char *old, int flags, char *aname) {
  uchar buf[1024];
  uchar *p = buf;

  // [fd 4] [afd 4] [old s] [flags 4] [aname s]
  pack32(p, fd);
  p += 4;
  pack32(p, afd);
  p += 4;
  p += packstr(p, old);
  pack32(p, flags);
  p += 4;
  p += packstr(p, aname);

  u64int ret;
  if (do_syscall(SYS_MOUNT, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

int sys_sleep(long ms) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyssleep;
  tx.tag = 1;
  tx.count = (u32int)ms;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return 0;
}

/* Exchange Pool Syscalls */

ExchangeCapability *sys_exchange_alloc(void) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_ALLOC; // Should be 67
  tx.scount = 0;
  tx.sdata = 0;

  if (lux_call(&tx, &rx) < 0)
    return nil;

  if (rx.retval == 0)
    return nil;

  return (ExchangeCapability *)rx.retval;
}

int sys_exchange_free(ExchangeCapability *cap) {
  uchar buf[sizeof(uintptr)];
  uchar *p = buf;

  // Pack the capability pointer
  uintptr cap_addr = (uintptr)cap;
  pack32(p, (uint)(cap_addr & 0xFFFFFFFF));
  if (sizeof(uintptr) > 4) {
    pack32(p + 4, (uint)(cap_addr >> 32));
  }

  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_FREE; // Should be 68
  tx.sdata = buf;
  tx.scount = sizeof(uintptr);

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return 0;
}

/* Exchange Pool IPC Syscalls */

#define SYS_EXCHANGE_PUBLISH 69
#define SYS_EXCHANGE_SUBSCRIBE 70
#define SYS_EXCHANGE_UNSUBSCRIBE 71
#define SYS_EXCHANGE_RECEIVE 72

ExchangeCapability *sys_exchange_publish(char *topic, void *data, ulong len) {
  uchar buf[1024]; // Buffer for packed arguments
  uchar *p = buf;

  // Pack arguments: topic (string), data (pointer), len (ulong)
  int topic_len = 0;
  char *t = topic;
  while (*t++)
    topic_len++;

  if (topic_len >= sizeof(buf) - sizeof(ulong) - 8)
    return nil; // Topic too long

  // Copy topic string
  memmove(p, topic, topic_len + 1); // Include null terminator
  p += topic_len + 1;

  // Pack data pointer and length
  pack32(p, (uint)(uintptr)data);
  p += 4;
  if (sizeof(uintptr) > 4) {
    pack32(p, (uint)((uintptr)data >> 32));
    p += 4;
  }
  pack32(p, (uint)len);
  p += 4;

  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_PUBLISH;
  tx.sdata = buf;
  tx.scount = p - buf;

  if (lux_call(&tx, &rx) < 0)
    return nil;

  if (rx.retval == 0)
    return nil;

  return (ExchangeCapability *)rx.retval;
}

int sys_exchange_subscribe(char *topic) {
  uchar buf[256]; // Buffer for topic string
  int topic_len = 0;
  char *t = topic;
  while (*t++)
    topic_len++;

  if (topic_len >= sizeof(buf))
    return -1; // Topic too long

  memmove(buf, topic, topic_len + 1); // Include null terminator

  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_SUBSCRIBE;
  tx.sdata = buf;
  tx.scount = topic_len + 1;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return (int)rx.retval;
}

int sys_exchange_unsubscribe(char *topic) {
  uchar buf[256]; // Buffer for topic string
  int topic_len = 0;
  char *t = topic;
  while (*t++)
    topic_len++;

  if (topic_len >= sizeof(buf))
    return -1; // Topic too long

  memmove(buf, topic, topic_len + 1); // Include null terminator

  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_UNSUBSCRIBE;
  tx.sdata = buf;
  tx.scount = topic_len + 1;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  return (int)rx.retval;
}

Notification *sys_exchange_receive(void) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_EXCHANGE_RECEIVE;
  tx.scount = 0;
  tx.sdata = 0;

  if (lux_call(&tx, &rx) < 0)
    return nil;

  if (rx.retval == 0)
    return nil;

  return (Notification *)rx.retval;
}
