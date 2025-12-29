#ifndef _LIB9P_H_
#define _LIB9P_H_

#include <fcall.h>
#include <u.h>

int p9_open(char *path, int mode);
int p9_close(int fd);
int p9_read(int fd, void *buf, int count);
int p9_write(int fd, void *buf, int count);
void p9_exit(char *status);

#endif
