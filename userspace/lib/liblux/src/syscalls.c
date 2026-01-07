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
     Parent receives Reply Message.
     Child receives 0 in RAX and NO Message (empty exchange page).
  */
  uchar buf[16];
  Fcall tx, rx;
  uchar *p = buf;

  memset(&tx, 0, sizeof(Fcall));
  tx.type = Tsyscall;
  tx.tag = 1;
  tx.scallnr = SYS_RFORK;

  pack32(p, flags);
  tx.sdata = buf;
  tx.scount = 4;

  uchar *page = (uchar *)EXCHANGE_PAGE_ADDR;

  /* 1. Marshal Request */
  int n = convS2M(&tx, page + P9_MSG_OFFSET, P9_MSG_SIZE);
  if (n <= 0)
    return -100;

  /* 2. Syscall */
  long reg_ret = _syscall();

  sys_write(1, "DEBUG: _syscall returned\n", 25);

  /* 3. Handle Child */
  if (reg_ret == 0) {
    /* Child */
    return 0;
  }

  /* Parent */
  char buf[32];
  int i = 0;
  long n = reg_ret;
  if (n == 0)
    buf[i++] = '0';
  while (n > 0) {
    buf[i++] = (n % 10) + '0';
    n /= 10;
  }
  buf[i] = '\n';
  sys_write(1, "rfork_ret: ", 11);
  sys_write(1, buf, i);

  /* Parent reads reply from Exchange Page ... or just returns AX value? */
  /* We fixed kernel to set AX, so we can just return reg_ret */
  return (int)reg_ret;

  /* 4. Handle Parent - Read Reply */
  if ((u64int)&rx > 0x7FFFFFFFFFFF)
    return -2;
  memset(&rx, 0, sizeof(Fcall));

  uint ret = convM2S(page + P9_MSG_OFFSET, P9_MSG_SIZE, &rx);
  if ((int)ret <= 0)
    return -200;

  if (rx.type == Rerror)
    return -1;

  /* For Parent, kernel returns PID in retval */
  return (int)rx.retval;
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
  tx.name = path;
  tx.argc = 0; /* No additional args for now */

  lux_call(&tx, &rx);
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
