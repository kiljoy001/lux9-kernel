#pragma once

/*
 * Software Attestation Levels (Phase 2)
 *
 * Used by Pebble token economy to determine trust-based resource limits
 */

enum {
	ATTEST_NONE = 0,      /* No verification */
	ATTEST_HASH = 1,      /* Hash matched manifest (unsigned) */
	ATTEST_SIGNED = 2,    /* Hash matched signed manifest */
};

/* Check if process meets required attestation level */
int proc_attest_check(Proc *p, int required_level);

/* Get attestation level name for logging */
const char* attest_level_name(int level);
