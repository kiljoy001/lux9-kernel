#include "dat.h"
#include "edf.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include "ureg.h"
#include <error.h>

enum {
  Qdir,
  Qctl,
  Qmem,
  Qnote,
  Qstatus,
  Qpid2,
  Qppid,
};

enum {
  CMclose,
  CMkill,
  CMstop,
  CMstart,
};

#define STATSIZE (2 * 28 + 12 + 9 * 12)

Dirtab procdir[] = {
    "ctl",    {Qctl},    0,        0000,
    "mem",    {Qmem},    0,        0000,
    "note",   {Qnote},   0,        0000,
    "status", {Qstatus}, STATSIZE, 0444,
    "pid2",   {Qpid2},   16,       0444,
    "ppid",   {Qppid},   12,       0444,
};

static Cmdtab proccmd[] = {
    {CMclose, "close", 2},
    {CMkill, "kill", 1},
    {CMstop, "stop", 1},
    {CMstart, "start", 1},
};

#define QSHIFT 5
#define QID(q) ((((ulong)(q).path) & 0x0000001F) >> 0)
#define SLOTMAX 0x4000000
#define SLOT(q) ((int)(((((ulong)(q).path) >> QSHIFT) & (0x4000000UL - 1)) - 1))
#define PID(q) ((q).vers)

/* Symbols needed by other kernel parts (e.g. edf.c) */
void (*proctrace)(Proc *, int, vlong);

static void procctlreq(Proc *, char *, int);
static long procctlmemio(Chan *, Proc *, uintptr, void *, long, int);

static int procgen(Chan *c, char *name, Dirtab *tab, int, int s, Dir *dp) {
  Qid qid;
  Proc *p;
  char *ename;
  ulong pid, path, perm;

  if (s == DEVDOTDOT) {
    mkqid(&qid, (vlong)Qdir, 0, QTDIR);
    devdir(c, qid, "#p", 0, eve, 0555, dp);
    return 1;
  }

  if (c->qid.path == Qdir) {
    if (name != nil) {
      pid = (ulong)strtol(name, &ename, 10);
      if (pid == 0 || ename[0] != '\0')
        return -1;
      s = (int)procindex(pid);
      if (s < 0)
        return -1;
    } else if (--s >= (int)conf.nproc)
      return -1;

    p = proctab(s);
    if (p == nil || p->pid == 0)
      return 0;

    snprint(up->genbuf, sizeof(up->genbuf), "%lud", (ulong)p->pid);
    if (name != nil && strcmp(name, up->genbuf) != 0)
      return -1;
    mkqid(&qid, (vlong)((ulong)(s + 1) << QSHIFT), (ulong)p->pid, QTDIR);
    devdir(c, qid, up->genbuf, 0, p->user, 0555, dp);
    return 1;
  }

  if (s >= (int)nelem(procdir))
    return -1;

  tab = &procdir[s];
  path = (ulong)c->qid.path & ~((1UL << QSHIFT) - 1);
  p = proctab(SLOT(c->qid));

  perm = (ulong)tab->perm;
  if (perm == 0)
    perm = (ulong)p->procmode;
  else
    perm |= (ulong)p->procmode & 0444UL;

  mkqid(&qid, (vlong)(path | (ulong)tab->qid.path), c->qid.vers, QTFILE);
  devdir(c, qid, tab->name, tab->length, p->user, (long)perm, dp);
  return 1;
}

static void procinit(void) {}

static Chan *procattach(char *spec) { return devattach('p', spec); }

static Walkqid *procwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, 0, 0, procgen);
}

static int procstat(Chan *c, uchar *db, int n) {
  return devstat(c, db, n, 0, 0, procgen);
}

static Chan *procopen(Chan *c, int omode) {
  return devopen(c, omode, 0, 0, procgen);
}

static void procclose(Chan *c) {
  Segio *sio;
  if ((c->flag & COPEN) == 0)
    return;
  if (QID(c->qid) == Qmem) {
    sio = c->aux;
    if (sio != nil) {
      c->aux = nil;
      segio(sio, nil, nil, 0, 0, 0);
      free(sio);
    }
  }
}

static long procread(Chan *c, void *va, long n, vlong off) {
  char statbuf[1024];
  ulong offset = (ulong)off;
  int j;
  Proc *p;

  if (c->qid.type & QTDIR)
    return devdirread(c, va, n, 0, 0, procgen);

  p = proctab(SLOT(c->qid));
  if (p->pid != PID(c->qid))
    error(Eprocdied);

  switch (QID(c->qid)) {
  case Qmem:
    return procctlmemio(c, p, (uintptr)off, va, n, 1);
  case Qctl:
    return readnum(offset, va, n, (ulong)p->pid, NUMSIZE);
  case Qstatus:
    j = snprint(statbuf, sizeof(statbuf), "%-27s %-27s %s %11lud\n", p->text,
                p->user, statename[p->state],
                (ulong)(procpagecount(p) * BY2PG / 1024));
    if (offset >= (ulong)j)
      return 0;
    if (offset + (ulong)n > (ulong)j)
      n = (long)((ulong)j - offset);
    memmove(va, (statbuf + offset), (usize)n);
    return n;
  case Qnote:
    if (p->nnote == 0)
      return 0;
    j = (int)strlen(p->note[0]->msg) + 1;
    if (j < (int)n)
      n = (long)j;
    memmove(va, p->note[0]->msg, (usize)n - 1);
    ((char *)va)[n - 1] = '\0';
    return n;
  case Qpid2:
    if (offset >= 16)
      return 0;
    if (offset + (ulong)n > 16)
      n = (long)(16 - offset);
    memmove(va, p->pid2.data + offset, (usize)n);
    return n;
  case Qppid:
    return readnum(offset, va, n, (p->parent ? (ulong)p->parent->pid : 0),
                   NUMSIZE);
  }
  error(Ebadarg);
  return 0;
}

static long procwrite(Chan *c, void *va, long n, vlong off) {
  Proc *p;
  p = proctab(SLOT(c->qid));

  switch (QID(c->qid)) {
  case Qmem:
    return procctlmemio(c, p, (uintptr)off, va, n, 0);
  case Qctl:
    procctlreq(p, (char *)va, (int)n);
    return n;
  case Qnote:
    postnote(p, 0, (char *)va, NUser);
    return n;
  }
  error(Ebadarg);
  return 0;
}

static void procctlreq(Proc *p, char *va, int n) {
  Cmdbuf *cb;
  Cmdtab *ct;

  cb = parsecmd(va, n);
  if (waserror()) {
    free(cb);
    nexterror();
  }

  ct = lookupcmd(cb, proccmd, nelem(proccmd));
  switch (ct->index) {
  case CMkill:
    postnote(p, 0, "kill", NExit);
    break;
  case CMstop:
    p->procctl = (int)Proc_stopme;
    break;
  case CMstart:
    ready(p);
    break;
  }
  poperror();
  free(cb);
}

static long procctlmemio(Chan *c, Proc *p, uintptr offset, void *a, long n,
                         int read) {
  Segment *s;
  Segio *sio;

  eqlock(&p->seglock);
  if (waserror()) {
    qunlock(&p->seglock);
    nexterror();
  }
  s = seg(p, (uintptr)offset, 1);
  if (s == nil)
    error(Ebadarg);

  sio = c->aux;
  if (sio == nil) {
    sio = smalloc(sizeof(Segio));
    c->aux = sio;
  }
  n = segio(sio, s, a, n, (vlong)(offset - s->base), read);
  qunlock(&p->seglock);
  poperror();
  return n;
}

Dev procdevtab = {
    .dc = 'p',
    .name = "proc",
    .reset = devreset,
    .init = procinit,
    .shutdown = devshutdown,
    .attach = procattach,
    .walk = procwalk,
    .stat = procstat,
    .open = procopen,
    .create = devcreate,
    .close = procclose,
    .read = procread,
    .write = procwrite,
    .remove = devremove,
    .wstat = devwstat,
};
