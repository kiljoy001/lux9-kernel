/*
 * m3_simd_hal.h
 *
 * Hardware Abstraction Layer for WebAssembly SIMD (v128).
 * Maps WASM operations to native vector extensions (AVX/NEON) via generic
 * intrinsics.
 *
 * "Hardware Axiom": Correctness relies on the host compiler and CPU.
 */

#ifndef m3_simd_hal_h
#define m3_simd_hal_h

#include "m3_core.h"

#if d_m3HasSIMD

#if defined(__GNUC__) || defined(__clang__)

// -- Native Vector Extensions --

static inline v128 simd_load(const void *mem) { return *(const v128 *)mem; }

static inline void simd_store(void *mem, v128 val) { *(v128 *)mem = val; }

// Integer Arithmetic
#define SIMD_BINOP(NAME, OP)                                                   \
  static inline v128 simd_##NAME(v128 a, v128 b) { return a OP b; }

SIMD_BINOP(and, &)
SIMD_BINOP(or, |)
SIMD_BINOP(xor, ^)

// Note: GCC vector extensions support +,-,*,/ for integer vectors
SIMD_BINOP(i8x16_add, +)
SIMD_BINOP(i8x16_sub, -)
SIMD_BINOP(i16x8_add, +)
SIMD_BINOP(i16x8_sub, -)
SIMD_BINOP(i32x4_add, +)
SIMD_BINOP(i32x4_sub, -)
SIMD_BINOP(i64x2_add, +)
SIMD_BINOP(i64x2_sub, -)

// Shuffles / Swizzles require builtins
static inline v128 simd_v8x16_shuffle(v128 a, v128 b, v128 mask) {
  return __builtin_shufflevector(
      (u8 __attribute__((vector_size(16))))a,
      (u8 __attribute__((vector_size(16))))b,
      // Todo: generic shuffle mask handling is complex
      // For now, map specific shuffles or use fallbacks if needed
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
  // Placeholder; dynamic shuffle is hard with __builtin_shufflevector
}

#else

// -- Fallback (Software Emulation Stub) --
// This path exists to allow compilation on non-SIMD platforms,
// satisfying type CHECKs but not performing accelerated ops.

static inline v128 simd_load(const void *mem) { return *(const v128 *)mem; }

static inline void simd_store(void *mem, v128 val) { *(v128 *)mem = val; }

static inline v128 simd_i32x4_add(v128 a, v128 b) {
  v128 r;
  // Assuming v128 is { u64 i[2] } from m3_core.h fallback
  // This is just a stub for now.
  return r;
}

#endif // Compiler detection

#endif // d_m3HasSIMD

#endif // m3_simd_hal_h
