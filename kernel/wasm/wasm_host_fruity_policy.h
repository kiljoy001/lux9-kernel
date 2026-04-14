/* wasm_host_fruity_policy.h - Policy ADT for Fruity host bindings
 *
 * Separates access policy from the Fruity IR mechanism.
 */

#ifndef WASM_HOST_FRUITY_POLICY_H
#define WASM_HOST_FRUITY_POLICY_H

#include "../include/types_fwd.h"
#include "../include/acsl_bounds.h"

typedef enum {
  FRUITY_HOST_OP_MODULE_CREATE = 0,
  FRUITY_HOST_OP_ADD_FUNCTION,
  FRUITY_HOST_OP_ADD_BLOCK,
  FRUITY_HOST_OP_ADD_INSTR,
  FRUITY_HOST_OP_COMPILE,
} fruity_host_op_t;

typedef struct fruity_host_policy {
  int (*allow)(Proc *caller, fruity_host_op_t op);
  int (*validate_name)(const char *name);
} fruity_host_policy_t;

#ifdef __FRAMAC__
extern const fruity_host_policy_t *fruity_host_policy;
#endif

/*@ axiomatic FruityHostPolicy {
  @ predicate fruity_host_policy_allows(fruity_host_op_t op);
  @} */

/*@
  @ predicate fruity_host_name_ok(char *s) = p9_name_ok_slash(s, 0);
  @*/

/*@
  @ terminates \true;
  @ exits \false;
  @ assigns \result \from fruity_host_policy;
  @ ensures \result == \null || \valid(\result);
  @*/
const fruity_host_policy_t *fruity_host_get_policy(void);
/*@
  @ requires policy == \null || \valid(policy);
  @ assigns fruity_host_policy;
  @*/
void fruity_host_set_policy(const fruity_host_policy_t *policy);

#endif /* WASM_HOST_FRUITY_POLICY_H */
