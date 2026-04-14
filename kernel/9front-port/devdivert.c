#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "lib.h"
#include "mem.h"
#include "u.h"
#include <error.h>

/*
 * devdivert.c - Redirects kernel device calls to userspace HAL services via
 * /srv. Part of the "Thin-Lux9" TCB reduction strategy.
 */

typedef struct DivertEntry DivertEntry;
struct DivertEntry {
  char name[32];
  char srvname[64];
  Chan *hal_chan;
  int active;
};

static DivertEntry diverts[16];
static int ndiverts = 0;
static Lock divert_lock;

/* Register a kernel device name to a userspace HAL service in /srv */
int devdivert_map(char *name, char *srvname) {
  DivertEntry *d;

  lock(&divert_lock);
  if (ndiverts >= 16) {
    unlock(&divert_lock);
    return -1;
  }

  d = &diverts[ndiverts++];
  strncpy(d->name, name, sizeof(d->name));
  strncpy(d->srvname, srvname, sizeof(d->srvname));
  d->hal_chan = nil;
  d->active = 1;
  unlock(&divert_lock);

  return 0;
}

static Chan *divert_get_chan(DivertEntry *d) {
  USED(d->hal_chan);
  return srv_clone_chan(d->srvname);
}

static Chan *divert_attach(char *spec) {
  DivertEntry *d = &diverts[0];
  Chan *hal;
  Chan *root;
  char aname[] = "";

  USED(spec);
  if (!d->active)
    error("no diverted service configured");

  hal = divert_get_chan(d);
  if (hal == nil)
    error("HAL service not available in /srv");

  if (waserror()) {
    cclose(hal);
    nexterror();
  }
  root = mntattach(hal, nil, aname, 0);
  cclose(hal);
  poperror();

  if (root == nil)
    error(Eio);
  return root;
}

static Walkqid *divert_walk(Chan *c, Chan *nc, char **name, int nname) {
  DivertEntry *d = &diverts[0];
  Chan *hal;

  hal = divert_get_chan(d);
  if (hal == nil)
    error("HAL service not available in /srv");

  /* Map walk to HAL channel */
  return devwalk(c, nc, name, nname, nil, 0, devgen);
}

static int divert_stat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, nil, 0, devgen);
}

static Chan *divert_open(Chan *c, int omode) {
  return devopen(c, omode, nil, 0, devgen);
}

static long divert_read(Chan *c, void *a, long n, vlong offset) {
  DivertEntry *d = &diverts[0];
  Chan *hal = divert_get_chan(d);

  if (hal == nil)
    error(Eio);

  /* Direct 9P forward to HAL channel */
  return devtab[devno(hal->type, 0)]->read(hal, a, n, offset);
}

static long divert_write(Chan *c, void *a, long n, vlong offset) {
  DivertEntry *d = &diverts[0];
  Chan *hal = divert_get_chan(d);

  if (hal == nil)
    error(Eio);

  return devtab[devno(hal->type, 0)]->write(hal, a, n, offset);
}

void devdivert_init(void) { /* Lock is zero-initialized by kernel loader */ }

Dev divertdevtab = {
    .dc = 'D',
    .name = "divert",
    .reset = nil,
    .init = devdivert_init,
    .shutdown = nil,
    .attach = divert_attach,
    .walk = divert_walk,
    .stat = divert_stat,
    .open = divert_open,
    .create = devcreate,
    .close = nil,
    .read = divert_read,
    .write = divert_write,
    .remove = nil,
    .wstat = nil,
    .power = nil,
    .config = nil,
};
