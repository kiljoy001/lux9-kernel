#include "router.h"

int router_dispatch_proc(Proc *p, Fcall *t, Fcall *r) {
  /* Stub for process control syscalls */
  r->type = Rerror;
  r->ename = "Process syscall not implemented in router split yet";
  return -1;
}
