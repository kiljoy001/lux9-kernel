/*
 * rump_server.c
 *
 * Main entry point for the Lux9 Rump Server.
 * Initialized the NetBSD rump kernel and sets up the 9P server loop.
 */

#include <stddef.h>
#include <stdint.h>

/* Define types required by NetBSD headers */
typedef long off_t;
typedef unsigned int mode_t;
typedef uint64_t dev_t;
typedef int pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef unsigned long u_long;

#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#include <rump/rumpdefs.h>

/* Rump definitions */
struct rump_timespec {
  int64_t tv_sec;
  long tv_nsec;
};

/* Explicit layout matching NetBSD x86_64 stat struct */
struct rump_stat {
  dev_t st_dev;                          /* 0: 8 bytes */
  mode_t st_mode;                        /* 8: 4 bytes */
  uint32_t _pad0;                        /* 12: 4 bytes padding */
  uint64_t st_ino;                       /* 16: 8 bytes */
  uint32_t st_nlink;                     /* 24: 4 bytes */
  uid_t st_uid;                          /* 28: 4 bytes */
  gid_t st_gid;                          /* 32: 4 bytes */
  uint32_t _pad1;                        /* 36: 4 bytes padding */
  dev_t st_rdev;                         /* 40: 8 bytes */
  struct rump_timespec st_atimespec;     /* 48 */
  struct rump_timespec st_mtimespec;     /* 64 */
  struct rump_timespec st_ctimespec;     /* 80 */
  struct rump_timespec st_birthtimespec; /* 96 */
  off_t st_size;                         /* 112: 8 bytes */
  int64_t st_blocks;                     /* 120: 8 bytes */
  int32_t st_blksize;                    /* 128: 4 bytes */
  uint32_t st_flags;                     /* 132: 4 bytes */
  uint32_t st_gen;                       /* 136: 4 bytes */
  uint32_t st_spare[2];                  /* 140: 8 bytes */
}; /* Total ~148 */

#define RUMP_S_IFMT 0170000
#define RUMP_S_IFDIR 0040000
#define RUMP_S_IFREG 0100000

/* Lux9 syscall stubs for console */
void rumpuser_putchar(int c);
void print_str(const char *s) {
  while (*s)
    rumpuser_putchar(*s++);
}

/* 9P server definitions */
#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00

typedef unsigned char uchar;
typedef unsigned int uint;
typedef uint32_t u32int;
typedef uint16_t u16int;
typedef uint64_t u64int;

struct P9Control {
  uint doorbell;
  uint status;
  uint req_head;
  uint req_tail;
  uint rep_head;
  uint rep_tail;
};

/* 9P Message Types */
enum {
  P9_Tversion = 100,
  P9_Rversion,
  P9_Tauth = 102,
  P9_Rauth,
  P9_Tattach = 104,
  P9_Rattach,
  P9_Terror = 106,
  P9_Rerror,
  P9_Tflush = 108,
  P9_Rflush,
  P9_Twalk = 110,
  P9_Rwalk,
  P9_Topen = 112,
  P9_Ropen,
  P9_Tcreate = 114,
  P9_Rcreate,
  P9_Tread = 116,
  P9_Rread,
  P9_Twrite = 118,
  P9_Rwrite,
  P9_Tclunk = 120,
  P9_Rclunk,
  P9_Tremove = 122,
  P9_Rremove,
  P9_Tstat = 124,
  P9_Rstat,
  P9_Twstat = 126,
  P9_Rwstat
};

/* Qid */
typedef struct Qid {
  uchar type;
  u32int vers;
  u64int path;
} Qid;

#define QTDIR 0x80
#define QTFILE 0x00

/* FIDs */
#define MAX_FIDS 128
#define MAX_PATH 1024

typedef struct RumpFid {
  u32int fid;
  int type; /* 0=FREE, 1=BOUND */
  char path[MAX_PATH];
  int rfd; /* Rump FD, -1 if closed */
  Qid qid;
  int opened; /* 0=no, 1=yes */
} RumpFid;

static RumpFid fids[MAX_FIDS];

static volatile uchar *exchange;
static volatile struct P9Control *ctl;
static int rump_init_done = 0;

/* Helper functions */
void *memset(void *dst, int c, size_t n) {
  uchar *d = dst;
  while (n--)
    *d++ = (uchar)c;
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  uchar *d = dst;
  const uchar *s = src;
  while (n--)
    *d++ = *s++;
  return dst;
}

static u32int get_u32(const uchar *p) {
  return (u32int)p[0] | ((u32int)p[1] << 8) | ((u32int)p[2] << 16) |
         ((u32int)p[3] << 24);
}

static void put_u32(uchar *p, u32int val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
}

static u16int get_u16(const uchar *p) {
  return (u16int)p[0] | ((u16int)p[1] << 8);
}

static void put_u16(uchar *p, u16int val) {
  p[0] = val;
  p[1] = val >> 8;
}

static void put_u64(uchar *p, u64int val) {
  put_u32(p, val & 0xFFFFFFFF);
  put_u32(p + 4, val >> 32);
}

static u64int get_u64(const uchar *p) {
  return (u64int)get_u32(p) | ((u64int)get_u32(p + 4) << 32);
}

static int strlen(const char *s) {
  int n = 0;
  while (s[n])
    n++;
  return n;
}

static void strcpy(char *dst, const char *src) {
  while (*src)
    *dst++ = *src++;
  *dst = 0;
}

static void strcat(char *dst, const char *src) {
  while (*dst)
    dst++;
  while (*src)
    *dst++ = *src++;
  *dst = 0;
}

static int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/* FID Management */
static RumpFid *alloc_fid(u32int fid) {
  for (int i = 0; i < MAX_FIDS; i++) {
    if (fids[i].type == 0) {
      fids[i].fid = fid;
      fids[i].type = 1;
      fids[i].rfd = -1;
      fids[i].path[0] = 0;
      fids[i].opened = 0;
      return &fids[i];
    }
  }
  return NULL;
}

static RumpFid *get_fid(u32int fid) {
  for (int i = 0; i < MAX_FIDS; i++) {
    if (fids[i].type == 1 && fids[i].fid == fid)
      return &fids[i];
  }
  return NULL;
}

static void free_fid(RumpFid *f) {
  if (f) {
    if (f->rfd >= 0)
      rump_sys_close(f->rfd);
    f->type = 0;
    f->rfd = -1;
  }
}

/* Message Helpers */
static u32int mk_error(uchar *buf, u16int tag, const char *ename) {
  int n = strlen(ename);
  put_u32(buf, 4 + 1 + 2 + 2 + n);
  buf[4] = P9_Rerror;
  put_u16(buf + 5, tag);
  put_u16(buf + 7, n);
  memcpy(buf + 9, ename, n);
  return 4 + 1 + 2 + 2 + n;
}

static u32int mk_qid(uchar *buf, Qid *q) {
  buf[0] = q->type;
  put_u32(buf + 1, q->vers);
  put_u64(buf + 5, q->path);
  return 13;
}

/* Handlers */
static u32int handle_version(uchar *req, uchar *resp) {
  u16int tag = get_u16(req + 5);
  put_u32(resp, 4 + 1 + 2 + 4 + 2 + 6);
  resp[4] = P9_Rversion;
  put_u16(resp + 5, tag);
  put_u32(resp + 7, 8192);
  put_u16(resp + 11, 6);
  memcpy(resp + 13, "9P2000", 6);
  return 4 + 1 + 2 + 4 + 2 + 6;
}

static u32int handle_attach(uchar *req, uchar *resp) {
  u16int tag = get_u16(req + 5);
  u32int fid = get_u32(req + 7);
  /* afid, uname, aname ignored for now */

  RumpFid *f = alloc_fid(fid);
  if (!f)
    return mk_error(resp, tag, "no free fids");

  strcpy(f->path, "/"); // Root
  /* Stat root to get qid */
  struct rump_stat st;
  if (rump_sys_stat("/", (struct stat *)&st) < 0) {
    free_fid(f);
    return mk_error(resp, tag, "stat failed");
  }

  f->qid.type = (st.st_mode & RUMP_S_IFDIR) ? QTDIR : QTFILE;
  f->qid.vers = 0;
  f->qid.path = st.st_ino;

  put_u32(resp, 4 + 1 + 2 + 13);
  resp[4] = P9_Rattach;
  put_u16(resp + 5, tag);
  mk_qid(resp + 7, &f->qid);
  return 4 + 1 + 2 + 13;
}

static u32int handle_walk(uchar *req, uchar *resp) {
  u16int tag = get_u16(req + 5);
  u32int fid = get_u32(req + 7);
  u32int newfid = get_u32(req + 11);
  u16int nwname = get_u16(req + 15);

  RumpFid *oldf = get_fid(fid);
  if (!oldf)
    return mk_error(resp, tag, "unknown fid");

  RumpFid *newf = (fid == newfid) ? oldf : alloc_fid(newfid);
  if (!newf)
    return mk_error(resp, tag, "no free fids");

  if (newf != oldf) {
    strcpy(newf->path, oldf->path);
    newf->qid = oldf->qid;
  }

  uchar *p = req + 17;
  int nwqid = 0;
  Qid qids[16];

  for (int i = 0; i < nwname; i++) {
    u16int len = get_u16(p);
    p += 2;
    char name[256];
    memcpy(name, p, len);
    name[len] = 0;
    p += len;

    if (strcmp(name, ".") == 0) {
      /* No-op */
      qids[nwqid++] = newf->qid;
      continue;
    }

    if (strcmp(name, "..") == 0) {
      /* Handle parent walk */
      int l = strlen(newf->path);
      /* find last slash */
      while (l > 0 && newf->path[l] != '/')
        l--;
      if (l == 0) {
        /* at root or /foo */
        if (newf->path[0] == '/' && newf->path[1] == 0) { /* "/" */
        } else {
          newf->path[1] = 0;
        } /* "/foo" -> "/" */
      } else {
        newf->path[l] = 0; /* "/foo/bar" -> "/foo" */
      }
    } else {
      /* Append component */
      int pl = strlen(newf->path);
      if (pl > 1 && newf->path[pl - 1] != '/')
        strcat(newf->path, "/");
      strcat(newf->path, name);
    }

    /* Stat new path */
    struct rump_stat st;
    if (rump_sys_stat(newf->path, (struct stat *)&st) < 0) {
      /* Walk failed at this step */
      if (newf != oldf)
        free_fid(newf);
      if (i == 0)
        return mk_error(resp, tag, "not found");
      /* else return success so far (nwqid < nwname) */
      break;
    }

    newf->qid.type = (st.st_mode & RUMP_S_IFDIR) ? QTDIR : QTFILE;
    newf->qid.vers = 0;
    newf->qid.path = st.st_ino;
    qids[nwqid++] = newf->qid;
  }

  /* success */
  u32int size = 4 + 1 + 2 + 2 + (nwqid * 13);
  put_u32(resp, size);
  resp[4] = P9_Rwalk;
  put_u16(resp + 5, tag);
  put_u16(resp + 7, nwqid);
  int off = 9;
  for (int i = 0; i < nwqid; i++) {
    mk_qid(resp + off, &qids[i]);
    off += 13;
  }
  return size;
}

static u32int handle_open(uchar *req, uchar *resp) {
  u16int tag = get_u16(req + 5);
  u32int fid = get_u32(req + 7);
  int mode = req[11]; /* 9P mode */

  RumpFid *f = get_fid(fid);
  if (!f)
    return mk_error(resp, tag, "unknown fid");

  /* Convert 9P mode to O_ flags */
  int oflags = 0;
  if ((mode & 3) == 0)
    oflags = RUMP_O_RDONLY;
  else if ((mode & 3) == 1)
    oflags = RUMP_O_WRONLY;
  else if ((mode & 3) == 2)
    oflags = RUMP_O_RDWR;
  if (mode & 0x10)
    oflags |= RUMP_O_TRUNC;

  /* Always open, even directories */
  f->rfd = rump_sys_open(f->path, oflags, 0);
  if (f->rfd < 0)
    return mk_error(resp, tag, "open failed");

  f->opened = 1;

  /* Get iounit (use 8192) */
  put_u32(resp, 4 + 1 + 2 + 13 + 4);
  resp[4] = P9_Ropen;
  put_u16(resp + 5, tag);
  mk_qid(resp + 7, &f->qid);
  put_u32(resp + 20, 8192);
  return 4 + 1 + 2 + 13 + 4;
}

static u32int handle_read(uchar *req, uchar *resp) {
  u16int tag = get_u16(req + 5);
  u32int fid = get_u32(req + 7);
  u64int offset = get_u64(req + 11);
  u32int count = get_u32(req + 19);

  RumpFid *f = get_fid(fid);
  if (!f || !f->opened)
    return mk_error(resp, tag, "fid not open");

  /* Seek */
  rump_sys_lseek(f->rfd, offset, 0 /* SEEK_SET */);

  /* Cap count */
  if (count > 8000)
    count = 8000;

  ssize_t n = rump_sys_read(f->rfd, resp + 11, count);
  if (n < 0)
    return mk_error(resp, tag, "read error");

  put_u32(resp, 4 + 1 + 2 + 4 + n);
  resp[4] = P9_Rread;
  put_u16(resp + 5, tag);
  put_u32(resp + 7, n);
  return 4 + 1 + 2 + 4 + n;
}

static u32int handle_write(uchar *req, uchar *resp) {
  u16int tag = get_u16(req + 5);
  u32int fid = get_u32(req + 7);
  u64int offset = get_u64(req + 11);
  u32int count = get_u32(req + 19);

  RumpFid *f = get_fid(fid);
  if (!f || !f->opened)
    return mk_error(resp, tag, "fid not open");

  rump_sys_lseek(f->rfd, offset, 0);

  ssize_t n = rump_sys_write(f->rfd, req + 23, count);
  if (n < 0)
    return mk_error(resp, tag, "write error");

  put_u32(resp, 4 + 1 + 2 + 4);
  resp[4] = P9_Rwrite;
  put_u16(resp + 5, tag);
  put_u32(resp + 7, n);
  return 4 + 1 + 2 + 4;
}

static u32int handle_clunk(uchar *req, uchar *resp) {
  u16int tag = get_u16(req + 5);
  u32int fid = get_u32(req + 7);

  RumpFid *f = get_fid(fid);
  if (f)
    free_fid(f);

  put_u32(resp, 4 + 1 + 2);
  resp[4] = P9_Rclunk;
  put_u16(resp + 5, tag);
  return 4 + 1 + 2;
}

static void srv_loop(void) {
  exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  print_str("Lux9 Rump Server: 9P Loop Active\n");

  while (1) {
    if (ctl->doorbell) {
      uchar *req = (uchar *)exchange;
      u32int rsize = 0;
      uchar type = req[4];

      switch (type) {
      case P9_Tversion:
        rsize = handle_version(req, req);
        break;
      case P9_Tattach:
        rsize = handle_attach(req, req);
        break;
      case P9_Twalk:
        rsize = handle_walk(req, req);
        break;
      case P9_Topen:
        rsize = handle_open(req, req);
        break;
      case P9_Tread:
        rsize = handle_read(req, req);
        break;
      case P9_Twrite:
        rsize = handle_write(req, req);
        break;
      case P9_Tclunk:
        rsize = handle_clunk(req, req);
        break;
      default:
        rsize = mk_error(req, get_u16(req + 5), "unsupported op");
      }

      ctl->doorbell = 0;
    }
    /* Yield */
    for (volatile int i = 0; i < 5000; i++)
      ;
  }
}

int main(void) {
  int rv;

  print_str("Lux9 Rump Server: Starting...\n");

  /* Initialize Rump Kernel */
  rv = rump_init();
  if (rv != 0) {
    print_str("Lux9 Rump Server: rump_init failed!\n");
    return 1;
  }
  rump_init_done = 1;

  print_str("Lux9 Rump Server: rump_init success!\n");

  /* Create a directory to test VFS */
  // Struct stat dummy;
  // rv = rump_sys_mkdir("/test", 0777);
  // if (rv != 0) {
  //      print_str("Lux9 Rump Server: mkdir /test warning (exists?)\n");
  // }

  srv_loop();

  return 0;
}
