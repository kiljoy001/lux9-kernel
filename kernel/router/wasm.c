#include "../wasm/wasm_runtime.h"
#include "router.h"

int router_dispatch_wasm(Proc *p, Fcall *t, Fcall *r) {
  /*@
    @ requires \valid(p);
    @ requires \valid(t);
    @ requires \valid(r);
    @ ensures \result == 0 || \result == -1;
    @*/
  print("router_wasm: dispatching scallnr=%d\n", t->scallnr);

  if (waserror()) {
    r->type = Rerror;
    snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
    return -1;
  }

  int ret = -1;
  switch (t->scallnr) {
  case SYS_WASM_COMPILE:
    if (sys_wasm_compile(t, r) == 0)
      ret = 0;
    break;

  case SYS_WASM_EXECUTE:
    if (sys_wasm_execute(t, r) == 0)
      ret = 0;
    break;

  case SYS_WASM_DESTROY:
    if (sys_wasm_destroy(t, r) == 0)
      ret = 0;
    break;

  default:
    r->type = Rerror;
    r->ename = "unknown wasm syscall";
    ret = -1;
  }

  poperror();
  return ret;
}
