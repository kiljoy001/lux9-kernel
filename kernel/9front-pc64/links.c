#include "u.h"
#include "dat.h"
#include "fns.h"

/*
 * In this tree, boot files are populated from the initrd at runtime rather than
 * by a generated mkrootc payload. Keep the 9front bootlinks entry point so the
 * pc64 links() path matches the normal architecture split without relying on
 * globals.c.
 */
void bootlinks(void) {}

extern void devclrlink(void);

void links(void) {
  bootlinks();
  devclrlink();
}
