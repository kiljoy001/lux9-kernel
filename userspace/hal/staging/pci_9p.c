/*
 * PCI Family 9P Implementation - 9P Interface Driver
 *
 * Implements the 9P interface for the PCI device family.
 * Provides filesystem-style access to PCI devices for userspace driver servers.
 * Integrates with exchange pages and pebble system for security.
 */

#include "family/pci_9p.h"
#include "dat.h"
#include "family/family.h"
#include "family/pci_family_ops.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

/* External references to global PCI context */
extern struct PCIFamilyContext *global_pci_ctx;
extern struct FamilyExchangePage *global_pci_family;

enum {
  Qdir = 0,
  Qctl,
  Qbus,
  Qdevbase = 0x1000,
  Qdevconfig = 0x10000,
  Qdevctl = 0x20000,
};

/* Generator for PCI filesystem */
/*@
  @ requires \valid(c);
  @ requires \valid(dp);
  @ assigns *dp;
  @ ensures \result == 1 || \result == -1;
  @*/
static int pcigen(Chan *c, char *name, Dirtab *tab, int ntab, int pos,
                  Dir *dp) {
  Qid qid;
  int i, idx;
  struct PCIDeviceDescriptor *dev;
  struct PCIFamilyContext *ctx = global_pci_ctx;

  USED(c, tab, ntab);

  if (pos == 0) {
    /* "." */
    devdir(c, (Qid){Qdir, 0, QTDIR}, ".", 0, eve, 0555, dp);
    return 1;
  }
  pos--;

  /* Root directory */
  if (c->qid.path == Qdir) {
    if (pos == 0) {
      devdir(c, (Qid){Qctl, 0, 0}, "ctl", 0, eve, 0666, dp);
      return 1;
    }
    pos--;
    if (pos == 0) {
      devdir(c, (Qid){Qbus, 0, 0}, "bus", 0, eve, 0444, dp);
      return 1;
    }
    pos--;

    /* Iterate devices */
    if (ctx) {
      lock(&ctx->device_registry.device_registry_lock);
      idx = 0;
      for (i = 0; i < MAX_PCI_DEVICES; i++) {
        dev = ctx->device_registry.devices[i];
        if (dev && dev->address.domain_active) {
          if (idx == pos) {
            char buf[32];
            snprint(buf, sizeof(buf), "%04x:%02x:%02x.%d", dev->address.domain,
                    dev->address.bus, dev->address.device,
                    dev->address.function);

            qid.path = Qdevbase + i;
            qid.vers = 0;
            qid.type = QTDIR;
            unlock(&ctx->device_registry.device_registry_lock);
            devdir(c, qid, buf, 0, eve, 0555, dp);
            return 1;
          }
          idx++;
        }
      }
      unlock(&ctx->device_registry.device_registry_lock);
    }
    return -1;
  }

  /* Device directory */
  if (c->qid.path >= Qdevbase && c->qid.path < Qdevconfig) {
    int dev_idx = c->qid.path - Qdevbase;

    if (pos == 0) {
      qid.path = Qdevconfig + dev_idx;
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "config", 256, eve, 0666, dp);
      return 1;
    }
    pos--;
    if (pos == 0) {
      qid.path = Qdevctl + dev_idx;
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "ctl", 0, eve, 0666, dp);
      return 1;
    }
    return -1;
  }

  return -1;
}

/*@
  @ requires \valid(c);
  @ requires \valid(nc);
  @ requires nname > 0 ==> \valid(name + (0..nname-1));
  @ assigns *nc;
  @*/
Walkqid *pci_9p_walk(struct FamilyExchangePage *family, Chan *c, Chan *nc,
                     char **name, int nname) {
  USED(family);
  return devwalk(c, nc, name, nname, nil, 0, pcigen);
}

/*@
  @ requires \valid(c);
  @ requires n > 0 ==> \valid(dp + (0..n-1));
  @ assigns dp[0..n-1];
  @ ensures \result >= -1;
  @*/
int pci_9p_stat(struct FamilyExchangePage *family, Chan *c, uchar *dp, int n) {
  USED(family);
  return devstat(c, dp, n, nil, 0, pcigen);
}

/*@
  @ requires \valid(c);
  @ assigns *c;
  @*/
Chan *pci_9p_open(struct FamilyExchangePage *family, Chan *c, int omode) {
  USED(family);
  return devopen(c, omode, nil, 0, pcigen);
}

/*@
  @ requires \valid(family);
  @ requires \valid(c);
  @ assigns \nothing;
  @*/void pci_9p_close(struct FamilyExchangePage *family, Chan *c) {
  USED(family);
  USED(c);
}

/*@
  @ requires \valid(c);
  @ requires n > 0 ==> \valid((char *)buf + (0..n-1));
  @ assigns ((char *)buf)[0..n-1];
  @ ensures \result >= 0;
  @ ensures \result <= n;
  @*/
long pci_9p_read(struct FamilyExchangePage *family, Chan *c, void *buf, long n,
                 vlong off) {
  struct PCIFamilyContext *ctx = global_pci_ctx;
  struct PCIDeviceDescriptor *dev;
  int i, dev_idx;

  USED(family);

  if (c->qid.path == Qdir ||
      (c->qid.path >= Qdevbase && c->qid.path < Qdevconfig)) {
    return devdirread(c, buf, n, nil, 0, pcigen);
  }

  if (c->qid.path == Qbus) {
    /* List all PCI buses with devices */
    char *p = smalloc_driver(4096);
    int len = 0;
    int i;

    if (!p)
      error(Enomem);
    if (!ctx) {
      len = snprint(p, 4096, "no PCI context\n");
    } else {
      lock(&ctx->device_registry.device_registry_lock);
      for (i = 0; i < MAX_PCI_DEVICES && len < 4000; i++) {
        struct PCIDeviceDescriptor *dev = ctx->device_registry.devices[i];
        if (dev && dev->address.domain_active) {
          len += snprint(p + len, 4096 - len,
                         "%04x:%02x:%02x.%d %04x:%04x class %02x%02x\n",
                         dev->address.domain, dev->address.bus,
                         dev->address.device, dev->address.function,
                         dev->vendor_id, dev->device_id, dev->class_code,
                         dev->subclass_code);
        }
      }
      unlock(&ctx->device_registry.device_registry_lock);
      if (len == 0)
        len = snprint(p, 4096, "no devices\n");
    }
    n = readstr(off, buf, n, p);
    xfree_driver(p);
    return n;
  }

  if (c->qid.path == Qctl) {
    /* Return PCI subsystem info */
    char *p = smalloc_driver(512);
    int len = 0;

    if (!p)
      error(Enomem);
    if (!ctx) {
      len = snprint(p, 512, "pci: not initialized\n");
    } else {
      lock(&ctx->device_registry.device_registry_lock);
      int count = 0;
        /*@ loop invariant 0 <= i <= MAX_PCI_DEVICES;
    @ loop assigns i;
    @ loop variant MAX_PCI_DEVICES - i;
    @*/
  for (int i = 0; i < MAX_PCI_DEVICES; i++)
        if (ctx->device_registry.devices[i])
          count++;
      unlock(&ctx->device_registry.device_registry_lock);
      len = snprint(p, 512, "devices %d\nmax %d\n", count, MAX_PCI_DEVICES);
    }
    n = readstr(off, buf, n, p);
    xfree_driver(p);
    return n;
  }

  if (c->qid.path >= Qdevconfig && c->qid.path < Qdevctl) {
    /* Config read */
    dev_idx = c->qid.path - Qdevconfig;
    if (!ctx)
      error(Enonexist);

    lock(&ctx->device_registry.device_registry_lock);
    if (dev_idx < 0 || dev_idx >= MAX_PCI_DEVICES) {
      unlock(&ctx->device_registry.device_registry_lock);
      error(Enonexist);
    }
    dev = ctx->device_registry.devices[dev_idx];
    if (!dev || !dev->address.domain_active) {
      unlock(&ctx->device_registry.device_registry_lock);
      error(Enonexist);
    }

    /* Read config space */
    if (off >= 256) {
      unlock(&ctx->device_registry.device_registry_lock);
      return 0;
    }
    if (off + n > 256)
      n = 256 - off;

    /* Read byte by byte for simplicity */
    uint8_t *p = buf;
    for (i = 0; i < n; i++) {
      pci_config_read8(dev->address.bus, dev->address.device,
                       dev->address.function, off + i, &p[i]);
    }

    unlock(&ctx->device_registry.device_registry_lock);
    return n;
  }

  return 0;
}

/*@
  @ assigns \nothing;
  @ ensures \false;
  @*/
long pci_9p_write(struct FamilyExchangePage *family, Chan *c, void *buf, long n,
                  vlong off) {
  USED(family);
  USED(c);
  USED(buf);
  USED(n);
  USED(off);
  error(Eperm);
  return 0;
}
