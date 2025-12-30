/* wasi_lux9_shim.c - WASI to Lux9 9P Shim Implementation
 *
 * Implements wasi_snapshot_preview1 host functions by mapping them
 * to Lux9 9P file operations.
 */

#include "wasi_lux9_shim.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/portlib.h"
#include "../include/u.h"

#ifndef nil
#define nil ((void *)0)
#endif

/* Provide standard types if missing */
/* Standard types provided by wasi_lux9_shim.h */

/* WASI is Little Endian usually.
 * wasm3.h defines m3ApiReadMem32 which handles endianness if configured.
 * We rely on wasm3 macros.
 */

/* ========== WASI Constants ========== */
#define WASI_ERRNO_SUCCESS 0
#define WASI_ERRNO_BADF 8
#define WASI_ERRNO_INVAL 28

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

/* ========== Context Management ========== */

void wasi_lux9_init_context(wasi_context_t *ctx) {
  if (!ctx)
    return;
  memset(ctx, 0, sizeof(wasi_context_t));

  /* Pre-populate stdio (0, 1, 2) */
  ctx->fds[0].is_open = 1;
  ctx->fds[0].lux9_fid = 0; /* stdin */
  ctx->fds[1].is_open = 1;
  ctx->fds[1].lux9_fid = 1; /* stdout */
  ctx->fds[2].is_open = 1;
  ctx->fds[2].lux9_fid = 2; /* stderr */

  /* Pre-populate Root (3) */
  /* Try to open real root */
  int rootfd = kopen("/", 0); /* OREAD */
  if (rootfd >= 0) {
    ctx->fds[3].is_open = 1;
    ctx->fds[3].lux9_fid = rootfd;
    ctx->fds[3].is_dir = 1;
    print("WASI: Opened root at fd 3 (kernel fd %d)\n", rootfd);
  } else {
    print("WASI: Failed to open root, using stub\n");
    ctx->fds[3].is_open = 1;
    ctx->fds[3].lux9_fid = 3;
    ctx->fds[3].is_dir = 1;
  }
}

void wasi_lux9_destroy_context(wasi_context_t *ctx) {
  if (!ctx)
    return;
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

      /* Access WASI memory */
      m3ApiCheckMem(iovs_ptr, iovs_len * 8); /* 8 bytes per iovec struct */

  /* Get Process Context */
  Proc *p = up; // Current process
  if (!p->wasm.initialized || !p->wasm.wasi_ctx) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  if (fd < 0 || fd >= WASI_MAX_FDS || !ctx->fds[fd].is_open) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }

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
        // Error
      } else {
        total_written += n;
      }
    }
  }

  m3ApiWriteMem32(nwritten_ptr, total_written);

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_proc_exit(rval) */
m3ApiRawFunction(wasi_snapshot_preview1_proc_exit) {
  m3ApiGetArg(int32_t, rval)

      print("Pretend exiting with code %d\n", rval);

  m3ApiTrap(m3Err_trapExit);
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
  if (dirfd < 0 || dirfd >= WASI_MAX_FDS || !ctx->fds[dirfd].is_open) {
    m3ApiReturn(WASI_ERRNO_BADF);
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
    m3ApiReturn(24); /* EMFILE */

  /* Modes */
  int kmode = 0; // OREAD
  /* Very basic mode mapping */
  if (oflags & 1)
    kmode =
        2; /* OCREAT -> not exactly mapping, Plan 9 uses create separately */
  if (fs_rights_base & 64)
    kmode = 1; /* Write rights -> OWRITE */
  /* If read and write, O_RDWR (2 in posix, different in simple plan9? Plan 9:
   * OREAD=0, OWRITE=1, ORDWR=2, OEXEC=3) */
  if ((fs_rights_base & 2) && (fs_rights_base & 64))
    kmode = 2;

  /* We treat path as absolute for now, ignoring dirfd relative path logic for
     simplicity unless we implement full walk from dirfd's chan. Use kopen()
     which expects full path or relative to CWD. WASM usually sends relative
     paths.
  */

  int kfd = kopen(path, kmode);
  if (kfd < 0) {
    m3ApiReturn(44); /* ENOENT */
  }

  ctx->fds[new_fd].is_open = 1;
  ctx->fds[new_fd].lux9_fid = kfd;
  ctx->fds[new_fd].is_dir = 0; /* TODO check isdir */

  m3ApiWriteMem32(fd_out_ptr, new_fd);

  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_read(fd, iovs, iovs_len, nread) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_read) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)
      m3ApiGetArg(uint32_t, iovs_ptr) m3ApiGetArg(uint32_t, iovs_len)
          m3ApiGetArg(uint32_t, nread_ptr)

              m3ApiCheckMem(iovs_ptr, iovs_len * 8);

  Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  if (fd < 0 || fd >= WASI_MAX_FDS || !ctx->fds[fd].is_open) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }

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
      // Error handling could be better (map Plan 9 error string to WASI errno)
      break;
    }
    total_read += n;
    if (n < buf_len)
      break; // Short read (EOF or block)
  }

  m3ApiWriteMem32(nread_ptr, total_read);
  m3ApiReturn(WASI_ERRNO_SUCCESS);
}

/* wasi_fd_close(fd) */
m3ApiRawFunction(wasi_snapshot_preview1_fd_close) {
  m3ApiReturnType(uint32_t) m3ApiGetArg(int32_t, fd)

      Proc *p = up;
  if (!p->wasm.initialized || !p->wasm.wasi_ctx)
    m3ApiReturn(WASI_ERRNO_BADF);
  wasi_context_t *ctx = (wasi_context_t *)p->wasm.wasi_ctx;

  if (fd < 0 || fd >= WASI_MAX_FDS || !ctx->fds[fd].is_open) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }

  fdclose(ctx->fds[fd].lux9_fid, 0);
  ctx->fds[fd].is_open = 0;

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

  if (fd < 0 || fd >= WASI_MAX_FDS || !ctx->fds[fd].is_open) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }

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

  if (fd < 0 || fd >= WASI_MAX_FDS || !ctx->fds[fd].is_open) {
    m3ApiReturn(WASI_ERRNO_BADF);
  }

  // Fill with dummy data suitable for TTY/File
  uint8_t filetype = (fd <= 2) ? 2 : 4; // 2=char device, 4=regular file
  uint16_t flags = 0;
  uint64_t rights = (uint64_t)-1;
  uint64_t rights_inh = (uint64_t)-1;

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

/* ========== Linking (The Shim) ========== */

M3Result LinkWasi(IM3Module module) {
  M3Result result = m3Err_none;

  const char *wasi = "wasi_snapshot_preview1";

  // fd_write: (i32, i32, i32, i32) -> i32
  result = m3_LinkRawFunction(module, wasi, "fd_write", "i(iiii)",
                              &wasi_snapshot_preview1_fd_write);
  if (result)
    return result;

  // fd_read: (i32, i32, i32, i32) -> i32
  result = m3_LinkRawFunction(module, wasi, "fd_read", "i(iiii)",
                              &wasi_snapshot_preview1_fd_read);
  if (result)
    return result;

  // fd_seek: (i32, i64, i32, i32) -> i32
  result = m3_LinkRawFunction(module, wasi, "fd_seek", "i(iIii)",
                              &wasi_snapshot_preview1_fd_seek);
  if (result)
    return result;

  // fd_close: (i32) -> i32
  result = m3_LinkRawFunction(module, wasi, "fd_close", "i(i)",
                              &wasi_snapshot_preview1_fd_close);
  if (result)
    return result;

  // proc_exit: (i32) -> void
  result = m3_LinkRawFunction(module, wasi, "proc_exit", "v(i)",
                              &wasi_snapshot_preview1_proc_exit);
  if (result)
    return result;

  // path_open: (i32, i32, i32, i32, i32, i64, i64, i32, i32) -> i32
  result = m3_LinkRawFunction(module, wasi, "path_open", "i(iiiiiiIii)",
                              &wasi_snapshot_preview1_path_open);
  if (result)
    return result;

  // fd_fdstat_get: (i32, i32) -> i32
  result = m3_LinkRawFunction(module, wasi, "fd_fdstat_get", "i(ii)",
                              &wasi_snapshot_preview1_fd_fdstat_get);
  if (result)
    return result;

  return result;
}
