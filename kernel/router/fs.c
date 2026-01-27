#ifndef __FRAMAC__
#include "router.h"
#include <error.h>

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
    switch (t->scallnr) {

    case SYS_CLOSE: {
      extern void fdclose(int, int);
      extern Chan *fdtochan(int, int, int, int);
      /* Format: [fid 4] */
      ptr = tsyscall_skip_argc(ptr, ep, 1);
      if (ptr + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fd = GBIT32(ptr);
      ptr += 4;

      print("router_fs: SYS_CLOSE fd=%d\n", fd);

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      Chan *c = fdtochan(fd, -1, 0, 0);
      if (c)
        cclose(c);
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
      extern vlong sseek(int, vlong, int);

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

      print("router_fs: SYS_SEEK fd=%d offset=%lld whence=%d\n", fd, offset,
            whence);

      vlong newpos;
      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      newpos = sseek(fd, offset, whence);
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

      print("router_fs: %s fd=%d count=%d off=%lld\n",
            t->scallnr == SYS_WRITE ? "SYS_WRITE" : "SYS_PWRITE", fid, count,
            offset);

      extern Chan *fdtochan(int, int, int, int);
      Chan *c;
      long n;

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      c = fdtochan(fid, OWRITE, 1, 1);
      if (waserror()) {
        cclose(c);
        nexterror();
      }
      if (c->qid.type & QTDIR)
        error(Eisdir);
      n = devtab[c->type]->write(c, ptr, count, offset);
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

      print("router_fs: %s fd=%d count=%d off=%lld\n",
            t->scallnr == SYS_READ ? "SYS_READ" : "SYS_PREAD", fid, count,
            offset);

      extern Chan *fdtochan(int, int, int, int);
      Chan *c;
      long n;
      int rsyscall_hdr = 4 + 1 + 2 + 8 + 4;

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      c = fdtochan(fid, OREAD, 1, 1);
      if (waserror()) {
        cclose(c);
        nexterror();
      }
      if (c->qid.type & QTDIR)
        error(Eisdir);

      if (count > P9_REPLY_SIZE - rsyscall_hdr) {
        error("read count too large");
      }

      uchar *data = (uchar *)p->p9page + P9_MSG_OFFSET + rsyscall_hdr;
      n = devtab[c->type]->read(c, data, count, offset);
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

          print("router_fs: SYS_STAT '%s'\n", path);

          if (waserror()) {
            if (c)
              cclose(c);
            free(path);
            r->type = Rerror;
            snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
            return -1;
          }
          c = namec(path, Aaccess, 0, 0);
        } else if (remaining == 4) {
          int fid = GBIT32(ptr);
          ptr += 4;

          print("router_fs: SYS_STAT fd=%d\n", fid);

          if (waserror()) {
            if (c)
              cclose(c);
            r->type = Rerror;
            snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
            return -1;
          }
          c = fdtochan(fid, -1, 0, 1);
        } else {
          r->type = Rerror;
          r->ename = "bad stat msg";
          return -1;
        }
      }

      extern Chan *fdtochan(int, int, int, int);
      int rsyscall_hdr = 4 + 1 + 2 + 8 + 4;
      uchar *data = (uchar *)p->p9page + P9_MSG_OFFSET + rsyscall_hdr;
      n = devtab[c->type]->stat(c, data, P9_REPLY_SIZE - rsyscall_hdr);
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

      print("router_fs: SYS_WSTAT '%s' nstat=%d\n", path, nstat);

      extern void validstat(uchar * s, int n);
      Chan *c = nil;
      if (waserror()) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }

      validstat(ptr, nstat);
      c = namec(path, Aaccess, 0, 0);
      devtab[c->type]->wstat(c, ptr, nstat);
      poperror();
      cclose(c);
      free(path);

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

      c = namec(path, Aremove, 0, 0);
      poperror();
      /* devremove calls cclose(c) implicitly if successful? No, 9front remove
       * calls remove then cclose */
      /* BUT namec with Aremove returns a channel. We call
       * devtab[c->type]->remove(c) */
      devtab[c->type]->remove(c);
      /* remove closes the channel */

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
          snprint(r->ename, sizeof(r->ename),
                  "pebble: insufficient budget for mount");
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
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
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

      print("router_fs: SYS_OPEN '%s' mode=%d\n", path, mode);

      if (waserror()) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }

      openmode(mode);
      c = namec(path, Aopen, mode, 0);
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

      print("router_fs: SYS_CREATE '%s' mode=%d perm=%o\n", path, mode, perm);

      if (waserror()) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }

      openmode(mode);
      c = namec(path, Acreate, mode, perm);
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
    openmode(t->mode);
    c = namec(t->name, Aopen, t->mode, 0);
    fd = newfd(c, t->mode);
    poperror();

    /* Build Rsysopen response */
    r->type = Rsysopen;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;

    print("router_fs: Tsysopen '%s' -> fd=%d\n", t->name, fd);
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
    c = namec(t->name, Acreate, t->mode, t->perm);
    fd = newfd(c, t->mode);
    poperror();

    /* Build Rsyscreate response */
    r->type = Rsyscreate;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;

    print("router_fs: Tsyscreate '%s' perm=0%o -> fd=%d\n", t->name, t->perm,
          fd);
    return 0;
  }

  if (t->type == Tsysread || t->type == Tsyspread) {
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    c = fdtochan(t->fid, OREAD, 1, 1);
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

    n = devtab[c->type]->read(c, r->data, t->count, t->offset);
    poperror();
    cclose(c);
    poperror();

    /* Build Rsysread response */
    r->type = (t->type == Tsysread) ? Rsysread : Rsyspread;
    r->tag = t->tag;
    r->count = n;
    /* r->data now contains the data */

    print("router_fs: %s fd=%d count=%d offset=%lld -> %ld bytes\n",
          t->type == Tsysread ? "Tsysread" : "Tsyspread", t->fid, t->count,
          t->offset, n);
    return 0;
  }

  if (t->type == Tsyswrite || t->type == Tsyspwrite) {
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    c = fdtochan(t->fid, OWRITE, 1, 1);
    if (waserror()) {
      cclose(c);
      nexterror();
    }

    if (c->qid.type & QTDIR)
      error(Eisdir);

    n = devtab[c->type]->write(c, t->data, t->count, t->offset);
    poperror();
    cclose(c);
    poperror();

    /* Build Rsyswrite response */
    r->type = (t->type == Tsyswrite) ? Rsyswrite : Rsyspwrite;
    r->tag = t->tag;
    r->count = n;

    print("router_fs: %s fd=%d count=%d offset=%lld -> %ld bytes\n",
          t->type == Tsyswrite ? "Tsyswrite" : "Tsyspwrite", t->fid, t->count,
          t->offset, n);
    return 0;
  }

  if (t->type == Tsysclose) {
    extern void fdclose(int, int);
    extern Chan *fdtochan(int, int, int, int);

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    Chan *c = fdtochan(t->fid, -1, 0, 0);
    if (c)
      cclose(c);
    fdclose(t->fid, 0);
    poperror();

    /* Build Rsysclose response */
    r->type = Rsysclose;
    r->tag = t->tag;

    print("router_fs: Tsysclose fd=%d\n", t->fid);
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

    c = namec(t->name, Aremove, 0, 0);
    poperror();

    /* Build Rsysremove response */
    r->type = Rsysremove;
    r->tag = t->tag;

    print("router_fs: Tsysremove '%s'\n", t->name);
    return 0;
  }

  if (t->type == Tsysdup) {
    extern int newfd(Chan *, int);
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    int nfd;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    /* Get the channel from oldfd */
    c = fdtochan(t->fid, -1, 0, 1);
    incref(&c->ref);

    /* Create new fd */
    if (t->newfid == -1) {
      nfd = newfd(c, 0);
    } else {
      /* Dup to specific fd - use newfd to handle it properly */
      extern void fdclose(int, int);
      if (t->newfid >= 0) {
        fdclose(t->newfid, 0);
        /* Try to place channel at specific fd */
        up->fgrp->fd[t->newfid] = c;
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

    print("router_fs: Tsysdup oldfd=%d newfd=%d -> %d\n", t->fid, t->newfid,
          nfd);
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

    args[0] = (ulong)t->name;
    args[1] = (ulong)statbuf;
    args[2] = P9_REPLY_SIZE - rsysstat_hdr;
    n = (long)sysstat(args);
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

    args[0] = (ulong)t->name;
    args[1] = (ulong)t->stat;
    args[2] = (ulong)t->nstat;
    syswstat(args);
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
