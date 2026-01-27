#include "../inc/lux.h"
#include "lux_internal.h"
#include <string.h>

#define STATFIXLEN 49

/*
 * convD2M - Convert Dir to Machine-independent format
 * Returns number of bytes written.
 */
uint convD2M(Dir *d, uchar *buf, uint nbuf) {
  uchar *p, *ebuf;
  char *sv[4];
  int ns, i;
  uint sz;

  if (nbuf < BIT16SZ)
    return 0;

  p = buf;
  ebuf = buf + nbuf;

  sv[0] = d->name;
  sv[1] = d->uid;
  sv[2] = d->gid;
  sv[3] = d->muid;

  ns = 0;
  for (i = 0; i < 4; i++) {
    if (sv[i])
      ns += strlen(sv[i]);
  }

  /* Size = STATFIXLEN + string lengths */
  sz = STATFIXLEN + ns;

  if (sz > nbuf - BIT16SZ)
    return 0;

  PBIT16(p, sz - BIT16SZ);
  p += BIT16SZ;

  PBIT16(p, d->type);
  p += BIT16SZ;
  PBIT32(p, d->dev);
  p += BIT32SZ;
  PBIT8(p, d->qid.type);
  p += BIT8SZ;
  PBIT32(p, d->qid.vers);
  p += BIT32SZ;
  PBIT64(p, d->qid.path);
  p += BIT64SZ;
  PBIT32(p, d->mode);
  p += BIT32SZ;
  PBIT32(p, d->atime);
  p += BIT32SZ;
  PBIT32(p, d->mtime);
  p += BIT32SZ;
  PBIT64(p, d->length);
  p += BIT64SZ;

  for (i = 0; i < 4; i++) {
    if (sv[i]) {
      ns = strlen(sv[i]);
      PBIT16(p, ns);
      p += BIT16SZ;
      memmove(p, sv[i], ns);
      p += ns;
    } else {
      PBIT16(p, 0);
      p += BIT16SZ;
    }
  }

  return p - buf;
}

uint sizeD2M(Dir *d) {
  char *sv[4];
  int i, ns;

  sv[0] = d->name;
  sv[1] = d->uid;
  sv[2] = d->gid;
  sv[3] = d->muid;

  ns = 0;
  for (i = 0; i < 4; i++) {
    if (sv[i])
      ns += strlen(sv[i]);
  }

  return STATFIXLEN + ns;
}
