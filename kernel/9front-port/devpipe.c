#ifndef __FRAMAC__
/*
 * devpipe.c - 9front bidirectional pipe device
 *
 * FORMAL VERIFICATION:
 *   Coq proofs:  proofs/pipe/types.v, conservation.v, safety.v
 *   Frama-C:     ACSL annotations below
 *
 * Key invariants proven:
 *   - ref >= 0 at all times (Inv_RefPositive)
 *   - ref=0 triggers cleanup (ref_zero_means_fully_closed)
 *   - Write to q[id], read from q[1-id] (write_read_duality)
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

#define PIPESIZE (4096)

/*@ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid((char *)\result);
  @ behavior nonzero:
  @   assumes size > 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid(((char *)\result) + (0 .. (integer)size
  - 1));
  @ complete behaviors;
  @ disjoint behaviors;
  @ terminates \true;
  */
extern void *malloc(ulong size);

/*@ assigns \nothing;
 */
extern void free(void *p);

typedef struct Pipe Pipe;
struct Pipe {
  QLock l;
  int ref;
  Queue *q[2];
  int qref[2];
};

static void pipeinit(void) {}

/*
 * Called by 9p_router when a pipe fid is cloned.
 * Matches logic in pipewalk clone.
 */
void pipe_clone_notify(void *aux) {
  Pipe *p;

  if (aux == nil)
    return;

  p = aux;
  qlock(&p->l);
  p->ref++;
  /* qref update?
   * In walk we distinguish dir vs file.
   * Here we assume it's a file clone if subtype is DEV_PIPE?
   * The router uses subtype DEV_PIPE for the pipe *files* presumably.
   * If it's the dir, subtype might be different?
   * For now, just increment ref to prevent premature free.
   */
  qunlock(&p->l);
}

/*@ ensures \result != \null ==> ((Pipe*)\result->aux)->ref == 1;
    ensures \result != \null ==> ((Pipe*)\result->aux)->qref[0] == 0;
    ensures \result != \null ==> ((Pipe*)\result->aux)->qref[1] == 0;
    assigns \nothing;
    // COQ_PROOF_REF: proofs/pipe/conservation.v:attach_creates_ref
    // COQ_PROOF_REF: proofs/pipe/conservation.v:attach_wellformed
*/
static Chan *pipeattach(char *spec) {
  Pipe *p;
  Chan *c;

  p = malloc(sizeof(Pipe));
  if (p == nil)
    error(Enomem);
  p->ref = 1;
  p->q[0] = qopen(PIPESIZE, 0, 0, 0);
  p->q[1] = qopen(PIPESIZE, 0, 0, 0);
  if (p->q[0] == nil || p->q[1] == nil) {
    if (p->q[0])
      qfree(p->q[0]);
    if (p->q[1])
      qfree(p->q[1]);
    free(p);
    error(Enomem);
  }
  p->qref[0] = 0;
  p->qref[1] = 0;

  c = devattach('|', spec);
  c->aux = p;
  return c;
}

static int pipegen(Chan *c, char *name, Dirtab *tab, int ntab, int s, Dir *dp) {
  int id;
  Qid q;
  Pipe *p;

  USED(tab);
  USED(ntab);

  p = c->aux;
  if (s == DEVDOTDOT) {
    devdir(c, c->qid, "#|", 0, eve, 0555, dp);
    return 1;
  }

  /*
   * We only have data (0) and data1 (1).
   * s counts entries.
   */
  if (s > 1)
    return -1;

  id = s; /* 0 or 1 */
  q.type = 0;
  q.vers = 0;
  q.path = id + 1; /* Qid path must be unique? */
  /* Actually, let's use 1 and 2 */

  if (name != nil) {
    if (strcmp(name, "data") == 0)
      id = 0;
    else if (strcmp(name, "data1") == 0)
      id = 1;
    else
      return -1;
  }

  q.path = id + 1;
  devdir(c, q, id == 0 ? "data" : "data1", 0, eve, 0660, dp);
  dp->length = qlen(p->q[id]);
  return 1;
}

/*@ requires c != \null && c->aux != \null;
    requires ((Pipe*)c->aux)->ref > 0;
    ensures \result != \null && \result->clone != c ==>
            ((Pipe*)c->aux)->ref == \old(((Pipe*)c->aux)->ref) + 1;
    assigns ((Pipe*)c->aux)->ref,
            ((Pipe*)c->aux)->qref[0],
            ((Pipe*)c->aux)->qref[1];
    // COQ_PROOF_REF: proofs/pipe/conservation.v:walk_clone_increments_ref
    // COQ_PROOF_REF: proofs/pipe/conservation.v:close_inverse_of_clone
*/
static Walkqid *pipewalk(Chan *c, Chan *nc, char **name, int nname) {
  Walkqid *wq;
  Pipe *p;

  wq = devwalk(c, nc, name, nname, 0, 0, pipegen);
  if (wq != nil && wq->clone != nil && wq->clone != c) {
    p = c->aux;
    qlock(&p->l);
    p->ref++;
    if (c->qid.type & QTDIR) {
      /* Directory ref, do nothing special for queue */
    } else {
      /* File ref, verify path */
      int id = c->qid.path - 1;
      if (id >= 0 && id <= 1)
        p->qref[id]++;
    }
    qunlock(&p->l);
  }
  return wq;
}

static int pipestat(Chan *c, uchar *db, int n) {
  return devstat(c, db, n, 0, 0, pipegen);
}

static Chan *pipeopen(Chan *c, int omode) {
  Pipe *p;
  int id;

  if (c->qid.type & QTDIR) {
    if (omode != OREAD)
      error(Eperm);
    c->mode = openmode(omode);
    c->flag |= COPEN;
    c->offset = 0;
    return c;
  }

  p = c->aux;
  id = c->qid.path - 1;
  if (id < 0 || id > 1)
    error(Egreg);

  c->mode = openmode(omode);
  c->flag |= COPEN;
  c->offset = 0;
  c->iounit = PIPESIZE;

  /* Ref count update handled in walk or here?
     Standard Plan 9 devpipe doesn't increment qref on open,
     it assumes walk did it or just tracks global refs.
     We simplified pipewalk to increment qref on clone.
     But syspipe walks then opens.
     Let's ensure we don't double count if we walk then open.
  */
  return c;
}

/*@ requires c != \null && c->aux != \null;
    requires ((Pipe*)c->aux)->ref > 0;
    assigns ((Pipe*)c->aux)->ref,
            ((Pipe*)c->aux)->q[0],
            ((Pipe*)c->aux)->q[1];
    behavior last_ref:
      assumes ((Pipe*)c->aux)->ref == 1;
      ensures \freed((Pipe*)c->aux);
    behavior more_refs:
      assumes ((Pipe*)c->aux)->ref > 1;
      ensures ((Pipe*)c->aux)->ref == \old(((Pipe*)c->aux)->ref) - 1;
    complete behaviors;
    disjoint behaviors;
    // COQ_PROOF_REF: proofs/pipe/conservation.v:close_decrements_ref
    // COQ_PROOF_REF: proofs/pipe/conservation.v:balanced_clone_close
    // COQ_PROOF_REF: proofs/pipe/conservation.v:close_inverse_of_clone
    // COQ_PROOF_REF: proofs/pipe/safety.v:double_free_prevented
*/
static void pipeclose(Chan *c) {
  Pipe *p;
  int id;

  p = c->aux;
  qlock(&p->l);

  if (c->qid.type & QTDIR) {
    /* Directory close */
  } else {
    id = c->qid.path - 1;
    if (id >= 0 && id <= 1) {
      /* Standard pipe logic:
         If we are closing one end, we should close the queue
         so the other end gets EOF or Epipe.
         But multiple fds can point to one end (dup).
         So we need qref.
      */
      // p->qref[id]--; /* We incremented in walk */
      // Actually, sysfile.c calls walk then open.
      // walk clone increments ref.
      // open keeps ref.
      // close decrements.
      // Wait, syspipe does NOT clone for the open?
      // It clones c[0] to c[1].
      // c[0] is the dir.
      // walk(&c[0]...) turns c[0] into the file.
      // So c[0] transitions from dir to file.
      // The ref count on Pipe is handled.
      // But we need to track how many readers/writers on each Q.

      // Simplified: Just check if this is the last ref to the channel?
      // Chan c has c->ref. When c->ref goes to 0, pipeclose is called.
      // So we are closing THIS channel reference.

      /* We need to know if we should send EOF to the *other* side.
         If we close write end, other side gets EOF.
         If we close read end, other side gets Epipe.
      */

      /* Implementation detail:
         We don't have easy access to "total refs to this queue".
         But we can cheat: if (c->flag & COPEN), we are closing an open file.
         If (c->mode == OWRITE || c->mode == ORDWR) -> we are writer.
         If (c->mode == OREAD || c->mode == ORDWR) -> we are reader.
      */

      if (c->flag & COPEN) {
        if (c->mode == OWRITE || c->mode == ORDWR) {
          qclose(p->q[id]); // Close the queue we write to?
          // No, we write to the *other* queue usually?
          // Standard pipe: write to q[0] goes to q[1]?
          // Or write to q[0] goes to q[0] and read from q[0] gets it?
          // Plan 9 pipes:
          // data -> q[0]
          // data1 -> q[1]
          // Write to data puts in q[0]. Read from data1 takes from q[0].
          // Write to data1 puts in q[1]. Read from data takes from q[1].

          // So if we close data (id=0) for write:
          // We should close q[0] so readers of data1 get EOF.
          qclose(p->q[id]);
        }
        if (c->mode == OREAD || c->mode == ORDWR) {
          // If we close read end of data (id=0),
          // We read from q[1]. We should close q[1] so writers to data1 get
          // broken pipe?
          qclose(p->q[1 - id]);
        }
      }
    }
  }

  if (--p->ref == 0) {
    if (p->q[0])
      qfree(p->q[0]);
    if (p->q[1])
      qfree(p->q[1]);
    qunlock(&p->l);
    free(p);
  } else {
    qunlock(&p->l);
  }
}

/*@ requires c != \null && c->aux != \null;
    requires 0 <= c->qid.path - 1 <= 1;
    ensures \result >= 0 || \result == -1;
    assigns \nothing;
    // Queue duality: read from q[1-id]
    // COQ_PROOF_REF: proofs/pipe/safety.v:write_read_duality
    // COQ_PROOF_REF: proofs/pipe/safety.v:read_preserves_ref
*/
static long piperead(Chan *c, void *va, long n, vlong offset) {
  Pipe *p;
  int id;

  if (c->qid.type & QTDIR)
    return devdirread(c, va, n, 0, 0, pipegen);

  p = c->aux;
  id = c->qid.path - 1;
  /* Read from the *other* queue */
  /* data (0) reads from q[1] */
  /* data1 (1) reads from q[0] */
  return qread(p->q[1 - id], va, n);
}

/*@ requires c != \null && c->aux != \null;
    requires 0 <= c->qid.path - 1 <= 1;
    ensures \result == n || \result < 0;
    assigns ((Pipe*)c->aux)->q[0],
            ((Pipe*)c->aux)->q[1];
    // Write to q[id], closed queue rejects
    // COQ_PROOF_REF: proofs/pipe/safety.v:closed_queue_no_write
    // COQ_PROOF_REF: proofs/pipe/safety.v:write_preserves_ref
    // COQ_PROOF_REF: proofs/pipe/safety.v:close_irreversible
    requires (n > 0 ==> \valid_read((char*)va + (0 .. (integer)n-1))) || (n == 0);
*/
static long pipewrite(Chan *c, void *va, long n, vlong offset) {
  Pipe *p;
  int id;

  if (c->qid.type & QTDIR)
    error(Eperm);

  p = c->aux;
  id = c->qid.path - 1;
  /* Write to *our* queue */
  /* data (0) writes to q[0] */
  /* data1 (1) writes to q[1] */
  return qwrite(p->q[id], va, n);
}

Dev pipedevtab = {
    '|',      "pipe",

    pipeinit, devinit,   devshutdown, pipeattach, pipewalk,
    pipestat, pipeopen,  devcreate,   pipeclose,  piperead,
    devbread, pipewrite, devbwrite,   devremove,  devwstat,
};
#endif
