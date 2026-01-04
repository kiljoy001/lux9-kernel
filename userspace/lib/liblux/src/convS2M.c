/* Userspace version of convS2M */
#include "../inc/libc.h"
#include <fcall.h>
#include <u.h>

static uchar *pstring(uchar *p, char *s) {
  uint n;

  if (s == nil) {
    PBIT16(p, 0);
    p += BIT16SZ;
    return p;
  }

  n = strlen(s);
  /*
   * We are moving the string before the length,
   * so you can S2M a struct into an existing message
   */
  memmove(p + BIT16SZ, s, n);
  PBIT16(p, n);
  p += n + BIT16SZ;
  return p;
}

static uchar *pqid(uchar *p, Qid *q) {
  PBIT8(p, q->type);
  p += BIT8SZ;
  PBIT32(p, q->vers);
  p += BIT32SZ;
  PBIT64(p, q->path);
  p += BIT64SZ;
  return p;
}

static uint stringsz(char *s) {
  if (s == nil)
    return BIT16SZ;

  return BIT16SZ + strlen(s);
}

uint sizeS2M(Fcall *f) {
  uint n;
  int i;

  n = 0;
  n += BIT32SZ; /* size */
  n += BIT8SZ;  /* type */
  n += BIT16SZ; /* tag */

  switch (f->type) {
  default:
    return 0;

  case Tversion:
    n += BIT32SZ;
    n += stringsz(f->version);
    break;

  case Tflush:
    n += BIT16SZ;
    break;

  case Tauth:
    n += BIT32SZ;
    n += stringsz(f->uname);
    n += stringsz(f->aname);
    break;

  case Tattach:
    n += BIT32SZ;
    n += BIT32SZ;
    n += stringsz(f->uname);
    n += stringsz(f->aname);
    break;

  case Twalk:
    n += BIT32SZ;
    n += BIT32SZ;
    n += BIT16SZ;
    for (i = 0; i < f->nwname; i++)
      n += stringsz(f->wname[i]);
    break;

  case Topen:
    n += BIT32SZ;
    n += BIT8SZ;
    break;

  case Tcreate:
    n += BIT32SZ;
    n += stringsz(f->name);
    n += BIT32SZ;
    n += BIT8SZ;
    break;

  case Tread:
    n += BIT32SZ;
    n += BIT64SZ;
    n += BIT32SZ;
    break;

  case Twrite:
    n += BIT32SZ;
    n += BIT64SZ;
    n += BIT32SZ;
    n += f->count;
    break;

  case Tclunk:
  case Tremove:
    n += BIT32SZ;
    break;

  case Tstat:
    n += BIT32SZ;
    break;

  case Twstat:
    n += BIT32SZ;
    n += BIT16SZ;
    n += f->nstat;
    break;
    /*
     */

  case Tsyscall:
    n += BIT32SZ;   /* scallnr */
    n += BIT32SZ;   /* sflags */
    n += BIT32SZ;   /* scount */
    n += f->scount; /* sdata */
    break;

  case Rsyscall:
    n += BIT64SZ;   /* retval */
    n += BIT32SZ;   /* scount */
    n += f->scount; /* sdata */
    break;

  /* Tsys* - specific syscall message types */
  case Tsysopen:
    n += BIT32SZ; /* fid */
    n += stringsz(f->name);
    n += BIT8SZ; /* mode */
    break;

  case Tsyscreate:
    n += BIT32SZ; /* fid */
    n += stringsz(f->name);
    n += BIT32SZ; /* perm */
    n += BIT8SZ;  /* mode */
    break;

  case Tsysread:
  case Tsyspread:
    n += BIT32SZ; /* fid */
    n += BIT64SZ; /* offset */
    n += BIT32SZ; /* count */
    break;

  case Tsyswrite:
  case Tsyspwrite:
    n += BIT32SZ;  /* fid */
    n += BIT64SZ;  /* offset */
    n += BIT32SZ;  /* count */
    n += f->count; /* data */
    break;

  case Tsysclose:
  case Tsysfstat:
  case Tsysfd2path:
    n += BIT32SZ; /* fid */
    break;

  case Tsysremove:
  case Tsysstat:
  case Tsyschdir:
    n += stringsz(f->name);
    break;

  case Tsyswstat:
    n += stringsz(f->name);
    n += BIT16SZ;  /* nstat */
    n += f->nstat; /* stat */
    break;

  case Tsysfwstat:
    n += BIT32SZ;  /* fid */
    n += BIT16SZ;  /* nstat */
    n += f->nstat; /* stat */
    break;

  case Tsysfork:
  case Tsysalarm:
    n += BIT32SZ; /* flags/count */
    break;

  case Tsysexec:
    n += stringsz(f->name);
    n += BIT32SZ; /* argc */
    break;

  case Tsysexit:
    n += stringsz(f->ename);
    break;

  case Tsyswait:
  case Tsyspipe:
    /* No arguments */
    break;

  case Tsysbrk:
    n += BIT64SZ; /* addr */
    break;

  case Tsyssleep:
    n += BIT32SZ; /* millisecs */
    break;

  case Tsysbind:
    n += stringsz(f->name);
    n += stringsz(f->oldpath);
    n += BIT32SZ; /* flags */
    break;

  case Tsysmount:
    n += BIT32SZ; /* fd */
    n += BIT32SZ; /* afid */
    n += stringsz(f->oldpath);
    n += BIT32SZ; /* flags */
    n += stringsz(f->aname);
    break;

  case Tsysunmount:
    n += stringsz(f->name);
    n += stringsz(f->oldpath);
    break;

  case Tsysdup:
    n += BIT32SZ; /* oldfd */
    n += BIT32SZ; /* newfd */
    break;

  case Tsysseek:
    n += BIT32SZ; /* fid */
    n += BIT64SZ; /* offset */
    n += BIT32SZ; /* whence */
    break;

  case Tsysnotify:
    n += BIT64SZ; /* handler */
    break;

  /* Rsys* - syscall replies */
  case Rsysopen:
  case Rsyscreate:
    n += BIT32SZ; /* fid */
    n += QIDSZ;   /* qid */
    n += BIT32SZ; /* iounit */
    break;

  case Rsysread:
  case Rsyspread:
    n += BIT32SZ;  /* count */
    n += f->count; /* data */
    break;

  case Rsyswrite:
  case Rsyspwrite:
  case Rsysfork:
  case Rsysalarm:
  case Rsysdup:
    n += BIT32SZ; /* count/pid/fid */
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
  case Rsyswstat:
  case Rsysfwstat:
    /* No response data */
    break;

  case Rsysstat:
  case Rsysfstat:
    n += BIT16SZ;  /* nstat */
    n += f->nstat; /* stat */
    break;

  case Rsyswait:
    n += BIT32SZ; /* pid */
    n += stringsz(f->ename);
    break;

  case Rsysbrk:
  case Rsysseek:
    n += BIT64SZ; /* addr/offset */
    break;

  case Rsyspipe:
    n += BIT32SZ; /* fid0 */
    n += BIT32SZ; /* fid1 */
    break;

  case Rsysfd2path:
    n += stringsz(f->name);
    break;

  case Rversion:
    n += BIT32SZ;
    n += stringsz(f->version);
    break;

  case Rerror:
    n += stringsz(f->ename);
    break;

  case Rflush:
    break;

  case Rauth:
    n += QIDSZ;
    break;

  case Rattach:
    n += QIDSZ;
    break;

  case Rwalk:
    n += BIT16SZ;
    n += f->nwqid * QIDSZ;
    break;

  case Ropen:
  case Rcreate:
    n += QIDSZ;
    n += BIT32SZ;
    break;

  case Rread:
    n += BIT32SZ;
    n += f->count;
    break;

  case Rwrite:
    n += BIT32SZ;
    break;

  case Rclunk:
    break;

  case Rremove:
    break;

  case Rstat:
    n += BIT16SZ;
    n += f->nstat;
    break;

  case Rwstat:
    break;
  }
  return n;
}

uint convS2M(Fcall *f, uchar *ap, uint nap) {
  uchar *p;
  uint i, size;

  size = sizeS2M(f);
  if (size == 0)
    return 0;
  if (size > nap)
    return 0;

  p = (uchar *)ap;

  PBIT32(p, size);
  p += BIT32SZ;
  PBIT8(p, f->type);
  p += BIT8SZ;
  PBIT16(p, f->tag);
  p += BIT16SZ;

  switch (f->type) {
  default:
    return 0;

  case Tversion:
    PBIT32(p, f->msize);
    p += BIT32SZ;
    p = pstring(p, f->version);
    break;

  case Tflush:
    PBIT16(p, f->oldtag);
    p += BIT16SZ;
    break;

  case Tauth:
    PBIT32(p, f->afid);
    p += BIT32SZ;
    p = pstring(p, f->uname);
    p = pstring(p, f->aname);
    break;

  case Tattach:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT32(p, f->afid);
    p += BIT32SZ;
    p = pstring(p, f->uname);
    p = pstring(p, f->aname);
    break;

  case Twalk:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT32(p, f->newfid);
    p += BIT32SZ;
    PBIT16(p, f->nwname);
    p += BIT16SZ;
    if (f->nwname > MAXWELEM)
      return 0;
    for (i = 0; i < f->nwname; i++)
      p = pstring(p, f->wname[i]);
    break;

  case Topen:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT8(p, f->mode);
    p += BIT8SZ;
    break;

  case Tcreate:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    p = pstring(p, f->name);
    PBIT32(p, f->perm);
    p += BIT32SZ;
    PBIT8(p, f->mode);
    p += BIT8SZ;
    break;

  case Tread:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT64(p, f->offset);
    p += BIT64SZ;
    PBIT32(p, f->count);
    p += BIT32SZ;
    break;

  case Twrite:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT64(p, f->offset);
    p += BIT64SZ;
    PBIT32(p, f->count);
    p += BIT32SZ;
    memmove(p, f->data, f->count);
    p += f->count;
    break;

  case Tclunk:
  case Tremove:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    break;

  case Tstat:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    break;

  case Twstat:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT16(p, f->nstat);
    p += BIT16SZ;
    memmove(p, f->stat, f->nstat);
    p += f->nstat;
    break;
    /*
     */

  case Tsyscall:
    PBIT32(p, f->scallnr);
    p += BIT32SZ;
    PBIT32(p, f->sflags);
    p += BIT32SZ;
    PBIT32(p, f->scount);
    p += BIT32SZ;
    if (f->scount > 0)
      memmove(p, f->sdata, f->scount);
    p += f->scount;
    break;

  case Rsyscall:
    PBIT64(p, f->retval);
    p += BIT64SZ;
    PBIT32(p, f->scount);
    p += BIT32SZ;
    if (f->scount > 0)
      memmove(p, f->sdata, f->scount);
    p += f->scount;
    break;

  /* Tsys* - specific syscall message serialization */
  case Tsysopen:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    p = pstring(p, f->name);
    PBIT8(p, f->mode);
    p += BIT8SZ;
    break;

  case Tsyscreate:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    p = pstring(p, f->name);
    PBIT32(p, f->perm);
    p += BIT32SZ;
    PBIT8(p, f->mode);
    p += BIT8SZ;
    break;

  case Tsysread:
  case Tsyspread:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT64(p, f->offset);
    p += BIT64SZ;
    PBIT32(p, f->count);
    p += BIT32SZ;
    break;

  case Tsyswrite:
  case Tsyspwrite:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT64(p, f->offset);
    p += BIT64SZ;
    PBIT32(p, f->count);
    p += BIT32SZ;
    memmove(p, f->data, f->count);
    p += f->count;
    break;

  case Tsysclose:
  case Tsysfstat:
  case Tsysfd2path:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    break;

  case Tsysremove:
  case Tsysstat:
  case Tsyschdir:
    p = pstring(p, f->name);
    break;

  case Tsyswstat:
    p = pstring(p, f->name);
    PBIT16(p, f->nstat);
    p += BIT16SZ;
    memmove(p, f->stat, f->nstat);
    p += f->nstat;
    break;

  case Tsysfwstat:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT16(p, f->nstat);
    p += BIT16SZ;
    memmove(p, f->stat, f->nstat);
    p += f->nstat;
    break;

  case Tsysfork:
    PBIT32(p, f->flags);
    p += BIT32SZ;
    break;

  case Tsysexec:
    p = pstring(p, f->name);
    PBIT32(p, f->argc);
    p += BIT32SZ;
    break;

  case Tsysexit:
    p = pstring(p, f->ename);
    break;

  case Tsyswait:
  case Tsyspipe:
    /* No arguments */
    break;

  case Tsysbrk:
    PBIT64(p, f->addr);
    p += BIT64SZ;
    break;

  case Tsyssleep:
  case Tsysalarm:
    PBIT32(p, f->count);
    p += BIT32SZ;
    break;

  case Tsysbind:
    p = pstring(p, f->name);
    p = pstring(p, f->oldpath);
    PBIT32(p, f->flags);
    p += BIT32SZ;
    break;

  case Tsysmount:
    PBIT32(p, f->fd);
    p += BIT32SZ;
    PBIT32(p, f->afid);
    p += BIT32SZ;
    p = pstring(p, f->oldpath);
    PBIT32(p, f->flags);
    p += BIT32SZ;
    p = pstring(p, f->aname);
    break;

  case Tsysunmount:
    p = pstring(p, f->name);
    p = pstring(p, f->oldpath);
    break;

  case Tsysdup:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT32(p, f->newfid);
    p += BIT32SZ;
    break;

  case Tsysseek:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    PBIT64(p, f->offset);
    p += BIT64SZ;
    PBIT32(p, (u32int)f->whence);
    p += BIT32SZ;
    break;

  case Tsysnotify:
    PBIT64(p, f->handler);
    p += BIT64SZ;
    break;

  /* Rsys* - syscall replies */
  case Rsysopen:
  case Rsyscreate:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    p = pqid(p, &f->qid);
    PBIT32(p, f->iounit);
    p += BIT32SZ;
    break;

  case Rsysread:
  case Rsyspread:
    PBIT32(p, f->count);
    p += BIT32SZ;
    memmove(p, f->data, f->count);
    p += f->count;
    break;

  case Rsyswrite:
  case Rsyspwrite:
    PBIT32(p, f->count);
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
  case Rsyswstat:
  case Rsysfwstat:
    /* No response data */
    break;

  case Rsysstat:
  case Rsysfstat:
    PBIT16(p, f->nstat);
    p += BIT16SZ;
    memmove(p, f->stat, f->nstat);
    p += f->nstat;
    break;

  case Rsysfork:
    PBIT32(p, f->pid);
    p += BIT32SZ;
    break;

  case Rsyswait:
    PBIT32(p, f->pid);
    p += BIT32SZ;
    p = pstring(p, f->ename);
    break;

  case Rsysbrk:
    PBIT64(p, f->addr);
    p += BIT64SZ;
    break;

  case Rsysdup:
    PBIT32(p, f->fid);
    p += BIT32SZ;
    break;

  case Rsyspipe:
    PBIT32(p, f->fid0);
    p += BIT32SZ;
    PBIT32(p, f->fid1);
    p += BIT32SZ;
    break;

  case Rsysfd2path:
    p = pstring(p, f->name);
    break;

  case Rsysseek:
    PBIT64(p, f->offset);
    p += BIT64SZ;
    break;

  case Rsysalarm:
    PBIT32(p, f->count);
    p += BIT32SZ;
    break;

  case Rversion:
    PBIT32(p, f->msize);
    p += BIT32SZ;
    p = pstring(p, f->version);
    break;

  case Rerror:
    p = pstring(p, f->ename);
    break;

  case Rflush:
    break;

  case Rauth:
    p = pqid(p, &f->aqid);
    break;

  case Rattach:
    p = pqid(p, &f->qid);
    break;

  case Rwalk:
    PBIT16(p, f->nwqid);
    p += BIT16SZ;
    if (f->nwqid > MAXWELEM)
      return 0;
    for (i = 0; i < f->nwqid; i++)
      p = pqid(p, &f->wqid[i]);
    break;

  case Ropen:
  case Rcreate:
    p = pqid(p, &f->qid);
    PBIT32(p, f->iounit);
    p += BIT32SZ;
    break;

  case Rread:
    PBIT32(p, f->count);
    p += BIT32SZ;
    memmove(p, f->data, f->count);
    p += f->count;
    break;

  case Rwrite:
    PBIT32(p, f->count);
    p += BIT32SZ;
    break;

  case Rclunk:
    break;

  case Rremove:
    break;

  case Rstat:
    PBIT16(p, f->nstat);
    p += BIT16SZ;
    memmove(p, f->stat, f->nstat);
    p += f->nstat;
    break;

  case Rwstat:
    break;
  }
  if (size != p - ap)
    return 0;
  return size;
}
