#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

enum {
  Whinesecs = 10, /* frequency of out-of-resources printing */
};

/* Stub for namespace CID update until userspace migration is complete */
void namespace_cid_update(Pgrp *pgrp) { (void)pgrp; }
void namespace_cid_update_locked(Pgrp *pgrp) { (void)pgrp; }

static Lock nextmountlock;
static uvlong nextmountid;

static uvlong
nextmount(void)
{
  uvlong n;

  lock(&nextmountlock);
  n = ++nextmountid;
  unlock(&nextmountlock);
  return n;
}

static void
pgrpinsert(Mount **order, Mount *m)
{
  Mount *f;

  m->order = nil;
  for (f = *order; f != nil; f = f->order) {
    if (m->mountid < f->mountid) {
      m->order = f;
      *order = m;
      return;
    }
    order = &f->order;
  }
  *order = m;
}

Pgrp *newpgrp(void) {
  Pgrp *p;

  p = malloc(sizeof(Pgrp));
  if (p == nil)
    error(Enomem);
  memset(p, 0, sizeof(*p));
  p->ref = 1;

  /* Initialize spawn limits */
  p->spawn_limit = 256; /* Default limit (safeguard) */
  p->spawn_count = 0;

  /* Generate cryptographic identity for capability binding */
  uuid_new_v8((uuid_t *)p->identity_hash);
  namespace_cid_update_locked(p);

  return p;
}

Rgrp *newrgrp(void) {
  Rgrp *r;

  r = malloc(sizeof(Rgrp));
  if (r == nil)
    error(Enomem);
  memset(r, 0, sizeof(*r));
  r->ref = 1;
  return r;
}

void closergrp(Rgrp *r) {
  if (decref(r) == 0)
    free(r);
}

void closepgrp(Pgrp *p) {
  Mhead **h, **e, *f;
  Mount *m;

  if (decref(p))
    return;

  e = &p->mnthash[MNTHASH];
  for (h = p->mnthash; h < e; h++) {
    while ((f = *h) != nil) {
      *h = f->hash;
      wlock(&f->lock);
      m = f->mount;
      f->mount = nil;
      wunlock(&f->lock);
      mountfree(m);
      putmhead(f);
    }
  }

  free(p);
}

void pgrpcpy(Pgrp *to, Pgrp *from) {
  Mount *n, *m, **link, *order;
  Mhead *f, **l, *mh;
  int i;

  to->spawn_limit = from->spawn_limit;

  wlock(&to->ns);
  rlock(&from->ns);
  if (waserror()) {
    runlock(&from->ns);
    wunlock(&to->ns);
    nexterror();
  }

  order = nil;
  for (i = 0; i < MNTHASH; i++) {
    l = &to->mnthash[i];
    for (f = from->mnthash[i]; f != nil; f = f->hash) {
      rlock(&f->lock);
      if (waserror()) {
        runlock(&f->lock);
        nexterror();
      }

      mh = newmhead(f->from);
      *l = mh;
      l = &mh->hash;
      link = &mh->mount;
      for (m = f->mount; m != nil; m = m->next) {
        n = malloc(sizeof(Mount) + strlen(m->spec) + 1);
        if (n == nil)
          error(Enomem);
        memset(n, 0, sizeof(Mount) + strlen(m->spec) + 1);
        n->mountid = m->mountid;
        n->mflag = m->mflag;
        n->to = m->to;
        incref((Ref *)&n->to->ref);
        strcpy(n->spec, m->spec);
        pgrpinsert(&order, n);
        *link = n;
        link = &n->next;
      }
      runlock(&f->lock);
      poperror();
    }
  }

  for (m = order; m != nil; m = m->order)
    m->mountid = nextmount();
  namespace_cid_update_locked(to);

  runlock(&from->ns);
  wunlock(&to->ns);
  poperror();
}

Mount *newmount(Chan *to, int flag, char *spec) {
  Mount *m;

  if (spec == nil)
    spec = "";
  m = malloc(sizeof(Mount) + strlen(spec) + 1);
  if (m == nil)
    error(Enomem);
  memset(m, 0, sizeof(Mount) + strlen(spec) + 1);
  m->to = to;
  incref((Ref *)&to->ref);
  m->mountid = nextmount();
  m->mflag = flag;
  strcpy(m->spec, spec);
  return m;
}

void mountfree(Mount *m) {
  Mount *f;

  while ((f = m) != nil) {
    m = m->next;
    cclose(f->to);
    free(f);
  }
}

Fgrp *dupfgrp(Fgrp *f) {
  Fgrp *new;
  Chan *c;
  int i;

  new = malloc(sizeof(Fgrp));
  if (new == nil)
    error(Enomem);
  memset(new, 0, sizeof(*new)); /* Zero all fields including lock */
  new->ref.ref = 1;
  if (f == nil) {
    new->nfd = DELTAFD;
    new->fd = malloc(DELTAFD * sizeof(new->fd[0]));
    new->flag = malloc(DELTAFD * sizeof(new->flag[0]));
    if (new->fd == nil || new->flag == nil) {
      free(new->flag);
      free(new->fd);
      free(new);
      error(Enomem);
    }
    return new;
  }

  lock(&f->lock);
  /* Make new fd list shorter if possible, preserving quantization */
  new->nfd = f->maxfd + 1;
  i = new->nfd % DELTAFD;
  if (i != 0)
    new->nfd += DELTAFD - i;
  new->fd = malloc(new->nfd * sizeof(new->fd[0]));
  new->flag = malloc(new->nfd * sizeof(new->flag[0]));
  if (new->fd == nil || new->flag == nil) {
    unlock(&f->lock);
    free(new->flag);
    free(new->fd);
    free(new);
    error(Enomem);
  }
  new->maxfd = f->maxfd;
  for (i = 0; i <= f->maxfd; i++) {
    if ((c = f->fd[i]) != nil) {
      new->fd[i] = c;
      new->flag[i] = f->flag[i];
      incref((Ref *)&c->ref);
    }
  }
  unlock(&f->lock);

  return new;
}

void closefgrp(Fgrp *f) {
  int i;
  Chan *c;

  if (f == nil || decref(&f->ref))
    return;

  /*
   * If we get into trouble, forceclosefgrp
   * will bail us out.
   */
  up->closingfgrp = f;
  for (i = 0; i <= f->maxfd; i++)
    if ((c = f->fd[i]) != nil) {
      f->fd[i] = nil;
      cclose(c);
    }
  up->closingfgrp = nil;

  free(f->flag);
  free(f->fd);
  free(f);
}

void forceclosefgrp(void) {
  int i;
  Chan *c;
  Fgrp *f;

  if (up->procctl != Proc_exitme || up->closingfgrp == nil) {
    print("bad forceclosefgrp call");
    return;
  }

  f = up->closingfgrp;
  for (i = 0; i <= f->maxfd; i++)
    if ((c = f->fd[i]) != nil) {
      f->fd[i] = nil;
      /* ccloseq removed - using standard cclose */
      /* ccloseq was for queuing closures, but we simplified queues */
      cclose(c);
    }
}

void resrcwait(char *reason) {
  static ulong lastwhine;
  ulong now;
  char *p;

  if (up == nil)
    panic("resrcwait: %s", reason);

  p = up->psstate;
  if (reason != nil) {
    if (waserror()) {
      up->psstate = p;
      nexterror();
    }
    up->psstate = reason;
    now = seconds();
    /* don't tie up the console with complaints */
    if (now - lastwhine > Whinesecs) {
      lastwhine = now;
      print("%s\n", reason);
    }
  }
  tsleep(&up->sleep, return0, 0, 100 + nrand(200));
  if (reason != nil) {
    up->psstate = p;
    poperror();
  }
}
