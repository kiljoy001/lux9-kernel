/*
 * Lux9 Native Compatibility Layer
 * 
 * This header resolves the WASM/Pebble collision (syscalls 63-65) and provides
 * proper mapping for Lux9 native syscalls.
 * 
 * COLLISION RESOLUTION:
 *   Original numbers were conflicting with potential Plan 9 extensions:
 *   - SYS_WASM_COMPILE:  63 → 163
 *   - SYS_WASM_EXECUTE:  64 → 164
 *   - SYS_WASM_DESTROY:  65 → 165
 * 
 * This maintains full backward compatibility while avoiding future conflicts.
 */

#ifndef _LUX9_SYSCALL_LUX9_NATIVE_COMPAT_H_
#define _LUX9_SYSCALL_LUX9_NATIVE_COMPAT_H_

#include "lux9_syscall.h"

/*
 * ============================================================================
 * LUX9 NATIVE SYSCALL MAPPING
 * 
 * Maps original Lux9 syscall numbers to unified scheme.
 * ============================================================================
 */

/* WASM Subsystem (avoiding collision) */
#define SYS_WASM_COMPILE          LUX9_SYS_WASM_COMPILE      /* 163 */
#define SYS_WASM_EXECUTE          LUX9_SYS_WASM_EXECUTE      /* 164 */
#define SYS_WASM_DESTROY          LUX9_SYS_WASM_DESTROY      /* 165 */

/* Exchange Pool IPC */
#define SYS_EXCHANGE_ALLOC         LUX9_SYS_EXCHANGE_ALLOC     /* 171 */
#define SYS_EXCHANGE_FREE          LUX9_SYS_EXCHANGE_FREE      /* 172 */
#define SYS_EXCHANGE_PUBLISH       LUX9_SYS_EXCHANGE_PUBLISH   /* 173 */
#define SYS_EXCHANGE_SUBSCRIBE     LUX9_SYS_EXCHANGE_SUBSCRIBE /* 174 */
#define SYS_EXCHANGE_UNSUBSCRIBE   LUX9_SYS_EXCHANGE_UNSUBSCRIBE /* 175 */
#define SYS_EXCHANGE_RECEIVE       LUX9_SYS_EXCHANGE_RECEIVE   /* 176 */

/* Process Management */
#define SYS_GETPID2                LUX9_SYS_GETPID2            /* 179 */
#define SYS_WAIT                   LUX9_SYS_WAIT               /* 36/166 */

/*
 * ============================================================================
 * ORIGINAL CONSTANTS (DEPRECATED)
 * 
 * These are kept for backward compatibility but are discouraged.
 * Use the LUX9_SYS_* constants instead.
 * ============================================================================
 */

/* Legacy WASM syscall numbers (deprecated, use LUX9_SYS_WASM_*) */
#define SYS_WASM_COMPILE_OLD       63   /* DEPRECATED */
#define SYS_WASM_EXECUTE_OLD       64   /* DEPRECATED */
#define SYS_WASM_DESTROY_OLD       65   /* DEPRECATED */

/* Legacy Exchange Pool numbers (these were in 67-72 range) */
#define SYS_EXCHANGE_ALLOC_OLD     67   /* DEPRECATED */
#define SYS_EXCHANGE_FREE_OLD      68   /* DEPRECATED */
#define SYS_EXCHANGE_PUBLISH_OLD   69   /* DEPRECATED */
#define SYS_EXCHANGE_SUBSCRIBE_OLD 70   /* DEPRECATED */
#define SYS_EXCHANGE_UNSUBSCRIBE_OLD 71 /* DEPRECATED */
#define SYS_EXCHANGE_RECEIVE_OLD   72   /* DEPRECATED */

/*
 * ============================================================================
 * SYSCALL VALIDATION
 * 
 * Functions to validate and handle Lux9 native syscalls.
 * ============================================================================
 */

/* Validate Lux9 native syscall number */
static inline int lux9_native_syscall_is_valid(uint32_t syscall_num) {
    return LUX9_SYSCALL_IS_NATIVE(syscall_num);
}

/* Check if syscall is in WASM subsystem */
static inline int lux9_wasm_syscall(uint32_t syscall_num) {
    return (syscall_num >= LUX9_SYS_WASM_COMPILE && 
            syscall_num <= LUX9_SYS_WASM_DESTROY);
}

/* Check if syscall is in Exchange Pool subsystem */
static inline int lux9_exchange_syscall(uint32_t syscall_num) {
    return (syscall_num >= LUX9_SYS_EXCHANGE_ALLOC && 
            syscall_num <= LUX9_SYS_EXCHANGE_ACK);
}

/* Check if syscall is deprecated */
static inline int lux9_syscall_is_deprecated(uint32_t syscall_num) {
    return (syscall_num == SYS_WASM_COMPILE_OLD ||
            syscall_num == SYS_WASM_EXECUTE_OLD ||
            syscall_num == SYS_WASM_DESTROY_OLD ||
            (syscall_num >= SYS_EXCHANGE_ALLOC_OLD &&
             syscall_num <= SYS_EXCHANGE_RECEIVE_OLD));
}

/*
 * ============================================================================
 * MIGRATION HELPERS
 * 
 * Functions to help migrate from old syscall numbers.
 * ============================================================================
 */

/* Get new syscall number from deprecated old number */
static inline uint32_t lux9_get_new_syscall_number(uint32_t old_num) {
    switch (old_num) {
        case SYS_WASM_COMPILE_OLD:     return SYS_WASM_COMPILE;
        case SYS_WASM_EXECUTE_OLD:     return SYS_WASM_EXECUTE;
        case SYS_WASM_DESTROY_OLD:     return SYS_WASM_DESTROY;
        case SYS_EXCHANGE_ALLOC_OLD:   return SYS_EXCHANGE_ALLOC;
        case SYS_EXCHANGE_FREE_OLD:    return SYS_EXCHANGE_FREE;
        case SYS_EXCHANGE_PUBLISH_OLD: return SYS_EXCHANGE_PUBLISH;
        case SYS_EXCHANGE_SUBSCRIBE_OLD: return SYS_EXCHANGE_SUBSCRIBE;
        case SYS_EXCHANGE_UNSUBSCRIBE_OLD: return SYS_EXCHANGE_UNSUBSCRIBE;
        case SYS_EXCHANGE_RECEIVE_OLD: return SYS_EXCHANGE_RECEIVE;
        default:                       return old_num;  /* No mapping */
    }
}

/* Get old syscall number from new number (for debugging) */
static inline uint32_t lux9_get_old_syscall_number(uint32_t new_num) {
    switch (new_num) {
        case SYS_WASM_COMPILE:     return SYS_WASM_COMPILE_OLD;
        case SYS_WASM_EXECUTE:     return SYS_WASM_EXECUTE_OLD;
        case SYS_WASM_DESTROY:     return SYS_WASM_DESTROY_OLD;
        case SYS_EXCHANGE_ALLOC:   return SYS_EXCHANGE_ALLOC_OLD;
        case SYS_EXCHANGE_FREE:    return SYS_EXCHANGE_FREE_OLD;
        case SYS_EXCHANGE_PUBLISH: return SYS_EXCHANGE_PUBLISH_OLD;
        case SYS_EXCHANGE_SUBSCRIBE: return SYS_EXCHANGE_SUBSCRIBE_OLD;
        case SYS_EXCHANGE_UNSUBSCRIBE: return SYS_EXCHANGE_UNSUBSCRIBE_OLD;
        case SYS_EXCHANGE_RECEIVE: return SYS_EXCHANGE_RECEIVE_OLD;
        default:                   return new_num;  /* No mapping */
    }
}

/*
 * ============================================================================
 * SUBSYSTEM CLASSIFICATION
 * 
 * Helper functions to identify syscall subsystems.
 * ============================================================================
 */

static inline const char *lux9_syscall_subsystem(uint32_t syscall_num) {
    if (syscall_num >= LUX9_SYS_WASM_COMPILE && 
        syscall_num <= LUX9_SYS_WASM_DESTROY) {
        return "WASM";
    }
    if (syscall_num >= LUX9_SYS_PEBBLE_ALLOC && 
        syscall_num <= LUX9_SYS_PEBBLE_SYNC) {
        return "Pebble";
    }
    if (syscall_num >= LUX9_SYS_EXCHANGE_ALLOC && 
        syscall_num <= LUX9_SYS_EXCHANGE_ACK) {
        return "Exchange Pool";
    }
    if (syscall_num >= LUX9_SYS_GETPID2 && 
        syscall_num <= LUX9_SYS_GETPPID2) {
        return "Process Management";
    }
    if (syscall_num >= LUX9_SYS_ASYNC_READ && 
        syscall_num <= LUX9_SYS_IOVEC_WRITE) {
        return "Advanced I/O";
    }
    return "Other";
}

/*
 * ============================================================================
 * PERFORMANCE CHARACTERISTICS
 * 
 * Helper functions to identify syscall performance characteristics.
 * ============================================================================
 */

static inline int lux9_syscall_is_fast(uint32_t syscall_num) {
    /* Fast syscalls: WASM operations, basic memory ops */
    return (syscall_num >= LUX9_SYS_WASM_COMPILE && 
            syscall_num <= LUX9_SYS_WASM_DESTROY) ||
           (syscall_num >= LUX9_SYS_PEBBLE_ALLOC && 
            syscall_num <= LUX9_SYS_PEBBLE_SET);
}

static inline int lux9_syscall_is_slow(uint32_t syscall_num) {
    /* Slow syscalls: Exchange Pool IPC, file operations */
    return (syscall_num >= LUX9_SYS_EXCHANGE_PUBLISH && 
            syscall_num <= LUX9_SYS_EXCHANGE_RECEIVE) ||
           (syscall_num >= LUX9_SYS_ASYNC_READ && 
            syscall_num <= LUX9_SYS_IOVEC_WRITE);
}

/*
 * ============================================================================
 * COMPATIBILITY MODE
 * 
 * Control compatibility behavior at compile time.
 * ============================================================================
 */

#ifdef LUX9_DISABLE_LEGACY_SYSCALLS
    /* Legacy syscall numbers are disabled */
    #define LUX9_WARN_LEGACY(syscall_num) \
        do { \
            if (lux9_syscall_is_deprecated(syscall_num)) { \
                /* Emit warning about deprecated syscall */ \
            } \
        } while(0)
#else
    /* Legacy syscall numbers are supported */
    #define LUX9_WARN_LEGACY(syscall_num) do { } while(0)
#endif

/*
 * ============================================================================
 * USAGE GUIDELINES
 * 
 * 1. New code should use LUX9_SYS_* constants
 * 2. Legacy SYS_* constants are supported but discouraged
 * 3. Legacy constants will be removed in a future version
 * 4. Use lux9_syscall_subsystem() to identify syscall category
 * 5. Check for deprecation with lux9_syscall_is_deprecated()
 * 
 * Example migration:
 *   // Old code
 *   syscall(SYS_WASM_COMPILE, wasm_data, wasm_size);
 *   
 *   // New code
 *   syscall(LUX9_SYS_WASM_COMPILE, wasm_data, wasm_size);
 *   
 *   // Both work, but new code is preferred
 * ============================================================================
 */

#endif /* _LUX9_SYSCALL_LUX9_NATIVE_COMPAT_H_ */
