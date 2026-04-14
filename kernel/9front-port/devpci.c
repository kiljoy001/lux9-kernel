/*
 * /dev/pci - PCI device enumeration for SIP drivers
 *
 * Provides 9P-based access to PCI configuration space for userspace
 * device drivers to enumerate and configure PCI devices.
 *
 * The sharp-device entry point is #J rather than #P because #P is already
 * owned by the architecture device on pc64.
 *
 * Filesystem:
 *   /dev/pci/
 *   ├── ctl           - Control and status
 *   ├── bus           - List of all PCI devices
 *   ├── 0000:00:00.0/ - Device directory (bus:dev:func)
 *   │   ├── config    - Raw config space (256 bytes)
 *   │   ├── ctl       - Device control
 *   │   └── raw       - Alias for config
 *   ├── 0000:00:1f.2/ - Another device (e.g., AHCI)
 *   └── ...
 *
 * Usage:
 *   // List all devices
 *   cat /dev/pci/bus
 *
 *   // Read AHCI config
 *   cat /dev/pci/0000:00:1f.2/config
 *
 * Architecture Integration:
 *   - Uses existing pcimatch() for enumeration
 *   - Uses pcicfgr*() for config space access
 *   - Exports BARs, IRQ, vendor/device IDs
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <error.h>
#include <pci.h>

enum {
  Qdir = 0,
  Qctl,
  Qbus,
  Qdevbase = 0x1000,    /* Device directories start here */
  Qdevconfig = 0x10000, /* Config files */
  Qdevctl = 0x20000,    /* Device ctl files */
};

/*
 * PCI device list (cached from enumeration)
 */
typedef struct PCIDev PCIDev;
struct PCIDev {
  Pcidev *pci;   /* Pointer to PCI device */
  int busno;     /* Bus number */
  int devno;     /* Device number */
  int funcno;    /* Function number */
  char name[32]; /* "0000:00:1f.2" */
  PCIDev *next;
};

static struct {
  Lock lock;
  PCIDev *devlist;
  int ndevs;
} pcistate;

/*
 * Capability checking
 */
enum {
  CapPCI = 1 << 6, /* PCI device access */
};

static void checkcap(ulong required) {
  if (up == nil)
    return; /* kernel processes have full access */

  if ((up->capabilities & required) != required)
    error("insufficient capabilities for PCI access");
}

/*
 * Enumerate all PCI devices
 */
static void pci_enumerate(void) {
  Pcidev *p;
  PCIDev *pd, **tail;

  lock(&pcistate.lock);

  /* Free existing list */
  while (pcistate.devlist != nil) {
    pd = pcistate.devlist;
    pcistate.devlist = pd->next;
    free(pd);
  }
  pcistate.ndevs = 0;

  /* Enumerate all PCI devices */
  tail = &pcistate.devlist;
  p = nil;
  while ((p = pcimatch(p, 0, 0)) != nil) {
    pd = smalloc(sizeof(PCIDev));
    pd->pci = p;
    pd->busno = BUSBNO(p->tbdf);
    pd->devno = BUSDNO(p->tbdf);
    pd->funcno = BUSFNO(p->tbdf);
    snprint(pd->name, sizeof(pd->name), "%04x:%02x:%02x.%d", 0, pd->busno,
            pd->devno, pd->funcno);
    pd->next = nil;

    *tail = pd;
    tail = &pd->next;
    pcistate.ndevs++;
  }

  unlock(&pcistate.lock);
}

/*
 * Find PCI device by name
 */
static PCIDev *pci_find(char *name) {
  PCIDev *pd;

  lock(&pcistate.lock);
  for (pd = pcistate.devlist; pd != nil; pd = pd->next) {
    if (strcmp(pd->name, name) == 0) {
      unlock(&pcistate.lock);
      return pd;
    }
  }
  unlock(&pcistate.lock);
  return nil;
}

/*
 * Directory generation
 */
static int pcigen(Chan *c, char *name, Dirtab *tab, int ntab, int pos,
                  Dir *dp) {
  Qid qid;
  PCIDev *pd;
  int n;

  USED(tab, ntab);

  if (name != nil)
    print("pcigen: qpath=%#llux name='%s'\n", c->qid.path, name);

  if (name != nil && strcmp(name, ".") == 0) {
    devdir(c, (Qid){Qdir, 0, QTDIR}, ".", 0, eve, 0555, dp);
    return 1;
  }

  if (name == nil && pos == 0) {
    /* "." */
    devdir(c, (Qid){Qdir, 0, QTDIR}, ".", 0, eve, 0555, dp);
    return 1;
  }
  if (name == nil)
    pos--;

  /* Root directory */
  if ((c->qid.path & 0xFFFF0000) == 0) {
    if (name != nil) {
      if (strcmp(name, "ctl") == 0) {
        devdir(c, (Qid){Qctl, 0, 0}, "ctl", 0, eve, 0444, dp);
        return 1;
      }
      if (strcmp(name, "bus") == 0) {
        devdir(c, (Qid){Qbus, 0, 0}, "bus", 0, eve, 0444, dp);
        return 1;
      }

      n = 0;
      lock(&pcistate.lock);
      for (pd = pcistate.devlist; pd != nil; pd = pd->next, n++) {
        if (strcmp(pd->name, name) == 0) {
          qid.path = Qdevbase + n;
          qid.vers = 0;
          qid.type = QTDIR;
          unlock(&pcistate.lock);
          devdir(c, qid, pd->name, 0, eve, 0555, dp);
          return 1;
        }
      }
      unlock(&pcistate.lock);
      return -1;
    }

    if (pos == 0) {
      devdir(c, (Qid){Qctl, 0, 0}, "ctl", 0, eve, 0444, dp);
      return 1;
    }
    pos--;

    if (pos == 0) {
      devdir(c, (Qid){Qbus, 0, 0}, "bus", 0, eve, 0444, dp);
      return 1;
    }
    pos--;

    /* Device directories */
    n = 0;
    lock(&pcistate.lock);
    for (pd = pcistate.devlist; pd != nil; pd = pd->next) {
      if (n == pos) {
        qid.path = Qdevbase + n;
        qid.vers = 0;
        qid.type = QTDIR;
        unlock(&pcistate.lock);
        devdir(c, qid, pd->name, 0, eve, 0555, dp);
        return 1;
      }
      n++;
    }
    unlock(&pcistate.lock);

    return -1;
  }

  /* Device subdirectory */
  if (c->qid.path >= Qdevbase && c->qid.path < Qdevbase + 0x1000) {
    if (name != nil) {
      if (strcmp(name, "config") == 0 || strcmp(name, "raw") == 0) {
        qid.path = Qdevconfig + (c->qid.path - Qdevbase);
        qid.vers = 0;
        qid.type = 0;
        devdir(c, qid, strcmp(name, "config") == 0 ? "config" : "raw", 256,
               eve, 0444, dp);
        return 1;
      }
      if (strcmp(name, "ctl") == 0) {
        qid.path = Qdevctl + (c->qid.path - Qdevbase);
        qid.vers = 0;
        qid.type = 0;
        devdir(c, qid, "ctl", 0, eve, 0666, dp);
        return 1;
      }
      return -1;
    }

    if (pos == 0) {
      qid.path = Qdevconfig + (c->qid.path - Qdevbase);
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "config", 256, eve, 0444, dp);
      return 1;
    }
    pos--;

    if (pos == 0) {
      qid.path = Qdevconfig + (c->qid.path - Qdevbase);
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "raw", 256, eve, 0444, dp);
      return 1;
    }
    pos--;

    if (pos == 0) {
      qid.path = Qdevctl + (c->qid.path - Qdevbase);
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "ctl", 0, eve, 0666, dp);
      return 1;
    }

    return -1;
  }

  return -1;
}

static void devpcireset(void) { memset(&pcistate, 0, sizeof(pcistate)); }

static Chan *pciattach(char *spec) {
  /* Enumerate PCI devices on first attach */
  if (pcistate.ndevs == 0)
    pci_enumerate();

  return devattach('J', spec);
}

static Walkqid *pciwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, nil, 0, pcigen);
}

static int pcistat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, nil, 0, pcigen);
}

static Chan *pciopen(Chan *c, int omode) {
  print("pciopen: qpath=%#llux type=%d omode=%d\n", c->qid.path, c->qid.type,
        omode);
  checkcap(CapPCI);

  c = devopen(c, omode, nil, 0, pcigen);
  c->offset = 0;
  return c;
}

static void pciclose(Chan *c) { USED(c); }

/*@ requires c != \null;
    requires va != \null;
    requires (n > 0 ==> \valid((char*)va + (0 .. (integer)n-1))) || (n == 0);
    assigns ((char*)va)[0 .. (integer)n-1] \if n > 0;
*/
static long pciread(Chan *c, void *va, long n, vlong off) {
  char buf[4096];
  char *p, *e;
  int len, devno;
  PCIDev *pd;
  uchar *config;
  int i;

  /* Universal CBS: Check PCI capability for device access */
  if ((c->qid.path >= Qdevconfig) && !has_capability(up, PEBBLE_CAP_PCI))
    error(PEBBLE_E_PERM);

  switch (c->qid.path) {
  case Qdir:
    return devdirread(c, va, n, nil, 0, pcigen);

  case Qctl:
    /* Return PCI status */
    len = snprint(buf, sizeof(buf), "devices: %d\n", pcistate.ndevs);

    if (off >= len)
      return 0;
    if (off + n > len)
      n = len - off;
    memmove(va, buf + off, n);
    return n;

  case Qbus:
    /* List all PCI devices */
    p = buf;
    e = buf + sizeof(buf);
    lock(&pcistate.lock);
    for (pd = pcistate.devlist; pd != nil; pd = pd->next) {
      p = seprint(p, e,
                  "%s vendor=0x%04x device=0x%04x class=%02x.%02x.%02x "
                  "irq=%d\n",
                  pd->name, pd->pci->vid, pd->pci->did, pd->pci->ccrb,
                  pd->pci->ccru, pd->pci->ccrp, pd->pci->intl);
      if (p >= e)
        break;

      /* Add BAR information */
      for (i = 0; i < nelem(pd->pci->mem); i++) {
        if (pd->pci->mem[i].size > 0) {
          p = seprint(p, e, "  bar%d: addr=0x%llux size=0x%llux\n", i,
                      pd->pci->mem[i].bar, pd->pci->mem[i].size);
          if (p >= e)
            break;
        }
      }

      if (p >= e)
        break;
    }
    unlock(&pcistate.lock);
    len = (int)(p - buf);

    if (off >= len)
      return 0;
    if (off + n > len)
      n = len - off;
    memmove(va, buf + off, n);
    return n;

  default:
    /* Config space read */
    if (c->qid.path >= Qdevconfig && c->qid.path < Qdevconfig + 0x1000) {
      devno = c->qid.path - Qdevconfig;

      /* Find device */
      i = 0;
      lock(&pcistate.lock);
      for (pd = pcistate.devlist; pd != nil; pd = pd->next) {
        if (i == devno) {
          unlock(&pcistate.lock);

          /* Read config space */
          config = smalloc(256);
          for (i = 0; i < 256; i++)
            config[i] = pcicfgr8(pd->pci, i);

          if (off >= 256) {
            free(config);
            return 0;
          }
          if (off + n > 256)
            n = 256 - off;
          memmove(va, config + off, n);
          free(config);
          return n;
        }
        i++;
      }
      unlock(&pcistate.lock);
      error("device not found");
    }

    /* Device ctl read */
    if (c->qid.path >= Qdevctl && c->qid.path < Qdevctl + 0x1000) {
      devno = c->qid.path - Qdevctl;

      i = 0;
      lock(&pcistate.lock);
      for (pd = pcistate.devlist; pd != nil; pd = pd->next) {
        if (i == devno) {
          unlock(&pcistate.lock);

          /* Return device info */
          len = 0;
          len += snprint(buf + len, sizeof(buf) - len, "vendor: 0x%04x\n",
                         pd->pci->vid);
          len += snprint(buf + len, sizeof(buf) - len, "device: 0x%04x\n",
                         pd->pci->did);
          len +=
              snprint(buf + len, sizeof(buf) - len, "class: %02x.%02x.%02x\n",
                      pd->pci->ccrb, pd->pci->ccru, pd->pci->ccrp);
          len +=
              snprint(buf + len, sizeof(buf) - len, "irq: %d\n", pd->pci->intl);
          len += snprint(buf + len, sizeof(buf) - len, "tbdf: 0x%08x\n",
                         pd->pci->tbdf);

          for (i = 0; i < nelem(pd->pci->mem); i++) {
            if (pd->pci->mem[i].size > 0) {
              len += snprint(buf + len, sizeof(buf) - len,
                             "bar%d: 0x%llux %llud\n", i, pd->pci->mem[i].bar,
                             pd->pci->mem[i].size);
            }
          }

          if (off >= len)
            return 0;
          if (off + n > len)
            n = len - off;
          memmove(va, buf + off, n);
          return n;
        }
        i++;
      }
      unlock(&pcistate.lock);
      error("device not found");
    }

    error(Egreg);
    return 0;
  }
}

/*@ requires c != \null;
    requires va != \null;
    requires (n > 0 ==> \valid_read((char*)va + (0 .. (integer)n-1))) || (n ==
   0); assigns \nothing;
*/
static long pciwrite(Chan *c, void *va, long n, vlong off) {
  USED(va, off);

  /* Universal CBS: Check PCI capability for write access */
  if ((c->qid.path >= Qdevconfig) && !has_capability(up, PEBBLE_CAP_PCI))
    error(PEBBLE_E_PERM);

  switch (c->qid.path) {
  case Qdir:
  case Qbus:
    error(Eperm);
    return 0;

  case Qctl:
    /* Could add rescan command here */
    error(Eperm);
    return 0;

  default:
    error(Eperm);
    return 0;
  }
}

Dev pcidevtab = {
    'J',         "pci",

    devpcireset,   devinit,        devshutdown, pciattach, pciwalk,
    pcistat,       pciopen,        devcreate,   pciclose,  pciread,
    pciwrite,      devbread,       devbwrite,   devremove, devwstat,
};
