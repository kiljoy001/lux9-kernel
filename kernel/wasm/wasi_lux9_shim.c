/* wasi_lux9_shim.c - WASI to Lux9 9P Shim Implementation
 *
 * Implements wasi_snapshot_preview1 host functions by mapping them
 * to Lux9 9P file operations.
 */

#include "wasi_lux9_shim.h"
#include "../include/dat.h"
#include "../include/fcall.h"

extern uint convD2M(Dir *d, uchar *buf, uint n);

#include "../include/fns.h"
#include "../include/portlib.h"
#include "../include/u.h"
#include "wasm_runtime.h"

#ifndef nil
#define nil ((void *)0)
#endif

extern vlong nsec(void);

/* Provide standard types if missing */
/* Standard types provided by wasi_lux9_shim.h */

/* WASI is Little Endian usually.
 * wasm3.h defines m3ApiReadMem32 which handles endianness if configured.
 * We rely on wasm3 macros.
 */

/* ========== WASI Constants ========== */
#define WASI_ERRNO_SUCCESS 0
#define WASI_ERRNO_PERM 63
#define WASI_ERRNO_ACCES 2
#define WASI_ERRNO_BADF 8
#define WASI_ERRNO_EXIST 20
#define WASI_ERRNO_ISDIR 31
#define WASI_ERRNO_NOMEM 48
#define WASI_ERRNO_INVAL 28
#define WASI_ERRNO_IO 29
#define WASI_ERRNO_NOENT 44
#define WASI_ERRNO_NOTDIR 54
#define WASI_ERRNO_NAMETOOLONG 37
#define WASI_ERRNO_NOTCAPABLE 76
#define WASI_ERRNO_MFILE 33
#define WASI_ERRNO_NFILE 41
#define WASI_ERRNO_NOSPC 51
#define WASI_ERRNO_NOTEMPTY 55
#define WASI_ERRNO_ROFS 69
#define WASI_ERRNO_PIPE 64
#define WASI_ERRNO_AGAIN 6
#define WASI_ERRNO_INTR 27
#define WASI_ERRNO_NOTSUP 58
#define WASI_ERRNO_NOSYS 52

#define WASI_FILETYPE_UNKNOWN 0
#define WASI_FILETYPE_BLOCK_DEVICE 1
#define WASI_FILETYPE_CHARACTER_DEVICE 2
#define WASI_FILETYPE_DIRECTORY 3
#define WASI_FILETYPE_REGULAR_FILE 4
#define WASI_FILETYPE_SOCKET_DGRAM 5
#define WASI_FILETYPE_SOCKET_STREAM 6
#define WASI_FILETYPE_SYMBOLIC_LINK 7

#define WASI_O_CREAT 1
#define WASI_O_DIRECTORY 2
#define WASI_O_EXCL 4
#define WASI_O_TRUNC 8

#define WASI_RIGHT_FD_DATASYNC (1ULL << 0)
#define WASI_RIGHT_FD_READ (1ULL << 1)
#define WASI_RIGHT_FD_SEEK (1ULL << 2)
#define WASI_RIGHT_FD_FDSTAT_SET_FLAGS (1ULL << 3)
#define WASI_RIGHT_FD_SYNC (1ULL << 4)
#define WASI_RIGHT_FD_TELL (1ULL << 5)
#define WASI_RIGHT_FD_WRITE (1ULL << 6)
#define WASI_RIGHT_FD_ADVISE (1ULL << 7)
#define WASI_RIGHT_FD_ALLOCATE (1ULL << 8)
#define WASI_RIGHT_PATH_CREATE_DIRECTORY (1ULL << 9)
#define WASI_RIGHT_PATH_CREATE_FILE (1ULL << 10)
#define WASI_RIGHT_PATH_LINK_SOURCE (1ULL << 11)
#define WASI_RIGHT_PATH_LINK_TARGET (1ULL << 12)
#define WASI_RIGHT_PATH_OPEN (1ULL << 13)
#define WASI_RIGHT_FD_READDIR (1ULL << 14)
#define WASI_RIGHT_PATH_READLINK (1ULL << 15)
#define WASI_RIGHT_PATH_RENAME_SOURCE (1ULL << 16)
#define WASI_RIGHT_PATH_RENAME_TARGET (1ULL << 17)
#define WASI_RIGHT_PATH_FILESTAT_GET (1ULL << 18)
#define WASI_RIGHT_PATH_FILESTAT_SET_SIZE (1ULL << 19)
#define WASI_RIGHT_PATH_FILESTAT_SET_TIMES (1ULL << 20)
#define WASI_RIGHT_FD_FILESTAT_GET (1ULL << 21)
#define WASI_RIGHT_FD_FILESTAT_SET_SIZE (1ULL << 22)
#define WASI_RIGHT_FD_FILESTAT_SET_TIMES (1ULL << 23)
#define WASI_RIGHT_PATH_SYMLINK (1ULL << 24)
#define WASI_RIGHT_PATH_REMOVE_DIRECTORY (1ULL << 25)
#define WASI_RIGHT_PATH_UNLINK_FILE (1ULL << 26)
#define WASI_RIGHT_POLL_FD_READWRITE (1ULL << 27)
#define WASI_RIGHT_SOCK_SHUTDOWN (1ULL << 28)
#define WASI_RIGHT_SOCK_ACCEPT (1ULL << 29)

#define WASI_RIGHTS_ALL ((uint64_t)-1)

#define WASI_FILESTAT_SIZE 64

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
#define POSIX_PATH_CREATE_DIRECTORY 1
#define POSIX_PATH_REMOVE_DIRECTORY 1
#define POSIX_PATH_UNLINK_FILE 1
#define POSIX_PATH_RENAME 1
#define POSIX_PATH_SYMLINK 1
#define POSIX_PATH_READLINK 1
#define POSIX_PATH_SET_TIMES 1
#define POSIX_PATH_SET_SIZE 1
#define POSIX_PATH_LINK 1
#define POSIX_FD_SET_SIZE 2
#define POSIX_FD_SET_TIMES 3

#define WASI_RUMP_IO_MAX 8000
#define WASI_SUBSCRIPTION_SIZE 48
#define WASI_EVENT_SIZE 32

#define RUMP_S_IFMT 0170000
#define RUMP_S_IFDIR 0040000
#define RUMP_S_IFREG 0100000

#define WASI_BACKEND_NATIVE 0
#define WASI_BACKEND_POSIX 1

static const char wasi_root_path[] = "/wasm";
static const char wasi_posix_root_path[] = "/wasm/posix";

/* ========== WASI Types ========== */
typedef struct wasi_iovec_t {
  uint32_t buf;
  uint32_t buf_len;
} wasi_iovec_t;

// wasi_fd_entry_t defined in header

typedef struct wasi_fdstat_t {
  uint8_t fs_filetype;
  uint16_t fs_flags;
  uint64_t fs_rights_base;
  uint64_t fs_rights_inheriting;
} wasi_fdstat_t;

typedef struct wasi_prestat_t {
  uint8_t tag;
  uint8_t pad[3];
  uint32_t name_len;
} wasi_prestat_t;

typedef struct rump_timespec {
  int64_t tv_sec;
  long tv_nsec;
} rump_timespec;

typedef struct rump_stat {
  uint64_t st_dev;
  uint32_t st_mode;
  uint32_t _pad0;
  uint64_t st_ino;
  uint32_t st_nlink;
  uint32_t st_uid;
  uint32_t st_gid;
  uint32_t _pad1;
  uint64_t st_rdev;
  struct rump_timespec st_atimespec;
  struct rump_timespec st_mtimespec;
  struct rump_timespec st_ctimespec;
  struct rump_timespec st_birthtimespec;
  int64_t st_size;
  int64_t st_blocks;
  int32_t st_blksize;
  uint32_t st_flags;
  uint32_t st_gen;
  uint32_t st_spare[2];
} rump_stat;

static int wasi_rump_rpc(const char *path, const void *req, uint32_t req_len,
                         void *resp, uint32_t resp_len) {
  int fd = kopen((char *)path, ORDWR);
  if (fd < 0)
    return -1;
  long w = kwrite(fd, (void *)req, req_len);
  if (w < 0) {
    fdclose(fd, 0);
    return -1;
  }
  if (resp && resp_len > 0) {
    long r = kread(fd, resp, resp_len);
    fdclose(fd, 0);
    if (r < 0)
      return -1;
    return (int)r;
  }
  fdclose(fd, 0);
  return 0;
}

static uint32_t wasi_errno_from_posix(uint32_t err) {
  switch (err) {
  case 0:
    return WASI_ERRNO_SUCCESS;
  case 1:
    return WASI_ERRNO_PERM;
  case 2:
    return WASI_ERRNO_NOENT;
  case 4:
    return WASI_ERRNO_INTR;
  case 5:
    return WASI_ERRNO_IO;
  case 11:
    return WASI_ERRNO_AGAIN;
  case 13:
    return WASI_ERRNO_ACCES;
  case 17:
    return WASI_ERRNO_EXIST;
  case 20:
    return WASI_ERRNO_NOTDIR;
  case 21:
    return WASI_ERRNO_ISDIR;
  case 9:
    return WASI_ERRNO_BADF;
  case 12:
    return WASI_ERRNO_NOMEM;
  case 22:
    return WASI_ERRNO_INVAL;
  case 23:
    return WASI_ERRNO_NFILE;
  case 24:
    return WASI_ERRNO_MFILE;
  case 28:
    return WASI_ERRNO_NOSPC;
  case 30:
    return WASI_ERRNO_ROFS;
  case 32:
    return WASI_ERRNO_PIPE;
  case 36:
    return WASI_ERRNO_NAMETOOLONG;
  case 38:
  case 78:
    return WASI_ERRNO_NOSYS;
  case 39:
  case 66:
    return WASI_ERRNO_NOTEMPTY;
  case 35:
    return WASI_ERRNO_AGAIN;
  case 95:
    return WASI_ERRNO_NOTSUP;
  default:
    return WASI_ERRNO_IO;
  }
}

static void wasi_write_le32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

static void wasi_write_le64(uint8_t *p, uint64_t v) {
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
  p[4] = (uint8_t)((v >> 32) & 0xff);
  p[5] = (uint8_t)((v >> 40) & 0xff);
  p[6] = (uint8_t)((v >> 48) & 0xff);
  p[7] = (uint8_t)((v >> 56) & 0xff);
}

static uint32_t wasi_rump_simple_errno(const char *path) {
  uint32_t err = 0;
  int r = wasi_rump_rpc(path, nil, 0, &err, sizeof(err));
  if (r < (int)sizeof(err))
    return WASI_ERRNO_NOSYS;
  return wasi_errno_from_posix(err);
}

static int wasi_rump_rpc_read(const char *path, const void *req,
                              uint32_t req_len, void *resp, uint32_t resp_len) {
  int fd = kopen((char *)path, ORDWR);
  if (fd < 0)
    return -1;
  if (req_len > 0) {
    long w = kwrite(fd, (void *)req, req_len);
    if (w < 0) {
      fdclose(fd, 0);
      return -1;
    }
  }
  if (resp && resp_len > 0) {
    long r = kread(fd, resp, resp_len);
    fdclose(fd, 0);
    if (r < 0)
      return -1;
    return (int)r;
  }
  fdclose(fd, 0);
  return 0;
}

static uint8_t wasi_filetype_from_mode(uint32_t mode) {
  switch (mode & RUMP_S_IFMT) {
  case RUMP_S_IFDIR:
    return WASI_FILETYPE_DIRECTORY;
  case RUMP_S_IFREG:
    return WASI_FILETYPE_REGULAR_FILE;
  default:
    return WASI_FILETYPE_UNKNOWN;
  }
}

static uint64_t wasi_timespec_to_ns(const struct rump_timespec *ts) {
  uint64_t sec = (uint64_t)ts->tv_sec;
  uint64_t nsec = (uint64_t)ts->tv_nsec;
  if (sec > ((uint64_t)-1 / 1000000000ULL))
    return (uint64_t)-1;
  return sec * 1000000000ULL + nsec;
}

static uint32_t wasi_require_fd(wasi_context_t *ctx, int fd, uint64_t rights) {
  if (fd < 0 || fd >= WASI_MAX_FDS || !ctx->fds[fd].is_open)
    return WASI_ERRNO_BADF;
  if ((ctx->fds[fd].rights & rights) != rights)
    return WASI_ERRNO_NOTCAPABLE;
  return WASI_ERRNO_SUCCESS;
}

static int wasi_iovecs_size(uint32_t count, uint32_t *out_size) {
  if (count > (0xffffffffu / 8)) {
    return -1;
  }
  *out_size = count * 8;
  return 0;
}

static int wasi_fd_backend(wasi_context_t *ctx, int fd) {
  if (!ctx || fd < 0 || fd >= WASI_MAX_FDS)
    return WASI_BACKEND_NATIVE;
  return ctx->fds[fd].backend;
}

static uint32_t wasi_dirents_from_plan9(Proc *p, int fd, uint8_t *out,
                                        uint32_t out_len, uint64_t cookie,
                                        uint32_t *out_used) {
  uint8_t *raw = malloc(out_len);
  if (!raw)
    return WASI_ERRNO_NOMEM;

  if (cookie > 0) {
    if (kseek(fd, (vlong)cookie, 0) < 0) {
      free(raw);
      return WASI_ERRNO_IO;
    }
  }

  long n = kread(fd, raw, out_len);
  if (n < 0) {
    free(raw);
    return WASI_ERRNO_IO;
  }

  uint32_t used = 0;
  uint32_t off = 0;
  while (off + 2 <= (uint32_t)n) {
    uint16_t ent_len = GBIT16(raw + off);
    if (ent_len < 2 || off + ent_len > (uint32_t)n)
      break;

    Dir d;
    char *strs = malloc(ent_len + 1);
    if (!strs) {
      free(raw);
      return WASI_ERRNO_NOMEM;
    }

    uint32_t conv = convM2D(raw + off, ent_len, &d, strs);
    if (conv == 0) {
      free(strs);
      break;
    }

    uint32_t name_len = (uint32_t)strlen(d.name);
    uint32_t need = 24 + name_len;
    if (used + need > out_len) {
      free(strs);
      break;
    }

    uint64_t next = cookie + off + ent_len;
    wasi_write_le64(out + used + 0, next);
    wasi_write_le64(out + used + 8, d.qid.path);
    wasi_write_le32(out + used + 16, name_len);
    if (d.mode & DMDIR) {
      out[used + 20] = WASI_FILETYPE_DIRECTORY;
    } else {
      out[used + 20] = WASI_FILETYPE_REGULAR_FILE;
    }
    out[used + 21] = 0;
    out[used + 22] = 0;
    out[used + 23] = 0;
    memmove(out + used + 24, d.name, name_len);
    used += need;

    free(strs);
    off += ent_len;
  }

  free(raw);
  *out_used = used;
  return WASI_ERRNO_SUCCESS;
}

static int wasi_is_posix_path(const char *path, uint32_t path_len) {
  if (path_len < sizeof(wasi_posix_root_path) - 1)
    return 0;
  return memcmp(path, wasi_posix_root_path, sizeof(wasi_posix_root_path) - 1) ==
         0;
}

static int wasi_path_safe(const char *path, uint32_t path_len) {
  uint32_t i = 0;
  while (i < path_len) {
    while (i < path_len && path[i] == '/')
      i++;
    uint32_t start = i;
    while (i < path_len && path[i] != '/')
      i++;
    uint32_t seg_len = i - start;
    if (seg_len == 2 && path[start] == '.' && path[start + 1] == '.')
      return 0;
  }
  return 1;
}

static void wasi_fill_random(uint8_t *buf, uint32_t len) {
  u64int state = (u64int)fastticks(nil) ^ ((u64int)up->pid << 32) ^ len;
  for (uint32_t i = 0; i < len; i++) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    buf[i] = (uint8_t)state;
  }
}

static const char *wasi_posix_path(const char *full, uint32_t *out_len) {
  static const char posix_root[] = "/";

  if (wasi_is_posix_path(full, (uint32_t)strlen(full))) {
    const char *p = full + (sizeof(wasi_posix_root_path) - 1);
    if (*p == '\0') {
      *out_len = 1;
      return posix_root;
    }
    if (*p != '/') {
      *out_len = (uint32_t)strlen(p);
      return p;
    }
    *out_len = (uint32_t)strlen(p);
    return p;
  }
  *out_len = (uint32_t)strlen(full);
  return full;
}

static uint32_t wasi_build_path(wasi_context_t *ctx, int dirfd,
                                const char *path, uint32_t path_len,
                                char **out_path) {
  char *full = nil;
  size_t total = 0;
  int absolute = 0;

  if (path_len == 0) {
    return WASI_ERRNO_INVAL;
  }
  if (!wasi_path_safe(path, path_len)) {
    return WASI_ERRNO_NOTCAPABLE;
  }
  if (path[0] == '/' || path[0] == '#') {
    absolute = 1;
  }

  if (absolute) {
    if (path_len < 5 || memcmp(path, "/wasm", 5) != 0)
      return WASI_ERRNO_NOTCAPABLE;
    if (wasi_is_posix_path(path, path_len) && !ctx->fds[4].is_open)
      return WASI_ERRNO_NOTCAPABLE;
    total = (size_t)path_len + 1;
    full = malloc(total);
    if (!full) {
      return WASI_ERRNO_NOMEM;
    }
    memmove(full, path, path_len);
    full[path_len] = '\0';
    *out_path = full;
    return WASI_ERRNO_SUCCESS;
  }

  const char *dirpath = ctx->fds[dirfd].base_path;
  Chan *c = nil;
  if (dirpath == nil) {
    if (waserror()) {
      if (c) {
        cclose(c);
      }
      free(full);
      return WASI_ERRNO_BADF;
    }
    c = fdtochan(ctx->fds[dirfd].lux9_fid, -1, 0, 1);
    dirpath = chanpath(c);
  }
  size_t dirlen = strlen(dirpath);
  int need_sep = (dirlen > 0 && dirpath[dirlen - 1] != '/');

  total = dirlen + (need_sep ? 1 : 0) + path_len + 1;
  full = malloc(total);
  if (!full) {
    cclose(c);
    poperror();
    return WASI_ERRNO_NOMEM;
  }
  memmove(full, dirpath, dirlen);
  if (need_sep) {
    full[dirlen] = '/';
  }
  memmove(full + dirlen + (need_sep ? 1 : 0), path, path_len);
  full[dirlen + (need_sep ? 1 : 0) + path_len] = '\0';
  if (c) {
    cclose(c);
    poperror();
  }

  *out_path = full;
  return WASI_ERRNO_SUCCESS;
}

static int wasi_rump_path_req(const char *endpoint, uint32_t op,
                              const char *path, uint32_t path_len,
                              const void *extra, uint32_t extra_len, void *resp,
                              uint32_t resp_len) {
  uint32_t req_size = 8 + path_len + extra_len;
  uint8_t *req = malloc(req_size);
  if (!req)
    return -1;

  *(uint32_t *)(req + 0) = op;
  *(uint32_t *)(req + 4) = path_len;
  memmove(req + 8, path, path_len);
  if (extra_len > 0) {
    memmove(req + 8 + path_len, extra, extra_len);
  }

  int r = wasi_rump_rpc_read(endpoint, req, req_size, resp, resp_len);
  free(req);
  return r;
}

/* ========== Context Management ========== */

static void wasi_ctx_free_argv(wasi_context_t *ctx) {
  if (!ctx || !ctx->argv)
    return;
  for (int i = 0; i < ctx->argc; i++) {
    if (ctx->argv[i])
      free(ctx->argv[i]);
  }
  free(ctx->argv);
  ctx->argv = nil;
  ctx->argc = 0;
}

static void wasi_ctx_free_env(wasi_context_t *ctx) {
  if (!ctx || !ctx->envv)
    return;
  for (int i = 0; i < ctx->envc; i++) {
    if (ctx->envv[i])
      free(ctx->envv[i]);
  }
  free(ctx->envv);
  ctx->envv = nil;
  ctx->envc = 0;
}

static void wasi_build_argv(wasi_context_t *ctx, Proc *p) {
  if (!ctx || !p)
    return;
  if (!p->args || p->nargs <= 0)
    return;

  int len = p->nargs;
  char *buf = malloc(len + 1);
  if (!buf)
    return;
  memmove(buf, p->args, len);
  buf[len] = '\0';

  int cap = 8;
  ctx->argv = malloc(cap * sizeof(char *));
  if (!ctx->argv) {
    free(buf);
    return;
  }

  int argc = 0;
  char *s = buf;
  while (*s) {
    while (*s == ' ' || *s == '\t' || *s == '\n')
      s++;
    if (!*s)
      break;
    char *start = s;
    while (*s && *s != ' ' && *s != '\t' && *s != '\n')
      s++;
    int slen = s - start;
    if (slen == 0)
      continue;
    if (argc >= cap) {
      cap *= 2;
      char **newv = realloc(ctx->argv, cap * sizeof(char *));
      if (!newv)
        break;
      ctx->argv = newv;
    }
    ctx->argv[argc] = malloc(slen + 1);
    if (!ctx->argv[argc])
      break;
    memmove(ctx->argv[argc], start, slen);
    ctx->argv[argc][slen] = '\0';
    argc++;
  }
  ctx->argc = argc;
  free(buf);
}

static void wasi_build_env(wasi_context_t *ctx, Proc *p) {
  if (!ctx || !p || !p->egrp)
    return;
  Egrp *eg = p->egrp;
  rlock(&eg->rwlock);
  int cap = 16;
  int count = 0;
  char **envv = malloc(cap * sizeof(char *));
  if (!envv) {
    runlock(&eg->rwlock);
    return;
  }
  for (int i = 0; i < ENVHASH; i++) {
    for (Evalue *e = eg->hash[i]; e != nil; e = e->hash) {
      if (count >= cap) {
        cap *= 2;
        char **newv = realloc(envv, cap * sizeof(char *));
        if (!newv)
          goto done;
        envv = newv;
      }
      int name_len = strlen(e->name);
      int val_len = e->len;
      if (val_len < 0)
        val_len = 0;
      int total = name_len + 1 + val_len;
      char *entry = malloc(total + 1);
      if (!entry)
        goto done;
      memmove(entry, e->name, name_len);
      entry[name_len] = '=';
      if (val_len > 0)
        memmove(entry + name_len + 1, e->value, val_len);
      entry[total] = '\0';
      envv[count++] = entry;
    }
  }
done:
  runlock(&eg->rwlock);
  ctx->envv = envv;
  ctx->envc = count;
}

void wasi_lux9_init_context(wasi_context_t *ctx, Proc *p) {
  if (!ctx)
    return;
  memset(ctx, 0, sizeof(wasi_context_t));

  /* Pre-populate stdio (0, 1, 2) */
  ctx->fds[0].is_open = 1;
  ctx->fds[0].lux9_fid = 0; /* stdin */
  ctx->fds[0].backend = WASI_BACKEND_NATIVE;
  ctx->fds[0].rights = WASI_RIGHT_FD_READ | WASI_RIGHT_FD_SEEK |
                       WASI_RIGHT_FD_TELL | WASI_RIGHT_FD_FILESTAT_GET;
  ctx->fds[1].is_open = 1;
  ctx->fds[1].lux9_fid = 1; /* stdout */
  ctx->fds[1].backend = WASI_BACKEND_NATIVE;
  ctx->fds[1].rights = WASI_RIGHT_FD_WRITE | WASI_RIGHT_FD_FILESTAT_GET;
  ctx->fds[2].is_open = 1;
  ctx->fds[2].lux9_fid = 2; /* stderr */
  ctx->fds[2].backend = WASI_BACKEND_NATIVE;
  ctx->fds[2].rights = WASI_RIGHT_FD_WRITE | WASI_RIGHT_FD_FILESTAT_GET;

  /* Pre-populate WASM root (3) */
  int rootfd = kopen("/wasm", 0); /* OREAD */
  if (rootfd >= 0) {
    ctx->fds[3].is_open = 1;
    ctx->fds[3].lux9_fid = rootfd;
    ctx->fds[3].is_dir = 1;
    ctx->fds[3].backend = WASI_BACKEND_NATIVE;
    ctx->fds[3].base_path = (char *)wasi_root_path;
    ctx->fds[3].rights = WASI_RIGHTS_ALL;
    ctx->fds[3].rights_inheriting = WASI_RIGHTS_ALL;
    print("WASI: Opened /wasm at fd 3 (kernel fd %d)\n", rootfd);
  } else {
    print("WASI: Failed to open /wasm, using stub\n");
    ctx->fds[3].is_open = 1;
    ctx->fds[3].lux9_fid = 3;
    ctx->fds[3].is_dir = 1;
    ctx->fds[3].backend = WASI_BACKEND_NATIVE;
    ctx->fds[3].base_path = (char *)wasi_root_path;
    ctx->fds[3].rights = WASI_RIGHTS_ALL;
    ctx->fds[3].rights_inheriting = WASI_RIGHTS_ALL;
  }

  if (p && (p->capabilities & PERM_WASM_POSIX)) {
    ctx->fds[4].is_open = 1;
    ctx->fds[4].lux9_fid = -1;
    ctx->fds[4].is_dir = 1;
    ctx->fds[4].backend = WASI_BACKEND_POSIX;
    ctx->fds[4].base_path = (char *)wasi_posix_root_path;
    ctx->fds[4].rights = WASI_RIGHTS_ALL;
    ctx->fds[4].rights_inheriting = WASI_RIGHTS_ALL;
  }

  wasi_build_argv(ctx, p);
  wasi_build_env(ctx, p);
}

void wasi_lux9_destroy_context(wasi_context_t *ctx) {
  if (!ctx)
    return;
  wasi_ctx_free_argv(ctx);
  wasi_ctx_free_env(ctx);
  for (int i = 0; i < WASI_MAX_FDS; i++) {
    if (ctx->fds[i].is_open) {
      /* Use fdclose(fd, 0) to close Lux9 FD */
      /* Note: stdin(0), stdout(1), stderr(2) might be shared, be careful.
       * Usually we don't close them on process exit if they are system-wide,
       * but if they are duplicated for this proc, we should.
       * Lux9 Proc struct has fgrp which manages FDs.
       * If we rely on kernel process cleanup, we might not strictly need to
       * close here, but WASI expects explicit cleanup if we managed it.
       */
      if (ctx->fds[i].lux9_fid > 2) {
        fdclose(ctx->fds[i].lux9_fid, 0);
      }
      if (ctx->fds[i].base_path && ctx->fds[i].base_path != wasi_root_path &&
          ctx->fds[i].base_path != wasi_posix_root_path) {
        free(ctx->fds[i].base_path);
      }
      ctx->fds[i].is_open = 0;
    }
  }
}

/* ========== Implementations ========== */

/* wasi_fd_write(fd, iovs, iovs_len, nwritten) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_write) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, iovs_ptr) m3ApiGetArg(uint32_t, iovs_len)
          m3ApiGetArg(uint32_t, nwritten_ptr)

              uint32_t iov_size = 0;
  if (wasi_iovecs_size(iovs_len, &iov_size) < 0) {
    m3ApiReturn(WASI_ERRNO_INVAL);
  }
  m3ApiCheckMem(iovs_ptr, iov_size); /* 8 bytes per iovec struct */
  m3ApiCheckMem(nwritten_ptr, sizeof(uint32_t));

  /* Get Process Context */
  Proc *p = up; // Current process
  if (!p->wasm.initialized || !p->wasm.wasi_ctx) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_WRITE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  uint32_t total_written = 0;

  // Iterate over IO vectors
  for (int32_t i = 0; i < (int32_t)iovs_len; i++) {
    // Read iovec from WASM memory
    uint32_t iov_addr = iovs_ptr + (i * 8);

    // Manual read to avoid struct padding issues if any
    uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
    uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);

    if (buf_len == 0)
      continue;

    m3ApiCheckMem(buf_ptr, buf_len);
    void *buf = m3ApiOffsetToPtr(buf_ptr);

    // Write to Lux9 9P FID
    if (fd == 1 || fd == 2) {
      // Direct console write for debug AND kernel write
      print("%.*s", buf_len, (char *)buf);
      // Also write to underlying FD if valid (0,1,2 map to kernel 0,1,2
      // usually)
      kwrite(ctx->fds[fd].lux9_fid, buf, buf_len);
      total_written += buf_len;
    } else {
      long n = kwrite(ctx->fds[fd].lux9_fid, buf, buf_len);
      if (n < 0) {
        m3ApiReturn(WASI_ERRNO_IO);
      }
      total_written += n;
    }
  }

  m3ApiWriteMem32(nwritten_ptr, total_written);

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_proc_exit(rval) */
m3ApiRawFunction(wasi_snapshot_preview1_proc_exit) {
  m3ApiGetArg(int32_t, rval)

      print("Pretend exiting with code %d\n", rval);
  if (up->wasm.wasi_ctx)
    ((wasi_context_t *)up->wasm.wasi_ctx)->exit_code = (u32int)rval;

  m3ApiTrap(m3Err_trapExit);
}

/* wasi_proc_raise(sig) */
m3ApiRawFunction(wasi_snapshot_preview1_proc_raise) {
  m3ApiReturnType(uint32_t);
  m3ApiGetArg(uint32_t, sig);
  char note[64];
  snprint(note, sizeof(note), "wasm signal %ud", sig);
  postnote(up, 1, note, NUser);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_sched_yield() */
m3ApiRawFunction(wasi_snapshot_preview1_sched_yield) {
  m3ApiReturnType(uint32_t);
  yield();
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_args_sizes_get(argc, argv_buf_size) */
m3ApiRawFunction(wasi_snapshot_preview1_args_sizes_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, argc_ptr)
      m3ApiGetArg(uint32_t, argv_buf_size_ptr)

          m3ApiCheckMem(argc_ptr, sizeof(uint32_t));
  m3ApiCheckMem(argv_buf_size_ptr, sizeof(uint32_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t total = 0;
  for (int i = 0; i < ctx->argc; i++) {
    if (ctx->argv[i])
      total += (uint32_t)strlen(ctx->argv[i]) + 1;
  }
  m3ApiWriteMem32(argc_ptr, (uint32_t)ctx->argc);
  m3ApiWriteMem32(argv_buf_size_ptr, total);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_args_get(argv, argv_buf) */
m3ApiRawFunction(wasi_snapshot_preview1_args_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, argv_ptr)
      m3ApiGetArg(uint32_t, argv_buf_ptr)

          Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t total = 0;
  for (int i = 0; i < ctx->argc; i++) {
    if (ctx->argv[i])
      total += (uint32_t)strlen(ctx->argv[i]) + 1;
  }
  m3ApiCheckMem(argv_ptr, ctx->argc * sizeof(uint32_t));
  m3ApiCheckMem(argv_buf_ptr, total);

  uint32_t cur = 0;
  for (int i = 0; i < ctx->argc; i++) {
    const char *arg = ctx->argv[i] ? ctx->argv[i] : "";
    uint32_t len = (uint32_t)strlen(arg) + 1;
    m3ApiWriteMem32(argv_ptr + (i * 4), argv_buf_ptr + cur);
    memmove(m3ApiOffsetToPtr(argv_buf_ptr + cur), arg, len);
    cur += len;
  }
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_environ_sizes_get(env_count, env_buf_size) */
m3ApiRawFunction(wasi_snapshot_preview1_environ_sizes_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, env_count_ptr)
      m3ApiGetArg(uint32_t, env_buf_size_ptr)

          m3ApiCheckMem(env_count_ptr, sizeof(uint32_t));
  m3ApiCheckMem(env_buf_size_ptr, sizeof(uint32_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t total = 0;
  for (int i = 0; i < ctx->envc; i++) {
    if (ctx->envv[i])
      total += (uint32_t)strlen(ctx->envv[i]) + 1;
  }
  m3ApiWriteMem32(env_count_ptr, (uint32_t)ctx->envc);
  m3ApiWriteMem32(env_buf_size_ptr, total);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_environ_get(environ, environ_buf) */
m3ApiRawFunction(wasi_snapshot_preview1_environ_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, environ_ptr)
      m3ApiGetArg(uint32_t, environ_buf_ptr)

          Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t total = 0;
  for (int i = 0; i < ctx->envc; i++) {
    if (ctx->envv[i])
      total += (uint32_t)strlen(ctx->envv[i]) + 1;
  }
  m3ApiCheckMem(environ_ptr, ctx->envc * sizeof(uint32_t));
  m3ApiCheckMem(environ_buf_ptr, total);

  uint32_t cur = 0;
  for (int i = 0; i < ctx->envc; i++) {
    const char *env = ctx->envv[i] ? ctx->envv[i] : "";
    uint32_t len = (uint32_t)strlen(env) + 1;
    m3ApiWriteMem32(environ_ptr + (i * 4), environ_buf_ptr + cur);
    memmove(m3ApiOffsetToPtr(environ_buf_ptr + cur), env, len);
    cur += len;
  }
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_prestat_get(fd, prestat) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_prestat_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, prestat_ptr)

          m3ApiCheckMem(prestat_ptr, sizeof(wasi_prestat_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  if ((fd != 3 && fd != 4) || !ctx->fds[fd].is_open || !ctx->fds[fd].is_dir) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }

  m3ApiWriteMem8(prestat_ptr + 0, 0);
  m3ApiWriteMem32(prestat_ptr + 4, (fd == 4)
                                       ? (sizeof(wasi_posix_root_path) - 1)
                                       : (sizeof(wasi_root_path) - 1));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_prestat_dir_name(fd, path, path_len) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_prestat_dir_name) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, path_ptr) m3ApiGetArg(uint32_t, path_len)

          m3ApiCheckMem(path_ptr, path_len);

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  if ((fd != 3 && fd != 4) || !ctx->fds[fd].is_open || !ctx->fds[fd].is_dir) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }
  uint32_t want = (fd == 4) ? (sizeof(wasi_posix_root_path) - 1)
                            : (sizeof(wasi_root_path) - 1);
  if (path_len < want) {
    m3ApiReturn(WASI_ERRNO_NAMETOOLONG);
  }
  char *dst = (char *)m3ApiOffsetToPtr(path_ptr);
  if (fd == 4) {
    memmove(dst, wasi_posix_root_path, want);
  } else {
    memmove(dst, wasi_root_path, want);
  }
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_random_get(buf, buf_len) */
m3ApiRawFunction(wasi_snapshot_preview1_random_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, buf_ptr)
      m3ApiGetArg(uint32_t, buf_len)

          m3ApiCheckMem(buf_ptr, buf_len);
  uint8_t *buf = (uint8_t *)m3ApiOffsetToPtr(buf_ptr);
  wasi_fill_random(buf, buf_len);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_clock_res_get(clock_id, resolution) */
m3ApiRawFunction(wasi_snapshot_preview1_clock_res_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, clock_id)
      m3ApiGetArg(uint32_t, resolution_ptr)

          m3ApiCheckMem(resolution_ptr, sizeof(uint64_t));
  (void)clock_id;
  m3ApiWriteMem64(resolution_ptr, 1);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_clock_time_get(clock_id, precision, time) */
m3ApiRawFunction(wasi_snapshot_preview1_clock_time_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, clock_id)
      m3ApiGetArg(uint64_t, precision) m3ApiGetArg(uint32_t, time_ptr)

          m3ApiCheckMem(time_ptr, sizeof(uint64_t));
  (void)clock_id;
  (void)precision;
  m3ApiWriteMem64(time_ptr, (uint64_t)nsec());
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_path_open(...) */
m3ApiRawFunction(wasi_snapshot_preview1_path_open) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, dirflags) m3ApiGetArg(uint32_t, path_ptr)
          m3ApiGetArg(uint32_t, path_len) m3ApiGetArg(uint32_t, oflags)
              m3ApiGetArg(uint64_t, fs_rights_base)
                  m3ApiGetArg(uint64_t, fs_rights_inh)
                      m3ApiGetArg(uint32_t, fs_flags)
                          m3ApiGetArg(uint32_t, fd_out_ptr)

      /* Check memory for path */
      m3ApiCheckMem(path_ptr, path_len);
  m3ApiCheckMem(fd_out_ptr, sizeof(uint32_t));

  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  /* Safe copy/null-terminate not strictly needed if we obey len,
     but good practice if we pass to kernel functions */

  print("WASI: path_open(dirfd=%d, path='%.*s')\n", dirfd, path_len, path);

  /* Get Context */
  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  /* Validate dirfd */
  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_OPEN);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  if ((fs_rights_base & ~ctx->fds[dirfd].rights_inheriting) ||
      (fs_rights_inh & ~ctx->fds[dirfd].rights_inheriting)) {
    m3ApiReturn(WASI_ERRNO_NOTCAPABLE);
  }

  /* Allocate new FD */
  int new_fd = -1;
  for (int i = 3; i < WASI_MAX_FDS; i++) {
    if (!ctx->fds[i].is_open) {
      new_fd = i;
      break;
    }
  }
  if (new_fd == -1)
    m3ApiReturn(WASI_ERRNO_MFILE);

  /* Modes */
  int kmode = 0; // OREAD
  /* Very basic mode mapping */
  if (fs_rights_base & WASI_RIGHT_FD_WRITE)
    kmode = 1; /* Write rights -> OWRITE */
  /* If read and write, O_RDWR (2 in posix, different in simple plan9? Plan 9:
   * OREAD=0, OWRITE=1, ORDWR=2, OEXEC=3) */
  if ((fs_rights_base & WASI_RIGHT_FD_READ) &&
      (fs_rights_base & WASI_RIGHT_FD_WRITE))
    kmode = 2;
  if (oflags & WASI_O_TRUNC)
    kmode |= OTRUNC;
  if (oflags & WASI_O_EXCL)
    kmode |= OEXCL;

  /* We treat path as absolute for now, ignoring dirfd relative path logic for
     simplicity unless we implement full walk from dirfd's chan. Use kopen()
     which expects full path or relative to CWD. WASM usually sends relative
     paths.
  */

  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS) {
    m3ApiReturn(perr);
  }
  int backend = ctx->fds[dirfd].backend;
  if (wasi_is_posix_path(fullpath, (uint32_t)strlen(fullpath)))
    backend = WASI_BACKEND_POSIX;

  int kfd = -1;
  if (oflags & WASI_O_CREAT) {
    uint64_t need = WASI_RIGHT_PATH_CREATE_FILE;
    if (oflags & WASI_O_DIRECTORY)
      need = WASI_RIGHT_PATH_CREATE_DIRECTORY;
    cap = wasi_require_fd(ctx, dirfd, need);
    if (cap != WASI_ERRNO_SUCCESS)
      m3ApiReturn(cap);
    int perm = 0666;
    if (oflags & WASI_O_DIRECTORY)
      perm |= DMDIR;
    openmode(kmode & ~OEXCL);
    Chan *c = namec(fullpath, Acreate, kmode, perm);
    if (waserror()) {
      cclose(c);
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_NOENT);
    }
    kfd = newfd(c, kmode);
    if (kfd < 0) {
      cclose(c);
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_MFILE);
    }
    poperror();
  } else {
    kfd = kopen(fullpath, kmode);
  }
  free(fullpath);
  if (kfd < 0) {
    m3ApiReturn(WASI_ERRNO_NOENT);
  }

  ctx->fds[new_fd].is_open = 1;
  ctx->fds[new_fd].lux9_fid = kfd;
  ctx->fds[new_fd].is_dir = 0;
  ctx->fds[new_fd].backend = backend;
  ctx->fds[new_fd].base_path = nil;
  ctx->fds[new_fd].rights = fs_rights_base;
  ctx->fds[new_fd].rights_inheriting = fs_rights_inh;
  Chan *c = nil;
  if (waserror()) {
    if (c)
      cclose(c);
    fdclose(kfd, 0);
    ctx->fds[new_fd].is_open = 0;
    m3ApiReturn(WASI_ERRNO_IO);
  }
  c = fdtochan(kfd, -1, 0, 1);
  Dir *d = dirchanstat(c);
  if (d && (d->mode & DMDIR))
    ctx->fds[new_fd].is_dir = 1;
  free(d);
  cclose(c);
  poperror();
  if (ctx->fds[new_fd].is_dir) {
    size_t len = strlen(fullpath);
    char *dup = malloc(len + 1);
    if (dup) {
      memmove(dup, fullpath, len);
      dup[len] = '\0';
      ctx->fds[new_fd].base_path = dup;
    }
  }

  if ((oflags & WASI_O_DIRECTORY) && !ctx->fds[new_fd].is_dir) {
    fdclose(kfd, 0);
    ctx->fds[new_fd].is_open = 0;
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  }

  m3ApiWriteMem32(fd_out_ptr, new_fd);

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_read(fd, iovs, iovs_len, nread) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_read) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, iovs_ptr) m3ApiGetArg(uint32_t, iovs_len)
          m3ApiGetArg(uint32_t, nread_ptr)

              uint32_t iov_size = 0;
  if (wasi_iovecs_size(iovs_len, &iov_size) < 0) {
    m3ApiReturn(WASI_ERRNO_INVAL);
  }
  m3ApiCheckMem(iovs_ptr, iov_size);
  m3ApiCheckMem(nread_ptr, sizeof(uint32_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_READ);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  uint32_t total_read = 0;
  for (int32_t i = 0; i < (int32_t)iovs_len; i++) {
    uint32_t iov_addr = iovs_ptr + (i * 8);
    uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
    uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);

    if (buf_len == 0)
      continue;
    m3ApiCheckMem(buf_ptr, buf_len);
    void *buf = m3ApiOffsetToPtr(buf_ptr);

    long n = kread(ctx->fds[fd].lux9_fid, buf, buf_len);
    if (n < 0) {
      m3ApiReturn(WASI_ERRNO_IO);
    }
    total_read += n;
    if (n < buf_len)
      break; // Short read (EOF or block)
  }

  m3ApiWriteMem32(nread_ptr, total_read);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_pread(fd, iovs, iovs_len, offset, nread) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_pread) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, iovs_ptr) m3ApiGetArg(uint32_t, iovs_len)
          m3ApiGetArg(uint64_t, offset) m3ApiGetArg(uint32_t, nread_ptr)

              uint32_t iov_size = 0;
  if (wasi_iovecs_size(iovs_len, &iov_size) < 0)
    m3ApiReturn(WASI_ERRNO_INVAL);
  m3ApiCheckMem(iovs_ptr, iov_size);
  m3ApiCheckMem(nread_ptr, sizeof(uint32_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_READ);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_POSIX) {
    vlong saved = kseek(ctx->fds[fd].lux9_fid, 0, 1);
    if (saved < 0)
      m3ApiReturn(WASI_ERRNO_IO);
    if (kseek(ctx->fds[fd].lux9_fid, (vlong)offset, 0) < 0)
      m3ApiReturn(WASI_ERRNO_IO);
    uint32_t total_read = 0;
    for (int32_t i = 0; i < (int32_t)iovs_len; i++) {
      uint32_t iov_addr = iovs_ptr + (i * 8);
      uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
      uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
      if (buf_len == 0)
        continue;
      m3ApiCheckMem(buf_ptr, buf_len);
      void *buf = m3ApiOffsetToPtr(buf_ptr);
      long n = kread(ctx->fds[fd].lux9_fid, buf, buf_len);
      if (n < 0) {
        kseek(ctx->fds[fd].lux9_fid, saved, 0);
        m3ApiReturn(WASI_ERRNO_IO);
      }
      total_read += n;
      if ((uint32_t)n < buf_len)
        break;
    }
    kseek(ctx->fds[fd].lux9_fid, saved, 0);
    m3ApiWriteMem32(nread_ptr, total_read);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }

  uint32_t total = 0;
  for (uint32_t i = 0; i < iovs_len; i++) {
    uint32_t iov_addr = iovs_ptr + (i * 8);
    uint32_t len = m3ApiReadMem32(iov_addr + 4);
    if (len > WASI_RUMP_IO_MAX - total)
      m3ApiReturn(WASI_ERRNO_INVAL);
    total += len;
  }

  uint32_t req[5];
  uint8_t resp[8 + WASI_RUMP_IO_MAX];
  req[0] = POSIX_PREAD;
  req[1] = (uint32_t)fd;
  req[2] = (uint32_t)(offset & 0xffffffffu);
  req[3] = (uint32_t)(offset >> 32);
  req[4] = total;

  int r = wasi_rump_rpc_read("/srv/rump/posix/pread", req, sizeof(req), resp,
                             8 + total);
  if (r < 8)
    m3ApiReturn(WASI_ERRNO_IO);

  uint32_t err = *(uint32_t *)resp;
  if (err != 0)
    m3ApiReturn(wasi_errno_from_posix(err));

  uint32_t count = *(uint32_t *)(resp + 4);
  if (count > total)
    count = total;

  uint32_t remaining = count;
  uint32_t off = 8;
  for (uint32_t i = 0; i < iovs_len && remaining > 0; i++) {
    uint32_t iov_addr = iovs_ptr + (i * 8);
    uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
    uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
    uint32_t copy_len = buf_len;
    if (copy_len > remaining)
      copy_len = remaining;
    if (copy_len == 0)
      continue;
    m3ApiCheckMem(buf_ptr, copy_len);
    memmove(m3ApiOffsetToPtr(buf_ptr), resp + off, copy_len);
    off += copy_len;
    remaining -= copy_len;
  }

  m3ApiWriteMem32(nread_ptr, count);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_pwrite(fd, iovs, iovs_len, offset, nwritten) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_pwrite) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, iovs_ptr) m3ApiGetArg(uint32_t, iovs_len)
          m3ApiGetArg(uint64_t, offset) m3ApiGetArg(uint32_t, nwritten_ptr)

              uint32_t iov_size = 0;
  if (wasi_iovecs_size(iovs_len, &iov_size) < 0)
    m3ApiReturn(WASI_ERRNO_INVAL);
  m3ApiCheckMem(iovs_ptr, iov_size);
  m3ApiCheckMem(nwritten_ptr, sizeof(uint32_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_WRITE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_POSIX) {
    vlong saved = kseek(ctx->fds[fd].lux9_fid, 0, 1);
    if (saved < 0)
      m3ApiReturn(WASI_ERRNO_IO);
    if (kseek(ctx->fds[fd].lux9_fid, (vlong)offset, 0) < 0)
      m3ApiReturn(WASI_ERRNO_IO);
    uint32_t total_written = 0;
    for (int32_t i = 0; i < (int32_t)iovs_len; i++) {
      uint32_t iov_addr = iovs_ptr + (i * 8);
      uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
      uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
      if (buf_len == 0)
        continue;
      m3ApiCheckMem(buf_ptr, buf_len);
      void *buf = m3ApiOffsetToPtr(buf_ptr);
      long n = kwrite(ctx->fds[fd].lux9_fid, buf, buf_len);
      if (n < 0) {
        kseek(ctx->fds[fd].lux9_fid, saved, 0);
        m3ApiReturn(WASI_ERRNO_IO);
      }
      total_written += n;
      if ((uint32_t)n < buf_len)
        break;
    }
    kseek(ctx->fds[fd].lux9_fid, saved, 0);
    m3ApiWriteMem32(nwritten_ptr, total_written);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }

  uint32_t total = 0;
  for (uint32_t i = 0; i < iovs_len; i++) {
    uint32_t iov_addr = iovs_ptr + (i * 8);
    uint32_t len = m3ApiReadMem32(iov_addr + 4);
    if (len > WASI_RUMP_IO_MAX - total)
      m3ApiReturn(WASI_ERRNO_INVAL);
    total += len;
  }

  uint32_t req_size = 20 + total;
  uint8_t *req = malloc(req_size);
  if (!req)
    m3ApiReturn(WASI_ERRNO_NOMEM);

  *(uint32_t *)(req + 0) = POSIX_PWRITE;
  *(uint32_t *)(req + 4) = (uint32_t)fd;
  *(uint32_t *)(req + 8) = (uint32_t)(offset & 0xffffffffu);
  *(uint32_t *)(req + 12) = (uint32_t)(offset >> 32);
  *(uint32_t *)(req + 16) = total;

  uint32_t off = 20;
  for (uint32_t i = 0; i < iovs_len; i++) {
    uint32_t iov_addr = iovs_ptr + (i * 8);
    uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
    uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
    if (buf_len == 0)
      continue;
    m3ApiCheckMem(buf_ptr, buf_len);
    memmove(req + off, m3ApiOffsetToPtr(buf_ptr), buf_len);
    off += buf_len;
  }

  uint32_t resp[2];
  int r = wasi_rump_rpc_read("/srv/rump/posix/pwrite", req, req_size, resp,
                             sizeof(resp));
  free(req);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);

  uint32_t err = resp[0];
  if (err != 0)
    m3ApiReturn(wasi_errno_from_posix(err));

  m3ApiWriteMem32(nwritten_ptr, resp[1]);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_advise(fd, offset, len, advice) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_advise) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint64_t, offset) m3ApiGetArg(uint64_t, len)
          m3ApiGetArg(uint32_t, advice)

              Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_ADVISE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  struct {
    uint32_t fd;
    uint64_t offset;
    uint64_t len;
    uint32_t advice;
  } req;
  req.fd = (uint32_t)fd;
  req.offset = offset;
  req.len = len;
  req.advice = advice;

  uint32_t resp = 0;
  int r = wasi_rump_rpc_read("/srv/rump/posix/fd_advise", &req, sizeof(req),
                             &resp, sizeof(resp));
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_allocate(fd, offset, len) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_allocate) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint64_t, offset) m3ApiGetArg(uint64_t, len) Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_ALLOCATE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  struct {
    uint32_t fd;
    uint64_t offset;
    uint64_t len;
  } req;
  req.fd = (uint32_t)fd;
  req.offset = offset;
  req.len = len;

  uint32_t resp = 0;
  int r = wasi_rump_rpc_read("/srv/rump/posix/fd_allocate", &req, sizeof(req),
                             &resp, sizeof(resp));
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_close(fd) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_close) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)

      Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_SEEK);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  fdclose(ctx->fds[fd].lux9_fid, 0);
  ctx->fds[fd].is_open = 0;
  if (ctx->fds[fd].base_path && ctx->fds[fd].base_path != wasi_root_path &&
      ctx->fds[fd].base_path != wasi_posix_root_path) {
    free(ctx->fds[fd].base_path);
    ctx->fds[fd].base_path = nil;
  }

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_renumber(from, to) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_renumber) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, from) m3ApiGetArg(int32_t, to)

      Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  if (from < 0 || to < 0 || from >= WASI_MAX_FDS || to >= WASI_MAX_FDS)
    m3ApiReturn(WASI_ERRNO_BADF);
  if (!ctx->fds[from].is_open)
    m3ApiReturn(WASI_ERRNO_BADF);
  if (from == to)
    m3ApiReturn(WASI_ERRNO_SUCCESS);

  if (ctx->fds[to].is_open) {
    fdclose(ctx->fds[to].lux9_fid, 0);
    if (ctx->fds[to].base_path && ctx->fds[to].base_path != wasi_root_path &&
        ctx->fds[to].base_path != wasi_posix_root_path) {
      free(ctx->fds[to].base_path);
    }
  }

  ctx->fds[to] = ctx->fds[from];
  ctx->fds[from].is_open = 0;
  ctx->fds[from].lux9_fid = 0;
  ctx->fds[from].rights = 0;
  ctx->fds[from].rights_inheriting = 0;
  ctx->fds[from].is_dir = 0;
  ctx->fds[from].backend = 0;
  ctx->fds[from].base_path = nil;
  ctx->fds[from].offset = 0;

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_seek(fd, offset, whence, newoffset) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_seek) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(int64_t, offset) m3ApiGetArg(int32_t, whence)
          m3ApiGetArg(uint32_t, newoffset_ptr)

      /* WASI whence: 0=SET, 1=CUR, 2=END.
       * Plan 9 seek: 0=SET, 1=CUR, 2=END.
       * Mapping 1:1. */

      Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, 0);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  m3ApiCheckMem(newoffset_ptr, sizeof(uint64_t));
  vlong res = kseek(ctx->fds[fd].lux9_fid, offset, whence);
  if (res < 0) {
    m3ApiReturn(WASI_ERRNO_INVAL);
  }

  m3ApiWriteMem64(newoffset_ptr, res);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_fdstat_get(fd, buf) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_fdstat_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, buf_ptr)

          m3ApiCheckMem(buf_ptr, sizeof(wasi_fdstat_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_FILESTAT_GET);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  // Fill with dummy data suitable for TTY/File
  uint8_t filetype =
      (fd <= 2) ? WASI_FILETYPE_CHARACTER_DEVICE : WASI_FILETYPE_REGULAR_FILE;
  if (ctx->fds[fd].is_dir) {
    filetype = WASI_FILETYPE_DIRECTORY;
  }
  uint16_t flags = 0;
  uint64_t rights = ctx->fds[fd].rights;
  uint64_t rights_inh = ctx->fds[fd].rights_inheriting;

  m3ApiWriteMem8(buf_ptr + 0, filetype);
  m3ApiWriteMem16(buf_ptr + 2, flags); // Alignment padding? Check struct layout
  // wasi_fdstat_t layout:
  // 0: u8 filetype
  // 1: (pad)
  // 2: u16 flags
  // 4: (pad)
  // 8: u64 rights_base
  // 16: u64 rights_inh
  // Total 24 bytes

  // NOTE: We used packed struct in C logic maybe, but WASM is aligned.
  // Let's write explicitly to offsets.

  m3ApiWriteMem16(buf_ptr + 2, flags);
  m3ApiWriteMem64(buf_ptr + 8, rights);
  m3ApiWriteMem64(buf_ptr + 16, rights_inh);

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_fdstat_set_flags(fd, flags) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_fdstat_set_flags) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, flags)

          Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_FDSTAT_SET_FLAGS);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  struct {
    uint32_t fd;
    uint32_t flags;
  } req;
  req.fd = (uint32_t)fd;
  req.flags = flags;

  uint32_t resp = 0;
  int r = wasi_rump_rpc_read("/srv/rump/posix/fd_set_flags", &req, sizeof(req),
                             &resp, sizeof(resp));
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_fdstat_set_rights(fd, rights_base, rights_inheriting) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_fdstat_set_rights) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint64_t, rights_base)
          m3ApiGetArg(uint64_t, rights_inheriting)

              Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, 0);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  if ((rights_base & ~ctx->fds[fd].rights) ||
      (rights_inheriting & ~ctx->fds[fd].rights_inheriting)) {
    m3ApiReturn(WASI_ERRNO_NOTCAPABLE);
  }

  ctx->fds[fd].rights = rights_base;
  ctx->fds[fd].rights_inheriting = rights_inheriting;
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_readdir(fd, buf, buf_len, cookie, bufused) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_readdir) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, buf_ptr) m3ApiGetArg(uint32_t, buf_len)
          m3ApiGetArg(uint64_t, cookie) m3ApiGetArg(uint32_t, bufused_ptr)

              m3ApiCheckMem(buf_ptr, buf_len);
  m3ApiCheckMem(bufused_ptr, sizeof(uint32_t));

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_READDIR);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[fd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_POSIX) {
    uint32_t used = 0;
    uint32_t err = wasi_dirents_from_plan9(p, ctx->fds[fd].lux9_fid,
                                           (uint8_t *)m3ApiOffsetToPtr(buf_ptr),
                                           buf_len, cookie, &used);
    if (err != WASI_ERRNO_SUCCESS)
      m3ApiReturn(err);
    m3ApiWriteMem32(bufused_ptr, used);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }

  {
    uint32_t used = 0;
    uint32_t err = wasi_dirents_from_plan9(p, ctx->fds[fd].lux9_fid,
                                           (uint8_t *)m3ApiOffsetToPtr(buf_ptr),
                                           buf_len, cookie, &used);
    if (err != WASI_ERRNO_SUCCESS)
      m3ApiReturn(err);
    m3ApiWriteMem32(bufused_ptr, used);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }
}

/* wasi_fd_sync(fd) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_sync) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd) Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_SYNC);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_POSIX)
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  m3ApiReturn(wasi_rump_simple_errno("/srv/rump/posix/fd_sync"));
}

/* wasi_fd_datasync(fd) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_datasync) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd) Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_DATASYNC);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_POSIX)
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  m3ApiReturn(wasi_rump_simple_errno("/srv/rump/posix/fd_datasync"));
}

/* wasi_fd_tell(fd, offset) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_tell) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, offset_ptr) Proc *p = up;
  m3ApiCheckMem(offset_ptr, sizeof(uint64_t));
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_TELL);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_POSIX) {
    vlong pos = kseek(ctx->fds[fd].lux9_fid, 0, 1);
    if (pos < 0)
      m3ApiReturn(WASI_ERRNO_IO);
    m3ApiWriteMem64(offset_ptr, (uint64_t)pos);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }
  m3ApiReturn(wasi_rump_simple_errno("/srv/rump/posix/fd_tell"));
}

/* wasi_fd_filestat_set_size(fd, size) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_filestat_set_size) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd) m3ApiGetArg(uint64_t, size)
      Proc *p = up;
  (void)size;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_FILESTAT_SET_SIZE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  m3ApiReturn(wasi_rump_simple_errno("/srv/rump/posix/fd_set_size"));
}

/* wasi_fd_filestat_set_times(fd, atim, mtim, flags) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_filestat_set_times) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd) m3ApiGetArg(uint64_t, atim)
      m3ApiGetArg(uint64_t, mtim) m3ApiGetArg(uint32_t, flags) Proc *p = up;
  (void)atim;
  (void)mtim;
  (void)flags;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_FILESTAT_SET_TIMES);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  m3ApiReturn(wasi_rump_simple_errno("/srv/rump/posix/fd_set_times"));
}

/* wasi_path_filestat_set_times(dirfd, dirflags, path, path_len, atim, mtim,
 * flags) */
m3ApiRawFunction(wasi_snapshot_preview1_path_filestat_set_times) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, dirflags) m3ApiGetArg(uint32_t, path_ptr)
          m3ApiGetArg(uint32_t, path_len) m3ApiGetArg(uint64_t, atim)
              m3ApiGetArg(uint64_t, mtim) m3ApiGetArg(uint32_t, flags) Proc *p =
                  up;
  (void)dirflags;
  (void)path_ptr;
  (void)path_len;
  (void)atim;
  (void)mtim;
  (void)flags;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap =
      wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_FILESTAT_SET_TIMES);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);

  int backend = ctx->fds[dirfd].backend;
  uint32_t full_len = 0;
  const char *posix_path = (backend == WASI_BACKEND_POSIX)
                               ? wasi_posix_path(fullpath, &full_len)
                               : fullpath;
  if (backend != WASI_BACKEND_POSIX)
    full_len = (uint32_t)strlen(fullpath);

  uint32_t extra[5];
  extra[0] = (uint32_t)(atim & 0xffffffffu);
  extra[1] = (uint32_t)(atim >> 32);
  extra[2] = (uint32_t)(mtim & 0xffffffffu);
  extra[3] = (uint32_t)(mtim >> 32);
  extra[4] = flags;

  uint32_t resp = 0;
  int r = wasi_rump_path_req("/srv/rump/posix/path_set_times",
                             POSIX_PATH_SET_TIMES, posix_path, full_len, extra,
                             sizeof(extra), &resp, sizeof(resp));
  free(fullpath);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_path_filestat_set_size(dirfd, dirflags, path, path_len, size) */
m3ApiRawFunction(wasi_snapshot_preview1_path_filestat_set_size) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, dirflags) m3ApiGetArg(uint32_t, path_ptr)
          m3ApiGetArg(uint32_t, path_len) m3ApiGetArg(uint64_t, size) Proc *p =
              up;
  (void)dirflags;
  (void)path_ptr;
  (void)path_len;
  (void)size;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_FILESTAT_SET_SIZE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);

  int backend = ctx->fds[dirfd].backend;
  uint32_t full_len = 0;
  const char *posix_path = (backend == WASI_BACKEND_POSIX)
                               ? wasi_posix_path(fullpath, &full_len)
                               : fullpath;
  if (backend != WASI_BACKEND_POSIX)
    full_len = (uint32_t)strlen(fullpath);

  uint32_t extra[2];
  extra[0] = (uint32_t)(size & 0xffffffffu);
  extra[1] = (uint32_t)(size >> 32);

  uint32_t resp = 0;
  int r = wasi_rump_path_req("/srv/rump/posix/path_set_size",
                             POSIX_PATH_SET_SIZE, posix_path, full_len, extra,
                             sizeof(extra), &resp, sizeof(resp));
  free(fullpath);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_path_create_directory(dirfd, path, path_len) */
m3ApiRawFunction(wasi_snapshot_preview1_path_create_directory) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, path_ptr) m3ApiGetArg(uint32_t, path_len) Proc *p =
          up;
  (void)path_ptr;
  (void)path_len;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_CREATE_DIRECTORY);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);

  int backend = ctx->fds[dirfd].backend;
  uint32_t full_len = 0;
  const char *posix_path = (backend == WASI_BACKEND_POSIX)
                               ? wasi_posix_path(fullpath, &full_len)
                               : fullpath;
  if (backend != WASI_BACKEND_POSIX)
    full_len = (uint32_t)strlen(fullpath);

  uint32_t resp = 0;
  int r = wasi_rump_path_req("/srv/rump/posix/path_create_directory",
                             POSIX_PATH_CREATE_DIRECTORY, posix_path, full_len,
                             nil, 0, &resp, sizeof(resp));
  free(fullpath);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_path_remove_directory(dirfd, path, path_len) */
m3ApiRawFunction(wasi_snapshot_preview1_path_remove_directory) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, path_ptr) m3ApiGetArg(uint32_t, path_len) Proc *p =
          up;
  (void)path_ptr;
  (void)path_len;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_REMOVE_DIRECTORY);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);

  int backend = ctx->fds[dirfd].backend;
  uint32_t full_len = 0;
  if (backend == WASI_BACKEND_POSIX) {
    const char *posix_path = wasi_posix_path(fullpath, &full_len);
    uint32_t req_size = 8 + full_len;
    uint8_t *req = malloc(req_size);
    if (!req) {
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_NOMEM);
    }
    *(uint32_t *)(req + 0) = POSIX_PATH_REMOVE_DIRECTORY;
    *(uint32_t *)(req + 4) = full_len;
    memmove(req + 8, posix_path, full_len);
    free(fullpath);

    uint32_t resp = 0;
    int r = wasi_rump_rpc_read("/srv/rump/posix/path_remove_directory", req,
                               req_size, &resp, sizeof(resp));
    free(req);
    if (r < (int)sizeof(resp))
      m3ApiReturn(WASI_ERRNO_IO);
    if (resp != 0)
      m3ApiReturn(wasi_errno_from_posix(resp));
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  } else {
    /* Native Backend */
    Chan *c = namec(fullpath, Aremove, 0, 0);
    if (waserror()) {
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_NOENT);
    }
    devremove(c);
    poperror();
    free(fullpath);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }
}

/* wasi_path_unlink_file(dirfd, path, path_len) */
m3ApiRawFunction(wasi_snapshot_preview1_path_unlink_file) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, path_ptr) m3ApiGetArg(uint32_t, path_len) Proc *p =
          up;
  (void)path_ptr;
  (void)path_len;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_UNLINK_FILE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);

  int backend = ctx->fds[dirfd].backend;
  uint32_t full_len = 0;
  if (backend == WASI_BACKEND_POSIX) {
    const char *posix_path = wasi_posix_path(fullpath, &full_len);
    uint32_t req_size = 8 + full_len;
    uint8_t *req = malloc(req_size);
    if (!req) {
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_NOMEM);
    }
    *(uint32_t *)(req + 0) = POSIX_PATH_UNLINK_FILE;
    *(uint32_t *)(req + 4) = full_len;
    memmove(req + 8, posix_path, full_len);
    free(fullpath);

    uint32_t resp = 0;
    int r = wasi_rump_rpc_read("/srv/rump/posix/path_unlink_file", req,
                               req_size, &resp, sizeof(resp));
    free(req);
    if (r < (int)sizeof(resp))
      m3ApiReturn(WASI_ERRNO_IO);
    if (resp != 0)
      m3ApiReturn(wasi_errno_from_posix(resp));
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  } else {
    /* Native Backend */
    Chan *c = namec(fullpath, Aremove, 0, 0);
    if (waserror()) {
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_NOENT);
    }
    devremove(c);
    poperror();
    free(fullpath);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }
}

/* wasi_path_rename(old_fd, old_path, old_len, new_fd, new_path, new_len) */
m3ApiRawFunction(wasi_snapshot_preview1_path_rename) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, old_fd)
      m3ApiGetArg(uint32_t, old_path) m3ApiGetArg(uint32_t, old_len)
          m3ApiGetArg(int32_t, new_fd) m3ApiGetArg(uint32_t, new_path)
              m3ApiGetArg(uint32_t, new_len) Proc *p = up;
  (void)old_path;
  (void)old_len;
  (void)new_path;
  (void)new_len;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, old_fd, WASI_RIGHT_PATH_RENAME_SOURCE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  cap = wasi_require_fd(ctx, new_fd, WASI_RIGHT_PATH_RENAME_TARGET);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  char *oldp = (char *)m3ApiOffsetToPtr(old_path);
  char *newp = (char *)m3ApiOffsetToPtr(new_path);
  char *old_full = nil;
  char *new_full = nil;
  uint32_t perr = wasi_build_path(ctx, old_fd, oldp, old_len, &old_full);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);
  perr = wasi_build_path(ctx, new_fd, newp, new_len, &new_full);
  if (perr != WASI_ERRNO_SUCCESS) {
    free(old_full);
    m3ApiReturn(perr);
  }

  uint32_t old_len_full = (uint32_t)strlen(old_full);
  uint32_t new_len_full = (uint32_t)strlen(new_full);
  int backend = ctx->fds[old_fd].backend;
  if (backend == WASI_BACKEND_POSIX) {
    const char *oldp = wasi_posix_path(old_full, &old_len_full);
    const char *newp = wasi_posix_path(new_full, &new_len_full);
    uint32_t req_size = 12 + old_len_full + new_len_full;
    uint8_t *req = malloc(req_size);
    if (!req) {
      free(old_full);
      free(new_full);
      m3ApiReturn(WASI_ERRNO_NOMEM);
    }
    *(uint32_t *)(req + 0) = POSIX_PATH_RENAME;
    *(uint32_t *)(req + 4) = old_len_full;
    memmove(req + 8, oldp, old_len_full);
    *(uint32_t *)(req + 8 + old_len_full) = new_len_full;
    memmove(req + 12 + old_len_full, newp, new_len_full);

    uint32_t resp = 0;
    int r = wasi_rump_rpc_read("/srv/rump/posix/path_rename", req, req_size,
                               &resp, sizeof(resp));
    free(req);
    free(old_full);
    free(new_full);
    if (r < (int)sizeof(resp))
      m3ApiReturn(WASI_ERRNO_IO);
    if (resp != 0)
      m3ApiReturn(wasi_errno_from_posix(resp));
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  } else {
    /* Native Backend - Rename via wstat (name change) */
    /* Note: Plan 9 wstat only supports renaming within the same directory */
    Chan *c = namec(old_full, Aaccess, 0, 0);
    if (waserror()) {
      free(old_full);
      free(new_full);
      m3ApiReturn(WASI_ERRNO_NOENT);
    }
    Dir *d = dirchanstat(c);
    if (d == nil) {
      cclose(c);
      poperror(); /* namec */
      free(old_full);
      free(new_full);
      m3ApiReturn(WASI_ERRNO_IO);
    }
    /* Extract new name from new_full */
    char *slash = strrchr(new_full, '/');
    char *new_name = slash ? slash + 1 : new_full;

    /* Update Dir */
    /* We should ideally nulldir and only set name/type/dev... but dirchanstat
     * gives full info */
    /* Let's try just updating the name */
    free(d->name);
    d->name = malloc(strlen(new_name) + 1);
    if (d->name) {
      strcpy(d->name, new_name);
      uchar buf[256]; /* Should be enough for header + string */
      /* Proper sizing? */
      uint n = convD2M(d, buf, sizeof(buf));
      if (n <= sizeof(buf)) {
        devwstat(c, buf, n);
      }
    }

    free(d);
    cclose(c);
    poperror(); /* namec */
    free(old_full);
    free(new_full);

    /* Check if wstat failed? devwstat calls error() on failure, so it would
     * trigger waserror above if we wrapped devwstat? */
    /* Attempting to define a scope for devwstat error handling */
    /* NOTE: namec was the error barrier. Let's Refine logic. */
    /* Rethink: devwstat throws error. Need to catch it. */
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }
}

/* wasi_path_link(old_fd, old_path, old_len, new_fd, new_path, new_len) */
m3ApiRawFunction(wasi_snapshot_preview1_path_link) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, old_fd)
      m3ApiGetArg(uint32_t, old_path) m3ApiGetArg(uint32_t, old_len)
          m3ApiGetArg(int32_t, new_fd) m3ApiGetArg(uint32_t, new_path)
              m3ApiGetArg(uint32_t, new_len) Proc *p = up;
  (void)old_path;
  (void)old_len;
  (void)new_path;
  (void)new_len;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, old_fd, WASI_RIGHT_PATH_LINK_SOURCE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  cap = wasi_require_fd(ctx, new_fd, WASI_RIGHT_PATH_LINK_TARGET);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  char *oldp = (char *)m3ApiOffsetToPtr(old_path);
  char *newp = (char *)m3ApiOffsetToPtr(new_path);
  char *old_full = nil;
  char *new_full = nil;
  uint32_t perr = wasi_build_path(ctx, old_fd, oldp, old_len, &old_full);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);
  perr = wasi_build_path(ctx, new_fd, newp, new_len, &new_full);
  if (perr != WASI_ERRNO_SUCCESS) {
    free(old_full);
    m3ApiReturn(perr);
  }

  uint32_t old_len_full = (uint32_t)strlen(old_full);
  uint32_t new_len_full = (uint32_t)strlen(new_full);
  int backend = ctx->fds[old_fd].backend;
  if (backend == WASI_BACKEND_POSIX) {
    const char *oldp = wasi_posix_path(old_full, &old_len_full);
    const char *newp = wasi_posix_path(new_full, &new_len_full);
    uint32_t req_size = 12 + old_len_full + new_len_full;
    uint8_t *req = malloc(req_size);
    if (!req) {
      free(old_full);
      free(new_full);
      m3ApiReturn(WASI_ERRNO_NOMEM);
    }
    *(uint32_t *)(req + 0) = POSIX_PATH_LINK;
    *(uint32_t *)(req + 4) = old_len_full;
    memmove(req + 8, oldp, old_len_full);
    *(uint32_t *)(req + 8 + old_len_full) = new_len_full;
    memmove(req + 12 + old_len_full, newp, new_len_full);

    uint32_t resp = 0;
    int r = wasi_rump_rpc_read("/srv/rump/posix/path_link", req, req_size,
                               &resp, sizeof(resp));
    free(req);
    free(old_full);
    free(new_full);
    if (r < (int)sizeof(resp))
      m3ApiReturn(WASI_ERRNO_IO);
    if (resp != 0)
      m3ApiReturn(wasi_errno_from_posix(resp));
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }
  uint32_t req_size = 12 + old_len_full + new_len_full;
  uint8_t *req = malloc(req_size);
  if (!req) {
    free(old_full);
    free(new_full);
    m3ApiReturn(WASI_ERRNO_NOMEM);
  }
  *(uint32_t *)(req + 0) = POSIX_PATH_LINK;
  *(uint32_t *)(req + 4) = old_len_full;
  memmove(req + 8, old_full, old_len_full);
  *(uint32_t *)(req + 8 + old_len_full) = new_len_full;
  memmove(req + 12 + old_len_full, new_full, new_len_full);

  uint32_t resp = 0;
  int r = wasi_rump_rpc_read("/srv/rump/posix/path_link", req, req_size, &resp,
                             sizeof(resp));
  free(req);
  free(old_full);
  free(new_full);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_path_symlink(old_path, old_len, dirfd, new_path, new_len) */
m3ApiRawFunction(wasi_snapshot_preview1_path_symlink) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, old_path)
      m3ApiGetArg(uint32_t, old_len) m3ApiGetArg(int32_t, dirfd)
          m3ApiGetArg(uint32_t, new_path) m3ApiGetArg(uint32_t, new_len)
              Proc *p = up;
  (void)old_path;
  (void)old_len;
  (void)new_path;
  (void)new_len;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_SYMLINK);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  char *oldp = (char *)m3ApiOffsetToPtr(old_path);
  char *newp = (char *)m3ApiOffsetToPtr(new_path);
  char *new_full = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, newp, new_len, &new_full);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);

  int backend = ctx->fds[dirfd].backend;
  const char *old_use = oldp;
  uint32_t old_len_full = old_len;
  const char *new_use = new_full;
  uint32_t new_len_full = (uint32_t)strlen(new_full);
  if (backend == WASI_BACKEND_POSIX) {
    if (wasi_is_posix_path(oldp, old_len)) {
      uint32_t skip = (uint32_t)(sizeof(wasi_posix_root_path) - 1);
      if (old_len > skip) {
        old_use = oldp + skip;
        old_len_full = old_len - skip;
      } else {
        old_use = "/";
        old_len_full = 1;
      }
    }
    new_use = wasi_posix_path(new_full, &new_len_full);
  }
  uint32_t req_size = 12 + old_len_full + new_len_full;
  uint8_t *req = malloc(req_size);
  if (!req) {
    free(new_full);
    m3ApiReturn(WASI_ERRNO_NOMEM);
  }
  *(uint32_t *)(req + 0) = POSIX_PATH_SYMLINK;
  *(uint32_t *)(req + 4) = old_len_full;
  memmove(req + 8, old_use, old_len_full);
  *(uint32_t *)(req + 8 + old_len_full) = new_len_full;
  memmove(req + 12 + old_len_full, new_use, new_len_full);

  uint32_t resp = 0;
  int r = wasi_rump_rpc_read("/srv/rump/posix/path_symlink", req, req_size,
                             &resp, sizeof(resp));
  free(req);
  free(new_full);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);
  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_path_readlink(dirfd, path, path_len, buf, buf_len, bufused) */
m3ApiRawFunction(wasi_snapshot_preview1_path_readlink) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, path_ptr) m3ApiGetArg(uint32_t, path_len)
          m3ApiGetArg(uint32_t, buf_ptr) m3ApiGetArg(uint32_t, buf_len)
              m3ApiGetArg(uint32_t, bufused_ptr) Proc *p = up;
  (void)path_ptr;
  (void)path_len;
  (void)buf_ptr;
  (void)buf_len;
  m3ApiCheckMem(bufused_ptr, sizeof(uint32_t));
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_READLINK);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS)
    m3ApiReturn(perr);

  int backend = ctx->fds[dirfd].backend;
  uint32_t full_len = 0;
  const char *posix_path = (backend == WASI_BACKEND_POSIX)
                               ? wasi_posix_path(fullpath, &full_len)
                               : fullpath;
  if (backend != WASI_BACKEND_POSIX)
    full_len = (uint32_t)strlen(fullpath);

  uint32_t extra = buf_len;
  uint32_t resp_len = 8 + buf_len;
  uint8_t *resp = malloc(resp_len);
  if (!resp) {
    free(fullpath);
    m3ApiReturn(WASI_ERRNO_NOMEM);
  }
  int r = wasi_rump_path_req("/srv/rump/posix/path_readlink",
                             POSIX_PATH_READLINK, posix_path, full_len, &extra,
                             sizeof(extra), resp, resp_len);
  free(fullpath);
  if (r < 8) {
    free(resp);
    m3ApiReturn(WASI_ERRNO_IO);
  }
  uint32_t err = *(uint32_t *)resp;
  if (err != 0) {
    free(resp);
    m3ApiReturn(wasi_errno_from_posix(err));
  }
  uint32_t count = *(uint32_t *)(resp + 4);
  if (count > buf_len)
    count = buf_len;
  memmove(m3ApiOffsetToPtr(buf_ptr), resp + 8, count);
  free(resp);
  m3ApiWriteMem32(bufused_ptr, count);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_poll_oneoff(in, out, nsubscriptions, nevents) */
m3ApiRawFunction(wasi_snapshot_preview1_poll_oneoff) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(uint32_t, in_ptr)
      m3ApiGetArg(uint32_t, out_ptr) m3ApiGetArg(uint32_t, nsubscriptions)
          m3ApiGetArg(uint32_t, nevents_ptr)

              m3ApiCheckMem(nevents_ptr, sizeof(uint32_t));
  if (nsubscriptions > (0xffffffffu / WASI_SUBSCRIPTION_SIZE))
    m3ApiReturn(WASI_ERRNO_INVAL);
  uint32_t in_len = nsubscriptions * WASI_SUBSCRIPTION_SIZE;
  uint32_t out_len = nsubscriptions * WASI_EVENT_SIZE;
  m3ApiCheckMem(in_ptr, in_len);
  m3ApiCheckMem(out_ptr, out_len);

  uint32_t req_size = 8 + in_len;
  uint8_t *req = malloc(req_size);
  if (!req)
    m3ApiReturn(WASI_ERRNO_NOMEM);
  *(uint32_t *)(req + 0) = POSIX_POLL_ONEOFF;
  *(uint32_t *)(req + 4) = nsubscriptions;
  memmove(req + 8, m3ApiOffsetToPtr(in_ptr), in_len);

  uint32_t resp_size = 8 + out_len;
  uint8_t *resp = malloc(resp_size);
  if (!resp) {
    free(req);
    m3ApiReturn(WASI_ERRNO_NOMEM);
  }

  int r = wasi_rump_rpc_read("/srv/rump/posix/poll_oneoff", req, req_size, resp,
                             resp_size);
  free(req);
  if (r < 8) {
    free(resp);
    m3ApiReturn(WASI_ERRNO_IO);
  }

  uint32_t err = *(uint32_t *)resp;
  uint32_t nevents = *(uint32_t *)(resp + 4);
  if (err != 0) {
    free(resp);
    m3ApiReturn(wasi_errno_from_posix(err));
  }
  if (nevents > nsubscriptions)
    nevents = nsubscriptions;
  if (nevents > 0) {
    memmove(m3ApiOffsetToPtr(out_ptr), resp + 8, nevents * WASI_EVENT_SIZE);
  }
  free(resp);
  m3ApiWriteMem32(nevents_ptr, nevents);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_sock_accept(fd, flags, newfd) */
m3ApiRawFunction(wasi_snapshot_preview1_sock_accept) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, flags) m3ApiGetArg(uint32_t, newfd_ptr) Proc *p =
          up;
  (void)flags;
  m3ApiCheckMem(newfd_ptr, sizeof(uint32_t));
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_SOCK_ACCEPT);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_NATIVE)
    m3ApiReturn(WASI_ERRNO_NOTSUP);

  uint32_t req[3];
  uint32_t resp[2];
  req[0] = POSIX_SOCK_ACCEPT;
  req[1] = (uint32_t)fd;
  req[2] = flags;

  int r = wasi_rump_rpc_read("/srv/rump/posix/sock_accept", req, sizeof(req),
                             resp, sizeof(resp));
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);

  if (resp[0] != 0)
    m3ApiReturn(wasi_errno_from_posix(resp[0]));

  m3ApiWriteMem32(newfd_ptr, resp[1]);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_sock_recv(fd, ri_data, ri_data_len, ri_flags, ro_datalen, ro_flags) */
m3ApiRawFunction(wasi_snapshot_preview1_sock_recv) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, ri_data) m3ApiGetArg(uint32_t, ri_data_len)
          m3ApiGetArg(uint32_t, ri_flags) m3ApiGetArg(uint32_t, ro_datalen)
              m3ApiGetArg(uint32_t, ro_flags) Proc *p = up;
  m3ApiCheckMem(ro_datalen, sizeof(uint32_t));
  m3ApiCheckMem(ro_flags, sizeof(uint32_t));
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_READ);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  uint32_t iov_size = 0;
  if (wasi_iovecs_size(ri_data_len, &iov_size) < 0)
    m3ApiReturn(WASI_ERRNO_INVAL);
  m3ApiCheckMem(ri_data, iov_size);

  uint32_t total = 0;
  for (uint32_t i = 0; i < ri_data_len; i++) {
    uint32_t iov_addr = ri_data + (i * 8);
    uint32_t len = m3ApiReadMem32(iov_addr + 4);
    if (len > WASI_RUMP_IO_MAX - total)
      m3ApiReturn(WASI_ERRNO_INVAL);
    total += len;
  }

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_NATIVE) {
    uint32_t remaining = total;
    uint32_t count = 0;
    for (uint32_t i = 0; i < ri_data_len && remaining > 0; i++) {
      uint32_t iov_addr = ri_data + (i * 8);
      uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
      uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
      uint32_t want = buf_len;
      if (want > remaining)
        want = remaining;
      if (want == 0)
        continue;
      m3ApiCheckMem(buf_ptr, want);
      long n = kread(ctx->fds[fd].lux9_fid, m3ApiOffsetToPtr(buf_ptr), want);
      if (n < 0)
        m3ApiReturn(WASI_ERRNO_IO);
      count += (uint32_t)n;
      remaining -= (uint32_t)n;
      if ((uint32_t)n < want)
        break;
    }
    m3ApiWriteMem32(ro_datalen, count);
    m3ApiWriteMem32(ro_flags, 0);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }

  uint32_t req[4];
  req[0] = POSIX_SOCK_RECV;
  req[1] = (uint32_t)fd;
  req[2] = ri_flags;
  req[3] = total;

  uint8_t resp[12 + WASI_RUMP_IO_MAX];
  int r = wasi_rump_rpc_read("/srv/rump/posix/sock_recv", req, sizeof(req),
                             resp, 12 + total);
  if (r < 4)
    m3ApiReturn(WASI_ERRNO_IO);

  uint32_t err = *(uint32_t *)resp;
  if (err != 0)
    m3ApiReturn(wasi_errno_from_posix(err));

  uint32_t rof = 0;
  uint32_t count = 0;
  if (r >= 12) {
    rof = *(uint32_t *)(resp + 4);
    count = *(uint32_t *)(resp + 8);
  }
  if (count > total)
    count = total;

  uint32_t remaining = count;
  uint32_t off = 12;
  for (uint32_t i = 0; i < ri_data_len && remaining > 0; i++) {
    uint32_t iov_addr = ri_data + (i * 8);
    uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
    uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
    uint32_t copy_len = buf_len;
    if (copy_len > remaining)
      copy_len = remaining;
    if (copy_len == 0)
      continue;
    m3ApiCheckMem(buf_ptr, copy_len);
    memmove(m3ApiOffsetToPtr(buf_ptr), resp + off, copy_len);
    off += copy_len;
    remaining -= copy_len;
  }

  m3ApiWriteMem32(ro_datalen, count);
  m3ApiWriteMem32(ro_flags, rof);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_sock_send(fd, si_data, si_data_len, si_flags, so_datalen) */
m3ApiRawFunction(wasi_snapshot_preview1_sock_send) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, si_data) m3ApiGetArg(uint32_t, si_data_len)
          m3ApiGetArg(uint32_t, si_flags) m3ApiGetArg(uint32_t, so_datalen)
              Proc *p = up;
  m3ApiCheckMem(so_datalen, sizeof(uint32_t));
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_WRITE);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  uint32_t iov_size = 0;
  if (wasi_iovecs_size(si_data_len, &iov_size) < 0)
    m3ApiReturn(WASI_ERRNO_INVAL);
  m3ApiCheckMem(si_data, iov_size);

  uint32_t total = 0;
  for (uint32_t i = 0; i < si_data_len; i++) {
    uint32_t iov_addr = si_data + (i * 8);
    uint32_t len = m3ApiReadMem32(iov_addr + 4);
    if (len > WASI_RUMP_IO_MAX - total)
      m3ApiReturn(WASI_ERRNO_INVAL);
    total += len;
  }

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_NATIVE) {
    uint32_t written = 0;
    for (uint32_t i = 0; i < si_data_len; i++) {
      uint32_t iov_addr = si_data + (i * 8);
      uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
      uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
      if (buf_len == 0)
        continue;
      m3ApiCheckMem(buf_ptr, buf_len);
      long n =
          kwrite(ctx->fds[fd].lux9_fid, m3ApiOffsetToPtr(buf_ptr), buf_len);
      if (n < 0)
        m3ApiReturn(WASI_ERRNO_IO);
      written += (uint32_t)n;
      if ((uint32_t)n < buf_len)
        break;
    }
    m3ApiWriteMem32(so_datalen, written);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }

  uint32_t req_size = 16 + total;
  uint8_t *req = malloc(req_size);
  if (!req)
    m3ApiReturn(WASI_ERRNO_NOMEM);

  *(uint32_t *)(req + 0) = POSIX_SOCK_SEND;
  *(uint32_t *)(req + 4) = (uint32_t)fd;
  *(uint32_t *)(req + 8) = si_flags;
  *(uint32_t *)(req + 12) = total;

  uint32_t off = 16;
  for (uint32_t i = 0; i < si_data_len; i++) {
    uint32_t iov_addr = si_data + (i * 8);
    uint32_t buf_ptr = m3ApiReadMem32(iov_addr);
    uint32_t buf_len = m3ApiReadMem32(iov_addr + 4);
    if (buf_len == 0)
      continue;
    m3ApiCheckMem(buf_ptr, buf_len);
    memmove(req + off, m3ApiOffsetToPtr(buf_ptr), buf_len);
    off += buf_len;
  }

  uint32_t resp[2];
  int r = wasi_rump_rpc_read("/srv/rump/posix/sock_send", req, req_size, resp,
                             sizeof(resp));
  free(req);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);

  uint32_t err = resp[0];
  if (err != 0)
    m3ApiReturn(wasi_errno_from_posix(err));

  m3ApiWriteMem32(so_datalen, resp[1]);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_sock_shutdown(fd, how) */
m3ApiRawFunction(wasi_snapshot_preview1_sock_shutdown) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd) m3ApiGetArg(uint32_t, how)
      Proc *p = up;
  (void)how;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_SOCK_SHUTDOWN);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_NATIVE)
    m3ApiReturn(WASI_ERRNO_SUCCESS);

  uint32_t req[3];
  uint32_t resp = 0;
  req[0] = POSIX_SOCK_SHUTDOWN;
  req[1] = (uint32_t)fd;
  req[2] = how;

  int r = wasi_rump_rpc_read("/srv/rump/posix/sock_shutdown", req, sizeof(req),
                             &resp, sizeof(resp));
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);

  if (resp != 0)
    m3ApiReturn(wasi_errno_from_posix(resp));

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}
/* wasi_fd_filestat_get(fd, buf) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_filestat_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, buf_ptr)

          m3ApiCheckMem(buf_ptr, WASI_FILESTAT_SIZE);

  if (fd < 0)
    m3ApiReturn(WASI_ERRNO_BADF);
  if (!up->wasm.initialized || !up->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)up->wasm.wasi_ctx;
  uint32_t cap = wasi_require_fd(ctx, fd, WASI_RIGHT_FD_FILESTAT_GET);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);

  if (wasi_fd_backend(ctx, fd) == WASI_BACKEND_POSIX) {
    Chan *c = nil;
    Dir *d = nil;
    if (waserror()) {
      if (c)
        cclose(c);
      free(d);
      m3ApiReturn(WASI_ERRNO_IO);
    }
    c = fdtochan(ctx->fds[fd].lux9_fid, -1, 0, 1);
    d = dirchanstat(c);
    if (d == nil) {
      cclose(c);
      poperror();
      m3ApiReturn(WASI_ERRNO_IO);
    }

    uint8_t filetype = (d->mode & DMDIR) ? WASI_FILETYPE_DIRECTORY
                                         : WASI_FILETYPE_REGULAR_FILE;
    m3ApiWriteMem64(buf_ptr + 0, 0);
    m3ApiWriteMem64(buf_ptr + 8, d->qid.path);
    m3ApiWriteMem8(buf_ptr + 16, filetype);
    m3ApiWriteMem64(buf_ptr + 24, 1);
    m3ApiWriteMem64(buf_ptr + 32, (uint64_t)d->length);
    m3ApiWriteMem64(buf_ptr + 40, (uint64_t)d->atime * 1000000000ULL);
    m3ApiWriteMem64(buf_ptr + 48, (uint64_t)d->mtime * 1000000000ULL);
    m3ApiWriteMem64(buf_ptr + 56, (uint64_t)d->mtime * 1000000000ULL);

    free(d);
    cclose(c);
    poperror();
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }

  uint32_t req[2];
  uint8_t resp[4 + sizeof(rump_stat)];
  req[0] = POSIX_FDSTAT_GET;
  req[1] = (uint32_t)fd;

  int r = wasi_rump_rpc("/srv/rump/posix/fdstat", req, sizeof(req), resp,
                        (uint32_t)sizeof(resp));
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);

  uint32_t err = *(uint32_t *)resp;
  if (err != 0)
    m3ApiReturn(wasi_errno_from_posix(err));

  rump_stat *st = (rump_stat *)(resp + 4);

  m3ApiWriteMem64(buf_ptr + 0, st->st_dev);
  m3ApiWriteMem64(buf_ptr + 8, st->st_ino);
  m3ApiWriteMem8(buf_ptr + 16, wasi_filetype_from_mode(st->st_mode));
  m3ApiWriteMem64(buf_ptr + 24, st->st_nlink);
  m3ApiWriteMem64(buf_ptr + 32, st->st_size);
  m3ApiWriteMem64(buf_ptr + 40, wasi_timespec_to_ns(&st->st_atimespec));
  m3ApiWriteMem64(buf_ptr + 48, wasi_timespec_to_ns(&st->st_mtimespec));
  m3ApiWriteMem64(buf_ptr + 56, wasi_timespec_to_ns(&st->st_ctimespec));

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_path_filestat_get(dirfd, dirflags, path, path_len, buf) */
m3ApiRawFunction(wasi_snapshot_preview1_path_filestat_get) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, dirfd)
      m3ApiGetArg(uint32_t, dirflags) m3ApiGetArg(uint32_t, path_ptr)
          m3ApiGetArg(uint32_t, path_len) m3ApiGetArg(uint32_t, buf_ptr)

              m3ApiCheckMem(path_ptr, path_len);
  m3ApiCheckMem(buf_ptr, WASI_FILESTAT_SIZE);

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  uint32_t cap = wasi_require_fd(ctx, dirfd, WASI_RIGHT_PATH_FILESTAT_GET);
  if (cap != WASI_ERRNO_SUCCESS)
    m3ApiReturn(cap);
  if (!ctx->fds[dirfd].is_dir)
    m3ApiReturn(WASI_ERRNO_NOTDIR);
  (void)dirflags;

  char *path = (char *)m3ApiOffsetToPtr(path_ptr);
  char *fullpath = nil;
  uint32_t perr = wasi_build_path(ctx, dirfd, path, path_len, &fullpath);
  if (perr != WASI_ERRNO_SUCCESS) {
    m3ApiReturn(perr);
  }

  int backend = ctx->fds[dirfd].backend;
  uint32_t full_len = 0;
  const char *posix_path = (backend == WASI_BACKEND_POSIX)
                               ? wasi_posix_path(fullpath, &full_len)
                               : fullpath;
  if (backend != WASI_BACKEND_POSIX)
    full_len = (uint32_t)strlen(fullpath);

  if (backend == WASI_BACKEND_NATIVE) {
    Chan *c = nil;
    Dir *d = nil;
    if (waserror()) {
      if (c)
        cclose(c);
      free(d);
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_IO);
    }
    c = namec(fullpath, Aaccess, 0, 0);
    d = dirchanstat(c);
    if (d == nil) {
      cclose(c);
      poperror();
      free(fullpath);
      m3ApiReturn(WASI_ERRNO_IO);
    }

    uint8_t filetype = (d->mode & DMDIR) ? WASI_FILETYPE_DIRECTORY
                                         : WASI_FILETYPE_REGULAR_FILE;
    m3ApiWriteMem64(buf_ptr + 0, 0);                    // st_dev
    m3ApiWriteMem64(buf_ptr + 8, d->qid.path);          // st_ino
    m3ApiWriteMem8(buf_ptr + 16, filetype);             // st_filetype
    m3ApiWriteMem64(buf_ptr + 24, 1);                   // st_nlink
    m3ApiWriteMem64(buf_ptr + 32, (uint64_t)d->length); // st_size
    m3ApiWriteMem64(buf_ptr + 40,
                    (uint64_t)d->atime * 1000000000ULL); // st_atim
    m3ApiWriteMem64(buf_ptr + 48,
                    (uint64_t)d->mtime * 1000000000ULL); // st_mtim
    m3ApiWriteMem64(buf_ptr + 56,
                    (uint64_t)d->mtime * 1000000000ULL); // st_ctim

    free(d);
    cclose(c);
    poperror();
    free(fullpath);
    m3ApiReturn(WASI_ERRNO_SUCCESS);
  }

  uint32_t req_size = 8 + full_len;
  uint8_t *req = malloc(req_size);
  if (!req) {
    free(fullpath);
    m3ApiReturn(WASI_ERRNO_NOMEM);
  }
  *(uint32_t *)(req + 0) = POSIX_PATHSTAT_GET;
  *(uint32_t *)(req + 4) = full_len;
  memmove(req + 8, posix_path, full_len);
  free(fullpath);

  uint8_t resp[4 + sizeof(rump_stat)];
  int r = wasi_rump_rpc("/srv/rump/posix/pathstat", req, req_size, resp,
                        (uint32_t)sizeof(resp));
  free(req);
  if (r < (int)sizeof(resp))
    m3ApiReturn(WASI_ERRNO_IO);

  uint32_t err = *(uint32_t *)resp;
  if (err != 0)
    m3ApiReturn(wasi_errno_from_posix(err));

  rump_stat *st = (rump_stat *)(resp + 4);

  m3ApiWriteMem64(buf_ptr + 0, st->st_dev);
  m3ApiWriteMem64(buf_ptr + 8, st->st_ino);
  m3ApiWriteMem8(buf_ptr + 16, wasi_filetype_from_mode(st->st_mode));
  m3ApiWriteMem64(buf_ptr + 24, st->st_nlink);
  m3ApiWriteMem64(buf_ptr + 32, st->st_size);
  m3ApiWriteMem64(buf_ptr + 40, wasi_timespec_to_ns(&st->st_atimespec));
  m3ApiWriteMem64(buf_ptr + 48, wasi_timespec_to_ns(&st->st_mtimespec));
  m3ApiWriteMem64(buf_ptr + 56, wasi_timespec_to_ns(&st->st_ctimespec));

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* ========== Linking (The Shim) ========== */

static M3Result wasi_link_if(IM3Module module, u32int allow_mask, u32int flag,
                             const char *ns, const char *name, const char *sig,
                             M3RawCall func) {
  if ((allow_mask & flag) == 0)
    return m3Err_none;
  return m3_LinkRawFunction(module, ns, name, sig, func);
}

M3Result LinkWasi(IM3Module module, u32int allow_mask) {
  M3Result result = m3Err_none;

  const char *wasi = "wasi_snapshot_preview1";

  // args_sizes_get: (i32*, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_ARGS, wasi, "args_sizes_get",
                   "i(**)", &wasi_snapshot_preview1_args_sizes_get);
  if (result)
    return result;

  // args_get: (i32*, i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_ARGS, wasi, "args_get",
                        "i(**)", &wasi_snapshot_preview1_args_get);
  if (result)
    return result;

  // environ_sizes_get: (i32*, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_ARGS, wasi,
                        "environ_sizes_get", "i(**)",
                        &wasi_snapshot_preview1_environ_sizes_get);
  if (result)
    return result;

  // environ_get: (i32*, i32) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_ARGS, wasi, "environ_get",
                   "i(**)", &wasi_snapshot_preview1_environ_get);
  if (result)
    return result;

  // fd_prestat_get: (i32, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_DIR, wasi, "fd_prestat_get",
                   "i(i*)", &wasi_snapshot_preview1_fd_prestat_get);
  if (result)
    return result;

  // fd_prestat_dir_name: (i32, i32, i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_DIR, wasi,
                        "fd_prestat_dir_name", "i(i*i)",
                        &wasi_snapshot_preview1_fd_prestat_dir_name);
  if (result)
    return result;

  // random_get: (i32, i32) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_RANDOM, wasi, "random_get",
                   "i(*i)", &wasi_snapshot_preview1_random_get);
  if (result)
    return result;

  // clock_res_get: (i32, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_CLOCK, wasi, "clock_res_get",
                   "i(i*)", &wasi_snapshot_preview1_clock_res_get);
  if (result)
    return result;

  // clock_time_get: (i32, i64, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_CLOCK, wasi, "clock_time_get",
                   "i(iI*)", &wasi_snapshot_preview1_clock_time_get);
  if (result)
    return result;

  // fd_write: (i32, i32*, i32, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_write",
                        "i(i*i*)", &wasi_snapshot_preview1_fd_write);
  if (result)
    return result;

  // fd_read: (i32, i32*, i32, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_read",
                        "i(i*i*)", &wasi_snapshot_preview1_fd_read);
  if (result)
    return result;

  // fd_pread: (i32, i32*, i32, i64, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_pread",
                        "i(i*iI*)", &wasi_snapshot_preview1_fd_pread);
  if (result)
    return result;

  // fd_pwrite: (i32, i32*, i32, i64, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_pwrite",
                        "i(i*iI*)", &wasi_snapshot_preview1_fd_pwrite);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_advise",
                        "i(iIIi)", &wasi_snapshot_preview1_fd_advise);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_allocate",
                        "i(iII)", &wasi_snapshot_preview1_fd_allocate);
  if (result)
    return result;

  // fd_seek: (i32, i64, i32, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_seek",
                        "i(iIi*)", &wasi_snapshot_preview1_fd_seek);
  if (result)
    return result;

  // fd_close: (i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_close",
                        "i(i)", &wasi_snapshot_preview1_fd_close);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_renumber",
                        "i(ii)", &wasi_snapshot_preview1_fd_renumber);
  if (result)
    return result;

  // proc_exit: (i32) -> void
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PROC, wasi, "proc_exit",
                        "v(i)", &wasi_snapshot_preview1_proc_exit);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PROC, wasi, "proc_raise",
                        "i(i)", &wasi_snapshot_preview1_proc_raise);
  if (result)
    return result;

  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_PROC, wasi, "sched_yield",
                   "i()", &wasi_snapshot_preview1_sched_yield);
  if (result)
    return result;

  // path_open: (i32, i32, i32*, i32, i32, i64, i64, i32, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi, "path_open",
                        "i(ii*iiIIi*)", &wasi_snapshot_preview1_path_open);
  if (result)
    return result;

  // fd_fdstat_get: (i32, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_fdstat_get",
                   "i(i*)", &wasi_snapshot_preview1_fd_fdstat_get);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi,
                        "fd_fdstat_set_flags", "i(ii)",
                        &wasi_snapshot_preview1_fd_fdstat_set_flags);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi,
                        "fd_fdstat_set_rights", "i(iII)",
                        &wasi_snapshot_preview1_fd_fdstat_set_rights);
  if (result)
    return result;

  // fd_readdir: (i32, i32*, i32, i64, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_DIR, wasi, "fd_readdir",
                        "i(i*iI*)", &wasi_snapshot_preview1_fd_readdir);
  if (result)
    return result;

  // fd_sync: (i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_sync",
                        "i(i)", &wasi_snapshot_preview1_fd_sync);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_datasync",
                        "i(i)", &wasi_snapshot_preview1_fd_datasync);
  if (result)
    return result;

  // fd_tell: (i32, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_tell",
                        "i(i*)", &wasi_snapshot_preview1_fd_tell);
  if (result)
    return result;

  // fd_filestat_get: (i32, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi, "fd_filestat_get",
                   "i(i*)", &wasi_snapshot_preview1_fd_filestat_get);
  if (result)
    return result;

  // fd_filestat_set_size: (i32, i64) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi,
                        "fd_filestat_set_size", "i(iI)",
                        &wasi_snapshot_preview1_fd_filestat_set_size);
  if (result)
    return result;

  // fd_filestat_set_times: (i32, i64, i64, i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_FD, wasi,
                        "fd_filestat_set_times", "i(iIIi)",
                        &wasi_snapshot_preview1_fd_filestat_set_times);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi,
                        "path_filestat_set_size", "i(ii*iI)",
                        &wasi_snapshot_preview1_path_filestat_set_size);
  if (result)
    return result;

  // path_filestat_get: (i32, i32, i32*, i32, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi,
                        "path_filestat_get", "i(ii*i*)",
                        &wasi_snapshot_preview1_path_filestat_get);
  if (result)
    return result;

  // path_filestat_set_times: (i32, i32, i32*, i32, i64, i64, i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi,
                        "path_filestat_set_times", "i(ii*iIIi)",
                        &wasi_snapshot_preview1_path_filestat_set_times);
  if (result)
    return result;

  // path_create_directory: (i32, i32*, i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi,
                        "path_create_directory", "i(i*i)",
                        &wasi_snapshot_preview1_path_create_directory);
  if (result)
    return result;

  // path_remove_directory: (i32, i32*, i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi,
                        "path_remove_directory", "i(i*i)",
                        &wasi_snapshot_preview1_path_remove_directory);
  if (result)
    return result;

  // path_unlink_file: (i32, i32*, i32) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi,
                        "path_unlink_file", "i(i*i)",
                        &wasi_snapshot_preview1_path_unlink_file);
  if (result)
    return result;

  // path_rename: (i32, i32*, i32, i32, i32*, i32) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi, "path_rename",
                   "i(i*ii*i)", &wasi_snapshot_preview1_path_rename);
  if (result)
    return result;

  result = wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi, "path_link",
                        "i(i*ii*i)", &wasi_snapshot_preview1_path_link);
  if (result)
    return result;

  // path_symlink: (i32*, i32, i32, i32*, i32) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi, "path_symlink",
                   "i(*ii*i)", &wasi_snapshot_preview1_path_symlink);
  if (result)
    return result;

  // path_readlink: (i32, i32*, i32, i32*, i32, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_PATH, wasi, "path_readlink",
                   "i(i*i*i*)", &wasi_snapshot_preview1_path_readlink);
  if (result)
    return result;

  // poll_oneoff: (i32*, i32*, i32, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_POLL, wasi, "poll_oneoff",
                   "i(**i*)", &wasi_snapshot_preview1_poll_oneoff);
  if (result)
    return result;

  // sock_accept: (i32, i32, i32*) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_SOCK, wasi, "sock_accept",
                   "i(ii*)", &wasi_snapshot_preview1_sock_accept);
  if (result)
    return result;

  // sock_recv: (i32, i32*, i32, i32, i32*, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_SOCK, wasi, "sock_recv",
                        "i(i*ii**)", &wasi_snapshot_preview1_sock_recv);
  if (result)
    return result;

  // sock_send: (i32, i32*, i32, i32, i32*) -> i32
  result = wasi_link_if(module, allow_mask, WASI_ALLOW_SOCK, wasi, "sock_send",
                        "i(i*ii*)", &wasi_snapshot_preview1_sock_send);
  if (result)
    return result;

  // sock_shutdown: (i32, i32) -> i32
  result =
      wasi_link_if(module, allow_mask, WASI_ALLOW_SOCK, wasi, "sock_shutdown",
                   "i(ii)", &wasi_snapshot_preview1_sock_shutdown);
  if (result)
    return result;

  return result;
}
