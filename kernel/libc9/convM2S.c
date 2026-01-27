#include "../include/u.h"

#define _BREAK_SORT_1 1
#include <libc.h>

#define _BREAK_SORT_2 1
#include <fcall.h>

static uchar *gstring(uchar *p, uchar *ep, char **s) {
  uint n;

  if (p + BIT16SZ > ep)
    return nil;
  n = GBIT16(p);
  p += BIT16SZ - 1;
  if (p + n + 1 > ep)
    return nil;
  /* move it down, on top of count, to make room for '\0' */
  memmove(p, p + 1, n);
  p[n] = '\0';
  *s = (char *)p;
  p += n + 1;
  return p;
}

static uchar *gqid(uchar *p, uchar *ep, Qid *q) {
  if (p + QIDSZ > ep)
    return nil;
  q->type = GBIT8(p);
  p += BIT8SZ;
  q->vers = GBIT32(p);
  p += BIT32SZ;
  q->path = GBIT64(p);
  p += BIT64SZ;
  return p;
}

/*
 * no syntactic checks.
 * three causes for error:
 *  1. message size field is incorrect
 *  2. input buffer too short for its own data (counts too long, etc.)
 *  3. too many names or qids
 * gqid() and gstring() return nil if they would reach beyond buffer.
 * main switch statement checks range and also can fall through
 * to test at end of routine.
 */
/*@
  @ requires ap == \null || \valid(ap);
  @ requires f == \null || \valid(f);
  @ assigns \nothing;
  @*/
uint convM2S(uchar *ap, uint nap, Fcall *f) {
  uchar *p, *ep;
  uint i, size;

  p = ap;
  ep = p + nap;

  if (p + BIT32SZ + BIT8SZ + BIT16SZ > ep) {
    print("convM2S: header bounds check failed p=%p ep=%p\n", p, ep);
    return 0;
  }
  size = GBIT32(p);
  p += BIT32SZ;

  if (size < BIT32SZ + BIT8SZ + BIT16SZ) {
    print("convM2S: size check failed size=%d min=%d\n", size,
          BIT32SZ + BIT8SZ + BIT16SZ);
    return 0;
  }

  f->type = GBIT8(p);
  p += BIT8SZ;
  f->tag = GBIT16(p);
  p += BIT16SZ;

  // Debug print for Tsyscall
  if (f->type == 130) {
    /* print("convM2S: parsing Tsyscall size=%d tag=%d\n", size, f->tag); */
  }

  switch (f->type) {
  default:
    return 0;

  case Tversion:
    if (p + BIT32SZ > ep)
      return 0;
    f->msize = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->version);
    break;

  case Tflush:
    if (p + BIT16SZ > ep)
      return 0;
    f->oldtag = GBIT16(p);
    p += BIT16SZ;
    break;

  case Tauth:
    if (p + BIT32SZ > ep)
      return 0;
    f->afid = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->uname);
    if (p == nil)
      break;
    p = gstring(p, ep, &f->aname);
    if (p == nil)
      break;
    break;

  case Tattach:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    if (p + BIT32SZ > ep)
      return 0;
    f->afid = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->uname);
    if (p == nil)
      break;
    p = gstring(p, ep, &f->aname);
    if (p == nil)
      break;
    break;

  case Twalk:
    if (p + BIT32SZ + BIT32SZ + BIT16SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->newfid = GBIT32(p);
    p += BIT32SZ;
    f->nwname = GBIT16(p);
    p += BIT16SZ;
    if (f->nwname > MAXWELEM)
      return 0;
    for (i = 0; i < f->nwname; i++) {
      p = gstring(p, ep, &f->wname[i]);
      if (p == nil)
        break;
    }
    break;

  case Topen:
    if (p + BIT32SZ + BIT8SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->mode = GBIT8(p);
    p += BIT8SZ;
    break;

  case Tcreate:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->name);
    if (p == nil)
      break;
    if (p + BIT32SZ + BIT8SZ > ep)
      return 0;
    f->perm = GBIT32(p);
    p += BIT32SZ;
    f->mode = GBIT8(p);
    p += BIT8SZ;
    break;

  case Tread:
    if (p + BIT32SZ + BIT64SZ + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->offset = GBIT64(p);
    p += BIT64SZ;
    f->count = GBIT32(p);
    p += BIT32SZ;
    break;

  case Twrite:
    if (p + BIT32SZ + BIT64SZ + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->offset = GBIT64(p);
    p += BIT64SZ;
    f->count = GBIT32(p);
    p += BIT32SZ;
    if (p + f->count > ep)
      return 0;
    f->data = (char *)p;
    p += f->count;
    break;

  case Tclunk:
  case Tremove:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    break;

  case Texec:
    /* Texec format: count[4] payload[count]
     * where payload is: pathlen[2] path[pathlen] */
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    if (p + f->count > ep)
      return 0;
    f->data = (char *)p;
    p += f->count;
    break;

  case Tsyscall:
    if (p + BIT32SZ + BIT32SZ + BIT32SZ > ep)
      return 0;
    f->scallnr = GBIT32(p);
    p += BIT32SZ;
    f->sflags = GBIT32(p);
    p += BIT32SZ;
    f->scount = GBIT32(p);
    p += BIT32SZ;
    if (p + f->scount > ep)
      return 0;
    f->sdata = p;
    p += f->scount;
    break;

  case Rsyscall:
    if (p + BIT64SZ + BIT32SZ > ep)
      return 0;
    f->retval = GBIT64(p);
    p += BIT64SZ;
    f->scount = GBIT32(p);
    p += BIT32SZ;
    if (p + f->scount > ep)
      return 0;
    f->sdata = p;
    p += f->scount;
    break;

  /* Tsys* - specific syscall message types */

  /* I/O Operations */
  case Tsysopen:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->name);
    if (p == nil)
      break;
    if (p + BIT8SZ > ep)
      return 0;
    f->mode = GBIT8(p);
    p += BIT8SZ;
    break;

  case Tsyscreate:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->name);
    if (p == nil)
      break;
    if (p + BIT32SZ + BIT8SZ > ep)
      return 0;
    f->perm = GBIT32(p);
    p += BIT32SZ;
    f->mode = GBIT8(p);
    p += BIT8SZ;
    break;

  case Tsysread:
  case Tsyspread:
    if (p + BIT32SZ + BIT64SZ + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->offset = GBIT64(p);
    p += BIT64SZ;
    f->count = GBIT32(p);
    p += BIT32SZ;
    break;

  case Tsyswrite:
  case Tsyspwrite:
    if (p + BIT32SZ + BIT64SZ + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->offset = GBIT64(p);
    p += BIT64SZ;
    f->count = GBIT32(p);
    p += BIT32SZ;
    if (p + f->count > ep)
      return 0;
    f->data = (char *)p;
    p += f->count;
    break;

  case Tsysclose:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    break;

  case Tsysremove:
    p = gstring(p, ep, &f->name);
    break;

  /* File Info Operations */
  case Tsysstat:
    p = gstring(p, ep, &f->name);
    break;

  case Tsysfstat:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    break;

  case Tsyswstat:
    p = gstring(p, ep, &f->name);
    if (p == nil)
      break;
    if (p + BIT16SZ > ep)
      return 0;
    f->nstat = GBIT16(p);
    p += BIT16SZ;
    if (p + f->nstat > ep)
      return 0;
    f->stat = p;
    p += f->nstat;
    break;

  case Tsysfwstat:
    if (p + BIT32SZ + BIT16SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->nstat = GBIT16(p);
    p += BIT16SZ;
    if (p + f->nstat > ep)
      return 0;
    f->stat = p;
    p += f->nstat;
    break;

  /* Process Control */
  case Tsysfork:
    if (p + BIT32SZ > ep)
      return 0;
    f->flags = GBIT32(p);
    p += BIT32SZ;
    break;

  case Tsysexec:
    p = gstring(p, ep, &f->path);
    if (p == nil)
      break;
    if (p + BIT32SZ > ep)
      return 0;
    f->argc = GBIT32(p);
    p += BIT32SZ;
    if (f->argc > MAXWELEM)
      return 0;
      /*@ loop invariant 0 <= i <= f->argc;
    @ loop assigns i;
    @ loop variant f->argc - i;
    @*/
  for (u32int i = 0; i < f->argc; i++) {
      p = gstring(p, ep, &f->args[i]);
      if (p == nil)
        return 0;
    }
    f->argv = f->args;
    break;

  case Tsysexit:
    p = gstring(p, ep, &f->ename);
    break;

  case Tsyswait:
    /* No arguments */
    break;

  case Tsysbrk:
    if (p + BIT64SZ > ep)
      return 0;
    f->addr = GBIT64(p);
    p += BIT64SZ;
    break;

  case Tsyssleep:
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    break;

  /* Namespace Operations */
  case Tsysbind:
    p = gstring(p, ep, &f->name);
    if (p == nil)
      break;
    p = gstring(p, ep, &f->oldpath);
    if (p == nil)
      break;
    if (p + BIT32SZ > ep)
      return 0;
    f->flags = GBIT32(p);
    p += BIT32SZ;
    break;

  case Tsysmount:
    if (p + BIT32SZ + BIT32SZ > ep)
      return 0;
    f->fd = GBIT32(p);
    p += BIT32SZ;
    f->afid = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->oldpath);
    if (p == nil)
      break;
    if (p + BIT32SZ > ep)
      return 0;
    f->flags = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->aname);
    break;

  case Tsysunmount:
    p = gstring(p, ep, &f->name);
    if (p == nil)
      break;
    p = gstring(p, ep, &f->oldpath);
    break;

  case Tsyschdir:
    p = gstring(p, ep, &f->name);
    break;

  /* FD Operations */
  case Tsysdup:
    if (p + BIT32SZ + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->newfid = GBIT32(p);
    p += BIT32SZ;
    break;

  case Tsyspipe:
    /* No arguments */
    break;

  case Tsysfd2path:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    break;

  /* Misc Operations */
  case Tsysseek:
    if (p + BIT32SZ + BIT64SZ + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->offset = GBIT64(p);
    p += BIT64SZ;
    f->whence = (int)GBIT32(p);
    p += BIT32SZ;
    break;

  case Tsysnotify:
    if (p + BIT64SZ > ep)
      return 0;
    f->handler = GBIT64(p);
    p += BIT64SZ;
    break;

  case Tsysalarm:
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    break;

    /* Rsys* - syscall replies */

  case Rsysopen:
  case Rsyscreate:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    p = gqid(p, ep, &f->qid);
    if (p == nil)
      break;
    if (p + BIT32SZ > ep)
      return 0;
    f->iounit = GBIT32(p);
    p += BIT32SZ;
    break;

  case Rsysread:
  case Rsyspread:
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    if (p + f->count > ep)
      return 0;
    f->data = (char *)p;
    p += f->count;
    break;

  case Rsyswrite:
  case Rsyspwrite:
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    break;

  case Rsysclose:
  case Rsysremove:
  case Rsysexit:
  case Rsyssleep:
  case Rsysbind:
  case Rsysmount:
  case Rsysunmount:
  case Rsyschdir:
  case Rsysnotify:
  case Rsysexec:
    /* No response data */
    break;

  case Rsysstat:
  case Rsysfstat:
    if (p + BIT16SZ > ep)
      return 0;
    f->nstat = GBIT16(p);
    p += BIT16SZ;
    if (p + f->nstat > ep)
      return 0;
    f->stat = p;
    p += f->nstat;
    break;

  case Rsyswstat:
  case Rsysfwstat:
    /* No response data */
    break;

  case Rsysfork:
    if (p + BIT32SZ > ep)
      return 0;
    f->pid = GBIT32(p);
    p += BIT32SZ;
    break;

  case Rsyswait:
    if (p + BIT32SZ > ep)
      return 0;
    f->pid = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->ename);
    break;

  case Rsysbrk:
    if (p + BIT64SZ > ep)
      return 0;
    f->addr = GBIT64(p);
    p += BIT64SZ;
    break;

  case Rsysdup:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    break;

  case Rsyspipe:
    if (p + BIT32SZ + BIT32SZ > ep)
      return 0;
    f->fid0 = GBIT32(p);
    p += BIT32SZ;
    f->fid1 = GBIT32(p);
    p += BIT32SZ;
    break;

  case Rsysfd2path:
    p = gstring(p, ep, &f->name);
    break;

  case Rsysseek:
    if (p + BIT64SZ > ep)
      return 0;
    f->offset = GBIT64(p);
    p += BIT64SZ;
    break;

  case Rsysalarm:
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    break;

  case Tstat:
    if (p + BIT32SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    break;

  case Twstat:
    if (p + BIT32SZ + BIT16SZ > ep)
      return 0;
    f->fid = GBIT32(p);
    p += BIT32SZ;
    f->nstat = GBIT16(p);
    p += BIT16SZ;
    if (p + f->nstat > ep)
      return 0;
    f->stat = p;
    p += f->nstat;
    break;

    /*
     */
  case Rversion:
    if (p + BIT32SZ > ep)
      return 0;
    f->msize = GBIT32(p);
    p += BIT32SZ;
    p = gstring(p, ep, &f->version);
    break;

  case Rerror:
    p = gstring(p, ep, &f->ename);
    break;

  case Rflush:
    break;

  case Rauth:
    p = gqid(p, ep, &f->aqid);
    if (p == nil)
      break;
    break;

  case Rattach:
    p = gqid(p, ep, &f->qid);
    if (p == nil)
      break;
    break;

  case Rwalk:
    if (p + BIT16SZ > ep)
      return 0;
    f->nwqid = GBIT16(p);
    p += BIT16SZ;
    if (f->nwqid > MAXWELEM)
      return 0;
    for (i = 0; i < f->nwqid; i++) {
      p = gqid(p, ep, &f->wqid[i]);
      if (p == nil)
        break;
    }
    break;

  case Ropen:
  case Rcreate:
    p = gqid(p, ep, &f->qid);
    if (p == nil)
      break;
    if (p + BIT32SZ > ep)
      return 0;
    f->iounit = GBIT32(p);
    p += BIT32SZ;
    break;

  case Rread:
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    if (p + f->count > ep)
      return 0;
    f->data = (char *)p;
    p += f->count;
    break;

  case Rwrite:
    if (p + BIT32SZ > ep)
      return 0;
    f->count = GBIT32(p);
    p += BIT32SZ;
    break;

  case Rclunk:
  case Rremove:
    break;

  case Rstat:
    if (p + BIT16SZ > ep)
      return 0;
    f->nstat = GBIT16(p);
    p += BIT16SZ;
    if (p + f->nstat > ep)
      return 0;
    f->stat = p;
    p += f->nstat;
    break;

  case Rwstat:
    break;
  }

  if (p == nil || p > ep)
    return 0;
  if (ap + size == p)
    return size;
  return 0;
}
