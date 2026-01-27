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
#include <sys/poll.h>

#define POSIX_FDSTAT_GET 1
#define POSIX_PATHSTAT_GET 1
#define POSIX_PREAD 1
#define POSIX_PWRITE 1
#define POSIX_READDIR 1
#define POSIX_SOCK_RECV 1
#define POSIX_SOCK_SEND 1
#define POSIX_POLL_ONEOFF 1
#define POSIX_SOCK_ACCEPT 1
#define POSIX_SOCK_SHUTDOWN 1
#define POSIX_PATH_CREATE_DIR 1
#define POSIX_PATH_REMOVE_DIR 1
#define POSIX_PATH_UNLINK 1
#define POSIX_PATH_RENAME 1
#define POSIX_PATH_SYMLINK 1
#define POSIX_PATH_READLINK 1
#define POSIX_PATH_SET_TIMES 1
#define POSIX_PATH_SET_SIZE 1
#define POSIX_PATH_LINK 1
#define POSIX_FD_SET_SIZE 2
#define POSIX_FD_SET_TIMES 3
#define POSIX_FD_SYNC 4
#define POSIX_FD_DATASYNC 5
#define POSIX_FD_TELL 6

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
typedef uint8_t u8int;

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

static u32int posix_write_path_simple_op(RumpFid *f, const uchar *data,
                                         u32int count, int op_code) {
  if (count < 8)
    return 0;

  u32int op = get_u32(data);
  u32int path_len = get_u32(data + 4);
  if (count < 8 + path_len)
    return 0;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != (u32int)op_code) {
    put_u32(f->resp, 38); /* ENOSYS */
    return f->resp_len;
  }

  char path[MAX_PATH];
  if (path_len >= sizeof(path)) {
    put_u32(f->resp, 36); /* ENAMETOOLONG */
    return f->resp_len;
  }
  memcpy(path, data + 8, path_len);
  path[path_len] = 0;

  int ret = -1;
  if (op_code == POSIX_PATH_CREATE_DIR) {
    ret = rump_sys_mkdir(path, 0777);
  } else if (op_code == POSIX_PATH_REMOVE_DIR) {
    ret = rump_sys_rmdir(path);
  } else if (op_code == POSIX_PATH_UNLINK) {
    ret = rump_sys_unlink(path);
  }

  if (ret < 0) {
    /* TODO: Get actual errno from rump */
    put_u32(f->resp, 5); /* EIO */
    return f->resp_len;
  }

  put_u32(f->resp, 0);
  f->resp_len = 4;
  return f->resp_len;
}

static u32int posix_write_path_rename(RumpFid *f, const uchar *data,
                                      u32int count) {
  if (count < 12)
    return 0;

  u32int op = get_u32(data);
  u32int old_len = get_u32(data + 4);
  if (count < 8 + old_len + 4)
    return 0;

  u32int new_len = get_u32(data + 8 + old_len);
  if (count < 12 + old_len + new_len)
    return 0;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_PATH_RENAME) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  char old_path[MAX_PATH];
  char new_path[MAX_PATH];

  if (old_len >= sizeof(old_path) || new_len >= sizeof(new_path)) {
    put_u32(f->resp, 36);
    return f->resp_len;
  }

  memcpy(old_path, data + 8, old_len);
  old_path[old_len] = 0;

  memcpy(new_path, data + 12 + old_len, new_len);
  new_path[new_len] = 0;

  if (rump_sys_rename(old_path, new_path) < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp, 0);
  f->resp_len = 4;
  return f->resp_len;
}

static u32int posix_write_sock_accept(RumpFid *f, const uchar *data,
                                      u32int count) {
  if (count < 12)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  /* u32int flags = get_u32(data+8); // unused for now */

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  put_u32(f->resp + 4, 0);
  f->resp_len = 8;

  if (op != POSIX_SOCK_ACCEPT) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  int newfd = rump_sys_accept(fd, NULL, NULL);
  if (newfd < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp + 4, (u32int)newfd);
  return f->resp_len;
}

static u32int posix_write_sock_recv(RumpFid *f, const uchar *data,
                                    u32int count) {
  if (count < 16)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u32int flags = get_u32(data + 8);
  u32int len = get_u32(data + 12);

  if (len > sizeof(f->resp) - 12)
    len = sizeof(f->resp) - 12;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);     /* err */
  put_u32(f->resp + 4, 0); /* ro_flags */
  put_u32(f->resp + 8, 0); /* count */
  f->resp_len = 12;

  if (op != POSIX_SOCK_RECV) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  ssize_t n = rump_sys_recvfrom(fd, f->resp + 12, len, (int)flags, NULL, NULL);
  if (n < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp + 8, (u32int)n);
  f->resp_len = 12 + (u32int)n;
  return f->resp_len;
}

static u32int posix_write_sock_send(RumpFid *f, const uchar *data,
                                    u32int count) {
  if (count < 16)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u32int flags = get_u32(data + 8);
  u32int len = get_u32(data + 12);

  if (count < 16 + len)
    return 0;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  put_u32(f->resp + 4, 0);
  f->resp_len = 8;

  if (op != POSIX_SOCK_SEND) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  ssize_t n = rump_sys_sendto(fd, data + 16, len, (int)flags, NULL, 0);
  if (n < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp + 4, (u32int)n);
  return f->resp_len;
}

static u32int posix_write_sock_shutdown(RumpFid *f, const uchar *data,
                                        u32int count) {
  if (count < 12)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u32int how = get_u32(data + 8);

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_SOCK_SHUTDOWN) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  if (rump_sys_shutdown(fd, (int)how) < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  return f->resp_len;
}

static u32int posix_write_path_symlink(RumpFid *f, const uchar *data,
                                       u32int count) {
  if (count < 12)
    return 0;

  u32int op = get_u32(data);
  u32int old_len = get_u32(data + 4);
  if (count < 8 + old_len + 4)
    return 0;

  u32int new_len = get_u32(data + 8 + old_len);
  if (count < 12 + old_len + new_len)
    return 0;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_PATH_SYMLINK) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  char old_path[MAX_PATH];
  char new_path[MAX_PATH];

  if (old_len >= sizeof(old_path) || new_len >= sizeof(new_path)) {
    put_u32(f->resp, 36);
    return f->resp_len;
  }

  memcpy(old_path, data + 8, old_len);
  old_path[old_len] = 0;

  memcpy(new_path, data + 12 + old_len, new_len);
  new_path[new_len] = 0;

  if (rump_sys_symlink(old_path, new_path) < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp, 0);
  f->resp_len = 4;
  return f->resp_len;
}

static u32int posix_write_path_readlink(RumpFid *f, const uchar *data,
                                        u32int count) {
  if (count < 12)
    return 0;

  u32int op = get_u32(data);
  u32int path_len = get_u32(data + 4);
  if (count < 8 + path_len + 4)
    return 0;

  /* extra param in shim is buffer size */
  u32int buf_len = get_u32(data + 8 + path_len);

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);     /* err */
  put_u32(f->resp + 4, 0); /* count */
  f->resp_len = 8;

  if (op != POSIX_PATH_READLINK) {
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

  if (buf_len > sizeof(f->resp) - 8)
    buf_len = sizeof(f->resp) - 8;

  ssize_t n = rump_sys_readlink(path, (char *)(f->resp + 8), buf_len);
  if (n < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  put_u32(f->resp + 4, (u32int)n);
  f->resp_len = 8 + (u32int)n;
  return f->resp_len;
}

static u32int posix_write_fd_sync(RumpFid *f, const uchar *data, u32int count) {
  if (count < 8)
    return 0;
  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_FD_SYNC) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  if (rump_sys_fsync(fd) < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

  return f->resp_len;
}

static u32int posix_write_fd_set_size(RumpFid *f, const uchar *data,
                                      u32int count) {
  if (count < 16)
    return 0;
  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u64int size = get_u64(data + 8);

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_FD_SET_SIZE) {
    put_u32(f->resp, 38);
    return f->resp_len;
  }

  if (rump_sys_ftruncate(fd, (off_t)size) < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }

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
  ssize_t n = rump_sys_getdents(fd, (char *)(f->resp + 8), len);
  if (n < 0) {
    put_u32(f->resp, 5);
    return f->resp_len;
  }
  put_u32(f->resp + 4, (u32int)n);
  f->resp_len = 8 + (u32int)n;
  return f->resp_len;
}

/* WASI fd_set_times - Set file access/modification times via file descriptor
 * Payload format:
 *   offset 0-3:   u32 operation (POSIX_FD_SET_TIMES = 3)
 *   offset 4-7:   u32 fd
 *   offset 8-15:  u64 atim (nanoseconds since epoch, or special values)
 *   offset 16-23: u64 mtim (nanoseconds since epoch, or special values)
 *   offset 24-27: u32 fst_flags (WASI flags for NOW/OMIT)
 * Response: u32 error code (0 = success)
 */
static u32int posix_write_fd_set_times(RumpFid *f, const uchar *data,
                                       u32int count) {
  if (count < 28)
    return 0;

  u32int op = get_u32(data);
  u32int fd = get_u32(data + 4);
  u64int atim = get_u64(data + 8);
  u64int mtim = get_u64(data + 16);
  u32int flags = get_u32(data + 24);

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0); /* error code */
  f->resp_len = 4;

  if (op != POSIX_FD_SET_TIMES) {
    put_u32(f->resp, 38); /* ENOSYS */
    return f->resp_len;
  }

  /* Convert WASI timestamps (nanoseconds) to struct timespec
   * WASI flags: bit 0 = atim_now, bit 1 = atim_omit, bit 2 = mtim_now, bit 3 =
   * mtim_omit */
  struct timespec times[2];

  /* Access time */
  if (flags & (1 << 1)) { /* ATIM_OMIT */
    times[0].tv_sec = 0;
    times[0].tv_nsec = 1073741823; /* UTIME_OMIT (special value) */
  } else if (flags & (1 << 0)) {   /* ATIM_NOW */
    times[0].tv_sec = 0;
    times[0].tv_nsec = 1073741822; /* UTIME_NOW (special value) */
  } else {
    times[0].tv_sec = (long)(atim / 1000000000ULL);
    times[0].tv_nsec = (long)(atim % 1000000000ULL);
  }

  /* Modification time */
  if (flags & (1 << 3)) { /* MTIM_OMIT */
    times[1].tv_sec = 0;
    times[1].tv_nsec = 1073741823; /* UTIME_OMIT */
  } else if (flags & (1 << 2)) {   /* MTIM_NOW */
    times[1].tv_sec = 0;
    times[1].tv_nsec = 1073741822; /* UTIME_NOW */
  } else {
    times[1].tv_sec = (long)(mtim / 1000000000ULL);
    times[1].tv_nsec = (long)(mtim % 1000000000ULL);
  }

  int ret = rump_sys_futimens(fd, times);
  if (ret < 0) {
    put_u32(f->resp, 5); /* EIO */
    return f->resp_len;
  }

  return f->resp_len;
}

/* WASI path_set_times - Set file access/modification times via path
 * Payload format:
 *   offset 0-3:    u32 operation (POSIX_PATH_SET_TIMES = 1)
 *   offset 4-7:    u32 dirfd
 *   offset 8-11:   u32 path_len
 *   offset 12-...: char path[path_len]
 *   After path:
 *   offset X+0-7:  u64 atim (nanoseconds)
 *   offset X+8-15: u64 mtim (nanoseconds)
 *   offset X+16-19: u32 fst_flags
 *   offset X+20-23: u32 lookup_flags (SYMLINK_FOLLOW)
 * Response: u32 error code
 */
static u32int posix_write_path_set_times(RumpFid *f, const uchar *data,
                                         u32int count) {
  if (count < 12)
    return 0;

  u32int op = get_u32(data);
  u32int dirfd = get_u32(data + 4);
  u32int path_len = get_u32(data + 8);

  if (count < 12 + path_len + 24 || path_len >= MAX_PATH)
    return 0;

  f->resp_off = 0;
  f->resp_len = 0;
  put_u32(f->resp, 0);
  f->resp_len = 4;

  if (op != POSIX_PATH_SET_TIMES) {
    put_u32(f->resp, 38); /* ENOSYS */
    return f->resp_len;
  }

  char path_buf[MAX_PATH];
  for (u32int i = 0; i < path_len; i++)
    path_buf[i] = data[12 + i];
  path_buf[path_len] = 0;

  const uchar *time_data = data + 12 + path_len;
  u64int atim = get_u64(time_data);
  u64int mtim = get_u64(time_data + 8);
  u32int flags = get_u32(time_data + 16);
  u32int lookup_flags = get_u32(time_data + 20);

  struct timespec times[2];

  /* Access time */
  if (flags & (1 << 1)) {
    times[0].tv_sec = 0;
    times[0].tv_nsec = 1073741823; /* UTIME_OMIT */
  } else if (flags & (1 << 0)) {
    times[0].tv_sec = 0;
    times[0].tv_nsec = 1073741822; /* UTIME_NOW */
  } else {
    times[0].tv_sec = (long)(atim / 1000000000ULL);
    times[0].tv_nsec = (long)(atim % 1000000000ULL);
  }

  /* Modification time */
  if (flags & (1 << 3)) {
    times[1].tv_sec = 0;
    times[1].tv_nsec = 1073741823; /* UTIME_OMIT */
  } else if (flags & (1 << 2)) {
    times[1].tv_sec = 0;
    times[1].tv_nsec = 1073741822; /* UTIME_NOW */
  } else {
    times[1].tv_sec = (long)(mtim / 1000000000ULL);
    times[1].tv_nsec = (long)(mtim % 1000000000ULL);
  }

  /* AT_SYMLINK_NOFOLLOW = 0x200 (don't follow symlinks) */
  int at_flags = (lookup_flags & 1) ? 0 : 0x200;

  int ret = rump_sys_utimensat(dirfd, path_buf, times, at_flags);
  if (ret < 0) {
    put_u32(f->resp, 5); /* EIO */
    return f->resp_len;
  }

  return f->resp_len;
}

/* WASI poll_oneoff structure definitions
 * Based on WASI Snapshot Preview 1 specification
 */
#define WASI_EVENTTYPE_CLOCK 0
#define WASI_EVENTTYPE_FD_READ 1
#define WASI_EVENTTYPE_FD_WRITE 2
#define WASI_EVENT_FD_READWRITE_HANGUP (1 << 0)

/* WASI subscription structure layout (48 bytes total)
 * Offset 0-7:   u64 userdata
 * Offset 8:     u8 type (CLOCK=0, FD_READ=1, FD_WRITE=2)
 * Offset 16-19: u32 fd (for FD_READ/FD_WRITE)
 * Offset 24-31: u64 timeout (for CLOCK)
 */

/* WASI event structure layout (32 bytes total)
 * Offset 0-7:   u64 userdata (echoed from subscription)
 * Offset 8-9:   u16 error (WASI errno)
 * Offset 10:    u8 type
 * Offset 16-17: u16 nbytes (for FD events)
 * Offset 18-19: u16 flags (HANGUP bit)
 */

static u32int posix_write_poll(RumpFid *f, const uchar *data, u32int count) {
  u32int nsubscriptions;
  u32int timeout_ms = -1; /* Infinite timeout by default */
  int have_timeout = 0;
  struct pollfd pfds[64]; /* Stack allocation, max 64 subscriptions */
  int nfds = 0;
  u64int userdata_map[64]; /* Track userdata for each pollfd */
  u8int type_map[64];      /* Track subscription type */
  int i;

  /* Initialize response */
  f->resp_off = 0;
  f->resp_len = 0;

  /* Parse request header */
  if (count < 4) {
    put_u32(f->resp, 28);    /* WASI_ERRNO_INVAL */
    put_u32(f->resp + 4, 0); /* nevents = 0 */
    f->resp_len = 8;
    return f->resp_len;
  }

  nsubscriptions = get_u32(data);
  if (nsubscriptions > 64 || count < 4 + (nsubscriptions * 48)) {
    put_u32(f->resp, 28); /* WASI_ERRNO_INVAL */
    put_u32(f->resp + 4, 0);
    f->resp_len = 8;
    return f->resp_len;
  }

  /* Parse subscriptions and convert to pollfd */
  for (i = 0; i < (int)nsubscriptions; i++) {
    const uchar *sub = data + 4 + (i * 48);
    u64int userdata = get_u64(sub);
    u8int type = sub[8];

    if (type == WASI_EVENTTYPE_CLOCK) {
      /* Extract timeout from clock subscription */
      u64int timeout_ns = get_u64(sub + 24);
      u32int timeout_candidate =
          (u32int)(timeout_ns / 1000000ULL); /* ns to ms */
      if (!have_timeout || timeout_candidate < timeout_ms) {
        timeout_ms = timeout_candidate;
        have_timeout = 1;
      }
      /* Don't add to pollfd array, just use for timeout */
      continue;
    }

    if (type == WASI_EVENTTYPE_FD_READ || type == WASI_EVENTTYPE_FD_WRITE) {
      u32int fd = get_u32(sub + 16);

      if (nfds >= 64) {
        /* Too many FD subscriptions */
        put_u32(f->resp, 28); /* WASI_ERRNO_INVAL */
        put_u32(f->resp + 4, 0);
        f->resp_len = 8;
        return f->resp_len;
      }

      /* Convert to pollfd */
      pfds[nfds].fd = (int)fd;
      pfds[nfds].events = 0;
      pfds[nfds].revents = 0;

      if (type == WASI_EVENTTYPE_FD_READ) {
        pfds[nfds].events = POLLIN | POLLRDNORM;
      } else {
        pfds[nfds].events = POLLOUT | POLLWRNORM;
      }

      userdata_map[nfds] = userdata;
      type_map[nfds] = type;
      nfds++;
    }
  }

  /* Call rump_sys_poll */
  int ready = 0;
  if (nfds > 0) {
    ready = rump_sys_poll(pfds, (unsigned int)nfds,
                          have_timeout ? (int)timeout_ms : -1);
    if (ready < 0) {
      /* Poll failed */
      put_u32(f->resp, 29); /* WASI_ERRNO_IO */
      put_u32(f->resp + 4, 0);
      f->resp_len = 8;
      return f->resp_len;
    }
  }

  /* Build response */
  put_u32(f->resp, 0); /* errno = SUCCESS */

  u32int nevents = 0;
  uchar *event_ptr = f->resp + 8;
  u32int max_events = (sizeof(f->resp) - 8) / 32;

  /* Convert pollfd results to WASI events */
  for (i = 0; i < nfds && nevents < max_events; i++) {
    if (pfds[i].revents == 0)
      continue; /* No events for this FD */

    /* Calculate event offset */
    uchar *evt = event_ptr + (nevents * 32);

    /* Write userdata */
    put_u64(evt, userdata_map[i]);

    /* Determine error code */
    u16int error = 0;
    if (pfds[i].revents & POLLNVAL) {
      error = 8; /* WASI_ERRNO_BADF */
    } else if (pfds[i].revents & POLLERR) {
      error = 29; /* WASI_ERRNO_IO */
    }
    put_u16(evt + 8, error);

    /* Write type */
    evt[10] = type_map[i];

    /* Write padding */
    memset(evt + 11, 0, 5);

    /* Write fd_readwrite union */
    u16int nbytes = 0;
    u16int flags = 0;

    /* Estimate bytes available (conservative: assume some data) */
    if (pfds[i].revents & (POLLIN | POLLRDNORM)) {
      nbytes = 1; /* At least 1 byte readable */
    } else if (pfds[i].revents & (POLLOUT | POLLWRNORM)) {
      nbytes = 4096; /* Assume writable space */
    }

    if (pfds[i].revents & POLLHUP) {
      flags |= WASI_EVENT_FD_READWRITE_HANGUP;
    }

    put_u16(evt + 16, nbytes);
    put_u16(evt + 18, flags);

    /* Zero remaining bytes */
    memset(evt + 20, 0, 12);

    nevents++;
  }

  /* Write nevents */
  put_u32(f->resp + 4, nevents);
  f->resp_len = 8 + (nevents * 32);

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

#define POSIX_FDSTAT_GET 1
#define POSIX_PATHSTAT_GET 1
#define POSIX_PREAD 1
#define POSIX_PWRITE 1
#define POSIX_READDIR 1
#define POSIX_SOCK_RECV 1
#define POSIX_SOCK_SEND 1
#define POSIX_POLL_ONEOFF 1
#define POSIX_SOCK_ACCEPT 1
#define POSIX_SOCK_SHUTDOWN 1
#define POSIX_PATH_CREATE_DIR 1
#define POSIX_PATH_REMOVE_DIR 1
#define POSIX_PATH_UNLINK 1
#define POSIX_PATH_RENAME 1
#define POSIX_PATH_SYMLINK 1
#define POSIX_PATH_READLINK 1
#define POSIX_PATH_SET_TIMES 1
#define POSIX_PATH_SET_SIZE 1
#define POSIX_PATH_LINK 1
#define POSIX_FD_SET_SIZE 2
#define POSIX_FD_SET_TIMES 3
#define POSIX_FD_SYNC 4
#define POSIX_FD_DATASYNC 5
#define POSIX_FD_TELL 6

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
    case V_POSIX_PATH_CREATE_DIR:
      out_len =
          posix_write_path_simple_op(f, data, count, POSIX_PATH_CREATE_DIR);
      break;
    case V_POSIX_PATH_REMOVE_DIR:
      out_len =
          posix_write_path_simple_op(f, data, count, POSIX_PATH_REMOVE_DIR);
      break;
    case V_POSIX_PATH_UNLINK:
      out_len = posix_write_path_simple_op(f, data, count, POSIX_PATH_UNLINK);
      break;
    case V_POSIX_PATH_RENAME:
      out_len = posix_write_path_rename(f, data, count);
      break;
    case V_POSIX_SOCK_ACCEPT:
      out_len = posix_write_sock_accept(f, data, count);
      break;
    case V_POSIX_SOCK_RECV:
      out_len = posix_write_sock_recv(f, data, count);
      break;
    case V_POSIX_SOCK_SEND:
      out_len = posix_write_sock_send(f, data, count);
      break;
    case V_POSIX_SOCK_SHUTDOWN:
      out_len = posix_write_sock_shutdown(f, data, count);
      break;
    case V_POSIX_PATH_SYMLINK:
      out_len = posix_write_path_symlink(f, data, count);
      break;
    case V_POSIX_PATH_READLINK:
      out_len = posix_write_path_readlink(f, data, count);
      break;
    case V_POSIX_FD_SYNC:
      out_len = posix_write_fd_sync(f, data, count);
      break;
    case V_POSIX_FD_SET_SIZE:
      out_len = posix_write_fd_set_size(f, data, count);
      break;
    case V_POSIX_FD_TELL:
      out_len = posix_write_stub(f);
      break;
    case V_POSIX_FD_SET_TIMES:
      out_len = posix_write_fd_set_times(f, data, count);
      break;
    case V_POSIX_PATH_SET_TIMES:
      out_len = posix_write_path_set_times(f, data, count);
      break;
    case V_POSIX_POLL:
      out_len = posix_write_poll(f, data, count);
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
  exchange = (volatile uchar *)exchange_base();
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
