#ifndef _LIB9P_SIP_H_
#define _LIB9P_SIP_H_

#include <fcall.h>

/* Initialization */
int sip_init(void);

/* Core Transport */
int sip_transact(Fcall *tx, Fcall *rx);

/* System Call Replacements */
int sip_open(char *path, int mode);
int sip_create(char *path, int mode, int perm);
long sip_read(int fd, void *buf, long count);
long sip_write(int fd, void *buf, long count);
int sip_close(int fd);
int sip_remove(char *path);

/* Process Control (via /proc/self/ctl) */
int sip_spawn(char *path); /* Exec new process */
int sip_exits(char *status);
int sip_bind(char *new, char *old, int flags);
int sip_pipe(int fd[2]);

#endif
