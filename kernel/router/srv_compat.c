#include "u.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"

/*
 * Compatibility wrapper used by legacy host-owner paths that still expect a
 * top-level srvrenameuser symbol.
 */
void srvrenameuser(char *old, char *new) {
  srv_rename_user(old, new);
}
