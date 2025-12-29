/* lib9p_syscall.h - Pure 9P userspace syscall API
 *
 * These functions construct 9P messages and trap into the kernel.
 * No traditional syscall ABI - everything via 9P message passing.
 */

#ifndef LIB9P_SYSCALL_H
#define LIB9P_SYSCALL_H

/* Open a file/device */
int p9_open(const char *path, int mode);

/* Create a new file */
int p9_create(const char *path, int mode, int perm);

/* Close a file descriptor */
int p9_close(int fd);

/* Read from a file */
long p9_read(int fd, void *buf, long count);

/* Write to a file */
long p9_write(int fd, const void *buf, long count);

/* Exit the process */
void p9_exit(const char *status) __attribute__((noreturn));

/* Open modes (from Plan 9) */
#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define OEXEC 3
#define OTRUNC 16
#define OCEXEC 32
#define ORCLOSE 64

#endif /* LIB9P_SYSCALL_H */
