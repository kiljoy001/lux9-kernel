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
#define ORDER_RELAXED __ATOMIC_RELAXED
#define ORDER_ACQUIRE __ATOMIC_ACQUIRE
#define ORDER_RELEASE __ATOMIC_RELEASE
#define ORDER_SEQ_CST __ATOMIC_SEQ_CST

/* 
 * Atomic Load 
 * usage: int val = atomic_load(&var, ORDER_ACQUIRE);
 */
#define atomic_load(ptr, order) \
    __atomic_load_n((ptr), (order))

/* 
 * Atomic Store 
 * usage: atomic_store(&var, val, ORDER_RELEASE);
 */
#define atomic_store(ptr, val, order) \
    __atomic_store_n((ptr), (val), (order))

/*
 * Atomic Exchange (useful for locks)
 */
#define atomic_exchange(ptr, val, order) \
    __atomic_exchange_n((ptr), (val), (order))

/*
 * Compare and Swap
 * usage: if (atomic_cas(&var, &expected, desired)) ...
 */
#define atomic_cas(ptr, expected_ptr, desired) \
    __atomic_compare_exchange_n((ptr), (expected_ptr), (desired), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

/*
 * Memory Fences
 */
#define atomic_thread_fence(order) \
    __atomic_thread_fence(order)

#endif /* _LUX9_ATOMIC_H_ */
