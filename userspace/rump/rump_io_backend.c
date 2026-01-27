/* rump_io_backend.c - I/O abstraction layer for 9front filesystem servers */

#include "rump_io_backend.h"
#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#include <string.h>

/* Global error buffer */
static char error_buf[256] = {0};
static int debug_level = 0;

/* Helper to set error message */
static void set_error(const char *msg) {
  strncpy(error_buf, msg, sizeof(error_buf) - 1);
  error_buf[sizeof(error_buf) - 1] = 0;
}

const char *rump_backend_error(void) { return error_buf; }

void rump_backend_set_debug(int level) { debug_level = level; }

/* Initialize rump kernel with filesystem support */
int rump_backend_init(const char *fstype) {
  /* Rump kernel should already be initialized by rump_server
   * This function can be used to verify or init specific components */

  if (debug_level > 0) {
    /* Log initialization */
  }

  /* For now, assume rump is already initialized by rump_server */
  return 0;
}

/* Mount a filesystem via rump */
RumpMount *rump_backend_mount(const char *device, const char *mountpoint,
                              const char *fstype, int flags) {
  RumpMount *mnt = NULL;

  /* Allocate mount structure */
  if (rumpuser_malloc(sizeof(RumpMount), 8, (void **)&mnt) != 0) {
    set_error("Failed to allocate RumpMount structure");
    return NULL;
  }

  memset(mnt, 0, sizeof(RumpMount));

  /* Copy parameters */
  strncpy(mnt->fstype, fstype, sizeof(mnt->fstype) - 1);
  strncpy(mnt->device, device, sizeof(mnt->device) - 1);
  strncpy(mnt->mountpoint, mountpoint, sizeof(mnt->mountpoint) - 1);

  /* Open the device/image file for block access */
  int open_flags =
      (flags & 1) ? RUMP_O_RDONLY : RUMP_O_RDWR; /* MNT_RDONLY = 1 */
  mnt->rump_fd = rump_sys_open(device, open_flags, 0);

  if (mnt->rump_fd < 0) {
    set_error("Failed to open device via rump");
    rumpuser_free(mnt, sizeof(RumpMount));
    return NULL;
  }

  /* TODO: Perform actual rump_sys_mount() if needed
   * For block-level access (like dossrv), we just need the fd */

  mnt->mounted = 1;
  return mnt;
}

/* Unmount and cleanup */
int rump_backend_unmount(RumpMount *mnt) {
  if (!mnt)
    return -1;

  if (mnt->rump_fd >= 0) {
    rump_sys_close(mnt->rump_fd);
  }

  /* TODO: Call rump_sys_unmount() if we did rump_sys_mount() */

  rumpuser_free(mnt, sizeof(RumpMount));
  return 0;
}

/* Read blocks from device */
int rump_read_block(RumpMount *mnt, void *buf, uint64_t block, uint32_t count) {
  if (!mnt || mnt->rump_fd < 0) {
    set_error("Invalid mount handle");
    return -1;
  }

  /* Calculate byte offset */
  int64_t offset = (int64_t)block * RUMP_BLOCKSIZE;

  /* Use rump_sys_pread for positioned read */
  int n = rump_sys_pread(mnt->rump_fd, buf, count, offset);

  if (n < 0) {
    set_error("rump_sys_pread failed");
    return -1;
  }

  return n;
}

/* Write blocks to device */
int rump_write_block(RumpMount *mnt, const void *buf, uint64_t block,
                     uint32_t count) {
  if (!mnt || mnt->rump_fd < 0) {
    set_error("Invalid mount handle");
    return -1;
  }

  int64_t offset = (int64_t)block * RUMP_BLOCKSIZE;

  int n = rump_sys_pwrite(mnt->rump_fd, buf, count, offset);

  if (n < 0) {
    set_error("rump_sys_pwrite failed");
    return -1;
  }

  return n;
}

/* Read from a file via rump */
int rump_read_file(RumpMount *mnt, const char *path, void *buf, int64_t offset,
                   uint32_t count) {
  if (!mnt) {
    set_error("Invalid mount handle");
    return -1;
  }

  /* Build full path */
  char fullpath[512];
  if (path[0] == '/') {
    strncpy(fullpath, path, sizeof(fullpath) - 1);
  } else {
    snprintf(fullpath, sizeof(fullpath), "%s/%s", mnt->mountpoint, path);
  }
  fullpath[sizeof(fullpath) - 1] = 0;

  /* Open file */
  int fd = rump_sys_open(fullpath, RUMP_O_RDONLY, 0);
  if (fd < 0) {
    set_error("Failed to open file");
    return -1;
  }

  /* Read */
  int n = rump_sys_pread(fd, buf, count, offset);
  rump_sys_close(fd);

  if (n < 0) {
    set_error("Read failed");
    return -1;
  }

  return n;
}

/* Write to a file via rump */
int rump_write_file(RumpMount *mnt, const char *path, const void *buf,
                    int64_t offset, uint32_t count) {
  if (!mnt) {
    set_error("Invalid mount handle");
    return -1;
  }

  char fullpath[512];
  if (path[0] == '/') {
    strncpy(fullpath, path, sizeof(fullpath) - 1);
  } else {
    snprintf(fullpath, sizeof(fullpath), "%s/%s", mnt->mountpoint, path);
  }
  fullpath[sizeof(fullpath) - 1] = 0;

  int fd = rump_sys_open(fullpath, RUMP_O_WRONLY, 0);
  if (fd < 0) {
    set_error("Failed to open file for writing");
    return -1;
  }

  int n = rump_sys_pwrite(fd, buf, count, offset);
  rump_sys_close(fd);

  if (n < 0) {
    set_error("Write failed");
    return -1;
  }

  return n;
}

/* Get file status */
int rump_stat_file(RumpMount *mnt, const char *path, void *stat_buf) {
  if (!mnt) {
    set_error("Invalid mount handle");
    return -1;
  }

  char fullpath[512];
  if (path[0] == '/') {
    strncpy(fullpath, path, sizeof(fullpath) - 1);
  } else {
    snprintf(fullpath, sizeof(fullpath), "%s/%s", mnt->mountpoint, path);
  }
  fullpath[sizeof(fullpath) - 1] = 0;

  /* Use rump_sys_stat */
  int ret = rump_sys_stat(fullpath, stat_buf);
  if (ret < 0) {
    set_error("stat failed");
    return -1;
  }

  return 0;
}

/* Read directory contents */
int rump_readdir(RumpMount *mnt, const char *path,
                 rump_dirent_callback callback, void *arg) {
  if (!mnt || !callback) {
    set_error("Invalid arguments");
    return -1;
  }

  char fullpath[512];
  if (path[0] == '/') {
    strncpy(fullpath, path, sizeof(fullpath) - 1);
  } else {
    snprintf(fullpath, sizeof(fullpath), "%s/%s", mnt->mountpoint, path);
  }
  fullpath[sizeof(fullpath) - 1] = 0;

  /* Open directory */
  int fd = rump_sys_open(fullpath, RUMP_O_RDONLY | RUMP_O_DIRECTORY, 0);
  if (fd < 0) {
    set_error("Failed to open directory");
    return -1;
  }

  /* Read directory entries using rump_sys_getdents */
  char buf[4096];
  int nread;

  while ((nread = rump_sys_getdents(fd, buf, sizeof(buf))) > 0) {
    char *ptr = buf;
    while (ptr < buf + nread) {
      /* Parse directory entry structure (platform-specific) */
      /* For now, simplified - needs proper dirent structure handling */
      struct {
        uint64_t d_fileno;
        uint16_t d_reclen;
        uint8_t d_type;
        uint8_t d_namlen;
        char d_name[256];
      } *de = (void *)ptr;

      if (de->d_reclen == 0)
        break;

      /* Call callback for each entry */
      callback(de->d_name, arg);

      ptr += de->d_reclen;
    }
  }

  rump_sys_close(fd);
  return 0;
}

/* Helper: sprintf implementation using simple buffer */
static int snprintf(char *str, size_t size, const char *format, ...) {
  /* Simple implementation - just handle %s/%s case for now */
  /* TODO: Use proper vsnprintf if available in rump/liblux */
  const char *s1 = format;
  char *dest = str;
  size_t remaining = size - 1;

  /* Very basic - just copy until we hit the format string */
  while (*s1 && remaining > 0) {
    if (*s1 == '%' && *(s1 + 1) == 's') {
      /* Skip %s for now - need va_args for proper implementation */
      s1 += 2;
    } else {
      *dest++ = *s1++;
      remaining--;
    }
  }
  *dest = 0;
  return dest - str;
}
