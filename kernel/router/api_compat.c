#include "u.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"

/*
 * Compatibility entry point used by older lux9 API paths that still hand a raw
 * 9P buffer to the kernel. Keep it next to the active router implementation
 * instead of in globals.c.
 */
long p9_route_message(int pid, void *msg, ulong len) {
  Proc *p;
  Fcall t, r;
  uchar *buf;
  uint msg_size, rep_size;
  int slot;

  if (msg == nil || len < 7)
    return -1;

  p = up;
  if (pid > 0 && (p == nil || p->pid != (ulong)pid)) {
    slot = procindex((ulong)pid);
    if (slot < 0)
      return -1;
    p = proctab(slot);
  }
  if (p == nil || (pid > 0 && (p->pid != (ulong)pid || p->state == Dead)))
    return -1;

  buf = (uchar *)msg;
  msg_size = GBIT32(buf);
  if (msg_size < 7 || msg_size > len)
    return -1;

  t = (Fcall){0};
  if (convM2S(buf, msg_size, &t) == 0)
    return -1;

  r = (Fcall){0};
  if (p9_dispatch(p, &t, &r) < 0 && r.type != Rerror) {
    r.type = Rerror;
    r.tag = t.tag;
    r.ename = (up != nil && up->errstr[0] != 0) ? up->errstr
                                                : "dispatch failed";
  }

  if (len > 0xFFFFFFFFUL)
    return -1;
  rep_size = convS2M(&r, buf, (uint)len);
  if (rep_size == 0)
    return -1;
  return (long)rep_size;
}
