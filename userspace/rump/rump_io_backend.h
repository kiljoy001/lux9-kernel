/* rump_io_backend.h - I/O abstraction for 9front filesystem servers */

#ifndef _RUMP_IO_BACKEND_H_
#define _RUMP_IO_BACKEND_H_

#include <stdint.h>

/* Block size for block I/O operations */
#define RUMP_BLOCKSIZE 512

/* Mount handle for a rump-backed filesystem */
typedef struct RumpMount {
  void *rump_mount;     /* Rump mount structure pointer */
  char fstype[32];      /* Filesystem type: "msdos", "cd9660", "ffs", etc. */
  char device[256];     /* Device path for block device */
  char mountpoint[256]; /* Mount point in rump namespace */
  int rump_fd;          /* File descriptor for block device access */
  int mounted;          /* 1 if successfully mounted, 0 otherwise */
} RumpMount;

/* Initialize rump kernel with specified filesystem support
 * Returns: 0 on success, -1 on error */
int rump_backend_init(const char *fstype);

/* Mount a filesystem via rump
 * device: Path to block device or image file
 * mountpoint: Where to mount in rump namespace
 * fstype: Filesystem type ("msdos", "cd9660", "ffs", etc.)
 * flags: Mount flags (MNT_RDONLY, etc.)
 * Returns: RumpMount handle or NULL on error */
RumpMount *rump_backend_mount(const char *device, const char *mountpoint,
                              const char *fstype, int flags);

/* Unmount and cleanup */
int rump_backend_unmount(RumpMount *mnt);

/* Block-level I/O operations (for low-level filesystem servers like dossrv) */

/* Read blocks from device
 * mnt: Mount handle
 * buf: Buffer to read into
 * block: Starting block number
 * count: Number of bytes to read
 * Returns: Number of bytes read, or -1 on error */
int rump_read_block(RumpMount *mnt, void *buf, uint64_t block, uint32_t count);

/* Write blocks to device
 * Returns: Number of bytes written, or -1 on error */
int rump_write_block(RumpMount *mnt, const void *buf, uint64_t block,
                     uint32_t count);

/* File-level I/O operations (for higher-level access) */

/* Read from a file via rump
 * path: Path relative to mountpoint
 * buf: Buffer to read into
 * offset: Offset within file
 * count: Bytes to read
 * Returns: Bytes read or -1 on error */
int rump_read_file(RumpMount *mnt, const char *path, void *buf, int64_t offset,
                   uint32_t count);

/* Write to a file via rump
 * Returns: Bytes written or -1 on error */
int rump_write_file(RumpMount *mnt, const char *path, const void *buf,
                    int64_t offset, uint32_t count);

/* Get file status */
int rump_stat_file(RumpMount *mnt, const char *path, void *stat_buf);

/* Directory operations */

/* Callback for directory enumeration */
typedef void (*rump_dirent_callback)(const char *name, void *arg);

/* Read directory contents
 * path: Directory path relative to mountpoint
 * callback: Function called for each entry
 * arg: User argument passed to callback
 * Returns: 0 on success, -1 on error */
int rump_readdir(RumpMount *mnt, const char *path,
                 rump_dirent_callback callback, void *arg);

/* Utility functions */
const char *rump_backend_error(void);
void rump_backend_set_debug(int level);

#endif /* _RUMP_IO_BACKEND_H_ */
