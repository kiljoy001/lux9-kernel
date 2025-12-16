#include "../include/lux.h"
#include <fcall.h>
#include <stdarg.h>

int snprint(char *buf, int len, const char *fmt, ...);

/* 9P doorbell client */
#define EXCHANGE_PAGE_ADDR 0x7fffffff0000ULL
#define P9_REQUEST_OFFSET 0x000
#define P9_REQUEST_SIZE 0xF00
#define P9_REPLY_OFFSET 0x1000
#define P9_REPLY_SIZE 0x1000
#define P9_CONTROL_OFFSET 0xF00
#define P9_STATUS_COMPLETE 2

typedef struct P9Control {
  volatile u32 doorbell;
  volatile u32 status;
  volatile u32 req_head;
  volatile u32 req_tail;
  volatile u32 rep_head;
  volatile u32 rep_tail;
  volatile u32 req_seq;
  volatile u32 rep_seq;
  u8 session_pebble[32];
  u8 reserved[192];
} P9Control;

typedef struct FidEnt FidEnt;
struct FidEnt {
  int used;
  u32 fid;
  uvlong offset;
  char path[128];
};

#define MAXFID 32
static FidEnt fidtab[MAXFID];
static u32 nextfid = 1;
static u16 nexttag = 1;

static P9Control *p9ctl = (P9Control *)(EXCHANGE_PAGE_ADDR + P9_CONTROL_OFFSET);
static u8 *p9req = (u8 *)(EXCHANGE_PAGE_ADDR + P9_REQUEST_OFFSET);
static u8 *p9rep = (u8 *)(EXCHANGE_PAGE_ADDR + P9_REPLY_OFFSET);

static u32 allocfid(void) {
  for (int i = 0; i < MAXFID; i++) {
    if (!fidtab[i].used) {
      fidtab[i].used = 1;
      fidtab[i].fid = nextfid++;
      fidtab[i].offset = 0;
      fidtab[i].path[0] = 0;
      return fidtab[i].fid;
    }
  }
  return (u32)-1;
}

static FidEnt *lookupfid(u32 fid) {
  for (int i = 0; i < MAXFID; i++) {
    if (fidtab[i].used && fidtab[i].fid == fid)
      return &fidtab[i];
  }
  return nil;
}

static void freefid(u32 fid) {
  for (int i = 0; i < MAXFID; i++) {
    if (fidtab[i].used && fidtab[i].fid == fid) {
      fidtab[i].used = 0;
      return;
    }
  }
}

static long doorbell_call(Fcall *t, Fcall *r) {
  int len = convS2M(t, p9req, P9_REQUEST_SIZE);
  if (len <= 0)
    return -1;
  p9ctl->req_head = 0;
  p9ctl->req_tail = len;
  p9ctl->doorbell = 1;
  /* trap into kernel; number ignored by doorbell handler */
  _syscall(SYS_RFORK, 0);
  if (p9ctl->status != P9_STATUS_COMPLETE)
    return -1;
  u32 rep_size = p9ctl->rep_tail - p9ctl->rep_head;
  if (rep_size == 0 || rep_size > P9_REPLY_SIZE)
    return -1;
  if (convM2S(p9rep + p9ctl->rep_head, rep_size, r) == 0)
    return -1;
  return 0;
}

static int attach_path(const char *path, u32 fid) {
  Fcall t, r;
  memset(&t, 0, sizeof(t));
  t.type = Tattach;
  t.tag = nexttag++;
  t.fid = fid;
  t.afid = NOFID;
  t.uname = "user";
  t.aname = (char *)path;
  if (doorbell_call(&t, &r) < 0 || r.type != Rattach)
    return -1;
  return 0;
}

/* Wrapper functions for system calls */

int open(const char *path, int mode) {
  if (path == nil)
    return -1;
  u32 fid = allocfid();
  if (fid == (u32)-1)
    return -1;
  FidEnt *fe = lookupfid(fid);
  /* strncpy unimplemented in minimal libc, use simple copy */
  char *d = fe->path;
  const char *s = path;
  int n = sizeof(fe->path) - 1;
  while (n-- && *s)
    *d++ = *s++;
  *d = 0;

  if (attach_path(path, fid) < 0) {
    freefid(fid);
    return -1;
  }
  Fcall t, r;
  memset(&t, 0, sizeof(t));
  t.type = Topen;
  t.tag = nexttag++;
  t.fid = fid;
  t.mode = mode;
  if (doorbell_call(&t, &r) < 0 || r.type != Ropen) {
    freefid(fid);
    return -1;
  }
  return (int)fid;
}

int close(int fd) {
  u32 fid = (u32)fd;
  FidEnt *fe = lookupfid(fid);
  if (fe == nil)
    return -1;
  Fcall t, r;
  memset(&t, 0, sizeof(t));
  t.type = Tclunk;
  t.tag = nexttag++;
  t.fid = fid;
  if (doorbell_call(&t, &r) < 0 || r.type != Rclunk)
    return -1;
  freefid(fid);
  return 0;
}

long read(int fd, void *buf, long n) {
  u32 fid = (u32)fd;
  FidEnt *fe = lookupfid(fid);
  if (fe == nil)
    return -1;
  Fcall t, r;
  memset(&t, 0, sizeof(t));
  t.type = Tread;
  t.tag = nexttag++;
  t.fid = fid;
  t.offset = fe->offset;
  t.count = n;
  if (doorbell_call(&t, &r) < 0 || r.type != Rread)
    return -1;
  if (r.data && r.count > 0)
    memcpy(buf, r.data, r.count);
  fe->offset += r.count;
  return r.count;
}

long write(int fd, const void *buf, long n) {
  u32 fid = (u32)fd;
  FidEnt *fe = lookupfid(fid);
  if (fe == nil)
    return -1;
  Fcall t, r;
  memset(&t, 0, sizeof(t));
  t.type = Twrite;
  t.tag = nexttag++;
  t.fid = fid;
  t.offset = fe->offset;
  t.count = n;
  t.data = (uchar *)buf;
  if (doorbell_call(&t, &r) < 0 || r.type != Rwrite)
    return -1;
  fe->offset += r.count;
  return r.count;
}

int bind(const char *name, const char *old, int flag) { return -1; }

int mount(int fd, int afd, const char *old, int flag, const char *aname) {
  return -1;
}

long brk(void *addr) { return -1; }

void exits(const char *msg) {
  (void)msg;
  while (1)
    ;
}

int rfork(int flags) {
  (void)flags;
  return -1;
}

int await(char *status, int nstatus) {
  (void)status;
  (void)nstatus;
  return -1;
}

/*
 * spawn - Create a new process via 9P
 * Replaces fork/exec model.
 */
int spawn(const char *path, char *const argv[]) {
  int ctl;
  char cmd[512];
  int n = 0;
  int i = 0;

  /* 1. Construct command string: "spawn <path> <arg1> <arg2>..." */
  /* Note: simplified quoting for prototype */

  n = snprint(cmd, sizeof(cmd), "spawn %s", path);
  if (argv) {
    while (argv[i]) {
      /* Check buffer space (simple check) */
      if (n + strlen(argv[i]) + 4 >= sizeof(cmd))
        break;
      n += snprint(cmd + n, sizeof(cmd) - n, " \"%s\"", argv[i]);
      i++;
    }
  }

  /* 2. Open /proc/self/ctl */
  ctl = open("/proc/self/ctl", 1); /* O_WRITE = 1 */
  if (ctl < 0)
    return -1;

  /* 3. Write command */
  if (write(ctl, cmd, n) < 0) {
    close(ctl);
    return -1;
  }

  /* 4. Close */
  close(ctl);

  /* TODO: Return PID? For now, success/fail */
  return 0;
}

/* Utility functions */

void *memset(void *s, int c, ulong n) {
  unsigned char *p = s;
  while (n--)
    *p++ = (unsigned char)c;
  return s;
}

void *memcpy(void *dest, const void *src, ulong n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  while (n--)
    *d++ = *s++;
  return dest;
}

ulong strlen(const char *s) {
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

char *strcpy(char *dest, const char *src) {
  char *d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

char *strncpy(char *dest, const char *src, ulong n) {
  char *d = dest;
  while (n > 0 && *src) {
    *d++ = *src++;
    n--;
  }
  while (n > 0) {
    *d++ = 0;
    n--;
  }
  return dest;
}

char *strrchr(const char *s, int c) {
  const char *found = nil;
  while (*s) {
    if (*s == (char)c)
      found = s;
    s++;
  }
  if (c == 0)
    return (char *)s;
  return (char *)found;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* Minimal print support */
/* Helper for formatting to buffer */
static int vsnprint_minimal(char *buf, int len, const char *fmt, va_list ap) {
  char *p = buf;
  char *end = buf + len - 1;
  const char *f = fmt;

  while (*f) {
    if (p >= end)
      break;
    if (*f == '%') {
      f++;
      if (*f == 's') {
        char *s = va_arg(ap, char *);
        if (!s)
          s = "(null)";
        while (*s && p < end)
          *p++ = *s++;
      } else if (*f == 'd' || *f == 'l') {
        long v = va_arg(ap, long);
        if (v < 0) {
          if (p < end)
            *p++ = '-';
          v = -v;
        }
        char numbuf[32];
        int i = 0;
        if (v == 0)
          numbuf[i++] = '0';
        while (v > 0) {
          numbuf[i++] = '0' + (v % 10);
          v /= 10;
        }
        while (i > 0 && p < end)
          *p++ = numbuf[--i];
      } else if (*f == 'u') { // %lud etc
        unsigned long v = va_arg(ap, unsigned long);
        char numbuf[32];
        int i = 0;
        if (v == 0)
          numbuf[i++] = '0';
        while (v > 0) {
          numbuf[i++] = '0' + (v % 10);
          v /= 10;
        }
        while (i > 0 && p < end)
          *p++ = numbuf[--i];
      } else {
        if (p < end)
          *p++ = *f;
      }
    } else {
      *p++ = *f;
    }
    f++;
  }
  *p = 0;
  return p - buf;
}

int snprint(char *buf, int len, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprint_minimal(buf, len, fmt, args);
  va_end(args);
  return n;
}

int print(const char *fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  int n = vsnprint_minimal(buf, sizeof(buf), fmt, args);
  va_end(args);
  write(2, buf, n);
  return n;
}
