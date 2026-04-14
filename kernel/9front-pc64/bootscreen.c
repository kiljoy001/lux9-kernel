#include "u.h"
#include "dat.h"
#include "fns.h"

/*
 * The real display bring-up for this tree happens in fbconsoleinit().
 * Keep bootscreeninit() as a separate early-boot hook so pc64 startup no
 * longer depends on globals.c for this placeholder.
 */
void bootscreeninit(void) {}
