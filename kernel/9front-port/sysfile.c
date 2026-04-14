#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>

typedef ulong *syscall_va_list;
#define SYSCALL_ARG(list, type) (*(type *)((list)++))

extern void p9_ns_enforce_owner(const char *target, const char *other);
extern void p9_ns_publish_root(const char *path, Chan *mchan, const char *spec);
extern void p9_ns_unpublish_root(const char *path);

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

uintptr sysdup(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  Chan *c, *oc;
  Fgrp *f;

  f = up->fgrp;
  fd = SYSCALL_ARG(list, int);
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

uintptr sysopen(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  Chan *c;
  char *name;
  ulong mode;
  int trace_exchange;

  name = SYSCALL_ARG(list, char *);
  mode = SYSCALL_ARG(list, ulong);
  c = nil;
  trace_exchange = 0;
  openmode(mode);
  validaddr((uintptr)name, 1, 0);
  if (up != nil && up->text != nil && strcmp(up->text, "nsd") == 0 &&
      name[0] == '#' && name[1] == 'X') {
    trace_exchange = 1;
    print("sysopen[nsd]: path='%s' mode=%#lud\n", name, mode);
  }
  if (waserror()) {
    if (trace_exchange)
      print("sysopen[nsd]: path='%s' failed: %s\n", name, up->errstr);
    if (c != nil)
      cclose(c);
    nexterror();
  }
  c = namec(name, Aopen, mode, 0);
  if (trace_exchange)
    print("sysopen[nsd]: path='%s' -> fd ok type=%d qid=%#llux\n", name,
          c->type, (unsigned long long)c->qid.path);
  fd = newfd(c, mode);
  if (fd < 0)
    error(Enofd);
  poperror();
  return (uintptr)fd;
}

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
  for (i = 0; mount != nil && i < c->uri; i++)
    mount = mount->next;

  nr = 0;
  while (mount != nil) {
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

  p += BIT16SZ;
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

static void mountrock(Chan *c, uchar *p, uchar **pe) {
  uchar *e, *r;
  int len, n;

  e = *pe;
  for (;;) {
    len = BIT16SZ + GBIT16(p);
    if (p + len >= e)
      break;
    p += len;
  }

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

  *pe = p;
}

static int mountrockread(Chan *c, uchar *op, long n, long *nn) {
  long dirlen;
  uchar *rp, *erp, *ep, *p;

  if (c->nrock == 0)
    return 0;

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

  if (rp != erp)
    memmove(c->dirrock, rp, erp - rp);
  c->nrock = erp - rp;

  *nn = p - op;
  qunlock(&c->rockqlock);
  return 1;
}

static void mountrewind(Chan *c) { c->nrock = 0; }

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
      rlock(&mh->lock);
      for (m = mh->mount; m != nil; m = m->next) {
        if (eqchantdqid(m->to, d.type, d.dev, d.qid, 1)) {
          runlock(&mh->lock);
          goto norewrite;
        }
      }
      runlock(&mh->lock);

      if (waserror())
        goto norewrite;
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

      rest = e - (p + dirlen);
      if (l > dirlen) {
        while (p + l + rest > op + maxn) {
          mountrock(c, p, &e);
          if (e == p) {
            dirlen = 0;
            goto norewrite;
          }
          rest = e - (p + dirlen);
        }
      }
      if (l != dirlen) {
        memmove(p + l, p + dirlen, rest);
        dirlen = l;
        e = p + dirlen + rest;
      }

      memmove(p, buf, l);

    norewrite:
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

static long readsys(int fd, uchar *p, long n, vlong *offp) {
  long nn, nnn;
  Chan *c;
  vlong off;

  validaddr((uintptr)p, n, 1);
  c = fdtochan(fd, OREAD, 1, 1);

  if (waserror()) {
    cclose(c);
    nexterror();
  }

  if (offp == nil)
    off = c->offset;
  else
    off = *offp;
  if (off < 0)
    error(Enegoff);

  if (off == 0) {
    if (offp == nil || (c->qid.type & QTDIR)) {
      c->offset = 0;
      c->devoffset = 0;
    }
    mountrewind(c);
    unionrewind(c);
  }

  if (c->qid.type & QTDIR) {
    if (mountrockread(c, p, n, &nn)) {
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
    lock(&c->lock);
    c->devoffset += nn;
    c->offset += nnn;
    unlock(&c->lock);
  }

  poperror();
  cclose(c);
  return nnn;
}

uintptr sys_read(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  void *buf;
  long len;

  fd = SYSCALL_ARG(list, int);
  buf = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, long);
  return (uintptr)readsys(fd, buf, len, nil);
}

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
  return (uintptr)readsys(fd, buf, len, offp);
}

static long writesys(int fd, void *buf, long len, vlong *offp, int check) {
  Chan *c;
  long m, n;
  vlong off;

  if (check)
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

  m = devtab[c->type]->write(c, buf, n, off);
  if (offp == nil && m < n) {
    lock(&c->lock);
    c->offset -= n - m;
    unlock(&c->lock);
  }

  poperror();
  cclose(c);
  return m;
}

uintptr sys_write(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int fd;
  void *buf;
  long len;

  fd = SYSCALL_ARG(list, int);
  buf = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, long);
  return (uintptr)writesys(fd, buf, len, nil, 1);
}

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
  if (off != ~0ULL)
    offp = &off;
  else
    offp = nil;
  return (uintptr)writesys(fd, buf, len, offp, 1);
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
    lock(&c->lock);
    off = o + c->offset;
    if (off < 0) {
      unlock(&c->lock);
      error(Enegoff);
    }
    c->offset = off;
    unlock(&c->lock);
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
  s += STATFIXLEN - 4 * BIT16SZ;
  m = GBIT16(s);
  s += BIT16SZ;
  if (m + 1 > sizeof buf)
    return;
  memmove(buf, s, m);
  buf[m] = '\0';
  if (strcmp(buf, "/") != 0)
    validname(buf, 0);
}

static char *pathlast(Path *p) {
  char *s;

  if (p == nil || p->len == 0)
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

    c0 = mntattach(bc, ac, spec, flag & MCACHE);
    poperror();
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
  c1 = namec(arg1, Amount, 0, 0);
  if (waserror()) {
    cclose(c1);
    nexterror();
  }

  ret = cmount(c0, c1, flag, spec);
  if (ismount && ret == 0 && arg1 != nil && c0->mchan != nil)
    p9_ns_publish_root(arg1, c0->mchan, spec);

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

  if (!has_capability(up, PEBBLE_CAP_FS))
    error(PEBBLE_E_PERM);

  arg0 = SYSCALL_ARG(list, char *);
  arg1 = SYSCALL_ARG(list, char *);
  flag = SYSCALL_ARG(list, int);
  p9_ns_enforce_owner(arg1, nil);
  return (uintptr)bindmount(0, -1, -1, arg0, arg1, flag, nil);
}

uintptr sysmount(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *arg1, *spec;
  int flag;
  int fd, afd;

  if (!has_capability(up, PEBBLE_CAP_FS))
    error(PEBBLE_E_PERM);

  fd = SYSCALL_ARG(list, int);
  afd = SYSCALL_ARG(list, int);
  arg1 = SYSCALL_ARG(list, char *);
  flag = SYSCALL_ARG(list, int);
  spec = SYSCALL_ARG(list, char *);
  p9_ns_enforce_owner(arg1, nil);
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
  p9_ns_enforce_owner(arg1, nil);
  return (uintptr)bindmount(1, fd, -1, nil, arg1, flag, spec);
}

uintptr sysunmount(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Chan *cmount, *cmounted;
  char *name, *old;

  name = SYSCALL_ARG(list, char *);
  old = SYSCALL_ARG(list, char *);
  p9_ns_enforce_owner(old, name);

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
  if (name == nil && old != nil)
    p9_ns_unpublish_root(old);
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
  openmode(mode & ~OEXCL);
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
  if (c->ismtpt) {
    cclose(c);
    error(Eismtpt);
  }
  if (waserror()) {
    c->type = 0;
    cclose(c);
    nexterror();
  }
  devtab[c->type]->remove(c);
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

  p = buf;
  strncpy((char *)p, d->name, 28);
  p += 28;
  strncpy((char *)p, d->uid, 28);
  p += 28;
  strncpy((char *)p, d->gid, 28);
  p += 28;
  q = (ulong)d->qid.path & ~DMDIR;
  if (d->qid.type & QTDIR)
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
  USED(list_void);
  error("old wstat system call - recompile");
}

uintptr sys_fwstat(void *list_void) {
  USED(list_void);
  error("old fwstat system call - recompile");
}
