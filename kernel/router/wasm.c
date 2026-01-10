#include "router.h"
#include "../wasm/wasm_runtime.h"

int router_dispatch_wasm(Proc *p, Fcall *t, Fcall *r) {
  print("router_wasm: dispatching scallnr=%d\n", t->scallnr);

  switch (t->scallnr) {
  case SYS_WASM_COMPILE:
    if (sys_wasm_compile(t, r) != 0) return -1;
    return 0;

  case SYS_WASM_EXECUTE:
    if (sys_wasm_execute(t, r) != 0) return -1;
    return 0;

  case SYS_WASM_DESTROY:
    if (sys_wasm_destroy(t, r) != 0) return -1;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "unknown wasm syscall";
    return -1;
  }
}
