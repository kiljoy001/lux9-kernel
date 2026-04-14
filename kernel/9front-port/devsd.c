/* devsd.c - SD storage device Diverter Client for Lux9
 *
 * This file replaces the monolithic SD driver with a lightweight proxy
 * that diverts all 9P operations to the userspace HAL service (hal_pci).
 * Part of the "Thin-Lux9" TCB reduction strategy.
 */

#include "dat.h"
#include "fns.h"
#include "io.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

extern int devdivert_map(char *name, char *srvname);
extern Dev divertdevtab;

static void sdreset(void) {
  /* Register this device as diverted to HAL */
  devdivert_map("sd", "hal_pci");
  print("sd: storage driver evicted to userspace HAL (hal_pci)\n");
}

static void sdinit(void) { /* Initialization handled by reset/HAL */ }

static Chan *sdattach(char *spec) { return divertdevtab.attach(spec); }

static Walkqid *sdwalk(Chan *c, Chan *nc, char **name, int nname) {
  return divertdevtab.walk(c, nc, name, nname);
}

static int sdstat(Chan *c, uchar *dp, int n) {
  return divertdevtab.stat(c, dp, n);
}

static Chan *sdopen(Chan *c, int omode) { return divertdevtab.open(c, omode); }

static void sdclose(Chan *c) { divertdevtab.close(c); }

static long sdread(Chan *c, void *a, long n, vlong offset) {
  if (c->qid.type & QTDIR)
    return devdirread(c, a, n, nil, 0, devgen);
  return divertdevtab.read(c, a, n, offset);
}

static long sdwrite(Chan *c, void *a, long n, vlong offset) {
  return divertdevtab.write(c, a, n, offset);
}

static Block *sdbread(Chan *c, long n, vlong offset) {
  if (divertdevtab.bread != nil)
    return divertdevtab.bread(c, n, offset);
  return nil;
}

static long sdbwrite(Chan *c, Block *bp, vlong offset) {
  if (divertdevtab.bwrite != nil)
    return divertdevtab.bwrite(c, bp, offset);
  return 0;
}

Dev sdisabidevtab = {
    .dc = 'S',
    .name = "sd",
    .reset = sdreset,
    .init = sdinit,
    .shutdown = nil,
    .attach = sdattach,
    .walk = sdwalk,
    .stat = sdstat,
    .open = sdopen,
    .create = nil,
    .close = sdclose,
    .read = sdread,
    .bread = sdbread,
    .write = sdwrite,
    .bwrite = sdbwrite,
    .remove = nil,
    .wstat = nil,
    .power = nil,
    .config = nil,
};
