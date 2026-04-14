/* clang-format off */
#include "u.h"

/* Local Plan 9 Syscall ABI fix */
#include <u.h>
typedef ulong *syscall_va_list;
#define SYSCALL_ARG(list, type) (*(type*)((list)++))
/* va_list macro removed to prevent stdarg.h conflict */
#define va_start(list, start) ((void)0)
#define va_end(list) ((void)0)




#define SYSCALL_ARG(list, type) (*(type*)((list)++))

#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>

/* clang-format on */

/*
 * The sys*() routines needn't poperror() as they return directly to syscall().
 */

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

/*@
  @ requires c != \null;
  @ assigns \nothing;
  @ ensures \result >= -1;
  @*/
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

/*
 * kopen - open a file from kernel context
 * Used to set up initial file descriptors for init process
 */
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

static int newfd2(int fd[2], Chan *c[2]) {
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

/*@
  @ requires fd >= -1;
  @ ensures \result == \null || \valid(\result);
  @*/
Chan *fdtochan(int fd, int mode, int chkmnt, int iref) {
  Chan *c;
  Fgrp *f;

  f = up->fgrp;

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

uintptr sysfd2path(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Chan *c;
  char *buf;
  uint len;
  int fd;

  fd = SYSCALL_ARG(list, int);
  buf = SYSCALL_ARG(list, char *);
  len = SYSCALL_ARG(list, uint);
  validaddr((uintptr)buf, len, 1);
  c = fdtochan(fd, -1, 0, 1);
  snprint(buf, len, "%s", chanpath(c));
  cclose(c);
  return 0;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == 0;
  @*/
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
  c[0] = devtab[c[0]->type]->open(c[0], ORDWR);
  c[1] = devtab[c[1]->type]->open(c[1], ORDWR);
  if (newfd2(fd, c) < 0)
    error(Enofd);
  ufd[0] = fd[0];
  ufd[1] = fd[1];
  if (up != nil && up->pid == 4) {
    bprint("syspipe: pid=%lud fd0=%d fd1=%d\n", up->pid, fd[0], fd[1]);
  }
  poperror();
  return 0;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= -1;
  @*/
uintptr sysdup(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  Chan *c, *oc;
  Fgrp *f = up->fgrp;

  fd = SYSCALL_ARG(list, int);

  /*
   * Close after dup'ing, so date > #d/1 works
   */
  c = fdtochan(fd, -1, 0, 1);
  fd = SYSCALL_ARG(list, int);
  if (fd != -1) {
    lock(&f->lock);
    if (fd < 0 || growfd(f, fd) < 0) {
      unlockfgrp(f);
      cclose(c);
      error(Ebadfd);
    }
    if (fd > f->maxfd)
      f->maxfd = fd;

    oc = f->fd[fd];
    f->fd[fd] = c;
    f->flag[fd] = 0;
    unlockfgrp(f);
    if (oc != nil)
      cclose(oc);
  } else {
    if (waserror()) {
      cclose(c);
      nexterror();
    }
    fd = newfd(c, 0);
    if (fd < 0)
      error(Enofd);
    poperror();
  }
  return (uintptr)fd;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= -1;
  @*/
uintptr sysopen(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  Chan *c;
  char *name;
  ulong mode;

  name = SYSCALL_ARG(list, char *);
  mode = SYSCALL_ARG(list, ulong);
  openmode(mode); /* error check only */
  validaddr((uintptr)name, 1, 0);
  c = namec(name, Aopen, mode, 0);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  fd = newfd(c, mode);
  if (fd < 0)
    error(Enofd);
  poperror();
  return (uintptr)fd;
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

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == 0;
  @*/
uintptr sysclose(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;

  fd = SYSCALL_ARG(list, int);
  fdtochan(fd, -1, 0, 0);
  fdclose(fd, 0);
  return 0;
}

long unionread(Chan *c, void *va, long n) {
  int i;
  long nr;
  Mhead *m;
  Mount *mount;

  eqlock(&c->umqlock);
  m = c->umh;
  rlock(&m->lock);
  mount = m->mount;
  /* bring mount in sync with c->uri and c->umc */
  for (i = 0; mount != nil && i < c->uri; i++)
    mount = mount->next;

  nr = 0;
  while (mount != nil) {
    /* Error causes component of union to be skipped */
    if (mount->to != nil && !waserror()) {
      if (c->umc == nil) {
        c->umc = cclone(mount->to);
        c->umc = devtab[c->umc->type]->open(c->umc, OREAD);
      }

      nr = devtab[c->umc->type]->read(c->umc, va, n, c->umc->offset);
      c->umc->offset += nr;
      poperror();
    }
    if (nr > 0)
      break;

    /* Advance to next element */
    c->uri++;
    if (c->umc != nil) {
      cclose(c->umc);
      c->umc = nil;
    }
    mount = mount->next;
  }
  runlock(&m->lock);
  qunlock(&c->umqlock);
  return nr;
}

static void unionrewind(Chan *c) {
  eqlock(&c->umqlock);
  c->uri = 0;
  if (c->umc != nil) {
    cclose(c->umc);
    c->umc = nil;
  }
  qunlock(&c->umqlock);
}

static int dirfixed(uchar *p, uchar *e, Dir *d) {
  int len;

  len = GBIT16(p) + BIT16SZ;
  if (p + len > e)
    return -1;

  p += BIT16SZ; /* ignore size */
  d->type = devno(GBIT16(p), 1);
  p += BIT16SZ;
  d->dev = GBIT32(p);
  p += BIT32SZ;
  d->qid.type = GBIT8(p);
  p += BIT8SZ;
  d->qid.vers = GBIT32(p);
  p += BIT32SZ;
  d->qid.path = GBIT64(p);
  p += BIT64SZ;
  d->mode = GBIT32(p);
  p += BIT32SZ;
  d->atime = GBIT32(p);
  p += BIT32SZ;
  d->mtime = GBIT32(p);
  p += BIT32SZ;
  d->length = GBIT64(p);

  return len;
}

static char *dirname(uchar *p, int *n) {
  p += BIT16SZ + BIT16SZ + BIT32SZ + BIT8SZ + BIT32SZ + BIT64SZ + BIT32SZ +
       BIT32SZ + BIT32SZ + BIT64SZ;
  *n = GBIT16(p);
  return (char *)p + BIT16SZ;
}

static long dirsetname(char *name, int len, uchar *p, long n, long maxn) {
  char *oname;
  int olen;
  long nn;

  if (n == BIT16SZ)
    return BIT16SZ;

  oname = dirname(p, &olen);

  nn = n + len - olen;
  PBIT16(p, nn - BIT16SZ);
  if (nn > maxn)
    return BIT16SZ;

  if (len != olen)
    memmove(oname + len, oname + olen, p + n - (uchar *)(oname + olen));
  PBIT16((uchar *)(oname - 2), len);
  memmove(oname, name, len);
  return nn;
}

/*
 * Mountfix might have caused the fixed results of the directory read
 * to overflow the buffer.  Catch the overflow in c->dirrock.
 */
static void mountrock(Chan *c, uchar *p, uchar **pe) {
  uchar *e, *r;
  int len, n;

  e = *pe;

  /* find last directory entry */
  for (;;) {
    len = BIT16SZ + GBIT16(p);
    if (p + len >= e)
      break;
    p += len;
  }

  /* save it away */
  qlock(&c->rockqlock);
  if (c->nrock + len > c->mrock) {
    n = ROUND(c->nrock + len, 1024);
    r = smalloc(n);
    memmove(r, c->dirrock, c->nrock);
    free(c->dirrock);
    c->dirrock = r;
    c->mrock = n;
  }
  memmove(c->dirrock + c->nrock, p, len);
  c->nrock += len;
  qunlock(&c->rockqlock);

  /* drop it */
  *pe = p;
}

/*
 * Satisfy a directory read with the results saved in c->dirrock.
 */
static int mountrockread(Chan *c, uchar *op, long n, long *nn) {
  long dirlen;
  uchar *rp, *erp, *ep, *p;

  /* common case */
  if (c->nrock == 0)
    return 0;

  /* copy out what we can */
  qlock(&c->rockqlock);
  rp = c->dirrock;
  erp = rp + c->nrock;
  p = op;
  ep = p + n;
  while (rp + BIT16SZ <= erp) {
    dirlen = BIT16SZ + GBIT16(rp);
    if (p + dirlen > ep)
      break;
    memmove(p, rp, dirlen);
    p += dirlen;
    rp += dirlen;
  }

  if (p == op) {
    qunlock(&c->rockqlock);
    return 0;
  }

  /* shift the rest */
  if (rp != erp)
    memmove(c->dirrock, rp, erp - rp);
  c->nrock = erp - rp;

  *nn = p - op;
  qunlock(&c->rockqlock);
  return 1;
}

static void mountrewind(Chan *c) { c->nrock = 0; }

/*
 * Rewrite the results of a directory read to reflect current
 * name space bindings and mounts.  Specifically, replace
 * directory entries for bind and mount points with the results
 * of statting what is mounted there.  Except leave the old names.
 */
static long mountfix(Chan *c, uchar *op, long n, long maxn) {
  char *name;
  int nbuf, nname;
  Chan *nc;
  Mhead *mh;
  Mount *m;
  uchar *p;
  int dirlen, rest;
  long l;
  uchar *buf, *e;
  Dir d;

  p = op;
  buf = nil;
  nbuf = 0;
  for (e = &p[n]; p + BIT16SZ < e; p += dirlen) {
    dirlen = dirfixed(p, e, &d);
    if (dirlen < 0)
      break;
    nc = nil;
    mh = nil;
    if (findmount(&nc, &mh, d.type, d.dev, d.qid)) {
      /*
       * If it's a union directory and the original is
       * in the union, don't rewrite anything.
       */
      rlock(&mh->lock);
      for (m = mh->mount; m != nil; m = m->next) {
        if (eqchantdqid(m->to, d.type, d.dev, d.qid, 1)) {
          runlock(&mh->lock);
          goto Norewrite;
        }
      }
      runlock(&mh->lock);

      if (waserror())
        goto Norewrite;
      name = dirname(p, &nname);
      if (buf == nil) {
        nbuf = 4096;
        buf = smalloc(nbuf);
      }
      l = devtab[nc->type]->stat(nc, buf, nbuf);
      if (l < BIT16SZ)
        error(Eshortstat);
      rest = BIT16SZ + GBIT16(buf) + nname;
      if (rest > nbuf) {
        free(buf);
        nbuf = rest;
        buf = smalloc(nbuf);
        l = devtab[nc->type]->stat(nc, buf, nbuf);
      }
      l = dirsetname(name, nname, buf, l, nbuf);
      if (l <= BIT16SZ)
        error(Eshortstat);
      poperror();

      /*
       * Shift data in buffer to accomodate new entry,
       * possibly overflowing into rock.
       */
      rest = e - (p + dirlen);
      if (l > dirlen) {
        while (p + l + rest > op + maxn) {
          mountrock(c, p, &e);
          if (e == p) {
            dirlen = 0;
            goto Norewrite;
          }
          rest = e - (p + dirlen);
        }
      }
      if (l != dirlen) {
        memmove(p + l, p + dirlen, rest);
        dirlen = l;
        e = p + dirlen + rest;
      }

      /*
       * Rewrite directory entry.
       */
      memmove(p, buf, l);

    Norewrite:
      cclose(nc);
      putmhead(mh);
    }
  }
  if (buf != nil)
    free(buf);

  if (p != e)
    error("oops in rockfix");

  return e - op;
}

static long read(int fd, uchar *p, long n, vlong *offp) {
  long nn, nnn;
  Chan *c;
  vlong off;

  validaddr((uintptr)p, n, 1);
  c = fdtochan(fd, OREAD, 1, 1);

  if (waserror()) {
    cclose(c);
    nexterror();
  }

  /*
   * The offset is passed through on directories, normally.
   * Sysseek complains, but pread is used by servers like exportfs,
   * that shouldn't need to worry about this issue.
   *
   * Notice that c->devoffset is the offset that c's dev is seeing.
   * The number of bytes read on this fd (c->offset) may be different
   * due to rewritings in rockfix.
   */
  if (offp == nil) /* use and maintain channel's offset */
    off = c->offset;
  else
    off = *offp;
  if (off < 0)
    error(Enegoff);

  if (off == 0) { /* rewind to the beginning of the directory */
    if (offp == nil || (c->qid.type & QTDIR)) {
      c->offset = 0;
      c->devoffset = 0;
    }
    mountrewind(c);
    unionrewind(c);
  }

  if (c->qid.type & QTDIR) {
    if (mountrockread(c, p, n, &nn)) {
      /* do nothing: mountrockread filled buffer */
    } else if (c->umh != nil)
      nn = unionread(c, p, n);
    else {
      if (off != c->offset)
        error(Edirseek);
      nn = devtab[c->type]->read(c, p, n, c->devoffset);
    }
    nnn = mountfix(c, p, nn, n);
  } else
    nnn = nn = devtab[c->type]->read(c, p, n, off);

  if (offp == nil || (c->qid.type & QTDIR)) {
    lock(c);
    c->devoffset += nn;
    c->offset += nnn;
    unlock(c);
  }

  poperror();
  cclose(c);
  return nnn;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= -1;
  @*/
uintptr sys_read(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  void *buf;
  long len;

  fd = SYSCALL_ARG(list, int);
  buf = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, long);
  return (uintptr)read(fd, buf, len, nil);
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= -1;
  @*/
uintptr syspread(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  void *buf;
  long len;
  vlong off, *offp;

  fd = SYSCALL_ARG(list, int);
  buf = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, long);
  off = SYSCALL_ARG(list, vlong);
  if (off != ~0ULL)
    offp = &off;
  else
    offp = nil;
  return (uintptr)read(fd, buf, len, offp);
}

static long write(int fd, void *buf, long len, vlong *offp, int check) {
  Chan *c;
  long m, n;
  vlong off;

  if (boot_verbose)
    print("write: fd=%d buf=%p len=%ld\n", fd, buf, len);
  if (check)
    validaddr((uintptr)buf, len, 0);
  if (boot_verbose)
    print("write: validaddr passed\n");
  n = 0;
  c = fdtochan(fd, OWRITE, 1, 1);
  if (boot_verbose)
    print("write: fdtochan returned c=%p type=%d\n", c, c ? c->type : -1);
  if (waserror()) {
    if (offp == nil) {
      lock(c);
      c->offset -= n;
      unlock(c);
    }
    cclose(c);
    nexterror();
  }

  if (c->qid.type & QTDIR)
    error(Eisdir);

  n = len;

  if (offp == nil) { /* use and maintain channel's offset */
    lock(c);
    off = c->offset;
    c->offset += n;
    unlock(c);
  } else
    off = *offp;

  if (off < 0)
    error(Enegoff);

  m = devtab[c->type]->write(c, buf, n, off);
  if (offp == nil && m < n) {
    lock(c);
    c->offset -= n - m;
    unlock(c);
  }

  poperror();
  cclose(c);
  return m;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= -1;
  @*/
uintptr sys_write(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  void *buf;
  long len;
  static int stdout_trace_count;

  fd = SYSCALL_ARG(list, int);
  buf = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, long);
  if ((fd == 1 || fd == 2) && stdout_trace_count < 50) {
    char trace_buf[96];
    stdout_trace_count++;
    snprint(trace_buf, sizeof(trace_buf), "sys_write: pid=%d fd=%d len=%ld\n",
            up ? up->pid : -1, fd, len);
    uartputs(trace_buf, (int)strlen(trace_buf));
  }
  return (uintptr)write(fd, buf, len, nil, 1);
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= -1;
  @*/
uintptr syspwrite(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  void *buf;
  long len;
  vlong off, *offp;

  fd = SYSCALL_ARG(list, int);
  buf = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, long);
  off = SYSCALL_ARG(list, vlong);

  /* Debug: print PWRITE arguments */
  if (boot_verbose)
    print("syspwrite: fd=%d buf=%p len=%ld off=%lld\n", fd, buf, len, off);

  if (off != ~0ULL)
    offp = &off;
  else
    offp = nil;
  {
    long ret = write(fd, buf, len, offp, 1);
    if (boot_verbose)
      print("syspwrite: write returned %ld\n", ret);
    return (uintptr)ret;
  }
}

vlong sseek(int fd, vlong o, int type) {
  Dir *d;
  Chan *c;
  vlong off;

  c = fdtochan(fd, -1, 1, 1);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  if (devtab[c->type]->dc == L'|')
    error(Eisstream);

  switch (type) {
  case 0:
    off = o;
    if ((c->qid.type & QTDIR) && off != 0)
      error(Eisdir);
    if (off < 0)
      error(Enegoff);
    c->offset = off;
    break;

  case 1:
    if (c->qid.type & QTDIR)
      error(Eisdir);
    lock(c); /* lock for read/write update */
    off = o + c->offset;
    if (off < 0) {
      unlock(c);
      error(Enegoff);
    }
    c->offset = off;
    unlock(c);
    break;

  case 2:
    if (c->qid.type & QTDIR)
      error(Eisdir);
    d = dirchanstat(c);
    off = d->length + o;
    free(d);
    if (off < 0)
      error(Enegoff);
    c->offset = off;
    break;

  default:
    error(Ebadarg);
  }
  c->uri = 0;
  c->dri = 0;
  cclose(c);
  poperror();
  return off;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == 0;
  @*/
uintptr sysseek(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd, t;
  vlong n, *v;

  v = SYSCALL_ARG(list, vlong *);
  evenaddr((uintptr)v);
  validaddr((uintptr)v, sizeof(vlong), 1);

  fd = SYSCALL_ARG(list, int);
  n = SYSCALL_ARG(list, vlong);
  t = SYSCALL_ARG(list, int);

  *v = sseek(fd, n, t);

  return 0;
}

uintptr sysoseek(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd, t;
  long n;

  fd = SYSCALL_ARG(list, int);
  n = SYSCALL_ARG(list, long);
  t = SYSCALL_ARG(list, int);
  return (uintptr)sseek(fd, n, t);
}

void validstat(uchar *s, int n) {
  int m;
  char buf[64];

  if (statcheck(s, n) < 0)
    error(Ebadstat);
  /* verify that name entry is acceptable */
  s += STATFIXLEN - 4 * BIT16SZ; /* location of first string */
  /*
   * s now points at count for first string.
   * if it's too long, let the server decide; this is
   * only for his protection anyway. otherwise
   * we'd have to allocate and waserror.
   */
  m = GBIT16(s);
  s += BIT16SZ;
  if (m + 1 > sizeof buf)
    return;
  memmove(buf, s, m);
  buf[m] = '\0';
  /* name could be '/' */
  if (strcmp(buf, "/") != 0)
    validname(buf, 0);
}

static char *pathlast(Path *p) {
  char *s;

  if (p == nil)
    return nil;
  if (p->len == 0)
    return nil;
  s = strrchr(p->s, '/');
  if (s != nil)
    return s + 1;
  return p->s;
}

uintptr sysfstat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *name;
  uchar *s;
  Chan *c;
  int fd;
  uint l, r;

  fd = SYSCALL_ARG(list, int);
  s = SYSCALL_ARG(list, uchar *);
  l = SYSCALL_ARG(list, uint);
  validaddr((uintptr)s, l, 1);
  c = fdtochan(fd, -1, 0, 1);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  r = devtab[c->type]->stat(c, s, l);
  if ((name = pathlast(c->path)) != nil)
    r = dirsetname(name, strlen(name), s, r, l);
  poperror();
  cclose(c);
  return r;
}

uintptr sysstat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *name;
  uchar *s;
  Chan *c;
  uint l, r;

  name = SYSCALL_ARG(list, char *);
  s = SYSCALL_ARG(list, uchar *);
  l = SYSCALL_ARG(list, uint);
  validaddr((uintptr)s, l, 1);
  validaddr((uintptr)name, 1, 0);
  c = namec(name, Aaccess, 0, 0);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  r = devtab[c->type]->stat(c, s, l);
  if ((name = pathlast(c->path)) != nil)
    r = dirsetname(name, strlen(name), s, r, l);
  poperror();
  cclose(c);
  return r;
}

uintptr syschdir(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Chan *c;
  char *name;

  name = SYSCALL_ARG(list, char *);
  validaddr((uintptr)name, 1, 0);
  c = namec(name, Atodir, 0, 0);
  cclose(up->dot);
  up->dot = c;
  return 0;
}

static int bindmount(int ismount, int fd, int afd, char *arg0, char *arg1,
                     int flag, char *spec) {
  int ret;
  Chan *c0, *c1, *ac, *bc;
  int trace = (up != nil && up->pid == 4);

  if (trace)
    bprint("bindmount: pid=%lud enter ismount=%d fd=%d afd=%d arg0=%s arg1=%s flags=%d spec=%s\n",
           up->pid, ismount, fd, afd, arg0 ? arg0 : "", arg1 ? arg1 : "",
           flag, spec ? spec : "");
  if (arg1 != nil && strcmp(arg1, "/proc") == 0) {
    bprint("bindmount: target=/proc ismount=%d fd=%d flags=%d spec=%s\n",
           ismount, fd, flag, spec ? spec : "");
  }
  if ((flag & ~MMASK) || (flag & MORDER) == (MBEFORE | MAFTER))
    error(Ebadarg);

  if (ismount) {
    validaddr((uintptr)spec, 1, 0);
    spec = validnamedup(spec, 1);
    if (waserror()) {
      free(spec);
      nexterror();
    }

    if (!canmount(up->pgrp))
      error(Enoattach);

    ac = nil;
    bc = fdtochan(fd, ORDWR, 0, 1);
    if (waserror()) {
      if (ac != nil)
        cclose(ac);
      cclose(bc);
      nexterror();
    }

    if (afd >= 0)
      ac = fdtochan(afd, ORDWR, 0, 1);

    if (trace)
      bprint("bindmount: pid=%lud mntattach start\n", up->pid);
    c0 = mntattach(bc, ac, spec, flag & MCACHE);
    if (trace)
      bprint("bindmount: pid=%lud mntattach done\n", up->pid);
    poperror(); /* ac bc */
    if (ac != nil)
      cclose(ac);
    cclose(bc);
  } else {
    spec = nil;
    validaddr((uintptr)arg0, 1, 0);
    c0 = namec(arg0, Abind, 0, 0);
  }

  if (waserror()) {
    cclose(c0);
    nexterror();
  }

  validaddr((uintptr)arg1, 1, 0);
  if (trace)
    bprint("bindmount: pid=%lud namec Amount start\n", up->pid);
  c1 = namec(arg1, Amount, 0, 0);
  if (trace)
    bprint("bindmount: pid=%lud namec Amount done\n", up->pid);
  if (waserror()) {
    cclose(c1);
    nexterror();
  }

  if (trace)
    bprint("bindmount: pid=%lud cmount start\n", up->pid);
  ret = cmount(c0, c1, flag, spec);
  if (trace)
    bprint("bindmount: pid=%lud cmount done ret=%d\n", up->pid, ret);

  poperror();
  cclose(c1);
  poperror();
  cclose(c0);
  if (ismount) {
    fdclose(fd, 0);
    poperror();
    free(spec);
  }
  return ret;
}

uintptr sysbind(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *arg0, *arg1;
  int flag;

  /* Universal CBS: Check filesystem capability before namespace modifications
   */
  if (!has_capability(up, PEBBLE_CAP_FS))
    error(PEBBLE_E_PERM);

  arg0 = SYSCALL_ARG(list, char *);
  arg1 = SYSCALL_ARG(list, char *);
  flag = SYSCALL_ARG(list, int);
  return (uintptr)bindmount(0, -1, -1, arg0, arg1, flag, nil);
}

uintptr sysmount(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *arg1, *spec;
  int flag;
  int fd, afd;

  /* Universal CBS: Check filesystem capability before mounting */
  if (!has_capability(up, PEBBLE_CAP_FS))
    error(PEBBLE_E_PERM);

  fd = SYSCALL_ARG(list, int);
  afd = SYSCALL_ARG(list, int);
  arg1 = SYSCALL_ARG(list, char *);
  flag = SYSCALL_ARG(list, int);
  spec = SYSCALL_ARG(list, char *);
  if (up != nil && up->pid == 4) {
    bprint("sysmount: pid=%lud fd=%d afd=%d flags=%d arg=%s spec=%s\n", up->pid,
           fd, afd, flag, arg1 ? arg1 : "", spec ? spec : "");
    uintptr ret = (uintptr)bindmount(1, fd, afd, nil, arg1, flag, spec);
    bprint("sysmount: pid=%lud ret=%#p\n", up->pid, ret);
    return ret;
  } else if (arg1 != nil && strcmp(arg1, "/proc") == 0) {
    bprint("sysmount: /proc fd=%d afd=%d flags=%d spec=%s\n", fd, afd, flag,
           spec ? spec : "");
  }
  return (uintptr)bindmount(1, fd, afd, nil, arg1, flag, spec);
}

uintptr sys_mount(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *arg1, *spec;
  int flag;
  int fd;

  fd = SYSCALL_ARG(list, int);
  arg1 = SYSCALL_ARG(list, char *);
  flag = SYSCALL_ARG(list, int);
  spec = SYSCALL_ARG(list, char *);
  if (up != nil && up->pid == 4) {
    bprint("sys_mount: pid=%lud fd=%d flags=%d arg=%s spec=%s\n", up->pid, fd,
           flag, arg1 ? arg1 : "", spec ? spec : "");
    uintptr ret = (uintptr)bindmount(1, fd, -1, nil, arg1, flag, spec);
    bprint("sys_mount: pid=%lud ret=%#p\n", up->pid, ret);
    return ret;
  }
  return (uintptr)bindmount(1, fd, -1, nil, arg1, flag, spec);
}

uintptr sysunmount(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Chan *cmount, *cmounted;
  char *name, *old;

  name = SYSCALL_ARG(list, char *);
  old = SYSCALL_ARG(list, char *);

  cmounted = nil;
  validaddr((uintptr)old, 1, 0);
  cmount = namec(old, Amount, 0, 0);
  if (waserror()) {
    cclose(cmount);
    if (cmounted != nil)
      cclose(cmounted);
    nexterror();
  }
  if (name != nil) {
    validaddr((uintptr)name, 1, 0);
    cmounted = namec(name, Aunmount, OREAD, 0);
  }
  cunmount(cmount, cmounted);
  poperror();
  cclose(cmount);
  if (cmounted != nil)
    cclose(cmounted);
  return 0;
}

uintptr syscreate(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd, mode, perm;
  char *name;
  Chan *c;

  name = SYSCALL_ARG(list, char *);
  mode = SYSCALL_ARG(list, int);
  perm = SYSCALL_ARG(list, int);
  openmode(mode & ~OEXCL); /* error check only; OEXCL okay here */
  validaddr((uintptr)name, 1, 0);
  c = namec(name, Acreate, mode, perm);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  fd = newfd(c, mode);
  if (fd < 0)
    error(Enofd);
  poperror();
  return (uintptr)fd;
}

uintptr sysremove(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *name;
  Chan *c;

  name = SYSCALL_ARG(list, char *);
  validaddr((uintptr)name, 1, 0);
  c = namec(name, Aremove, 0, 0);
  /*
   * Removing mount points is disallowed to avoid surprises
   * (which should be removed: the mount point or the mounted Chan?).
   */
  if (c->ismtpt) {
    cclose(c);
    error(Eismtpt);
  }
  if (waserror()) {
    c->type = 0; /* see below */
    cclose(c);
    nexterror();
  }
  devtab[c->type]->remove(c);
  /*
   * Remove clunks the fid, but we need to recover the Chan
   * so fake it up.  rootclose() is known to be a nop.
   */
  c->type = 0;
  poperror();
  cclose(c);
  return 0;
}

static long wstat(Chan *c, uchar *d, int nd) {
  long l;
  int namelen;
  char *p;

  if (waserror()) {
    cclose(c);
    nexterror();
  }
  if (c->ismtpt) {
    /*
     * Renaming mount points is disallowed to avoid surprises
     * (which should be renamed? the mount point or the mounted Chan?).
     */
    dirname(d, &namelen);
    if (namelen) {
      p = chanpath(c);
      namelenerror(p, strlen(p), Eismtpt);
    }
  }
  l = devtab[c->type]->wstat(c, d, nd);
  poperror();
  cclose(c);
  return l;
}

uintptr syswstat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *name;
  uchar *s;
  Chan *c;
  uint l;

  name = SYSCALL_ARG(list, char *);
  s = SYSCALL_ARG(list, uchar *);
  l = SYSCALL_ARG(list, uint);
  validaddr((uintptr)s, l, 0);
  validstat(s, l);
  validaddr((uintptr)name, 1, 0);
  c = namec(name, Aaccess, 0, 0);
  return (uintptr)wstat(c, s, l);
}

uintptr sysfwstat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  uchar *s;
  Chan *c;
  uint l;
  int fd;

  fd = SYSCALL_ARG(list, int);
  s = SYSCALL_ARG(list, uchar *);
  l = SYSCALL_ARG(list, uint);
  validaddr((uintptr)s, l, 0);
  validstat(s, l);
  c = fdtochan(fd, -1, 1, 1);
  return (uintptr)wstat(c, s, l);
}

static void packoldstat(uchar *buf, Dir *d) {
  uchar *p;
  ulong q;

  /* lay down old stat buffer - grotty code but it's temporary */
  p = buf;
  strncpy((char *)p, d->name, 28);
  p += 28;
  strncpy((char *)p, d->uid, 28);
  p += 28;
  strncpy((char *)p, d->gid, 28);
  p += 28;
  q = (ulong)d->qid.path &
      ~DMDIR; /* make sure doesn't accidentally look like directory */
  if (d->qid.type & QTDIR) /* this is the real test of a new directory */
    q |= DMDIR;
  PBIT32(p, q);
  p += BIT32SZ;
  PBIT32(p, d->qid.vers);
  p += BIT32SZ;
  PBIT32(p, d->mode);
  p += BIT32SZ;
  PBIT32(p, d->atime);
  p += BIT32SZ;
  PBIT32(p, d->mtime);
  p += BIT32SZ;
  PBIT64(p, d->length);
  p += BIT64SZ;
  PBIT16(p, d->type);
  p += BIT16SZ;
  PBIT16(p, d->dev);
}

uintptr sys_stat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *name;
  uchar *s;
  Chan *c;
  Dir *d;

  name = SYSCALL_ARG(list, char *);
  s = SYSCALL_ARG(list, uchar *);
  validaddr((uintptr)s, 116, 1);
  validaddr((uintptr)name, 1, 0);
  c = namec(name, Aaccess, 0, 0);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  d = dirchanstat(c);
  if (waserror()) {
    free(d);
    nexterror();
  }
  if ((name = pathlast(c->path)) != nil)
    d->name = name;
  packoldstat(s, d);
  poperror();
  free(d);
  poperror();
  cclose(c);
  return 0;
}

uintptr sys_fstat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *name;
  uchar *s;
  Chan *c;
  Dir *d;
  int fd;

  fd = SYSCALL_ARG(list, int);
  s = SYSCALL_ARG(list, uchar *);
  validaddr((uintptr)s, 116, 1);
  c = fdtochan(fd, -1, 0, 1);
  if (waserror()) {
    cclose(c);
    nexterror();
  }
  d = dirchanstat(c);
  if (waserror()) {
    free(d);
    nexterror();
  }
  if ((name = pathlast(c->path)) != nil)
    d->name = name;
  packoldstat(s, d);
  poperror();
  free(d);
  poperror();
  cclose(c);
  return 0;
}

uintptr sys_wstat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  error("old wstat system call - recompile");
}

uintptr sys_fwstat(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  error("old fwstat system call - recompile");
}

/* Exposed kernel read function */
long kread(int fd, void *buf, long n) { return read(fd, buf, n, nil); }

/* Exposed kernel write function */
long kwrite(int fd, void *buf, long n) { return write(fd, buf, n, nil, 0); }

/* Exposed kernel seek function */
vlong kseek(int fd, vlong offset, int whence) {
  return sseek(fd, offset, whence);
}
