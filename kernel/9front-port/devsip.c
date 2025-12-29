#include "../port/error.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"
#include "u.h"
#include <lib.h>

/*
 * /dev/sip - Universal Capability-Based Security Manager
 *
 * Refactored from opt-in SIP model to universal CBS:
 * - Every process is capability-isolated by default
 * - This driver provides a window into the Process Table
 * - Allows querying and modifying process capabilities
 * - Administrative capability (PEBBLE_CAP_ADMIN) required for modifications
 *
 * File hierarchy:
 *   /dev/sip/
 *   /dev/sip/clone         - Open to get a new control channel
 *   /dev/sip/ctl           - Control file for current process
 *   /dev/sip/<pid>/        - Directory for each process
 *   /dev/sip/<pid>/ctl     - Read capabilities, write to reduce them
 *   /dev/sip/<pid>/status  - Process status and capability info
 *
 *
 * SMT: Validated by proofs/sip/sip_model.v
 * Description: Models SIP capability system and page ownership transitions
 */

enum {
  Qdir = 0,
  Qclone,
  Qctl,      /* Control file for accessing process */
  Qprocbase, /* Base for per-process directories */
};

#define TYPE(q) ((q).path & 0xff)
#define PID(q) (((q).path >> 8) & 0xffffff)
#define QID(pid, type) (((pid) << 8) | (type))

static int sipgen(Chan *c, char *name, Dirtab *tab, int ntab, int s, Dir *dp) {
  Qid q;
  Proc *p;
  char buf[32];

  USED(tab, ntab, name);

  if (s == DEVDOTDOT) {
    mkqid(&q, Qdir, 0, QTDIR);
    devdir(c, q, "#Y", 0, eve, 0555, dp);
    return 1;
  }

  switch (TYPE(c->qid)) {
  case Qdir:
    /* Top-level directory: clone, ctl */
    if (s == 0) {
      mkqid(&q, Qclone, 0, QTFILE);
      devdir(c, q, "clone", 0, eve, 0666, dp);
      return 1;
    }
    if (s == 1) {
      mkqid(&q, Qctl, 0, QTFILE);
      devdir(c, q, "ctl", 0, eve, 0666, dp);
      return 1;
    }
    /* List process directories starting at s-2 */
    s -= 2;
    {
      int i;
      for (i = 0; (p = proctab(i)) != nil; i++) {
        if (p->state == Dead)
          continue;
        if (s-- == 0) {
          snprint(buf, sizeof(buf), "%d", p->pid);
          mkqid(&q, QID(p->pid, Qdir), 0, QTDIR);
          devdir(c, q, buf, 0, eve, 0555, dp);
          return 1;
        }
      }
    }
    return -1;

  case Qprocbase:
    /* Per-process directory: ctl, status */
    if (s == 0) {
      mkqid(&q, QID(PID(c->qid), Qctl), 0, QTFILE);
      devdir(c, q, "ctl", 0, eve, 0666, dp);
      return 1;
    }
    return -1;

  default:
    return -1;
  }
}

static Chan *sipattach(char *spec) { return devattach('Y', spec); }

static Walkqid *sipwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, nil, 0, sipgen);
}

static long sipstat(Chan *c, uchar *dp, long n) {
  return devstat(c, dp, n, nil, 0, sipgen);
}

static Chan *sipopen(Chan *c, int omode) {
  /* Universal CBS: Opening process ctl files requires ADMIN for others'
   * processes */
  int pid = PID(c->qid);

  if (TYPE(c->qid) == Qctl && pid != 0 && pid != up->pid) {
    if (!has_capability(up, PEBBLE_CAP_ADMIN))
      error(PEBBLE_E_PERM);
  }

  return devopen(c, omode, nil, 0, sipgen);
}

static void sipclose(Chan *c) { USED(c); }

static long sipread(Chan *c, void *va, long n, vlong off) {
  char buf[512];
  int len, pid;
  Proc *p;
  ulong caps;

  switch (TYPE(c->qid)) {
  case Qdir:
  case Qprocbase:
    return devdirread(c, va, n, nil, 0, sipgen);

  case Qclone:
    /* Return current process PID */
    len = snprint(buf, sizeof(buf), "%d\n", up->pid);
    if (off >= len)
      return 0;
    if (off + n > len)
      n = len - off;
    memmove(va, buf + off, n);
    return n;

  case Qctl:
    /* Return capabilities of target process */
    pid = PID(c->qid);
    if (pid == 0)
      p = up; /* Current process */
    else {
      /* Find target process */
      p = proctab(pid);
      if (p == nil || p->state == Dead)
        error("process not found");
    }

    caps = p->capabilities;
    len = snprint(buf, sizeof(buf),
                  "pid: %d\n"
                  "capabilities: %#lux\n"
                  "device: %d\n"
                  "ioport: %d\n"
                  "net: %d\n"
                  "irq: %d\n"
                  "dma: %d\n"
                  "pci: %d\n"
                  "fs: %d\n"
                  "admin: %d\n",
                  p->pid, caps, !!(caps & PEBBLE_CAP_DEVICE),
                  !!(caps & PEBBLE_CAP_IOPORT), !!(caps & PEBBLE_CAP_NET),
                  !!(caps & PEBBLE_CAP_IRQ), !!(caps & PEBBLE_CAP_DMA),
                  !!(caps & PEBBLE_CAP_PCI), !!(caps & PEBBLE_CAP_FS),
                  !!(caps & PEBBLE_CAP_ADMIN));

    if (off >= len)
      return 0;
    if (off + n > len)
      n = len - off;
    memmove(va, buf + off, n);
    return n;

  default:
    error(Egreg);
    return 0;
  }
}

static long sipwrite(Chan *c, void *va, long n, vlong off) {
  char buf[256];
  char *fields[8];
  int nfields, pid;
  Proc *p;
  ulong new_caps, mask;

  USED(off);

  switch (TYPE(c->qid)) {
  case Qctl:
    /* Write to reduce capabilities (monotonic decrease only) */

    /* Universal CBS: Require ADMIN capability to modify other processes */
    pid = PID(c->qid);
    if (pid == 0)
      p = up;
    else {
      if (!has_capability(up, PEBBLE_CAP_ADMIN))
        error(PEBBLE_E_PERM);

      p = proctab(pid);
      if (p == nil || p->state == Dead)
        error("process not found");
    }

    /* Parse command */
    if (n >= sizeof(buf))
      n = sizeof(buf) - 1;
    memmove(buf, va, n);
    buf[n] = '\0';

    nfields = tokenize(buf, fields, nelem(fields));
    if (nfields < 2)
      error("usage: drop <capability> OR set <mask>");

    if (strcmp(fields[0], "drop") == 0) {
      /* Drop specific capability: drop device */
      mask = 0;
      if (strcmp(fields[1], "device") == 0)
        mask = PEBBLE_CAP_DEVICE;
      else if (strcmp(fields[1], "ioport") == 0)
        mask = PEBBLE_CAP_IOPORT;
      else if (strcmp(fields[1], "net") == 0)
        mask = PEBBLE_CAP_NET;
      else if (strcmp(fields[1], "irq") == 0)
        mask = PEBBLE_CAP_IRQ;
      else if (strcmp(fields[1], "dma") == 0)
        mask = PEBBLE_CAP_DMA;
      else if (strcmp(fields[1], "pci") == 0)
        mask = PEBBLE_CAP_PCI;
      else if (strcmp(fields[1], "fs") == 0)
        mask = PEBBLE_CAP_FS;
      else if (strcmp(fields[1], "admin") == 0)
        mask = PEBBLE_CAP_ADMIN;
      else
        error("unknown capability");

      /* Monotonic decrease: can only remove capabilities */
      p->capabilities &= ~mask;

    } else if (strcmp(fields[0], "set") == 0) {
      /* Set capability mask directly (monotonic decrease enforced) */
      new_caps = strtoul(fields[1], nil, 0);

      /* Ensure we only decrease capabilities (monotonic) */
      if ((new_caps & ~p->capabilities) != 0)
        error("cannot gain capabilities (monotonic decrease only)");

      p->capabilities = new_caps;

    } else {
      error("unknown command");
    }

    return n;

  default:
    error(Eperm);
    return 0;
  }
}

Dev sipdevtab = {
    'Y',      "sip",

    devreset, devinit,  devshutdown, sipattach, sipwalk,
    sipstat,  sipopen,  devcreate,   sipclose,  sipread,
    devbread, sipwrite, devbwrite,   devremove, devwstat,
};
