#include "u.h"
#include "dat.h"
#include "fns.h"

/*
 * Minimal compatibility surface for generated syscall formatting and old DTrace
 * hooks. These remain intentionally inert in the current image.
 */
int nosyscall(Sargs *args) {
  (void)args;
  return -1;
}

char *sysctab[] = {nil};

void sysexit(Sargs *args, uintptr *ret) {
  (void)args;
  (void)ret;
}

void dtracytick(Ureg *u) { (void)u; }
