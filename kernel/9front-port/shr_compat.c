#include "u.h"
#include "dat.h"
#include "fns.h"

/*
 * The shared-resource device is not active in the current image. Keep the
 * rename hook separate from globals.c so host-owner compatibility can still
 * call into a stable symbol.
 */
void shrrenameuser(char *old, char *new) {
  (void)old;
  (void)new;
}
