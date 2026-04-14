#include "kernel.h"
#include "pebble_kernel.h"
#include "uuid.h"

Chan *devclone(Chan *c) {
  Chan *nc;
  nc = newchan();
  nc->type = c->type;
  nc->dev = c->dev;
  nc->mode = c->mode;
  nc->qid = c->qid;
  nc->aux = c->aux;
  nc->mchan = c->mchan;
  if (nc->mchan != nil)
    incref(&nc->mchan->ref);
  return nc;
}

/*@
  @ requires q == \null || \valid(q);
  @ assigns \nothing;
  @*/
void mkqid(Qid *q, vlong path, ulong vers, int type) {
  q->type = type;
  q->vers = vers;
  q->path = path;
}

/*@
  @ assigns \nothing;
  @*/
int devno(int c, int user) {
  int i;

  for (i = 0; devtab[i] != nil; i++) {
    if (devtab[i]->dc == c)
      return i;
  }
  if (user == 0) {
    print("devno: PANIC - searching for char '%c' (0x%x), caller=%p\n", c, c, getcallerpc(&c));
    panic("devno %C %#ux", c, c);
  }

  return -1;
}

void devmask(Pgrp *pgrp, int invert, char *devs) {
  int i, t, width_bits;
  char *p;
  Rune r;
  u64int mask[nelem(pgrp->notallowed)];

  if (invert)
    memset(mask, 0xFF, sizeof mask);
  else
    memset(mask, 0, sizeof mask);

  width_bits = sizeof mask[0] * 8;
  /*@ loop invariant valid_string(p);
    @ loop invariant \base_addr(p) == \base_addr(devs);
    @ loop invariant p >= devs;
    @ loop assigns p, t, r, mask[0..nelem(mask)-1];
    @*/
  for (p = devs; *p != '\0';) {
    p += chartorune(&r, p);
    t = devno(r, 1);
    if (t == -1)
      continue;
    if (t >= nelem(mask) * width_bits)
      continue; /* Safety bound */
    if (invert)
      mask[t / width_bits] &= ~(1 << t % width_bits);
    else
      mask[t / width_bits] |= 1 << t % width_bits;
  }

  wlock(&pgrp->ns);
  for (i = 0; i < nelem(pgrp->notallowed); i++)
    pgrp->notallowed[i] |= mask[i];
  namespace_cid_update_locked(pgrp);
  if (up != nil && up->pgrp == pgrp) {
    uuid_t *parent_p = nil;
    u8int *ns_cid = pgrp->namespace_cid;
    if (up->parent)
      parent_p = &up->parent->pid2;
    uuid_pack_pid_lux9(&up->pid2, parent_p, ns_cid, up->text_hash);
  }
  wunlock(&pgrp->ns);
}

/*@
  @ requires pgrp == \null || \valid(pgrp);
  @ assigns \nothing;
  @*/
int devallowed(Pgrp *pgrp, int r) {
  int t, width_bits, b;

  t = devno(r, 1);
  if (t == -1)
    return 0;

  width_bits = sizeof(u64int) * 8;
  rlock(&pgrp->ns);
  if (waserror()) {
    runlock(&pgrp->ns);
    nexterror();
  }
  b = !(pgrp->notallowed[t / width_bits] & 1 << t % width_bits);
  poperror();
  runlock(&pgrp->ns);
  return b;
}
/* Standard device init/shutdown stubs */
void devinit(void) {}
void devshutdown(void) {}
void devreset(void) {}

extern Dev *devtab[];

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
int canmount(Pgrp *p) {
  if (p == nil)
    return 1;
  return devallowed(p, 'M');
}

/*@
  @ requires c == \null || \valid(c);
  @ requires q == \null || \valid(q);
  @ requires n == \null || \valid(n);
  @ assigns \nothing;
  @*/
void devdir(Chan *c, Qid qid, char *n, vlong length, char *user, long perm,
            Dir *db) {
  ulong now;

  /*
   * db (dir) comes from stack in devwalk and is often uninitialized or
   * corrupted (e.g. holding 0x2 or stack addresses). Since devwalk doesn't free
   * dir.name anyway (causing a small leak), we unconditionally clear it to
   * prevent kstrdup from calling free() on garbage.
   */
  db->name = nil;
  db->uid = nil;
  db->gid = nil;
  db->muid = nil;
  kstrdup(&db->name, n);
  db->qid = qid;
  /* c->type is the device character (like '/', 'c', etc.)
   * During walk, cloned channels may temporarily have type=0.
   * If type is valid, use it directly; otherwise use 0 as fallback. */
  if (c->type != 0 && devno(c->type, 1) != -1) {
    db->type = c->type;
    db->dev = c->dev;
  } else {
    /* Channel type not set (during walk clone) - use 0 as placeholder */
    db->type = 0;
    db->dev = 0;
  }
  /* db->qshift removed - obsolete */
  db->mode = perm;
  now = (ulong)seconds();
  db->atime = now;
  db->mtime = now;
  db->length = length;
  kstrdup(&db->uid, user);
  kstrdup(&db->gid, eve);
  kstrdup(&db->muid, user);
}

/*@
  @ requires c == \null || \valid(c);
  @ requires spec == \null || \valid(spec);
  @ assigns \nothing;
  @*/
Chan *devattach(int tc, char *spec) {
  Chan *c;

  c = newchan();
  c->type = tc;  /* Store device CHARACTER, not index */
  if (devno(tc, 1) == -1)
    panic("devattach: bad dev char %c", tc);
  c->qid.type = QTDIR;
  c->qid.path = 0;
  c->qid.vers = 0;
  if (spec == nil)
    spec = "";
  /* kstrdup(&tn, spec); - tn unused and uninitialized */
  c->path = newpath((BString){"/", 1});
  /* manual path construction if needed */
  /* free(tn); */
  return c;
}

/*@
  @ requires c == \null || \valid(c);
  @ requires name == \null || \valid(name);
  @ requires tab == \null || \valid(tab);
  @ requires dir == \null || \valid(dir);
  @ assigns \nothing;
  @*/
int devgen(Chan *c, char *name, Dirtab *tab, int ntab, int i, Dir *dir) {
  Dirtab *dp;

  if (i == DEVDOTDOT) {
    devdir(c, c->qid, "..", 0, eve, 0555, dir);
    return 1;
  }
  if (i >= ntab)
    return -1;
  dp = &tab[i];
  if (name != nil && strcmp(name, dp->name) != 0)
    return 0;
  devdir(c, dp->qid, dp->name, dp->length, eve, dp->perm, dir);
  return 1;
}
/* ... devwalk ... */
/*@
  @ requires c == \null || \valid(c);
  @ requires nc == \null || \valid(nc);
  @ requires name == \null || \valid(name);
  @ requires tab == \null || \valid(tab);
  @ requires gen == \null || \valid(gen);
  @ assigns \nothing;
  @*/
Walkqid *devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab,
                 int ntab, Devgen *gen) {
  /* Stack canaries to detect corruption */
  volatile uintptr canary_top = 0xDEADBEEFCAFEBABEULL;
  volatile int alloc;
  int i, j;
  Walkqid *volatile walkq_ptr;
  Walkqid *volatile savedwq;
  Chan *volatile savedclone;
  volatile int savedalloc;
  char *n;
  Dir dir;
  memset(&dir, 0, sizeof(dir));
  if (dir.name != nil)
    uartputs("devwalk: corrupted after memset\n", 32);
  volatile uintptr canary_bottom = 0xFEEDFACEDEADC0DEULL;

  if (nname > 0)
    isdir(c);

  /* Check canaries */
  if (canary_top != 0xDEADBEEFCAFEBABEULL) {
    panic("stack corruption detected");
  }
  if (canary_bottom != 0xFEEDFACEDEADC0DEULL) {
    panic("stack corruption detected");
  }

  alloc = (nc == nil);
  walkq_ptr = smalloc(sizeof(Walkqid) + (nname - 1) * sizeof(Qid));
  if (dir.name != nil)
    uartputs("devwalk: corrupted after smalloc\n", 33);
  walkq_ptr->clone = nc;

  savedwq = up != nil ? up->walkq : nil;
  savedclone = up != nil ? up->walkclone : nil;
  savedalloc = up != nil ? up->walkalloc : 0;

  if (up != nil) {
    up->walkq = walkq_ptr;
    up->walkclone = nc;
    up->walkalloc = alloc;
  }
  if (dir.name != nil)
    uartputs("devwalk: corrupted after up setup\n", 34);
  if (waserror()) {
    /* Check canaries in error handler */
    if (canary_top != 0xDEADBEEFCAFEBABEULL) {
      panic("stack corruption detected in error path");
    }
    if (canary_bottom != 0xFEEDFACEDEADC0DEULL) {
      panic("stack corruption detected in error path");
    }
    Walkqid *cwq = up != nil && up->walkq != nil ? up->walkq : walkq_ptr;
    Chan *clone =
        up != nil ? up->walkclone : (walkq_ptr != nil ? walkq_ptr->clone : nil);
    int calloc = up != nil ? up->walkalloc : alloc;

    if (calloc && clone != nil)
      cclose(clone);
    if (up != nil) {
      up->walkq = savedwq;
      up->walkclone = savedclone;
      up->walkalloc = savedalloc;
    }
    free(cwq);
    return nil;
  }
  if (alloc) {
    nc = devclone(c);
    nc->type = 0; /* device doesn't know about this channel yet */
    walkq_ptr->clone = nc;
    if (up != nil) {
      up->walkclone = nc;
      up->walkalloc = alloc;
    }
  }

  for (j = 0; j < nname; j++) {

    if (!(nc->qid.type & QTDIR)) {
      if (j == 0)
        error(Enotdir);
      goto Done;
    }
    n = name[j];

    if (strcmp(n, ".") == 0) {
    Accept:
      walkq_ptr->qid[walkq_ptr->nqid++] = nc->qid;
      continue;
    }
    /* ... rest of loop ... */
    if (strcmp(n, "..") == 0) {
      if ((*gen)(nc, nil, tab, ntab, DEVDOTDOT, &dir) != 1) {
        print("devgen walk .. in dev%s %llux broken\n", devtab[devno(c->type, 0)]->name,
              c->qid.path);
        error("broken devgen");
      }
      nc->qid = dir.qid;
      goto Accept;
    }
    /*
     * Ugly problem: If we're using devgen, make sure we're
     * walking the directory itself, represented by the first
     * entry in the table, and not trying to step into a sub-
     * directory of the table, e.g. /net/net. Devgen itself
     * should take care of the problem, but it doesn't have
     * the necessary information (that we're doing a walk).
     */
    if (gen == devgen && nc->qid.path != tab[0].qid.path)
      goto Notfound;
    for (i = 0;; i++) {
      switch ((*gen)(nc, n, tab, ntab, i, &dir)) {
      case -1:
      Notfound:
        if (j == 0)
          error(Enonexist);
        kstrcpy(up->errstr, Enonexist, ERRMAX);
        goto Done;
      case 0:
        continue;
      case 1:
        if (strcmp(n, dir.name) == 0) {
          nc->qid = dir.qid;
          goto Accept;
        }
        continue;
      }
    }
  }
  /*
   * We processed at least one name, so will return some data.
   * If we didn't process all nname entries succesfully, we drop
   * the cloned channel and return just the Qids of the walks.
   */

Done:
  /* Check canaries before Done */
  if (canary_top != 0xDEADBEEFCAFEBABEULL) {
    panic("stack corruption detected at Done");
  }
  if (canary_bottom != 0xFEEDFACEDEADC0DEULL) {
    panic("stack corruption detected at Done");
  }
  poperror();
  Walkqid *retq = walkq_ptr;
  if (up != nil) {
    retq = up->walkq;
    if (retq != nil) {
      if (retq->nqid < nname) {
        if (up->walkalloc && retq->clone != nil)
          cclose(retq->clone);
        retq->clone = nil;
      } else if (retq->clone != nil) {
        retq->clone->type = c->type;
      }
    }
    up->walkq = savedwq;
    up->walkclone = savedclone;
    up->walkalloc = savedalloc;
  } else if (retq != nil) {
    if (retq->nqid < nname) {
      if (alloc && retq->clone != nil)
        cclose(retq->clone);
      retq->clone = nil;
    } else if (retq->clone != nil) {
      retq->clone->type = c->type;
    }
  }
  /* Check canaries before return */
  if (canary_top != 0xDEADBEEFCAFEBABEULL) {
    panic("stack corruption detected before return");
  }
  if (canary_bottom != 0xFEEDFACEDEADC0DEULL) {
    panic("stack corruption detected before return");
  }
  return retq;
}

/*@
  @ requires c == \null || \valid(c);
  @ requires db == \null || \valid(db);
  @ requires tab == \null || \valid(tab);
  @ requires gen == \null || \valid(gen);
  @ assigns \nothing;
  @*/
int devstat(Chan *c, uchar *db, int n, Dirtab *tab, int ntab, Devgen *gen) {
  int i;
  Dir dir;
  char *p, *elem;

  for (i = 0;; i++) {
    switch ((*gen)(c, nil, tab, ntab, i, &dir)) {
    case -1:
      if (c->qid.type & QTDIR) {
        if (c->path == nil)
          elem = "???";
        else if (strcmp(c->path->s, "/") == 0)
          elem = "/";
        else
          for (elem = p = c->path->s; *p; p++)
            if (*p == '/')
              elem = p + 1;
        devdir(c, c->qid, elem, 0, eve, 0555, &dir);
        n = convD2M(&dir, db, n);
        if (n == 0)
          error(Ebadarg);
        return n;
      }

      error(Enonexist);
    case 0:
      break;
    case 1:
      if (c->qid.path == dir.qid.path) {
        if (c->flag & CMSG)
          dir.mode |= DMMOUNT;
        n = convD2M(&dir, db, n);
        if (n == 0)
          error(Ebadarg);
        return n;
      }
      break;
    }
  }
}

/*@
  @ requires c == \null || \valid(c);
  @ requires d == \null || \valid(d);
  @ requires tab == \null || \valid(tab);
  @ requires gen == \null || \valid(gen);
  @ assigns \nothing;
  @*/
long devdirread(Chan *c, char *d, long n, Dirtab *tab, int ntab, Devgen *gen) {
  long m, dsz;
  Dir dir;

  for (m = 0; m < n; c->dri++) {
    switch ((*gen)(c, nil, tab, ntab, c->dri, &dir)) {
    case -1:
      return m;

    case 0:
      break;

    case 1:
      dsz = convD2M(&dir, (uchar *)d, n - m);
      if (dsz <=
          BIT16SZ) { /* <= not < because this isn't stat; read is stuck */
        if (m == 0)
          error(Eshort);
        return m;
      }
      m += dsz;
      d += dsz;
      break;
    }
  }

  return m;
}

/*
 * error(Eperm) if open permission not granted for up->user.
 */
/*@
  @ requires fileuid == \null || \valid(fileuid);
  @ assigns \nothing;
  @*/
void devpermcheck(char *fileuid, ulong perm, int omode) {
  ulong t;
  static int access[] = {0400, 0200, 0600, 0100};

  if (strcmp(up->user, fileuid) == 0)
    perm <<= 0;
  else if (strcmp(up->user, eve) == 0)
    perm <<= 3;
  else
    perm <<= 6;

  t = access[omode & 3];
  if ((t & perm) != t)
    error(Eperm);
}

Chan *devopen(Chan *c, int omode, Dirtab *tab, int ntab, Devgen *gen) {
  int i;
  Dir dir;

  for (i = 0;; i++) {
    switch ((*gen)(c, nil, tab, ntab, i, &dir)) {
    case -1:
      goto Return;
    case 0:
      break;
    case 1:
      if (c->qid.path == dir.qid.path) {
        devpermcheck(dir.uid, dir.mode, omode);
        goto Return;
      }
      break;
    }
  }
Return:
  c->offset = 0;
  if ((c->qid.type & QTDIR) && omode != OREAD)
    error(Eperm);
  c->mode = openmode(omode);
  c->flag |= COPEN;
  return c;
}

Chan *devcreate(Chan *, char *, int, ulong) { error(Eperm); }

void devremove(Chan *) { error(Eperm); }

int devwstat(Chan *, uchar *, int) { error(Eperm); }

void devpower(int) { error(Eperm); }

int devconfig(int, char *, DevConf *) { error(Eperm); }
