/*
 * devdistress.c - Wave 7 Distress Signal Device
 *
 * Provides /dev/distress for resurrection server to read distress events.
 * Only accessible by processes with CAP_SERVICE_CONTROL capability.
 *
 * Read: Returns DistressEvent structures (blocking read).
 * Write: Not supported.
 */
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

enum {
  Qdistress,
  Qdistressctl,
};

enum {
  CMclear,
  CMtest,
  CMinject,
};

static Dirtab distressdir[] = {
    {".", {Qdistress, 0, QTDIR}, 0, DMDIR | 0555},
    {"distress", {Qdistress, 0, QTFILE}, 0, 0444},
    {"distressctl", {Qdistressctl, 0, QTFILE}, 0, 0644},
};

static Cmdtab distresscmd[] = {
    {CMclear, "clear", 1},
    {CMtest, "test", 0},
    {CMinject, "inject", 0},
};

static void distressreadperm(void) {
  if (up != nil && (up->kp || has_capability(up, PEBBLE_CAP_ADMIN)))
    return;
  error("permission denied: requires CAP_SERVICE_CONTROL");
}

static void distressctlperm(void) {
  if (up != nil && (up->kp || has_capability(up, PEBBLE_CAP_ADMIN)))
    return;
  error("permission denied: requires CAP_SERVICE_CONTROL");
}

static int distressparsereason(const char *s, int *reason) {
  long v;
  char *ep;

  if (s == nil || reason == nil)
    return -1;

  if (strcmp(s, "memory_corrupt") == 0 || strcmp(s, "mem") == 0) {
    *reason = DISTRESS_MEMORY_CORRUPT;
    return 0;
  }
  if (strcmp(s, "cap_violation") == 0 || strcmp(s, "cap") == 0) {
    *reason = DISTRESS_CAP_VIOLATION;
    return 0;
  }
  if (strcmp(s, "panic_imminent") == 0 || strcmp(s, "panic") == 0) {
    *reason = DISTRESS_PANIC_IMMINENT;
    return 0;
  }
  if (strcmp(s, "resource_exhaust") == 0 || strcmp(s, "oom") == 0) {
    *reason = DISTRESS_RESOURCE_EXHAUST;
    return 0;
  }
  if (strcmp(s, "user_abort") == 0 || strcmp(s, "abort") == 0) {
    *reason = DISTRESS_USER_ABORT;
    return 0;
  }
  if (strcmp(s, "borrow_fault") == 0 || strcmp(s, "borrow") == 0) {
    *reason = DISTRESS_BORROW_FAULT;
    return 0;
  }
  if (strcmp(s, "vault_breach") == 0 || strcmp(s, "vault") == 0) {
    *reason = DISTRESS_VAULT_BREACH;
    return 0;
  }

  v = strtol(s, &ep, 0);
  if (ep != nil && *ep == 0 && v >= DISTRESS_MEMORY_CORRUPT &&
      v <= DISTRESS_VAULT_BREACH) {
    *reason = (int)v;
    return 0;
  }

  return -1;
}

static Chan *distressattach(char *spec) { return devattach('D', spec); }

static Walkqid *distresswalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, distressdir, nelem(distressdir), devgen);
}

static long distressstat(Chan *c, uchar *dp, long n) {
  return devstat(c, dp, n, distressdir, nelem(distressdir), devgen);
}

static Chan *distressopen(Chan *c, int omode) {
  int access = omode & 3;

  if (c->qid.path == Qdistress && (access == OREAD || access == ORDWR))
    distressreadperm();
  if (c->qid.path == Qdistressctl && (omode & 3) != OREAD)
    distressctlperm();
  return devopen(c, omode, distressdir, nelem(distressdir), devgen);
}

static void distressclose(Chan *c) { USED(c); }

static long distressread(Chan *c, void *buf, long n, vlong offset) {
  DistressEvent ev;
  int ret;

  USED(offset);

  switch ((ulong)c->qid.path) {
  case Qdistress:
    /* Read distress events (blocking) */
    if (n < sizeof(DistressEvent))
      error("buffer too small");

    ret = pebble_read_distress(&ev);
    if (ret <= 0) {
      /* No event or error */
      return 0;
    }

    memmove(buf, &ev, sizeof(DistressEvent));
    return sizeof(DistressEvent);

  case Qdistressctl:
    /* Return pending event count as ASCII */
    {
      char tmp[32];
      int pending = pebble_distress_pending();
      int len = snprint(tmp, sizeof(tmp), "%d\n", pending);
      if (offset >= len)
        return 0;
      if (n > len - offset)
        n = len - offset;
      memmove(buf, tmp + offset, n);
      return n;
    }

  default:
    error("bad qid");
    return 0;
  }
}

static long distresswrite(Chan *c, void *buf, long n, vlong offset) {
  Cmdbuf *cb;
  Cmdtab *ct;
  int reason;
  uvlong context;
  char *ep;

  USED(offset);

  switch ((ulong)c->qid.path) {
  case Qdistress:
    error("cannot write to distress");
    return 0;

  case Qdistressctl:
    if (offset != 0)
      error(Ebadarg);
    distressctlperm();

    cb = parsecmd(buf, n);
    if (waserror()) {
      free(cb);
      nexterror();
    }

    ct = lookupcmd(cb, distresscmd, nelem(distresscmd));
    switch (ct->index) {
    case CMclear:
      pebble_clear_distress();
      break;

    case CMtest:
    case CMinject:
      reason = DISTRESS_USER_ABORT;
      context = (up != nil) ? up->pid : 0;
      if (cb->nf > 1 && distressparsereason(cb->f[1], &reason) < 0)
        error("bad distress reason");
      if (cb->nf > 2) {
        context = strtoull(cb->f[2], &ep, 0);
        if (ep == nil || *ep != 0)
          error("bad distress context");
      }
      pebble_signal_distress(up, reason, context);
      break;
    }

    poperror();
    free(cb);
    return n;

  default:
    error("bad qid");
    return 0;
  }
}

Dev distressdevtab = {
    .dc = 'D',
    .name = "distress",

    .reset = devreset,
    .init = devinit,
    .shutdown = devshutdown,
    .attach = distressattach,
    .walk = distresswalk,
    .stat = distressstat,
    .open = distressopen,
    .create = devcreate,
    .close = distressclose,
    .read = distressread,
    .write = distresswrite,
    .remove = devremove,
    .wstat = devwstat,
};
