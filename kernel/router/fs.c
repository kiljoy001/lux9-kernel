#ifndef __FRAMAC__
#include "router.h"
#include <error.h>

static char *router_path_last(char *path) {
  char *p;

  if (path == nil)
    return nil;
  p = path + strlen(path);
  while (p > path && p[-1] == '/')
    *--p = 0;
  while (p > path && p[-1] != '/')
    p--;
  return p;
}

static char *router_dirname(uchar *p, int *n) {
  p += BIT16SZ + BIT16SZ + BIT32SZ + BIT8SZ + BIT32SZ + BIT64SZ + BIT32SZ +
       BIT32SZ + BIT32SZ + BIT64SZ;
  *n = GBIT16(p);
  return (char *)p + BIT16SZ;
}

static long router_dirsetname(char *name, int len, uchar *p, long n,
                              long maxn) {
  char *oname;
  int olen;
  long nn;

  if (n == BIT16SZ)
    return BIT16SZ;

  oname = router_dirname(p, &olen);
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

static Chan *router_set_chan_path(Chan *c, char *path) {
  if (c == nil || path == nil)
    return c;
  c = cunique(c);
  if (c->path != nil)
    pathclose(c->path);
  c->path = newpath((BString){path, (int)strlen(path)});
  return c;
}

static int router_split_rel(char *rel, char **names, int maxnames) {
  int n = 0;
  char *p;

  if (rel == nil || *rel == 0)
    return 0;
  p = rel;
  while (*p != 0) {
    while (*p == '/')
      p++;
    if (*p == 0)
      break;
    if (n >= maxnames)
      error("path too deep");
    names[n++] = p;
    while (*p != 0 && *p != '/')
      p++;
    if (*p == '/') {
      *p = 0;
      p++;
    }
  }
  return n;
}

static void router_walk_relative(Chan **cp, char **names, int nname) {
  Walkqid *wq;
  Chan *c;
  Chan *nc;

  if (nname == 0)
    return;

  c = *cp;
  wq = devtab[devno(c->type, 0)]->walk(c, nil, names, nname);
  if (wq == nil || wq->clone == nil || wq->nqid != nname) {
    if (wq != nil) {
      if (wq->clone != nil)
        cclose(wq->clone);
      free(wq);
    }
    error(Enonexist);
  }

  nc = wq->clone;
  free(wq);
  cclose(c);
  *cp = nc;
}

static Chan *router_attach_managed(char *path, char **rel_out) {
  if (!p9_ns_root_available(path))
    return nil;
  return p9_ns_attach_root(path, rel_out);
}

static Chan *router_open_managed(char *path, int mode) {
  Chan *c;
  char *rel;
  char *scratch;
  char *names[MAXWELEM];
  int nname;

  c = router_attach_managed(path, &rel);
  if (c == nil)
    return nil;

  scratch = smalloc(strlen(rel) + 1);
  strcpy(scratch, rel);

  if (waserror()) {
    cclose(c);
    free(scratch);
    nexterror();
  }

  nname = router_split_rel(scratch, names, nelem(names));
  router_walk_relative(&c, names, nname);
  c = devtab[devno(c->type, 0)]->open(c, mode);
  c = router_set_chan_path(c, path);

  poperror();
  free(scratch);
  return c;
}

static Chan *router_create_managed(char *path, int mode, ulong perm) {
  Chan *c;
  char *rel;
  char *scratch;
  char *names[MAXWELEM];
  int nname;
  char *leaf;

  c = router_attach_managed(path, &rel);
  if (c == nil)
    return nil;

  scratch = smalloc(strlen(rel) + 1);
  strcpy(scratch, rel);

  if (waserror()) {
    cclose(c);
    free(scratch);
    nexterror();
  }

  nname = router_split_rel(scratch, names, nelem(names));
  if (nname <= 0)
    error(Ebadarg);
  leaf = names[nname - 1];
  router_walk_relative(&c, names, nname - 1);
  c = devtab[devno(c->type, 0)]->create(c, leaf, mode, perm);
  c = router_set_chan_path(c, path);

  poperror();
  free(scratch);
  return c;
}

static Chan *router_access_managed(char *path) {
  Chan *c;
  char *rel;
  char *scratch;
  char *names[MAXWELEM];
  int nname;

  c = router_attach_managed(path, &rel);
  if (c == nil)
    return nil;

  scratch = smalloc(strlen(rel) + 1);
  strcpy(scratch, rel);

  if (waserror()) {
    cclose(c);
    free(scratch);
    nexterror();
  }

  nname = router_split_rel(scratch, names, nelem(names));
  router_walk_relative(&c, names, nname);
  c = router_set_chan_path(c, path);

  poperror();
  free(scratch);
  return c;
}

static long router_stat_managed(char *path, uchar *buf, long maxn) {
  Chan *c;
  long n;
  char *name;

  c = router_access_managed(path);
  if (c == nil)
    return -1;

  if (waserror()) {
    cclose(c);
    nexterror();
  }
  n = devtab[devno(c->type, 0)]->stat(c, buf, maxn);
  name = router_path_last(path);
  if (name != nil)
    n = router_dirsetname(name, (int)strlen(name), buf, n, maxn);
  poperror();
  cclose(c);
  return n;
}

static long router_wstat_managed(char *path, uchar *buf, int nbuf) {
  Chan *c;
  long n;

  c = router_access_managed(path);
  if (c == nil)
    return -1;

  if (waserror()) {
    cclose(c);
    nexterror();
  }
  n = devtab[devno(c->type, 0)]->wstat(c, buf, nbuf);
  poperror();
  cclose(c);
  return n;
}

static int router_remove_managed(char *path) {
  Chan *c;

  c = router_access_managed(path);
  if (c == nil)
    return -1;

  if (waserror()) {
    c->type = 0;
    cclose(c);
    nexterror();
  }
  devtab[devno(c->type, 0)]->remove(c);
  c->type = 0;
  poperror();
  cclose(c);
  return 0;
}

static void router_note_last_elem(char *path) {
  char *p, *last;
  Rune r;
  int n;

  if (path == nil || path[0] == 0) {
    kstrcpy(up->genbuf, ".", sizeof(up->genbuf));
    return;
  }

  p = path;
  if (p[0] == '#') {
    p++;
    if (*p != 0) {
      p += chartorune(&r, p);
      while (*p != 0 && *p != '/')
        p++;
      if (*p == '/')
        p++;
    }
  } else if (p[0] == '/') {
    p++;
  }

  last = nil;
  while (*p != 0) {
    while (*p == '/')
      p++;
    if (*p == 0)
      break;
    last = p;
    while (*p != 0 && *p != '/')
      p++;
  }

  if (last == nil) {
    kstrcpy(up->genbuf, ".", sizeof(up->genbuf));
    return;
  }

  n = 0;
  while (last[n] != 0 && last[n] != '/' && n < (int)sizeof(up->genbuf) - 1) {
    up->genbuf[n] = last[n];
    n++;
  }
  up->genbuf[n] = 0;
}

static void router_walk_mutable(Chan **cp, char *path) {
  char *names[MAXWELEM];
  char *p;
  int nname;

  p = path;
  nname = 0;
  while (*p != 0) {
    while (*p == '/')
      p++;
    if (*p == 0)
      break;
    names[nname++] = p;
    while (*p != 0 && *p != '/')
      p++;
    if (*p == '/')
      *p++ = 0;
    if (nname == nelem(names)) {
      router_walk_relative(cp, names, nname);
      nname = 0;
    }
  }
  if (nname > 0)
    router_walk_relative(cp, names, nname);
}

static Chan *router_attach_device_mutable(char *path, char **rel_out) {
  char *p, *spec;
  Rune r;
  int t;

  if (path == nil || path[0] != '#' || path[1] == 0)
    error(Ebadsharp);

  p = path + 1;
  p += chartorune(&r, p);
  t = devno(r, 1);
  if (t == -1)
    error(Ebadsharp);
  if (!devallowed(up->pgrp, r))
    error(Enoattach);

  spec = p;
  while (*p != 0 && *p != '/')
    p++;
  if (*p == '/') {
    *p = 0;
    *rel_out = p + 1;
  } else {
    *rel_out = p;
  }

  return devtab[t]->attach(spec);
}

Chan *router_resolve_path(char *path, int amode, int omode) {
  Chan *c;
  char *scratch;
  char *rel;

  if (path == nil || path[0] == 0)
    error("empty file name");

  router_note_last_elem(path);

  if (path[0] == '/' && p9_ns_managed_path(path)) {
    switch (amode) {
    case Aopen:
      openmode(omode);
      c = router_open_managed(path, omode);
      if (c == nil)
        error("namespace root unavailable");
      return c;
    case Aaccess:
    case Aremove:
    case Atodir:
    case Amount:
    case Abind:
      c = router_access_managed(path);
      if (c == nil)
        error("namespace root unavailable");
      if (amode == Atodir && (c->qid.type & QTDIR) == 0) {
        cclose(c);
        error(Enotdir);
      }
      return c;
    default:
      error("unsupported router amode");
    }
  }

  scratch = smalloc(strlen(path) + 1);
  strcpy(scratch, path);
  c = nil;

  if (waserror()) {
    if (c != nil)
      cclose(c);
    free(scratch);
    nexterror();
  }

  if (scratch[0] == '/') {
    c = devattach('/', nil);
    rel = scratch + 1;
  } else if (scratch[0] == '#') {
    c = router_attach_device_mutable(scratch, &rel);
  } else {
    if (up->dot == nil)
      error("current directory unavailable");
    c = up->dot;
    incref((Ref *)&c->ref);
    rel = scratch;
  }

  router_walk_mutable(&c, rel);

  switch (amode) {
  case Aopen:
    openmode(omode);
    if ((omode & 3) == OEXEC && (c->qid.type & QTDIR) != 0)
      error("cannot exec directory");
    c = cunique(c);
    c = devtab[devno(c->type, 0)]->open(c, omode & ~OCEXEC);
    if (omode & ORCLOSE)
      c->flag |= CRCLOSE;
    break;
  case Aaccess:
  case Aremove:
  case Amount:
  case Abind:
    break;
  case Atodir:
    if ((c->qid.type & QTDIR) == 0)
      error(Enotdir);
    break;
  default:
    error("unsupported router amode");
  }

  poperror();
  free(scratch);
  return c;
}

int router_dispatch_fs(Proc *p, Fcall *t, Fcall *r) {
  /*@
    @ requires \valid(p);
    @ requires \valid(t);
    @ requires \valid(r);
    @ ensures \result == 0 || \result == -1;
    @*/
  uchar *ep = t->sdata + t->scount;
  uchar *ptr = t->sdata;

  /* Handle Generic Tsyscall (130) */
  if (t->type == Tsyscall) {
    if (t->scallnr == BIND) {
      extern uintptr sysbind(void *list_void);
      ulong args[3];
      int oldlen, newlen;
      uchar *oldp, *newp, *scratch, *msg_end;
      uchar *oldk, *newk;
      ulong flags;
      uintptr kpage, ubase, uold, unew;

      ptr = tsyscall_skip_argc(ptr, ep, 3);
      if (ptr + 2 > ep) {
        r->type = Rerror;
        r->ename = "short bind msg";
        return -1;
      }

      oldlen = GBIT16(ptr);
      ptr += 2;
      if (ptr + oldlen + 2 + 4 > ep) {
        r->type = Rerror;
        r->ename = "short bind msg";
        return -1;
      }
      oldp = ptr;
      ptr += oldlen;

      newlen = GBIT16(ptr);
      ptr += 2;
      if (ptr + newlen + 4 > ep) {
        r->type = Rerror;
        r->ename = "short bind msg";
        return -1;
      }
      newp = ptr;
      ptr += newlen;
      flags = GBIT32(ptr);

      msg_end = (uchar *)p->p9page + P9_MSG_OFFSET + P9_MSG_SIZE;
      if (ep + oldlen + newlen + 2 > msg_end) {
        r->type = Rerror;
        r->ename = "bind scratch overflow";
        return -1;
      }

      scratch = ep;
      oldk = scratch;
      memmove(oldk, oldp, oldlen);
      oldk[oldlen] = 0;
      scratch += oldlen + 1;
      newk = scratch;
      memmove(newk, newp, newlen);
      newk[newlen] = 0;

      kpage = (uintptr)p->p9page;
      ubase = p9_user_base(p);
      uold = ubase + ((uintptr)oldk - kpage);
      unew = ubase + ((uintptr)newk - kpage);

      args[0] = (ulong)uold;
      args[1] = (ulong)unew;
      args[2] = flags;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      sysbind(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    if (t->scallnr == FD2PATH) {
      extern uintptr sysfd2path(void *list_void);
      ulong args[3];
      int fd, len, rsyscall_hdr;
      uchar *data;
      uintptr kpage, ubase, udata;

      ptr = tsyscall_skip_argc(ptr, ep, 2);
      if (ptr + 8 > ep) {
        r->type = Rerror;
        r->ename = "short fd2path msg";
        return -1;
      }

      fd = GBIT32(ptr);
      ptr += 4;
      len = GBIT32(ptr);
      if (len <= 0) {
        r->type = Rerror;
        r->ename = "bad fd2path len";
        return -1;
      }

      rsyscall_hdr = 4 + 1 + 2 + 8 + 4;
      if (len > P9_REPLY_SIZE - rsyscall_hdr)
        len = P9_REPLY_SIZE - rsyscall_hdr;
      data = (uchar *)p->p9page + P9_MSG_OFFSET + rsyscall_hdr;
      kpage = (uintptr)p->p9page;
      ubase = p9_user_base(p);
      udata = ubase + ((uintptr)data - kpage);

      args[0] = (ulong)fd;
      args[1] = (ulong)udata;
      args[2] = (ulong)len;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      sysfd2path(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = (u32int)strlen((char *)data) + 1;
      r->sdata = data;
      return 0;
    }

    if (t->scallnr == UNMOUNT) {
      extern uintptr sysunmount(void *list_void);
      ulong args[2];
      int namelen, oldlen;
      uchar *namep, *oldp, *scratch, *msg_end;
      uchar *namek, *oldk;
      uintptr kpage, ubase, uname, uold;

      ptr = tsyscall_skip_argc(ptr, ep, 2);
      if (ptr + 2 > ep) {
        r->type = Rerror;
        r->ename = "short unmount msg";
        return -1;
      }

      namelen = GBIT16(ptr);
      ptr += 2;
      if (ptr + namelen + 2 > ep) {
        r->type = Rerror;
        r->ename = "short unmount msg";
        return -1;
      }
      namep = ptr;
      ptr += namelen;

      oldlen = GBIT16(ptr);
      ptr += 2;
      if (ptr + oldlen > ep) {
        r->type = Rerror;
        r->ename = "short unmount msg";
        return -1;
      }
      oldp = ptr;

      msg_end = (uchar *)p->p9page + P9_MSG_OFFSET + P9_MSG_SIZE;
      if (ep + namelen + oldlen + 2 > msg_end) {
        r->type = Rerror;
        r->ename = "unmount scratch overflow";
        return -1;
      }

      scratch = ep;
      namek = scratch;
      memmove(namek, namep, namelen);
      namek[namelen] = 0;
      scratch += namelen + 1;
      oldk = scratch;
      memmove(oldk, oldp, oldlen);
      oldk[oldlen] = 0;

      kpage = (uintptr)p->p9page;
      ubase = p9_user_base(p);
      uname = ubase + ((uintptr)namek - kpage);
      uold = ubase + ((uintptr)oldk - kpage);

      args[0] = namelen > 0 ? (ulong)uname : (ulong)0;
      args[1] = (ulong)uold;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      sysunmount(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    switch (t->scallnr) {

    case SYS_NSROOT_PUBLISH: {
      extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
      Chan *c = nil;
      char *path = nil;
      char *aname = nil;
      int fd, pathlen, anamelen;

      ptr = tsyscall_skip_argc(ptr, ep, 3);
      if (ptr + 4 + 2 + 2 > ep) {
        r->type = Rerror;
        r->ename = "short nsroot publish msg";
        return -1;
      }

      fd = GBIT32(ptr);
      ptr += 4;
      pathlen = GBIT16(ptr);
      ptr += 2;
      if (ptr + pathlen + 2 > ep) {
        r->type = Rerror;
        r->ename = "short nsroot publish msg";
        return -1;
      }
      path = smalloc(pathlen + 1);
      memmove(path, ptr, pathlen);
      path[pathlen] = 0;
      ptr += pathlen;

      anamelen = GBIT16(ptr);
      ptr += 2;
      if (ptr + anamelen > ep) {
        free(path);
        r->type = Rerror;
        r->ename = "short nsroot publish msg";
        return -1;
      }
      aname = smalloc(anamelen + 1);
      memmove(aname, ptr, anamelen);
      aname[anamelen] = 0;

      if (waserror()) {
        if (c != nil)
          cclose(c);
        free(path);
        free(aname);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }

      if (!p9_ns_root_exact(path))
        error("bad namespace root");
      p9_ns_enforce_owner(path, nil);
      c = fdtochan_fgrp(p->fgrp, fd, ORDWR, 0, 1);
      p9_ns_publish_root(path, c, aname);
      poperror();

      cclose(c);
      free(path);
      free(aname);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_NSROOT_UNPUBLISH: {
      char *path;
      int len;

      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 2 > ep) {
        r->type = Rerror;
        r->ename = "short nsroot unpublish msg";
        return -1;
      }

      len = GBIT16(ptr);
      ptr += 2;
      if (ptr + len > ep) {
        r->type = Rerror;
        r->ename = "short nsroot unpublish msg";
        return -1;
      }

      path = smalloc(len + 1);
      memmove(path, ptr, len);
      path[len] = 0;

      if (waserror()) {
        free(path);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }

      if (!p9_ns_root_exact(path))
        error("bad namespace root");
      p9_ns_enforce_owner(path, nil);
      p9_ns_unpublish_root(path);
      poperror();

      free(path);
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_CLOSE: {
      extern void fdclose(int, int);
      extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
      /* Format: [fid 4] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fd = GBIT32(ptr);
      ptr += 4;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      /* fdclose handles everything: removes from fd table and calls cclose */
      fdclose(fd, 0);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_SEEK: {
      /* Seek syscall - Format: [fd 4] [offset 8] [whence 4]
       * whence: 0=SEEK_SET, 1=SEEK_CUR, 2=SEEK_END
       * Returns new position as retval
       */
      ptr = tsyscall_skip_argc(ptr, ep, 3);
      if (ptr + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        r->ename = "short seek msg";
        return -1;
      }
      int fd = GBIT32(ptr);
      ptr += 4;
      vlong offset = GBIT64(ptr);
      ptr += 8;
      int whence = GBIT32(ptr);
      ptr += 4;

      extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
      Chan *c;
      vlong newpos;
      int dev;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      c = fdtochan_fgrp(p->fgrp, fd, -1, 1, 1);
      if (waserror()) {
        cclose(c);
        nexterror();
      }

      dev = devno(c->type, 0);
      if (devtab[dev]->dc == L'|')
        error(Eisstream);

      switch (whence) {
      case 0:
        newpos = offset;
        if ((c->qid.type & QTDIR) && newpos != 0)
          error(Eisdir);
        if (newpos < 0)
          error(Enegoff);
        c->offset = newpos;
        break;
      case 1:
        if (c->qid.type & QTDIR)
          error(Eisdir);
        lock(&c->lock);
        newpos = c->offset + offset;
        if (newpos < 0) {
          unlock(&c->lock);
          error(Enegoff);
        }
        c->offset = newpos;
        unlock(&c->lock);
        break;
      case 2:
        if (c->qid.type & QTDIR)
          error(Eisdir);
        {
          Dir *d;

          d = dirchanstat(c);
          newpos = d->length + offset;
          free(d);
        }
        if (newpos < 0)
          error(Enegoff);
        c->offset = newpos;
        break;
      default:
        error(Ebadarg);
      }
      c->uri = 0;
      c->dri = 0;
      poperror();
      cclose(c);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = newpos;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_WRITE:
    case SYS_PWRITE: {
      /* Format: [fid 4] [offset 8] [count 4] [data...] */
      ptr = tsyscall_skip_argc(ptr, ep, 3);
      if (ptr + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fid = GBIT32(ptr);
      ptr += 4;
      vlong offset = GBIT64(ptr);
      ptr += 8;
      int count = GBIT32(ptr);
      ptr += 4;
      if (ptr + count > ep) {
        r->type = Rerror;
        return -1;
      }

      extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
      Chan *c;
      long n;
      vlong ioff;
      int autoff;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      autoff = 0;
      ioff = 0;
      c = fdtochan_fgrp(p->fgrp, fid, OWRITE, 1, 1);
      if (waserror()) {
        if (autoff) {
          lock(&c->lock);
          c->offset = ioff;
          unlock(&c->lock);
        }
        cclose(c);
        nexterror();
      }
      if (c->qid.type & QTDIR)
        error(Eisdir);
      if (t->scallnr == SYS_WRITE) {
        lock(&c->lock);
        ioff = c->offset;
        c->offset += count;
        unlock(&c->lock);
        autoff = 1;
      } else {
        ioff = offset;
      }
      if (ioff < 0)
        error(Enegoff);
      n = devtab[devno(c->type, 0)]->write(c, ptr, count, ioff);
      if (t->scallnr == SYS_WRITE && n < count) {
        lock(&c->lock);
        c->offset -= count - n;
        unlock(&c->lock);
      }
      poperror();
      cclose(c);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = n;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_READ:
    case SYS_PREAD: {
      /* Format: [fid 4] [offset 8] [count 4] */
      ptr = tsyscall_skip_argc(ptr, ep, 3);
      if (ptr + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fid = GBIT32(ptr);
      ptr += 4;
      vlong offset = GBIT64(ptr);
      ptr += 8;
      int count = GBIT32(ptr);
      ptr += 4;

      extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
      Chan *c;
      long n;
      int rsyscall_hdr = 4 + 1 + 2 + 8 + 4;
      vlong ioff;

      /* Clamp read count to what fits in the exchange page reply area */
      int max_count = P9_REPLY_SIZE - rsyscall_hdr;
      if (count > max_count)
        count = max_count;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      c = fdtochan_fgrp(p->fgrp, fid, OREAD, 1, 1);
      if (waserror()) {
        cclose(c);
        nexterror();
      }
      if (c->qid.type & QTDIR)
        error(Eisdir);
      if (t->scallnr == SYS_READ) {
        ioff = c->offset;
        if (ioff < 0)
          error(Enegoff);
      } else {
        ioff = offset;
      }

      uchar *data = (uchar *)p->p9page + P9_MSG_OFFSET + rsyscall_hdr;
      n = devtab[devno(c->type, 0)]->read(c, data, count, ioff);
      if (t->scallnr == SYS_READ) {
        lock(&c->lock);
        c->offset = ioff + n;
        unlock(&c->lock);
      }
      poperror();
      cclose(c);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = n;
      r->scount = n;
      r->sdata = data;
      return 0;
    }

    case SYS_STAT: {
      /* Format: [path s] OR [fid 4] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }

      int remaining = ep - ptr;
      Chan *c = nil;
      char *path = nil;
      long n;

      if (remaining >= 2) {
        int len = GBIT16(ptr);
        if (2 + len == remaining) {
          ptr += 2;
          path = smalloc(len + 1);
          memmove(path, ptr, len);
          path[len] = 0;
          ptr += len;

          if (waserror()) {
            if (c)
              cclose(c);
            free(path);
            r->type = Rerror;
            r->ename = up->errstr;
            return -1;
          }
          c = router_resolve_path(path, Aaccess, 0);
        } else if (remaining == 4) {
          int fid = GBIT32(ptr);
          ptr += 4;

          if (waserror()) {
            if (c)
              cclose(c);
            r->type = Rerror;
            r->ename = up->errstr;
            return -1;
          }
          c = fdtochan_fgrp(p->fgrp, fid, -1, 0, 1);
        } else {
          r->type = Rerror;
          r->ename = "bad stat msg";
          return -1;
        }
      }

      extern Chan *fdtochan(int, int, int, int);
      int rsyscall_hdr = 4 + 1 + 2 + 8 + 4;
      uchar *data = (uchar *)p->p9page + P9_MSG_OFFSET + rsyscall_hdr;
      n = devtab[devno(c->type, 0)]->stat(c, data, P9_REPLY_SIZE - rsyscall_hdr);
      if (path)
        free(path);
      poperror();
      cclose(c);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = n;
      r->scount = n;
      r->sdata = data;
      return 0;
    }

    case SYS_WSTAT: {
      /* Format: [path s] [nstat 2] [stat bytes] */
      ptr = tsyscall_skip_argc(ptr, ep, 3);
      if (ptr + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      int len = GBIT16(ptr);
      ptr += 2;
      if (ptr + len + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }

      char *path = smalloc(len + 1);
      memmove(path, ptr, len);
      path[len] = 0;
      ptr += len;

      int nstat = GBIT16(ptr);
      ptr += 2;
      if (ptr + nstat > ep) {
        free(path);
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }

      extern void validstat(uchar * s, int n);
      Chan *c = nil;
      if (waserror()) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }

      validstat(ptr, nstat);
      if (p9_ns_managed_path(path)) {
        router_wstat_managed(path, ptr, nstat);
        poperror();
        free(path);
      } else {
        c = router_resolve_path(path, Aaccess, 0);
        devtab[devno(c->type, 0)]->wstat(c, ptr, nstat);
        poperror();
        cclose(c);
        free(path);
      }

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_REMOVE: {
      Chan *c = nil;
      /* Format: [path s] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      int len = GBIT16(ptr);
      ptr += 2;
      if (ptr + len > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }

      char *path = smalloc(len + 1);
      memmove(path, ptr, len);
      path[len] = 0;
      ptr += len;

      if (waserror()) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }

      if (p9_ns_managed_path(path)) {
        router_remove_managed(path);
        poperror();
      } else {
        c = router_resolve_path(path, Aremove, 0);
        poperror();
        /* devremove calls cclose(c) implicitly if successful? No, 9front
         * remove calls remove then cclose */
        devtab[devno(c->type, 0)]->remove(c);
      }

      free(path);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_MOUNT: {
      extern uintptr sysmount(void *list_void);

      /* Pebble: Check/deduct budget for mount (userspace only).
       * TCB processes (kp == 1) are exempt. */
      if (up != nil && up->kp == 0) {
        lock(&pebble_global_lock);
        if (up->pebble.colorless_bank < PEBBLE_MOUNT_COST) {
          unlock(&pebble_global_lock);
          r->type = Rerror;
          r->ename = "pebble: insufficient budget for mount";
          return -1;
        }
        up->pebble.colorless_bank -= PEBBLE_MOUNT_COST;
        unlock(&pebble_global_lock);
      }

      ptr = tsyscall_skip_argc(ptr, ep, 5);
      if (ptr + 4 + 4 + 2 > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }
      ulong fd = GBIT32(ptr);
      ptr += 4;
      ulong afd = GBIT32(ptr);
      ptr += 4;
      int oldlen = GBIT16(ptr);
      ptr += 2;
      if (ptr + oldlen + 4 + 2 > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }

      uchar *oldp = ptr;
      ptr += oldlen;
      if (oldp + oldlen > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }

      ulong flags = GBIT32(ptr);
      ptr += 4;

      int anamelen = GBIT16(ptr);
      ptr += 2;
      if (ptr + anamelen > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }
      uchar *anamep = ptr;

      ulong args[5];
      uchar *msg_end = (uchar *)p->p9page + P9_MSG_OFFSET + P9_MSG_SIZE;
      ulong scratch_needed = oldlen + 1 + anamelen + 1;
      if (ep + scratch_needed > msg_end) {
        r->type = Rerror;
        r->ename = "mount scratch overflow";
        return -1;
      }

      uchar *scratch = ep;
      uchar *oldk = scratch;
      memmove(oldk, oldp, oldlen);
      oldk[oldlen] = 0;
      scratch += oldlen + 1;

      uchar *anamek = scratch;
      if (anamelen > 0) {
        memmove(anamek, anamep, anamelen);
        anamek[anamelen] = 0;
        scratch += anamelen + 1;
      } else {
        anamek[0] = 0;
      }

      uintptr kpage = (uintptr)p->p9page;
      uintptr ubase = p9_user_base(p);
      uintptr uold = ubase + ((uintptr)oldk - kpage);
      uintptr uaname = ubase + ((uintptr)anamek - kpage);

      args[0] = fd;
      args[1] = afd;
      args[2] = (ulong)uold;
      args[3] = flags;
      args[4] = (ulong)uaname;

      if (waserror()) {
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      sysmount(args);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_OPEN: {
      extern int newfd(Chan * c, int mode);
      extern int openmode(ulong o);
      Chan *c = nil;
      int trace_exchange = 0;
      /* Format: [path s] [mode 1] */
      ptr = tsyscall_skip_argc(ptr, ep, 2);
      if (ptr + 2 > ep) {
      short_msg_open:
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      int len = GBIT16(ptr);
      ptr += 2;
      if (ptr + len + 1 > ep)
        goto short_msg_open;

      char *path = smalloc(len + 1);
      memmove(path, ptr, len);
      path[len] = 0;
      ptr += len;

      int mode = GBIT8(ptr);
      ptr += 1;

      if (up != nil && up->text != nil && strcmp(up->text, "nsd") == 0 &&
          path[0] == '#' && path[1] == 'X') {
        trace_exchange = 1;
        print("router SYS_OPEN[nsd]: path='%s' mode=%d\n", path, mode);
      } else if (up != nil && up->text != nil && strcmp(up->text, "hal") == 0 &&
                 path[0] == '#' &&
                 (path[1] == 'J' || path[1] == 'Y')) {
        trace_exchange = 1;
        print("router SYS_OPEN[hal]: path='%s' mode=%d\n", path, mode);
      }

      if (waserror()) {
        if (c)
          cclose(c);
        if (trace_exchange)
          print("router SYS_OPEN[%s]: path='%s' failed: %s\n",
                up->text != nil ? up->text : "?", path, up->errstr);
        free(path);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }

      c = router_resolve_path(path, Aopen, mode);
      if (trace_exchange)
        print("router SYS_OPEN[%s]: path='%s' resolved type=%d qid=%#llux\n",
              up->text != nil ? up->text : "?", path, c->type,
              (unsigned long long)c->qid.path);
      int fd = newfd(c, mode);
      if (fd < 0)
        error(Enofd);

      poperror();
      free(path);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = fd;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    case SYS_CREATE: {
      extern int newfd(Chan * c, int mode);
      extern int openmode(ulong o);
      Chan *c = nil;
      /* Format: [path s] [mode 4] [perm 4] */
      ptr = tsyscall_skip_argc(ptr, ep, 3);
      if (ptr + 2 > ep) {
      short_msg_create:
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }

      int len = GBIT16(ptr);
      ptr += 2;
      if (ptr + len + 8 > ep)
        goto short_msg_create;

      char *path = smalloc(len + 1);
      memmove(path, ptr, len);
      path[len] = 0;
      ptr += len;

      int mode = GBIT32(ptr);
      ptr += 4;
      int perm = GBIT32(ptr);
      ptr += 4;

      if (waserror()) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }

      openmode(mode);
      if (p9_ns_managed_path(path)) {
        c = router_create_managed(path, mode, perm);
        if (c == nil)
          error("namespace root unavailable");
      } else {
        c = namec(path, Acreate, mode, perm);
      }
      int fd = newfd(c, mode);
      if (fd < 0)
        error(Enofd);

      poperror();
      free(path);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = fd;
      r->scount = 0;
      r->sdata = nil;
      return 0;
    }

    default:
      r->type = Rerror;
      r->ename = "FS syscall not found";
      return -1;
    }
  }

  /* Handle Tsys* variants */
  if (t->type == Tsysopen) {
    extern int newfd(Chan *, int);
    extern int openmode(ulong);
    Chan *c = nil;
    int fd;

    if (waserror()) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    /* t->name contains path, t->mode contains mode */
    c = router_resolve_path(t->name, Aopen, t->mode);
    fd = newfd(c, t->mode);
    poperror();

    /* Build Rsysopen response */
    r->type = Rsysopen;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;

    return 0;
  }

  if (t->type == Tsyscreate) {
    extern int newfd(Chan *, int);
    extern int openmode(ulong);
    Chan *c = nil;
    int fd;

    if (waserror()) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    /* t->name contains path, t->perm contains permissions, t->mode contains
     * mode */
    openmode(t->mode);
    if (p9_ns_managed_path(t->name)) {
      c = router_create_managed(t->name, t->mode, t->perm);
      if (c == nil)
        error("namespace root unavailable");
    } else {
      c = namec(t->name, Acreate, t->mode, t->perm);
    }
    fd = newfd(c, t->mode);
    poperror();

    /* Build Rsyscreate response */
    r->type = Rsyscreate;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;

    return 0;
  }

  if (t->type == Tsysread || t->type == Tsyspread) {
    extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
    Chan *c;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    c = fdtochan_fgrp(p->fgrp, t->fid, OREAD, 1, 1);
    if (waserror()) {
      cclose(c);
      nexterror();
    }

    if (c->qid.type & QTDIR)
      error(Eisdir);

    /* Allocate buffer for read data - use exchange page data area */
    /* Rsysread header is 4+1+2+4 = 11 bytes. Data starts at msg_buf+11 */
    r->data = (char *)p->p9page + P9_MSG_OFFSET + 11;

    if (t->count > P9_REPLY_SIZE - 100) {
      error("read count too large");
    }

    n = devtab[devno(c->type, 0)]->read(c, r->data, t->count, t->offset);
    poperror();
    cclose(c);
    poperror();

    /* Build Rsysread response */
    r->type = (t->type == Tsysread) ? Rsysread : Rsyspread;
    r->tag = t->tag;
    r->count = n;
    /* r->data now contains the data */

    return 0;
  }

  if (t->type == Tsyswrite || t->type == Tsyspwrite) {
    extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
    Chan *c;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    c = fdtochan_fgrp(p->fgrp, t->fid, OWRITE, 1, 1);
    if (waserror()) {
      cclose(c);
      nexterror();
    }

    if (c->qid.type & QTDIR)
      error(Eisdir);

    n = devtab[devno(c->type, 0)]->write(c, t->data, t->count, t->offset);
    poperror();
    cclose(c);
    poperror();

    /* Build Rsyswrite response */
    r->type = (t->type == Tsyswrite) ? Rsyswrite : Rsyspwrite;
    r->tag = t->tag;
    r->count = n;

    return 0;
  }

  if (t->type == Tsysclose) {
    extern void fdclose(int, int);
    extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    Chan *c = fdtochan_fgrp(p->fgrp, t->fid, -1, 0, 0);
    if (c)
      cclose(c);
    fdclose(t->fid, 0);
    poperror();

    /* Build Rsysclose response */
    r->type = Rsysclose;
    r->tag = t->tag;

    return 0;
  }

  if (t->type == Tsysremove) {
    Chan *c = nil;

    if (waserror()) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    if (p9_ns_managed_path(t->name)) {
      router_remove_managed(t->name);
      poperror();
    } else {
      c = router_resolve_path(t->name, Aremove, 0);
      poperror();
      devtab[devno(c->type, 0)]->remove(c);
    }

    /* Build Rsysremove response */
    r->type = Rsysremove;
    r->tag = t->tag;

    return 0;
  }

  if (t->type == Tsysdup) {
    extern int newfd(Chan *, int);
    extern Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
    Chan *c;
    int nfd;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    /* Get the channel from oldfd in target process's fgrp */
    c = fdtochan_fgrp(p->fgrp, t->fid, -1, 0, 1);
    incref((Ref *)&c->ref);

    /* Create new fd */
    if (t->newfid == -1) {
      nfd = newfd(c, 0);
    } else {
      /* Dup to specific fd - use newfd to handle it properly */
      extern void fdclose(int, int);
      if (t->newfid >= 0) {
        fdclose(t->newfid, 0);
        /* Try to place channel at specific fd */
        p->fgrp->fd[t->newfid] = c;
        nfd = t->newfid;
      } else {
        cclose(c);
        error("invalid fd");
      }
    }
    poperror();

    /* Build Rsysdup response */
    r->type = Rsysdup;
    r->tag = t->tag;
    r->fid = nfd;

    return 0;
  }

  if (t->type == Tsysstat) {
    extern uintptr sysstat(void *list_void);
    ulong args[3];
    int rsysstat_hdr = 4 + 1 + 2 + 2;
    uchar *statbuf = (uchar *)p->p9page + P9_MSG_OFFSET + rsysstat_hdr;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    if (p9_ns_managed_path(t->name)) {
      n = router_stat_managed(t->name, statbuf, P9_REPLY_SIZE - rsysstat_hdr);
    } else {
      args[0] = (ulong)t->name;
      args[1] = (ulong)statbuf;
      args[2] = P9_REPLY_SIZE - rsysstat_hdr;
      n = (long)sysstat(args);
    }
    poperror();

    r->type = Rsysstat;
    r->tag = t->tag;
    r->nstat = n;
    r->stat = statbuf;
    return 0;
  }

  if (t->type == Tsysfstat) {
    extern uintptr sysfstat(void *list_void);
    ulong args[3];
    int rsysstat_hdr = 4 + 1 + 2 + 2;
    uchar *statbuf = (uchar *)p->p9page + P9_MSG_OFFSET + rsysstat_hdr;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    args[0] = (ulong)t->fid;
    args[1] = (ulong)statbuf;
    args[2] = P9_REPLY_SIZE - rsysstat_hdr;
    n = (long)sysfstat(args);
    poperror();

    r->type = Rsysfstat;
    r->tag = t->tag;
    r->nstat = n;
    r->stat = statbuf;
    return 0;
  }

  if (t->type == Tsyswstat) {
    extern uintptr syswstat(void *list_void);
    ulong args[3];

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    if (p9_ns_managed_path(t->name)) {
      validstat(t->stat, t->nstat);
      router_wstat_managed(t->name, t->stat, t->nstat);
    } else {
      args[0] = (ulong)t->name;
      args[1] = (ulong)t->stat;
      args[2] = (ulong)t->nstat;
      syswstat(args);
    }
    poperror();

    r->type = Rsyswstat;
    r->tag = t->tag;
    return 0;
  }

  if (t->type == Tsysfwstat) {
    extern uintptr sysfwstat(void *list_void);
    ulong args[3];

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    args[0] = (ulong)t->fid;
    args[1] = (ulong)t->stat;
    args[2] = (ulong)t->nstat;
    sysfwstat(args);
    poperror();

    r->type = Rsysfwstat;
    r->tag = t->tag;
    return 0;
  }

  return -1;
}
#endif
