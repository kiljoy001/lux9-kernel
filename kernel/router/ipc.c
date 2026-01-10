#include "router.h"

int router_dispatch_ipc(Proc *p, Fcall *t, Fcall *r) {
  /* Stub for IPC syscalls */
  r->type = Rerror;
  r->ename = "IPC syscall not implemented in router split yet";
  return -1;
}
