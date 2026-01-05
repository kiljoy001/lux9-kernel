#include "dat.h"
#include "fns.h"
#include "lock_borrow.h"
#include "mem.h"
#include "monocypher.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

enum {
  Whinesecs = 10, /* frequency of out-of-resources printing */
};

static LockDagNode lockdag_nextmount = LOCKDAG_NODE("pgrp-nextmount");
static uintptr nextmount_lock_key;
static BorrowLock nextmount_lock = {
    .key = (uintptr)&nextmount_lock_key,
    .dag_node = &lockdag_nextmount,
};

void namespace_cid_update(Pgrp *pgrp);
void namespace_cid_update_locked(Pgrp *pgrp);

uvlong nextmount(void) {
  static uvlong next = 0;
  uvlong n;

  borrow_lock(&nextmount_lock);
  n = ++next;
  borrow_unlock(&nextmount_lock);
  return n;
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
  /* Initial Namespace CID: deterministic hash of empty namespace config */
  /* No lock needed - pgrp is brand new and not yet shared */
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

static void pgrpinsert(Mount **order, Mount *m) {
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

/*
 * pgrpcpy MUST preserve the mountid allocation order of the parent group
 */
void pgrpcpy(Pgrp *to, Pgrp *from) {
  Mount *n, *m, **link, *order;
  Mhead *f, **l, *mh;
  int i;

  /* Inherit spawn limit from parent namespace */
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
  /*
   * Allocate mount ids in the same sequence as the parent group
   */
  for (m = order; m != nil; m = m->order)
    m->mountid = nextmount();
  namespace_cid_update_locked(to);
  runlock(&from->ns);
  wunlock(&to->ns);
  poperror();
}

typedef struct NsMountEntry NsMountEntry;
struct NsMountEntry {
  uvlong mountid;
  Mhead *mhead;
  Mount *mount;
};

static void ns_hash_u32(crypto_blake2b_ctx *ctx, u32int v) {
  u8int buf[4];
  buf[0] = (u8int)(v >> 24);
  buf[1] = (u8int)(v >> 16);
  buf[2] = (u8int)(v >> 8);
  buf[3] = (u8int)(v);
  crypto_blake2b_update(ctx, buf, sizeof(buf));
}

static void ns_hash_u64(crypto_blake2b_ctx *ctx, u64int v) {
  u8int buf[8];
  buf[0] = (u8int)(v >> 56);
  buf[1] = (u8int)(v >> 48);
  buf[2] = (u8int)(v >> 40);
  buf[3] = (u8int)(v >> 32);
  buf[4] = (u8int)(v >> 24);
  buf[5] = (u8int)(v >> 16);
  buf[6] = (u8int)(v >> 8);
  buf[7] = (u8int)(v);
  crypto_blake2b_update(ctx, buf, sizeof(buf));
}

static void namespace_cid_hash_mount(crypto_blake2b_ctx *ctx, Mhead *mh,
                                     Mount *m) {
  u32int spec_len = 0;

  ns_hash_u32(ctx, (u32int)m->mflag);

  if (mh && mh->from) {
    ns_hash_u64(ctx, (u64int)mh->from->qid.path);
    ns_hash_u32(ctx, (u32int)mh->from->qid.type);
  } else {
    ns_hash_u64(ctx, 0);
    ns_hash_u32(ctx, 0);
  }

  if (m->to) {
    ns_hash_u64(ctx, (u64int)m->to->qid.path);
    ns_hash_u32(ctx, (u32int)m->to->qid.type);
    ns_hash_u32(ctx, (u32int)m->to->dev);
    ns_hash_u32(ctx, (u32int)m->to->type);
  } else {
    ns_hash_u64(ctx, 0);
    ns_hash_u32(ctx, 0);
    ns_hash_u32(ctx, 0);
    ns_hash_u32(ctx, 0);
  }

  if (m->spec != nil)
    spec_len = (u32int)strlen(m->spec);
  ns_hash_u32(ctx, spec_len);
  if (spec_len > 0)
    crypto_blake2b_update(ctx, (const u8int *)m->spec, spec_len);
}

static void namespace_cid_hash_unsorted(crypto_blake2b_ctx *ctx, Pgrp *pgrp) {
  Mhead *mh;
  Mount *m;
  int i;

  for (i = 0; i < MNTHASH; i++) {
    for (mh = pgrp->mnthash[i]; mh != nil; mh = mh->hash) {
      for (m = mh->mount; m != nil; m = m->next)
        namespace_cid_hash_mount(ctx, mh, m);
    }
  }
}

void namespace_cid_update_locked(Pgrp *pgrp) {
  crypto_blake2b_ctx ctx;
  Mhead *mh;
  Mount *m;
  NsMountEntry *entries = nil;
  int count = 0;
  int filled = 0;
  int i;

  if (!pgrp)
    return;

  crypto_blake2b_init(&ctx, 32);
  crypto_blake2b_update(&ctx, (const u8int *)"NSCIDv1", 7);

  for (i = 0; i < nelem(pgrp->notallowed); i++)
    ns_hash_u64(&ctx, pgrp->notallowed[i]);
  ns_hash_u32(&ctx, pgrp->spawn_limit);

  for (i = 0; i < MNTHASH; i++) {
    for (mh = pgrp->mnthash[i]; mh != nil; mh = mh->hash) {
      for (m = mh->mount; m != nil; m = m->next)
        count++;
    }
  }

  if (count == 0) {
    crypto_blake2b_final(&ctx, pgrp->namespace_cid);
    return;
  }

  entries = malloc(sizeof(*entries) * count);
  if (entries == nil) {
    namespace_cid_hash_unsorted(&ctx, pgrp);
    crypto_blake2b_final(&ctx, pgrp->namespace_cid);
    return;
  }

  for (i = 0; i < MNTHASH; i++) {
    for (mh = pgrp->mnthash[i]; mh != nil; mh = mh->hash) {
      for (m = mh->mount; m != nil; m = m->next) {
        int pos = filled;
        while (pos > 0 && entries[pos - 1].mountid > m->mountid)
          pos--;
        if (pos < filled)
          memmove(&entries[pos + 1], &entries[pos],
                  (filled - pos) * sizeof(*entries));
        entries[pos].mountid = m->mountid;
        entries[pos].mhead = mh;
        entries[pos].mount = m;
        filled++;
      }
    }
  }

  for (i = 0; i < filled; i++)
    namespace_cid_hash_mount(&ctx, entries[i].mhead, entries[i].mount);

  free(entries);
  crypto_blake2b_final(&ctx, pgrp->namespace_cid);
}

void namespace_cid_update(Pgrp *pgrp) {
  if (!pgrp)
    return;
  wlock(&pgrp->ns);
  namespace_cid_update_locked(pgrp);
  wunlock(&pgrp->ns);
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

/*
 * Called from interrupted() because up is in the middle
 * of closefgrp and just got a kill ctl message.
 * This usually means that up has wedged because
 * of some kind of deadly embrace with mntclose
 * trying to talk to itself.  To break free, hand the
 * unclosed channels to the close queue.  Once they
 * are finished, the blocked cclose that we've
 * interrupted will finish by itself.
 */
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
      ccloseq(c);
    }
}

Mount *newmount(Chan *to, int flag, char *spec) {
  Mount *m;

  if (spec == nil)
    spec = "";
  m = malloc(sizeof(Mount) + strlen(spec) + 1);
  if (m == nil)
    error(Enomem);
  m->to = to;
  incref((Ref *)&to->ref);
  m->mountid = nextmount();
  m->mflag = flag;
  strcpy(m->spec, spec);
  setmalloctag(m, getcallerpc(&to));
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
