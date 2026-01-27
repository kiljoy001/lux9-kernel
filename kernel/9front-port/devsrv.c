/*
 * devsrv.c - /srv service registry device
 *
 * Exposes a dynamic directory of service entries. Writing a file posts
 * a file descriptor (as decimal text) which is stored as a channel for
 * later lookup and mounting.
 */

#include "9p_router.h"
#include "dat.h"
#include "error.h"
#include "fns.h"
#include "u.h"

enum {
  Qdir = 0,
};

static void srvinit(void) { srv_init(); }

static Chan *srvattach(char *spec) { return devattach('s', spec); }

/*@ requires \valid(c);
    requires name == \null || \valid(name);
    requires dp == \null || \valid(dp);
    assigns \nothing;
*/
static int srvgen(Chan *c, char *name, Dirtab *, int, int s, Dir *dp) {
  Qid qid;
  char nbuf[64];

  if (s == DEVDOTDOT) {
    mkqid(&qid, Qdir, 0, QTDIR);
    devdir(c, qid, "#s", 0, eve, 0555, dp);
    return 1;
  }

  if (c->qid.path != Qdir)
    return -1;

  if (name != nil) {
    int idx = srv_index_of_for_proc(up, name);
    if (idx < 0)
      return -1;
    mkqid(&qid, idx + 1, 0, QTDIR);
    devdir(c, qid, name, 0, eve, DMDIR | 0777, dp);
    return 1;
  }

  if (srv_get_by_index_for_proc(up, s, nbuf, sizeof(nbuf)) < 0)
    return -1;

  mkqid(&qid, s + 1, 0, QTDIR);
  devdir(c, qid, nbuf, 0, eve, DMDIR | 0777, dp);
  return 1;
}

static Walkqid *srvwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, nil, 0, srvgen);
}

/*@
  @ requires c == \null || \valid(c);
  @ requires db == \null || \valid(db);
  @ assigns \nothing;
  @*/
static int srvstat(Chan *c, uchar *db, int n) {
  return devstat(c, db, n, nil, 0, srvgen);
}

static Chan *srvopen(Chan *c, int omode) {
  int i;
  Dir dir;

  for (i = 0;; i++) {
    switch (srvgen(c, nil, nil, 0, i, &dir)) {
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
  /* Allow writing to service dirs for posting */
  c->mode = openmode(omode);
  c->flag |= COPEN;
  return c;
}

static void srvclose(Chan *) {}

/*@ requires c != \null;
    requires va != \null;
    requires (n > 0 ==> \valid((char*)va + (0 .. (integer)n-1))) || (n == 0);
    assigns ((char*)va)[0 .. (integer)n-1] \if n > 0;
*/
static long srvread(Chan *c, void *va, long n, vlong offset) {
  if (c->qid.path == Qdir)
    return devdirread(c, va, n, nil, 0, srvgen);
  return 0;
}

/*@ requires c != \null;
    requires va != \null;
    requires (n > 0 ==> \valid_read((char*)va + (0 .. (integer)n-1))) || (n == 0);
    assigns \nothing;
*/
static long srvwrite(Chan *c, void *va, long n, vlong) {
  char buf[32];
  int fd;
  char *name;

  if (c->qid.path == Qdir)
    error(Eperm);

  if (n <= 0)
    return 0;
  if (n >= (long)sizeof(buf))
    n = sizeof(buf) - 1;
  memmove(buf, va, n);
  buf[n] = 0;
  fd = (int)strtoul(buf, 0, 0);

  /* Extract service name from channel path */
  name = "unknown";
  if (c->path && c->path->s)
    name = c->path->s;

  if (srv_post_fd(up, name, fd) < 0)
    error(Eio);
  return n;
}

static Chan *srvcreate(Chan *c, char *name, int omode, ulong perm) {
  if (c->qid.path != Qdir)
    error(Eperm);
  (void)perm;
  if (srv_create_entry(up, name) < 0)
    error(Eio);
  return devopen(c, omode, nil, 0, srvgen);
}

/*@
  @ requires c == \null || \valid(c);
  @ assigns \nothing;
  @*/
static void srvremove(Chan *c) {
  char *name;

  if (c->qid.path == Qdir)
    error(Eperm);

  /* Extract service name from channel path */
  name = "unknown";
  if (c->path && c->path->s)
    name = c->path->s;

  if (srv_remove_entry(up, name) < 0)
    error(Eperm);
}

Dev srvdevtab = {
    's',      "srv",

    devreset, srvinit,  devshutdown, srvattach, srvwalk,
    srvstat,  srvopen,  srvcreate,   srvclose,  srvread,
    devbread, srvwrite, devbwrite,   srvremove, devwstat,
};
