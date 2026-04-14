#include "lux_internal.h"
#include <stdarg.h>

/* Use custom string functions from string.c, NOT glibc */
extern void *memmove(void *dst, const void *src, ulong n);
extern void *memset(void *dst, int c, ulong n);
extern ulong strlen(const char *s);

/* Helper function declarations (must be after lux_internal.h for types) */
extern uint convS2M(struct Fcall *f, uchar *ap, uint n);
extern uint convM2S(uchar *ap, uint n, struct Fcall *f);
extern uint convM2S(uchar *ap, uint n, struct Fcall *f);
extern int vsnprint(char *buf, int len, const char *fmt, va_list args);
extern void *malloc(unsigned long);
extern void free(void *);

#ifndef OREAD
#define OREAD 0
#endif
#ifndef OWRITE
#define OWRITE 1
#endif
#ifndef BIND
#define BIND 2
#endif
#ifndef FD2PATH
#define FD2PATH 23
#endif
#ifndef UNMOUNT
#define UNMOUNT 35
#endif

static uchar *rx_exchange_data(Fcall *rx);
int lux_call(struct Fcall *tx, struct Fcall *rx);

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

static int path_under_srv(const char *path) {
  if (!path)
    return 0;
  if (path[0] != '/' || path[1] != 's' || path[2] != 'r' || path[3] != 'v')
    return 0;
  return path[4] == 0 || path[4] == '/';
}

static int path_is_srv_publish(const char *path) {
  const char *p;

  if (!path_under_srv(path) || path[4] != '/')
    return 0;
  p = path + 5;
  if (*p == 0)
    return 0;
  while (*p != 0) {
    if (*p == '/')
      return 0;
    p++;
  }
  return 1;
}

static int srv_endpoint_path(const char *path, char *buf, int buflen) {
  const char *name;

  if (!path_is_srv_publish(path))
    return -1;
  name = path + 5;
  if (snprint(buf, buflen, "/srv/%s.endpoint", name) >= buflen)
    return -1;
  return 0;
}

static int fd_to_path(int fd, char *buf, int buflen) {
  uchar sbuf[16];
  uchar *p = sbuf;
  Fcall tx, rx;
  uchar *src;
  int n;

  if (!buf || buflen <= 0)
    return -1;

  pack32(p, fd);
  p += 4;
  pack32(p, buflen);
  p += 4;

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = FD2PATH;
  tx.sdata = sbuf;
  tx.scount = p - sbuf;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  if (rx.scount <= 0)
    return -1;

  src = rx_exchange_data(&rx);
  if (!src)
    return -1;

  n = (int)rx.scount;
  if (n > buflen)
    n = buflen;
  memmove(buf, src, n);
  buf[buflen - 1] = 0;
  return 0;
}

static int mnt_ctl_write(const char *cmd) {
  int fd, len;

  fd = sys_open("/mnt/ctl", OWRITE);
  if (fd < 0)
    return -1;

  len = (int)strlen(cmd);
  if (sys_write(fd, (void *)cmd, len) != len) {
    sys_close(fd);
    return -1;
  }
  sys_close(fd);
  return 0;
}

static int write_text_path(const char *path, const char *text) {
  int fd, len;

  fd = sys_open((char *)path, OWRITE);
  if (fd < 0)
    fd = sys_create((char *)path, OWRITE, 0666);
  if (fd < 0)
    return -1;

  len = (int)strlen(text);
  if (sys_write(fd, (void *)text, len) != len) {
    sys_close(fd);
    return -1;
  }
  sys_close(fd);
  return 0;
}

static int srv_publish_fd(int fd, const char *srvpath) {
  char endpoint[256];
  char endpoint_file[320];

  if (fd_to_path(fd, endpoint, sizeof(endpoint)) < 0)
    return -1;
  if (srv_endpoint_path(srvpath, endpoint_file, sizeof(endpoint_file)) < 0)
    return -1;
  return write_text_path(endpoint_file, endpoint);
}

int sys_srv_publish(char *path, int fd) {
  if (path == nil)
    return -1;
  if (!path_is_srv_publish(path))
    return -1;
  return srv_publish_fd(fd, path);
}

int lux_call(struct Fcall *tx, struct Fcall *rx) {
  uchar *page = (uchar *)lux_exchange_page();
  P9Control *ctl = (P9Control *)(page + P9_CONTROL_OFFSET);
  u32int req_seq;

  /* 1. Marshal Request */
  int n = convS2M(tx, page + P9_MSG_OFFSET, P9_MSG_SIZE);
  if (n <= 0)
    return -100; /* convS2M failed */

  /* 2. Monotonic request: advance seq + mark pending */
  req_seq = ctl->req_seq + 1;
  ctl->req_seq = req_seq;
  ctl->status = P9_STATUS_PENDING;
  __asm__ volatile("mfence" ::: "memory");

  /* 3. Ring Doorbell */
  _syscall();
  __asm__ volatile("mfence" ::: "memory");

  if (ctl->status != P9_STATUS_COMPLETE || ctl->rep_seq != req_seq)
    return -300; /* monotonicity violation */

  /* 4. Unmarshal Reply */
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

extern uint convS2M(Fcall *f, uchar *ap, uint n);
extern uint convM2S(uchar *ap, uint n, Fcall *f);
extern int vsnprint(char *, int, const char *, va_list);

static uchar *rx_exchange_data(Fcall *rx) {
  uintptr base = lux_exchange_page() + P9_MSG_OFFSET;
  uintptr end = base + P9_MSG_SIZE;
  uintptr p = (uintptr)rx->sdata;
  uintptr fallback = base + (4 + 1 + 2 + 8 + 4);

  if (p >= base && p <= end && rx->scount <= end - p)
    return (uchar *)p;
  if (fallback >= base && fallback <= end && rx->scount <= end - fallback)
    return (uchar *)fallback;
  return nil;
}

/* Generic syscall wrapper with sdata buffer management */
static int do_syscall(int scallnr, uchar *sdata, int scount, u64int *retval) {
  Fcall tx, rx;
  volatile u32int *trace;

  /* Clear structures to avoid any stack garbage */
  memset(&tx, 0, sizeof(Fcall));
  // memset(&rx, 0, sizeof(Fcall)); // lux_call clears this

  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = scallnr;
  tx.sflags = 0;
  tx.sdata = sdata;
  tx.scount = scount;

  trace = (u32int *)(lux_exchange_page() + P9_MSG_OFFSET + P9_MSG_SIZE - 8);
  trace[0] = 0x54535953; /* 'TSYS' */
  trace[1] = (u32int)scallnr;

  int err = lux_call(&tx, &rx);
  if (err < 0)
    return err;

  if (retval)
    *retval = rx.retval;
  return 0;
}

// Implement sys_print
int sys_print(const char *fmt, ...) {
  char buf[256];
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprint(buf, sizeof(buf), fmt, args);
  va_end(args);

  return sys_write(1, buf, n);
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
  uchar *src;

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
  if (rx.scount > n)
    rx.scount = n;
  if (rx.scount > 0) {
    src = rx_exchange_data(&rx);
    if (!src)
      return -1;
    memmove(buf, src, rx.scount);
  }
  return rx.scount;
}

long sys_write(int fd, void *buf, long n) {
  uchar sbuf[1024];
  uchar *p, *allocbuf = nil;
  uchar *data_start;
  volatile u32int *trace;

  trace = (u32int *)(lux_exchange_page() + P9_MSG_OFFSET + P9_MSG_SIZE - 8);
  trace[0] = 0x57524954; /* 'WRIT' */
  trace[1] = (u32int)fd;

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
  Fcall tx;
  memset(&tx, 0, sizeof(Fcall));
  tx.type = Tsysfork;
  tx.tag = 1;
  tx.flags = flags;

  uchar *page = (uchar *)lux_exchange_page();
  P9Control *ctl = (P9Control *)(page + P9_CONTROL_OFFSET);
  int n = convS2M(&tx, page + P9_MSG_OFFSET, P9_MSG_SIZE);
  if (n <= 0)
    return -1;

  u32int req_seq = ctl->req_seq + 1;
  ctl->req_seq = req_seq;
  ctl->status = P9_STATUS_PENDING;
  __asm__ volatile("mfence" ::: "memory");

  long ret = _syscall();
  __asm__ volatile("mfence" ::: "memory");

  /*
   * Do not touch stack-resident exchange pointers after returning.
   * With RFMEM/vfork-style forks the child may run on this same user stack
   * until exec, so cached locals like 'page' and 'ctl' are not reliable.
   * The kernel already returns the child pid (or 0/-errno) in AX.
   */
  return (int)ret;
}

int sys_rfork_stack(int flags, void *stack_top, void (*func)(void *), void *arg)
{
  Fcall tx;
  uchar *page;
  P9Control *ctl;
  int n;
  u32int req_seq;

  memset(&tx, 0, sizeof(Fcall));
  tx.type = Tsysfork;
  tx.tag = 1;
  tx.flags = flags;

  page = (uchar *)lux_exchange_page();
  ctl = (P9Control *)(page + P9_CONTROL_OFFSET);
  n = convS2M(&tx, page + P9_MSG_OFFSET, P9_MSG_SIZE);
  if (n <= 0)
    return -1;

  req_seq = ctl->req_seq + 1;
  ctl->req_seq = req_seq;
  ctl->status = P9_STATUS_PENDING;
  __asm__ volatile("mfence" ::: "memory");
  return (int)_syscall_rfork_stack(stack_top, func, arg);
}

int sys_bind_raw(char *old, char *new, int flags) {
  uchar buf[1024];
  uchar *p = buf;

  p += packstr(p, old);
  p += packstr(p, new);
  pack32(p, flags);
  p += 4;

  return do_syscall(BIND, buf, p - buf, nil);
}

int sys_bind(char *old, char *new, int flags) {
  char cmd[512];

  if (path_under_srv(new))
    return -1;
  if (snprint(cmd, sizeof(cmd), "bind %s %s %d", old, new, flags) >=
      (int)sizeof(cmd))
    return -1;
  return mnt_ctl_write(cmd);
}

int sys_dup(int oldfd, int newfd) {
  Fcall tx, rx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));

  tx.type = Tsysdup;
  tx.tag = 1;
  tx.fid = oldfd;
  tx.newfid = (u32int)newfd;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  if (rx.type != Rsysdup)
    return -1;
  return (int)rx.fid;
}
int sys_getpid2(uuid_t *out, ulong len) {
  uchar buf[32];
  uchar *p = buf;

  if (len + 32 > sizeof(buf))
    return -1;

  *(void **)p = out;
  p += sizeof(void *);
  *(ulong *)p = len;
  p += sizeof(ulong);

  return do_syscall(SYS_GETPID2, buf, p - buf, nil);
}

void *segattach(int attr, char *spec, void *addr, ulong len) {
  uchar buf[64];
  uchar *p = buf;

  *(int *)p = attr;
  p += sizeof(int);
  *(char **)p = spec;
  p += sizeof(char *);
  *(void **)p = addr;
  p += sizeof(void *);
  *(ulong *)p = len;
  p += sizeof(ulong);

  u64int ret;
  if (do_syscall(SYS_SEGATTACH, buf, p - buf, &ret) < 0)
    return (void *)-1;
  return (void *)ret;
}

/* Global to preserve error details across sys_exec call */
static Fcall sys_exec_rx_global;

int sys_exec(char *path, char *argv[]) {
  /* Use Tsysexec (162) which is cleaner and verified in kernel */
  Fcall tx;
  memset(&tx, 0, sizeof(Fcall));
  memset(&sys_exec_rx_global, 0, sizeof(Fcall));
  tx.type = Tsysexec;
  tx.tag = 1;
  tx.path = path;
  tx.argv = argv;

  // Count argc
  int argc = 0;
  if (argv != nil) {
    while (argv[argc] != nil) {
      argc++;
    }
  }
  tx.argc = argc;

  int ret = lux_call(&tx, &sys_exec_rx_global);
  /* If we return, exec failed - return error code */
  return ret;
}

/* Get the last exec error message (if any) - safe for NULL pointers */
char *sys_exec_error(void) {
  /* Check if we have a valid Rerror with an error name */
  if (sys_exec_rx_global.type == Rerror) {
    /* The ename might be in the exchange page, which could be invalid
     * Check if ename is a reasonable kernel or user address before using */
    if (sys_exec_rx_global.ename != nil &&
        (uintptr)sys_exec_rx_global.ename != 0) {
      return sys_exec_rx_global.ename;
    }
  }
  return "(no error message available)";
}

/*
 * sys_spawnx - secure spawn interface.
 *
 * v1 supports atomic spawn+exec with kernel-side fresh-image setup.
 * Extended pre-exec action vectors are reserved for future kernel support.
 */
int sys_spawnx(char *path, char *argv[], SpawnxSpec *spec) {
  Fcall tx, rx;
  int argc = 0;

  if (path == nil)
    return -1;

  if (spec != nil) {
    int default_flags = RFPROC | RFFDG;
    if (spec->rfork_flags != 0 && spec->rfork_flags != default_flags)
      return -1;
    if (spec->nfd_actions != 0 || spec->nbind_actions != 0 || spec->do_mount)
      return -1;
  }

  memset(&tx, 0, sizeof(Fcall));
  memset(&rx, 0, sizeof(Fcall));
  tx.type = Tsysspawn;
  tx.tag = 1;
  tx.path = path;
  tx.argv = argv;

  if (argv != nil) {
    while (argv[argc] != nil)
      argc++;
  }
  tx.argc = argc;

  if (lux_call(&tx, &rx) < 0)
    return -1;
  if (rx.type != Rsysspawn)
    return -1;
  return (int)rx.pid;
}

int sys_spawn(char *path, char *argv[]) { return sys_spawnx(path, argv, nil); }

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
  if (rx.scount >= 8) {
    uchar *p = rx_exchange_data(&rx);
    if (!p)
      return -1;
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

  if (rx.scount > n)
    rx.scount = n;
  if (rx.scount > 0) {
    uchar *src = rx_exchange_data(&rx);
    if (!src)
      return -1;
    memmove(buf, src, rx.scount);
  }
  return rx.scount;
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

int sys_mount_raw(int fd, int afd, char *old, int flags, char *aname) {
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

int sys_nsroot_publish_raw(int fd, char *path, char *aname) {
  uchar buf[1024];
  uchar *p = buf;
  u64int ret;

  pack32(p, fd);
  p += 4;
  p += packstr(p, path);
  p += packstr(p, aname ? aname : "");

  if (do_syscall(SYS_NSROOT_PUBLISH, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

int sys_nsroot_unpublish_raw(char *path) {
  uchar buf[256];
  uchar *p = buf;
  u64int ret;

  p += packstr(p, path);

  if (do_syscall(SYS_NSROOT_UNPUBLISH, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

int sys_mount(int fd, int afd, char *old, int flags, char *aname) {
  char endpoint[256];
  char cmd[768];

  if (afd >= 0)
    return -1;
  if (path_under_srv(old))
    return -1;
  if (fd_to_path(fd, endpoint, sizeof(endpoint)) < 0)
    return -1;
  if (aname && aname[0]) {
    if (snprint(cmd, sizeof(cmd), "mount %s %s %d %s", endpoint, old, flags,
                aname) >= (int)sizeof(cmd))
      return -1;
  } else {
    if (snprint(cmd, sizeof(cmd), "mount %s %s %d", endpoint, old, flags) >=
        (int)sizeof(cmd))
      return -1;
  }
  return mnt_ctl_write(cmd);
}

int sys_unmount_raw(char *name, char *old) {
  uchar buf[1024];
  uchar *p = buf;
  u64int ret;

  p += packstr(p, name ? name : "");
  p += packstr(p, old ? old : "");

  if (do_syscall(UNMOUNT, buf, p - buf, &ret) < 0)
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

int sys_exchange_prepare(uintptr vaddr, ExchangeCapability *out_cap) {
  uchar buf[sizeof(uintptr) * 2];
  uchar *p = buf;
  u64int ret;

  if (!out_cap)
    return -1;

  pack64(p, vaddr);
  p += sizeof(uintptr);
  pack64(p, (uvlong)(uintptr)out_cap);
  p += sizeof(uintptr);

  if (do_syscall(SYS_EXCHANGE_PREPARE, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

int sys_exchange_prepare_range(uintptr vaddr, ulong len,
                               ExchangeCapability *handles) {
  uchar buf[sizeof(uintptr) * 3];
  uchar *p = buf;
  u64int ret;

  if (!handles || len == 0)
    return -1;

  pack64(p, vaddr);
  p += sizeof(uintptr);
  pack64(p, len);
  p += sizeof(uintptr);
  pack64(p, (uvlong)(uintptr)handles);
  p += sizeof(uintptr);

  if (do_syscall(SYS_EXCHANGE_PREPARE_RANGE, buf, p - buf, &ret) < 0)
    return -1;
  return (int)ret;
}

int sys_exchange_accept(const ExchangeCapability *cap, uintptr dest_vaddr,
                        int prot) {
  uchar buf[sizeof(uintptr) * 2 + sizeof(uint)];
  uchar *p = buf;
  u64int ret;

  if (!cap)
    return -1;

  pack64(p, (uvlong)(uintptr)cap);
  p += sizeof(uintptr);
  pack64(p, dest_vaddr);
  p += sizeof(uintptr);
  pack32(p, prot);
  p += sizeof(uint);

  if (do_syscall(SYS_EXCHANGE_ACCEPT, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

int sys_exchange_cancel(const ExchangeCapability *cap) {
  uchar buf[sizeof(uintptr)];
  uchar *p = buf;
  u64int ret;

  if (!cap)
    return -1;

  pack64(p, (uvlong)(uintptr)cap);
  p += sizeof(uintptr);

  if (do_syscall(SYS_EXCHANGE_CANCEL, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

int sys_exchange_transfer(int from_pid, int to_pid,
                          const ExchangeCapability *cap, uintptr dest_vaddr) {
  uchar buf[sizeof(uint) * 2 + sizeof(uintptr) * 2];
  uchar *p = buf;
  u64int ret;

  if (!cap)
    return -1;

  pack32(p, from_pid);
  p += sizeof(uint);
  pack32(p, to_pid);
  p += sizeof(uint);
  pack64(p, (uvlong)(uintptr)cap);
  p += sizeof(uintptr);
  pack64(p, dest_vaddr);
  p += sizeof(uintptr);

  if (do_syscall(SYS_EXCHANGE_TRANSFER, buf, p - buf, &ret) < 0)
    return -1;
  return 0;
}

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
