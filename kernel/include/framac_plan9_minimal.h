/**
 * Minimal Frama-C Plan 9 Compatibility
 * Only defines what's missing, doesn't redefine Plan 9 types
 */

#ifndef FRAMAC_PLAN9_MINIMAL_H
#define FRAMAC_PLAN9_MINIMAL_H

#ifdef __FRAMAC__

// These types might be missing in preprocessed output
#ifndef _UUID_T_DEFINED
#define _UUID_T_DEFINED
typedef unsigned char uuid_t[16];
#endif

// Only stub out truly missing kernel-internal types
// Most Plan 9 types will come from the actual headers

#endif /* __FRAMAC__ */
#endif /* FRAMAC_PLAN9_MINIMAL_H */
