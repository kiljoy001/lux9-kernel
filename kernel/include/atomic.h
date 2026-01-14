/*
 * Lux9 Kernel Atomics Shim
 * Wraps GCC/Clang built-ins for safe memory ordering.
 */

#ifndef _LUX9_ATOMIC_H_
#define _LUX9_ATOMIC_H_

/*
 * Memory Order Definitions
 * (Matching C11 standard for documentation purposes)
 */
#ifdef __FRAMAC__
/* Frama-C doesn't support GCC atomic builtins; provide functional shims */
#define __ATOMIC_RELAXED 0
#define __ATOMIC_ACQUIRE 2
#define __ATOMIC_RELEASE 3
#define __ATOMIC_SEQ_CST 5

#define __atomic_load_n(ptr, order) (*(ptr))
#define __atomic_store_n(ptr, val, order) (*(ptr) = (val))
#define __atomic_exchange_n(ptr, val, order)                                   \
  ({                                                                           \
    __typeof__(*(ptr)) _tmp = *(ptr);                                          \
    *(ptr) = (val);                                                            \
    _tmp;                                                                      \
  })
#define __atomic_compare_exchange_n(ptr, exp, des, weak, success, fail)        \
  (*(ptr) == *(exp) ? (*(ptr) = (des), 1) : (*(exp) = *(ptr), 0))
#define __atomic_thread_fence(order)                                           \
  do {                                                                         \
  } while (0)
#endif

#define ORDER_RELAXED __ATOMIC_RELAXED
#define ORDER_ACQUIRE __ATOMIC_ACQUIRE
#define ORDER_RELEASE __ATOMIC_RELEASE
#define ORDER_SEQ_CST __ATOMIC_SEQ_CST

/*
 * Atomic Load
 * usage: int val = atomic_load(&var, ORDER_ACQUIRE);
 */
#define atomic_load(ptr, order) __atomic_load_n((ptr), (order))

/*
 * Atomic Store
 * usage: atomic_store(&var, val, ORDER_RELEASE);
 */
#define atomic_store(ptr, val, order) __atomic_store_n((ptr), (val), (order))

/*
 * Atomic Exchange (useful for locks)
 */
#define atomic_exchange(ptr, val, order)                                       \
  __atomic_exchange_n((ptr), (val), (order))

/*
 * Compare and Swap
 * usage: if (atomic_cas(&var, &expected, desired)) ...
 */
#define atomic_cas(ptr, expected_ptr, desired)                                 \
  __atomic_compare_exchange_n((ptr), (expected_ptr), (desired), 0,             \
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

/*
 * Memory Fences
 */
#define atomic_thread_fence(order) __atomic_thread_fence(order)

#endif /* _LUX9_ATOMIC_H_ */
