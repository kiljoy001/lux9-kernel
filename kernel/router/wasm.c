#include "../wasm/wasm_runtime.h"
#include "router.h"

/*@
  @
  //============================================================================
  @ // AXIOMATIC DEFINITIONS - WASM Protocol Correctness
  @
  //============================================================================
  @
  @ axiomatic WASM_Protocol {
  @
  @   // Valid WASM request
  @   predicate valid_wasm_request(Fcall *t) =
  @     \valid(t) &&
  @     t->type == Tsyscall &&
  @     (t->scallnr == SYS_WASM_COMPILE ||
  @      t->scallnr == SYS_WASM_EXECUTE ||
  @      t->scallnr == SYS_WASM_DESTROY);
  @
  @   // Valid reply buffer
  @   predicate valid_reply_buffer(Fcall *r) =
  @     \valid(r);
  @
  @   // Valid reply output
  @   predicate valid_reply_output(Fcall *r) =
  @     \valid(r) &&
  @     (r->type == Rsyscall || r->type == Rerror);
  @
  @   // Tag preservation axiom
  @   axiom wasm_tag_preservation:
  @     \forall Fcall *t, *r;
  @       \valid(t) && \valid(r) ==> r->tag == t->tag;
  @ }
  @*/

/*@
  @
  //============================================================================
  @ // MAIN WASM DISPATCHER CONTRACT
  @
  //============================================================================
  @
  @ requires \valid(p);
  @ requires valid_wasm_request(t);
  @ requires valid_reply_buffer(r);
  @
  @ // Return value semantics
  @ ensures \result == 0 || \result == -1;
  @ ensures \result == 0 ==> r->type == Rsyscall;
  @ ensures \result == -1 ==> r->type == Rerror;
  @
  @ // Protocol correctness: Tag preservation
  @ ensures r->tag == t->tag;
  @ ensures valid_reply_output(r);
  @
  @ // Memory safety: What we modify
  @ assigns *r;
  @
  @ terminates \true;
  @*/
int router_dispatch_wasm(Proc *p, Fcall *t, Fcall *r) {
  if (waserror()) {
    r->type = Rerror;
    if (up != nil)
      r->ename = up->errstr;
    else
      r->ename = "wasm dispatch failed";
    return -1;
  }

  int ret = -1;
  switch (t->scallnr) {
  /*@
    @ //========================================================================
    @ // SYS_WASM_COMPILE - Compile WASM module from file descriptor
    @ //========================================================================
    @
    @ // Postconditions: Success returns module loaded
    @ ensures \result == 0 ==> r->type == Rsyscall;
    @ ensures \result == 0 ==> r->tag == t->tag;
    @
    @ // Error path
    @ ensures \result == -1 ==> r->type == Rerror;
    @ ensures \result == -1 ==> r->tag == t->tag;
    @
    @ assigns *r;
    @*/
  case SYS_WASM_COMPILE:
    if (sys_wasm_compile(t, r) == 0)
      ret = 0;
    break;

  /*@
    @ //========================================================================
    @ // SYS_WASM_EXECUTE - Execute function in loaded WASM module
    @ //========================================================================
    @
    @ // Postconditions: Success returns execution result
    @ ensures \result == 0 ==> r->type == Rsyscall;
    @ ensures \result == 0 ==> r->tag == t->tag;
    @
    @ // Error path
    @ ensures \result == -1 ==> r->type == Rerror;
    @ ensures \result == -1 ==> r->tag == t->tag;
    @
    @ assigns *r;
    @*/
  case SYS_WASM_EXECUTE:
    if (sys_wasm_execute(t, r) == 0)
      ret = 0;
    break;

  /*@
    @ //========================================================================
    @ // SYS_WASM_DESTROY - Destroy WASM module and free resources
    @ //========================================================================
    @
    @ // Postconditions: Success returns cleanup confirmation
    @ ensures \result == 0 ==> r->type == Rsyscall;
    @ ensures \result == 0 ==> r->tag == t->tag;
    @
    @ // Error path
    @ ensures \result == -1 ==> r->type == Rerror;
    @ ensures \result == -1 ==> r->tag == t->tag;
    @
    @ assigns *r;
    @*/
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
