#ifndef CIL_SECURITY_H
#define CIL_SECURITY_H

#include "../clr_capability.h"
#include "cil_domtree.h"

/*
 * analyze_cfg_security
 *
 * Performs a security scan of the control flow graph.
 * Identifies basic blocks that contain security-sensitive operations
 * (e.g. calls, object creation, static field access) and marks them
 * as requiring runtime capability validation.
 *
 * Populates:
 *  - block->required_permissions
 *  - block->requires_validation
 *  - block->is_sensitive_op
 */
void analyze_cfg_security(dt_cfg_t *cfg,
                          clr_monotonic_capability_t *method_cap);

#endif /* CIL_SECURITY_H */
