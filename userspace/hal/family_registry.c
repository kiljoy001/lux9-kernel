/*
 * Family Registry - HAL Implementation
 *
 * Manages device family registrations in userspace.
 */

#include "family.h"
#include <libc.h>
#include <u.h>

/* Global registry state */
struct {
  FamilyExchangePage *families[FAMILY_MAX];
  int count;
  // Lock lock; // TODO: Userspace locking if needed
} registry;

void family_init_registry(void) {
  memset(&registry, 0, sizeof(registry));
  print("HAL: Family Registry initialized\n");
}

int family_register(int type, FamilyOps *ops, char *name) {
  FamilyExchangePage *fam;

  if (type >= FAMILY_MAX || !ops || !name)
    return FAMILY_EINVAL;

  if (registry.families[type] != nil)
    return FAMILY_ECONFLICT;

  fam = family_alloc_zero(sizeof(FamilyExchangePage));
  if (fam == nil)
    return FAMILY_ENOMEM;

  fam->family_type = type;
  fam->family_version = 1;
  strncpy(fam->family_name, name, sizeof(fam->family_name) - 1);
  fam->family_name[sizeof(fam->family_name) - 1] = 0;
  fam->ops = ops;

  // Call init if present
  if (ops->init) {
      if (ops->init(fam) < 0) {
      family_free(fam);
      return FAMILY_EINVAL;
    }
  }

  registry.families[type] = fam;
  registry.count++;

  print("HAL: Registered family %s (type %d)\n", fam->family_name, type);
  return FAMILY_OK;
}

int family_unregister(int type) {
  FamilyExchangePage *fam;

  if (type >= FAMILY_MAX)
    return FAMILY_EINVAL;

  fam = registry.families[type];
  if (fam == nil)
    return FAMILY_ENOTFOUND;

  if (fam->ops && fam->ops->shutdown)
    fam->ops->shutdown(fam);

  family_free(fam);
  registry.families[type] = nil;
  registry.count--;

  print("HAL: Unregistered family type %d\n", type);
  return FAMILY_OK;
}

FamilyExchangePage *family_lookup(int type) {
  if (type >= FAMILY_MAX)
    return nil;
  return registry.families[type];
}

int family_registry_count(void) { return registry.count; }

int family_registry_snapshot(char *buf, int nbuf) {
  int n = 0;
  int i;
  FamilyExchangePage *fam;

  if (buf == nil || nbuf <= 0)
    return -1;

  for (i = 0; i < FAMILY_MAX; i++) {
    fam = registry.families[i];
    if (fam == nil)
      continue;
    n += snprint(buf + n, nbuf - n, "%s type=%d caps=%#ux state=%d\n",
                 fam->family_name, fam->family_type, fam->capabilities_mask,
                 fam->state);
    if (n >= nbuf)
      return nbuf;
  }
  return n;
}
