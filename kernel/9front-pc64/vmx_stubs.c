#include "u.h"
#include "dat.h"
#include "fns.h"

/*
 * VMX support is currently not linked into the active image. Keep the hooks in
 * a dedicated stub file so globals.c only carries shared utility state.
 * The real implementation lives in kernel/unused_drivers/devvmx.c.
 */
void vmxshutdown(void) {}

void vmxprocrestore(Proc *p) { (void)p; }
