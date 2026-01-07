#ifndef _LIBLUX_H_
#define _LIBLUX_H_

/* Include kernel types */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <pebble.h>

/* Syscall Wrappers */
int sys_open(char *path, int mode);
int sys_close(int fd);
long sys_read(int fd, void *buf, long n);
long sys_write(int fd, void *buf, long n);
long sys_pwrite(int fd, void *buf, long n, long offset);
long sys_pread(int fd, void *buf, long n, long offset);
void sys_exit(char *msg);
int sys_create(char *path, int mode, uint perm);
int sys_rfork(int flags);
void sys_exec(char *path);
int sys_pipe(int *fds);
long sys_seek(int fd, long offset, int whence);
int sys_wait(void);
uvlong sys_nsec(void);
int sys_stat(char *path, uchar *buf, int nbuf);
int sys_wstat(char *path, uchar *buf, int nbuf);
int sys_mount(int fd, int afd, char *old, int flags, char *aname);
int sys_sleep(long ms);
int sys_getpid2(void *out, ulong len);
int sys_bind(char *old, char *new, int flags);

/* RFORK flags */
#define RFPROC (1 << 4)
#define RFMEM (1 << 5)
#define RFNOWAIT (1 << 6)

/* MsgOrd / Kinetic Defense */
int msgord_submit(char *path, Fcall *t);
int pow_solve(u64int context, int difficulty, u64int *nonce_out);

/* Pebble */
int pebble_alloc(ulong size, void **addr);
int pebble_free(void *addr);

#endif
