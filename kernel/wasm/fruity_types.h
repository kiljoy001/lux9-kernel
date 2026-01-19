/* fruity_types.h - Forward declarations and basic types for Fruity IR
 *
 * Minimal type definitions to avoid circular dependencies.
 * Uses Plan 9 native types from u.h - NO Linux headers.
 */

#ifndef FRUITY_TYPES_H
#define FRUITY_TYPES_H

/* When compiling standalone (not in kernel), include compatibility types */
#ifndef _U_H_
#include "fruity_standalone.h"
#endif

/* Basic CLR types - forward declarations */
typedef u32int tasklet_id_t;
typedef u32int channel_id_t;

/* CLR value types (from clr_runtime.h) */
typedef enum {
	CLR_INT32,
	CLR_INT64,
	CLR_BOOL,
	CLR_REF,
	CLR_NULL
} clr_value_type_t;

#endif /* FRUITY_TYPES_H */
