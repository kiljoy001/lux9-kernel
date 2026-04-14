#include "../include/u.h"

#define _BREAK_SORT_1 1
#include <libc.h>

#define _BREAK_SORT_2 1
#include <fcall.h>

/*@
  @ requires nbuf > 0 ==> \valid_read(buf + (0 .. nbuf-1));
  @ assigns \nothing;
  @ ensures \result == 0 || \result == -1;
  @ behavior valid:
  @   assumes nbuf >= STATFIXLEN;
  @   ensures \result == 0 || \result == -1;
  @ behavior too_small:
  @   assumes nbuf < STATFIXLEN;
  @   ensures \result == -1;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
int statcheck(uchar *buf, uint nbuf) {
  uchar *ebuf;
  int i;

  ebuf = buf + nbuf;

  if (nbuf < STATFIXLEN || nbuf != BIT16SZ + GBIT16(buf))
    return -1;

  buf += STATFIXLEN - 4 * BIT16SZ;

  /*@ loop invariant 0 <= i <= 4;
    @ loop assigns i, buf;
    @ loop variant 4 - i;
    @ loop pragma UNROLL 4;
    @*/
  for (i = 0; i < 4; i++) {
    if (buf + BIT16SZ > ebuf)
      return -1;
    buf += BIT16SZ + GBIT16(buf);
  }

  if (buf != ebuf)
    return -1;

  return 0;
}

static char nullstring[] = "";

/*@
  @ requires nbuf > 0 ==> \valid_read(buf + (0 .. nbuf-1));
  @ requires \valid(d);
  @ requires strs != \null ==> \valid(strs + (0 .. nbuf)); // Approximate bound
  @ assigns *d, strs[0 .. nbuf];
  @ ensures \result <= nbuf;
  @*/
uint convM2D(uchar *buf, uint nbuf, Dir *d, char *strs) {
  uchar *p, *ebuf;
  char *sv[4];
  int i, ns;

  if (nbuf < STATFIXLEN)
    return 0;

  p = buf;
  ebuf = buf + nbuf;

  p += BIT16SZ; /* ignore size */
  d->type = GBIT16(p);
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
  p += BIT64SZ;

  /*@ loop invariant 0 <= i <= 4;
    @ loop assigns i, p, ns, sv[0..3], strs[0..nbuf];
    @ loop variant 4 - i;
    @ loop pragma UNROLL 4;
    @*/
  for (i = 0; i < 4; i++) {
    if (p + BIT16SZ > ebuf)
      return 0;
    ns = GBIT16(p);
    p += BIT16SZ;
    if (p + ns > ebuf)
      return 0;
    if (strs) {
      sv[i] = strs;
      memmove(strs, p, ns);
      strs += ns;
      *strs++ = '\0';
    }
    p += ns;
  }

  if (strs) {
    d->name = sv[0];
    d->uid = sv[1];
    d->gid = sv[2];
    d->muid = sv[3];
  } else {
    d->name = nullstring;
    d->uid = nullstring;
    d->gid = nullstring;
    d->muid = nullstring;
  }

  return p - buf;
}
