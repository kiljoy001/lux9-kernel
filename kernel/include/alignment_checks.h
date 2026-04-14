/* Compile-time alignment checks for critical structures */
#ifndef _ALIGNMENT_CHECKS_H_
#define _ALIGNMENT_CHECKS_H_

#include "u.h"
#include "dat.h"

/* Ensure critical structures are properly aligned */
static_assert(offsetof(struct FPssestate, xmm) % 16 == 0, "FPssestate.xmm must be 16-byte aligned");
static_assert(offsetof(struct FPssestate, xmm) % 32 == 0, "FPssestate.xmm should be 32-byte aligned for AVX");
static_assert(sizeof(struct FPssestate) % 64 == 0, "FPssestate size must be cache-line aligned");

static_assert(sizeof(struct Lock) % 64 == 0, "Lock size must be cache-line aligned");
static_assert(sizeof(struct Mach) % 64 == 0, "Mach size must be cache-line aligned");
static_assert(sizeof(struct Proc) % 64 == 0, "Proc size must be cache-line aligned");
static_assert(sizeof(struct Page) % 64 == 0, "Page size must be cache-line aligned");
static_assert(sizeof(struct MMU) % 64 == 0, "MMU size must be cache-line aligned");
static_assert(sizeof(Tss) % 64 == 0, "Tss size must be cache-line aligned");

/* Ensure pointer-sized assumptions */
static_assert(sizeof(uintptr) == 8, "uintptr must be 64-bit on 64-bit platform");
static_assert(sizeof(void*) == 8, "Pointer size must be 64-bit");

/* Ensure type size assumptions */
static_assert(sizeof(ulong) == 8, "ulong must be 64-bit");
static_assert(sizeof(uvlong) == 8, "uvlong must be 64-bit");
static_assert(sizeof(usize) == 8, "usize must be 64-bit");

#endif /* _ALIGNMENT_CHECKS_H_ */