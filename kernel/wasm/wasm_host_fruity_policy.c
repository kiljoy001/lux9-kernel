/* wasm_host_fruity_policy.c - Default policy implementation */

#include "wasm_host_fruity_policy.h"
#include "../include/dat.h"
#include "../include/u.h"
#include "../include/portlib.h"
#include "wasm_runtime.h"

extern char isfrog[256];

/*@
  @ requires caller == \null || \valid_read(caller);
  @ assigns \result \from caller->wasm.permissions, op;
  @ ensures \result == 0 || \result == 1;
  @ ensures \result == 1 ==> (caller != \null &&
  @          (caller->wasm.permissions & PERM_WASM_FRUITY) != 0);
  @*/
static int fruity_host_default_allow(Proc *caller, fruity_host_op_t op) {
  (void)op;
  if (!caller)
    return 0;
  return (caller->wasm.permissions & PERM_WASM_FRUITY) != 0;
}

/*@
  @ requires name == \null || valid_string((char *)name);
  @ assigns \result \from name[0 .. ACSL_MAXSTR-1];
  @ ensures \result == 0 || \result == 1;
  @ ensures \result == 1 ==> fruity_host_name_ok((char *)name);
  @*/
static int fruity_host_default_validate_name(const char *name) {
  if (!name || name[0] == '\0')
    return 0;

  /*@
    @ loop assigns p;
    @ loop invariant 0 <= p - name <= ACSL_MAXSTR;
    @ loop variant ACSL_MAXSTR - (p - name);
    @*/
  for (const char *p = name; *p != '\0';) {
    uchar c = *(uchar *)p;
    if (c >= Runeself) {
      Rune r;
      int n = chartorune(&r, p);
      if (n <= 0)
        return 0;
      p += n;
      continue;
    }
    if (isfrog[c])
      return 0;
    p++;
  }
  return 1;
}

static const fruity_host_policy_t fruity_host_default_policy = {
    .allow = fruity_host_default_allow,
    .validate_name = fruity_host_default_validate_name,
};

#ifdef __FRAMAC__
const fruity_host_policy_t *fruity_host_policy =
    &fruity_host_default_policy;
#else
static const fruity_host_policy_t *fruity_host_policy =
    &fruity_host_default_policy;
#endif

/*@
  @ terminates \true;
  @ exits \false;
  @ assigns \result \from fruity_host_policy;
  @ ensures \result != \null;
  @ ensures \result->allow != \null;
  @ ensures \result->validate_name != \null;
  @*/
const fruity_host_policy_t *fruity_host_get_policy(void) {
  return fruity_host_policy;
}

/*@
  @ requires policy == \null || \valid_read(policy);
  @ terminates \true;
  @ exits \false;
  @ assigns fruity_host_policy;
  @ ensures fruity_host_policy == policy ||
  @         fruity_host_policy == \old(fruity_host_policy);
  @ ensures (policy == \null || policy->allow == \null ||
  @          policy->validate_name == \null) ==>
  @          fruity_host_policy == \old(fruity_host_policy);
  @ ensures (policy != \null && policy->allow != \null &&
  @          policy->validate_name != \null) ==>
  @          fruity_host_policy == policy;
  @*/
void fruity_host_set_policy(const fruity_host_policy_t *policy) {
  if (policy && policy->allow && policy->validate_name) {
    fruity_host_policy = policy;
  } else {
    fruity_host_policy = &fruity_host_default_policy;
  }
}
