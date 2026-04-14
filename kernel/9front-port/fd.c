#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>

void unlockfgrp(Fgrp *f) {
  int ex;

  ex = f->exceed;
  f->exceed = 0;
  unlock(&f->lock);
  if (ex)
    pprint("warning: process exceeds %d file descriptors\n", ex);
}

int growfd(Fgrp *f, int fd) /* fd is always >= 0 */
{
  Chan **newfd, **oldfd;
  uchar *newflag, *oldflag;
  int nfd;

  nfd = f->nfd;
  if (fd < nfd)
    return 0;
  if (fd >= nfd + DELTAFD)
    return -1; /* out of range */
  /*
   * Unbounded allocation is unwise; besides, there are only 16 bits
   * of fid in 9P
   */
  if (nfd >= 5000) {
  Exhausted:
    print("no free file descriptors\n");
    return -1;
  }
  oldfd = f->fd;
  oldflag = f->flag;
  newfd = malloc((nfd + DELTAFD) * sizeof(newfd[0]));
  newflag = malloc((nfd + DELTAFD) * sizeof(newflag[0]));
  if (newfd == nil || newflag == nil) {
    free(newflag);
    free(newfd);
    goto Exhausted;
  }
  memmove(newfd, oldfd, nfd * sizeof(newfd[0]));
  memmove(newflag, oldflag, nfd * sizeof(newflag[0]));
  f->fd = newfd;
  f->flag = newflag;
  f->nfd = nfd + DELTAFD;
  if (fd > f->maxfd) {
    if (fd / 100 > f->maxfd / 100)
      f->exceed = (fd / 100) * 100;
    f->maxfd = fd;
  }
  free(oldfd);
  free(oldflag);
  return 1;
}

/*
 *  this assumes that the fgrp is locked
 */
static int findfreefd(Fgrp *f, int start) {
  int fd;

  for (fd = start; fd < f->nfd; fd++)
    if (f->fd[fd] == nil)
      break;
  if (fd >= f->nfd && growfd(f, fd) < 0) {
    print("findfreefd: growfd failed for fd=%d\n", fd);
    return -1;
  }
  return fd;
}

int newfd(Chan *c, int mode) {
  int fd, flag;
  Fgrp *f;

  f = up->fgrp;
  lock(&f->lock);
  fd = findfreefd(f, 0);
  if (fd < 0) {
    unlockfgrp(f);
    return -1;
  }
  if (fd > f->maxfd)
    f->maxfd = fd;
  f->fd[fd] = c;

  /* per file-descriptor flags */
  flag = 0;
  if (mode & OCEXEC)
    flag |= CCEXEC;
  f->flag[fd] = flag;

  unlockfgrp(f);
  return fd;
}

int kopen(char *name, int mode) {
  int fd;
  Chan *c;

  openmode(mode); /* error check only */
  c = namec(name, Aopen, mode, 0);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  fd = newfd(c, mode);
  if (fd < 0)
    error(Enofd);
  poperror();
  return fd;
}

int newfd2(int fd[2], Chan *c[2]) {
  Fgrp *f;

  f = up->fgrp;
  lock(&f->lock);
  fd[0] = findfreefd(f, 0);
  if (fd[0] < 0) {
    unlockfgrp(f);
    return -1;
  }
  fd[1] = findfreefd(f, fd[0] + 1);
  if (fd[1] < 0) {
    unlockfgrp(f);
    return -1;
  }
  if (fd[1] > f->maxfd)
    f->maxfd = fd[1];
  f->fd[fd[0]] = c[0];
  f->fd[fd[1]] = c[1];
  f->flag[fd[0]] = 0;
  f->flag[fd[1]] = 0;
  unlockfgrp(f);
  return 0;
}

Chan *fdtochan(int fd, int mode, int chkmnt, int iref) {
  return fdtochan_fgrp(up->fgrp, fd, mode, chkmnt, iref);
}

Chan *fdtochan_fgrp(Fgrp *f, int fd, int mode, int chkmnt, int iref) {
  Chan *c;

  if (f == nil)
    error(Ebadfd);

  lock(&f->lock);
  if (fd < 0 || f->nfd <= fd || (c = f->fd[fd]) == nil) {
    unlock(&f->lock);
    error(Ebadfd);
  }
  if (iref)
    incref((Ref *)&c->ref);
  unlock(&f->lock);

  if (chkmnt && (c->flag & CMSG)) {
    if (iref)
      cclose(c);
    error(Ebadusefd);
  }

  if (mode < 0 || c->mode == ORDWR)
    return c;

  if ((mode & OTRUNC) && c->mode == OREAD) {
    if (iref)
      cclose(c);
    error(Ebadusefd);
  }

  if ((mode & ~OTRUNC) != c->mode) {
    if (iref)
      cclose(c);
    error(Ebadusefd);
  }
  return c;
}

int openmode(ulong o) {
  o &= ~(OTRUNC | OCEXEC | ORCLOSE);
  if (o > OEXEC)
    error(Ebadarg);
  if (o == OEXEC)
    return OREAD;
  return o;
}

void fdclose(int fd, int flag) {
  Chan *c;
  Fgrp *f = up->fgrp;

  lock(&f->lock);
  c = fd <= f->maxfd ? f->fd[fd] : nil;
  if (c == nil || (flag != 0 && ((f->flag[fd] | c->flag) & flag) == 0)) {
    unlock(&f->lock);
    return;
  }
  f->fd[fd] = nil;
  if (fd == f->maxfd) {
    while (fd > 0 && f->fd[fd] == nil)
      f->maxfd = --fd;
  }
  unlock(&f->lock);
  cclose(c);
}

/* Stubs for read/write/seek helpers if not in globals.c */
/* globals.c defined kread/kwrite/kseek but they called read/write/sseek */
/* read/write/sseek were static in sysfile.c */
/* We need to expose them or redefine them here */

/* Implementation of read/write/sseek for internal kernel use */

static long read_internal(int fd, uchar *p, long n, vlong *offp) {
  long nn;
  Chan *c;
  vlong off;

  validaddr((uintptr)p, n, 1);
  c = fdtochan(fd, OREAD, 1, 1);

  if (waserror()) {
    cclose(c);
    nexterror();
  }

  if (offp == nil) /* use and maintain channel's offset */
    off = c->offset;
  else
    off = *offp;
  if (off < 0)
    error(Enegoff);

  nn = devtab[devno(c->type, 0)]->read(c, p, n, off);

  if (offp == nil) {
    lock(&c->lock);
    c->offset += nn;
    unlock(&c->lock);
  }

  poperror();
  cclose(c);
  return nn;
}

long kread(int fd, void *buf, long n) { return read_internal(fd, buf, n, nil); }

static long write_internal(int fd, void *buf, long len, vlong *offp) {
  Chan *c;
  long n, written;
  vlong off;

  validaddr((uintptr)buf, len, 0);
  n = 0;
  c = fdtochan(fd, OWRITE, 1, 1);
  if (waserror()) {
    if (offp == nil) {
      lock(&c->lock);
      c->offset -= n;
      unlock(&c->lock);
    }
    cclose(c);
    nexterror();
  }

  if (c->qid.type & QTDIR)
    error(Eisdir);

  n = len;

  if (offp == nil) {
    lock(&c->lock);
    off = c->offset;
    c->offset += n;
    unlock(&c->lock);
  } else
    off = *offp;

  if (off < 0)
    error(Enegoff);

  written = devtab[devno(c->type, 0)]->write(c, buf, n, off);
  if (offp == nil && written < n) {
    lock(&c->lock);
    c->offset -= n - written;
    unlock(&c->lock);
  }

  poperror();
  cclose(c);
  return written;
}

long kwrite(int fd, void *buf, long n) { return write_internal(fd, buf, n, nil); }

vlong kseek(int fd, vlong offset, int whence) {
  extern vlong sseek(int, vlong, int);

  return sseek(fd, offset, whence);
}

/*
 * Pipe syscall implementation
 * Creates a bidirectional pipe using #| device
 */
typedef ulong* syscall_va_list;
#define SYSCALL_ARG(list, type) (*((type*)((list)++)))

uintptr syspipe(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  static char *datastr[] = {"data", "data1"};
  int fd[2], *ufd;
  Chan *c[2];

  ufd = SYSCALL_ARG(list, int *);
  validaddr((uintptr)ufd, sizeof(fd), 1);
  evenaddr((uintptr)ufd);

  ufd[0] = ufd[1] = fd[0] = fd[1] = -1;
  c[0] = namec("#|", Atodir, 0, 0);
  c[1] = nil;
  if (waserror()) {
    cclose(c[0]);
    if (c[1] != nil)
      cclose(c[1]);
    nexterror();
  }
  c[1] = cclone(c[0]);
  if (walk(&c[0], datastr + 0, 1, 1, nil) < 0)
    error(Egreg);
  if (walk(&c[1], datastr + 1, 1, 1, nil) < 0)
    error(Egreg);
  c[0] = devtab[devno(c[0]->type, 0)]->open(c[0], ORDWR);
  c[1] = devtab[devno(c[1]->type, 0)]->open(c[1], ORDWR);
  if (newfd2(fd, c) < 0)
    error(Enofd);
  ufd[0] = fd[0];
  ufd[1] = fd[1];
  poperror();
  return 0;
}
