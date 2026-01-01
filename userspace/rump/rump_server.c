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
  int is_virtual;
  u32int vtype;
  u32int resp_len;
  u32int resp_off;
  uchar resp[8192];
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

enum {
  V_NONE = 0,
  V_POSIX_DIR,
  V_POSIX_FDSTAT,
  V_POSIX_PATHSTAT,
  V_POSIX_PREAD,
  V_POSIX_PWRITE,
  V_POSIX_READDIR,
  V_POSIX_FD_SYNC,
  V_POSIX_FD_TELL,
  V_POSIX_FD_SET_SIZE,
  V_POSIX_FD_SET_TIMES,
  V_POSIX_PATH_SET_TIMES,
  V_POSIX_PATH_CREATE_DIR,
  V_POSIX_PATH_REMOVE_DIR,
  V_POSIX_PATH_UNLINK,
  V_POSIX_PATH_RENAME,
  V_POSIX_PATH_SYMLINK,
  V_POSIX_PATH_READLINK,
  V_POSIX_POLL,
  V_POSIX_SOCK_ACCEPT,
  V_POSIX_SOCK_RECV,
  V_POSIX_SOCK_SEND,
  V_POSIX_SOCK_SHUTDOWN,
};

enum {
  POSIX_FDSTAT_GET = 1,
  POSIX_PATHSTAT_GET = 1,
  POSIX_PREAD = 1,
  POSIX_PWRITE = 1,
  POSIX_READDIR = 1,
};

#define POSIX_ENOSYS 78

static void fid_set_virtual(RumpFid *f, u32int vtype) {
  f->is_virtual = 1;
  f->vtype = vtype;
  f->rfd = -1;
  f->opened = 0;
  f->resp_len = 0;
  f->resp_off = 0;
  f->qid.type = (vtype == V_POSIX_DIR) ? QTDIR : QTFILE;
  f->qid.vers = 0;
  f->qid.path = 0x50000000 | vtype;
}

static int virtual_lookup(RumpFid *f, const char *name) {
  if (f->is_virtual) {
    if (f->vtype != V_POSIX_DIR)
      return -1;
  } else {
    if (strcmp(f->path, "/") == 0 && strcmp(name, "posix") == 0) {
      fid_set_virtual(f, V_POSIX_DIR);
      return 0;
    }
    return -1;
  }

  if (strcmp(name, "fdstat") == 0) {
    fid_set_virtual(f, V_POSIX_FDSTAT);
    return 0;
  }
  if (strcmp(name, "pathstat") == 0) {
    fid_set_virtual(f, V_POSIX_PATHSTAT);
    return 0;
  }
  if (strcmp(name, "pread") == 0) {
    fid_set_virtual(f, V_POSIX_PREAD);
    return 0;
  }
  if (strcmp(name, "pwrite") == 0) {
    fid_set_virtual(f, V_POSIX_PWRITE);
    return 0;
  }
  if (strcmp(name, "readdir") == 0) {
    fid_set_virtual(f, V_POSIX_READDIR);
    return 0;
  }
  if (strcmp(name, "fd_sync") == 0) {
    fid_set_virtual(f, V_POSIX_FD_SYNC);
    return 0;
  }
  if (strcmp(name, "fd_tell") == 0) {
    fid_set_virtual(f, V_POSIX_FD_TELL);
    return 0;
  }
  if (strcmp(name, "fd_set_size") == 0) {
    fid_set_virtual(f, V_POSIX_FD_SET_SIZE);
    return 0;
  }
  if (strcmp(name, "fd_set_times") == 0) {
    fid_set_virtual(f, V_POSIX_FD_SET_TIMES);
    return 0;
  }
  if (strcmp(name, "path_set_times") == 0) {
    fid_set_virtual(f, V_POSIX_PATH_SET_TIMES);
    return 0;
  }
  if (strcmp(name, "path_create_directory") == 0) {
    fid_set_virtual(f, V_POSIX_PATH_CREATE_DIR);
    return 0;
  }
  if (strcmp(name, "path_remove_directory") == 0) {
    fid_set_virtual(f, V_POSIX_PATH_REMOVE_DIR);
    return 0;
  }
  if (strcmp(name, "path_unlink_file") == 0) {
    fid_set_virtual(f, V_POSIX_PATH_UNLINK);
    return 0;
  }
  if (strcmp(name, "path_rename") == 0) {
    fid_set_virtual(f, V_POSIX_PATH_RENAME);
    return 0;
  }
  if (strcmp(name, "path_symlink") == 0) {
    fid_set_virtual(f, V_POSIX_PATH_SYMLINK);
    return 0;
  }
  if (strcmp(name, "path_readlink") == 0) {
    fid_set_virtual(f, V_POSIX_PATH_READLINK);
    return 0;
  }
  if (strcmp(name, "poll_oneoff") == 0) {
    fid_set_virtual(f, V_POSIX_POLL);
    return 0;
  }
  if (strcmp(name, "sock_accept") == 0) {
    fid_set_virtual(f, V_POSIX_SOCK_ACCEPT);
    return 0;
  }
  if (strcmp(name, "sock_recv") == 0) {
    fid_set_virtual(f, V_POSIX_SOCK_RECV);
    return 0;
  }
  if (strcmp(name, "sock_send") == 0) {
    fid_set_virtual(f, V_POSIX_SOCK_SEND);
    return 0;
  }
  if (strcmp(name, "sock_shutdown") == 0) {
    fid_set_virtual(f, V_POSIX_SOCK_SHUTDOWN);
    return 0;
  }

  return -1;
}

static u32int posix_write_fdstat(RumpFid *f, const uchar *data, u32int count) {
  if (count < 8)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_FDSTAT_GET) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  struct rump_stat st;
  if (rump_sys_fstat(fd, (struct stat *)&st) < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp, 0);
  memcpy(f->resp + 4, &st, sizeof(st));
  f->resp_len = 4 + sizeof(st);
  return f->resp_len;
}

static u32int posix_write_pathstat(RumpFid *f, const uchar *data,
                                   u32int count) {
  if (count < 12)
    return 0;

  u32int op = get_u32(data);
  u32int path_len = get_u32(data + 4);
  if (count < 8 + path_len)
    return 0;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_PATHSTAT_GET) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  char path[MAX_PATH];
  if (path_len >= sizeof(path)) {
    put_u32(f->resp, 36);
    return f->resp_len;
  }
  memcpy(path, data + 8, path_len);
  path[path_len] = 0;

  struct rump_stat st;
  if (rump_sys_stat(path, (struct stat *)&st) < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp, 0);
  memcpy(f->resp + 4, &st, sizeof(st));
  f->resp_len = 4 + sizeof(st);
  return f->resp_len;
}

static u32int posix_write_pread(RumpFid *f, const uchar *data, u32int count) {
  if (count < 20)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u64int offset = get_u64(data + 8);
  u32int len = get_u32(data + 16);
  if (len > sizeof(f->resp) - 8)
    len = sizeof(f->resp) - 8;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  put_u32(f->resp + 4, 0);
  f->resp_len = 8;

  if (op != POSIX_PREAD) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  rump_sys_lseek(fd, offset, 0);
  ssize_t n = rump_sys_read(fd, f->resp + 8, len);
  if (n < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }
  put_u32(f->resp + 4, (u32int)n);
  f->resp_len = 8 + (u32int)n;
  return f->resp_len;
}

static u32int posix_write_pwrite(RumpFid *f, const uchar *data, u32int count) {
  if (count < 20)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u64int offset = get_u64(data + 8);
  u32int len = get_u32(data + 16);
  if (count < 20 + len)
    return 0;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  put_u32(f->resp + 4, 0);
  f->resp_len = 8;

  if (op != POSIX_PWRITE) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  rump_sys_lseek(fd, offset, 0);
  ssize_t n = rump_sys_write(fd, data + 20, len);
  if (n < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }
  put_u32(f->resp + 4, (u32int)n);
  f->resp_len = 8;
  return f->resp_len;
}

static u32int posix_write_readdir(RumpFid *f, const uchar *data, u32int count) {
  if (count < 20)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u64int offset = get_u64(data + 8);
  u32int len = get_u32(data + 16);
  if (len > sizeof(f->resp) - 8)
    len = sizeof(f->resp) - 8;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  put_u32(f->resp + 4, 0);
  f->resp_len = 8;

  if (op != POSIX_READDIR) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  rump_sys_lseek(fd, offset, 0);
  ssize_t n = rump_sys_getdents(fd, f->resp + 8, len);
  if (n < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }
  put_u32(f->resp + 4, (u32int)n);
  f->resp_len = 8 + (u32int)n;
  return f->resp_len;
}

static u32int posix_write_stub(RumpFid *f) {
  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, POSIX_ENOSYS);
  f->resp_len = 4;
  return f->resp_len;
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
      fids[i].is_virtual = 0;
      fids[i].vtype = V_NONE;
      fids[i].resp_len = 0;
      fids[i].resp_off = 0;
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
    f->is_virtual = 0;
    f->vtype = V_NONE;
    f->resp_len = 0;
    f->resp_off = 0;
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
    newf->is_virtual = oldf->is_virtual;
    newf->vtype = oldf->vtype;
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
      if (newf->is_virtual) {
        if (newf->vtype == V_POSIX_DIR) {
          newf->is_virtual = 0;
          newf->vtype = V_NONE;
          strcpy(newf->path, "/");
          /* Stat root */
          struct rump_stat st;
          if (rump_sys_stat(newf->path, (struct stat *)&st) < 0)
            return mk_error(resp, tag, "not found");
          newf->qid.type = (st.st_mode & RUMP_S_IFDIR) ? QTDIR : QTFILE;
          newf->qid.vers = 0;
          newf->qid.path = st.st_ino;
        }
        qids[nwqid++] = newf->qid;
        continue;
      }
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
      if (virtual_lookup(newf, name) == 0) {
        qids[nwqid++] = newf->qid;
        continue;
      }
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

  if (f->is_virtual) {
    f->opened = 1;
    put_u32(resp, 4 + 1 + 2 + 13 + 4);
    resp[4] = P9_Ropen;
    put_u16(resp + 5, tag);
    mk_qid(resp + 7, &f->qid);
    put_u32(resp + 20, 8192);
    return 4 + 1 + 2 + 13 + 4;
  }

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

  if (f->is_virtual) {
    if (f->vtype == V_POSIX_DIR) {
      put_u32(resp, 4 + 1 + 2 + 4);
      resp[4] = P9_Rread;
      put_u16(resp + 5, tag);
      put_u32(resp + 7, 0);
      return 4 + 1 + 2 + 4;
    }
    if (f->resp_len == 0 || f->resp_off >= f->resp_len) {
      put_u32(resp, 4 + 1 + 2 + 4);
      resp[4] = P9_Rread;
      put_u16(resp + 5, tag);
      put_u32(resp + 7, 0);
      return 4 + 1 + 2 + 4;
    }

    u32int avail = f->resp_len - f->resp_off;
    if (count < avail)
      avail = count;
    memcpy(resp + 11, f->resp + f->resp_off, avail);
    f->resp_off += avail;

    put_u32(resp, 4 + 1 + 2 + 4 + avail);
    resp[4] = P9_Rread;
    put_u16(resp + 5, tag);
    put_u32(resp + 7, avail);
    return 4 + 1 + 2 + 4 + avail;
  }

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

  if (f->is_virtual) {
    const uchar *data = req + 23;
    u32int out_len = 0;
    switch (f->vtype) {
    case V_POSIX_FDSTAT:
      out_len = posix_write_fdstat(f, data, count);
      break;
    case V_POSIX_PATHSTAT:
      out_len = posix_write_pathstat(f, data, count);
      break;
    case V_POSIX_PREAD:
      out_len = posix_write_pread(f, data, count);
      break;
    case V_POSIX_PWRITE:
      out_len = posix_write_pwrite(f, data, count);
      break;
    case V_POSIX_READDIR:
      out_len = posix_write_readdir(f, data, count);
      break;
    case V_POSIX_FD_SYNC:
    case V_POSIX_FD_TELL:
    case V_POSIX_FD_SET_SIZE:
    case V_POSIX_FD_SET_TIMES:
    case V_POSIX_PATH_SET_TIMES:
    case V_POSIX_PATH_CREATE_DIR:
    case V_POSIX_PATH_REMOVE_DIR:
    case V_POSIX_PATH_UNLINK:
    case V_POSIX_PATH_RENAME:
    case V_POSIX_PATH_SYMLINK:
    case V_POSIX_PATH_READLINK:
    case V_POSIX_POLL:
    case V_POSIX_SOCK_ACCEPT:
    case V_POSIX_SOCK_RECV:
    case V_POSIX_SOCK_SEND:
    case V_POSIX_SOCK_SHUTDOWN:
      out_len = posix_write_stub(f);
      break;
    default:
      out_len = 0;
      break;
    }

    if (out_len == 0) {
      return mk_error(resp, tag, "bad request");
    }

    put_u32(resp, 4 + 1 + 2 + 4);
    resp[4] = P9_Rwrite;
    put_u16(resp + 5, tag);
    put_u32(resp + 7, count);
    return 4 + 1 + 2 + 4;
  }

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
