/*
 * Lux9 Unified System Call Dispatcher
 * 
 * This module implements the core syscall dispatch mechanism that:
 * 1. Validates syscall numbers
 * 2. Routes to appropriate handlers
 * 3. Handles compatibility layers
 * 4. Manages performance optimization
 */

#include "lux9_syscall.h"
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global syscall table */
static struct lux9_syscall_entry syscall_table[LUX9_SYSCALL_MAX + 1];
static int syscall_table_initialized = 0;
static uint32_t registered_syscalls = 0;

/* Statistics */
static struct {
    uint64_t total_calls;
    uint64_t compat_calls;
    uint64_t native_calls;
    uint64_t extended_calls;
    uint64_t subsystem_calls;
    uint64_t invalid_calls;
} syscall_stats;

/* ============================================================================
 * SYSCALL TABLE MANAGEMENT
 * ============================================================================
 */

/* Initialize syscall table */
int lux9_syscall_table_init(void) {
    if (syscall_table_initialized)
        return 0;
    
    memset(syscall_table, 0, sizeof(syscall_table));
    memset(&syscall_stats, 0, sizeof(syscall_stats));
    
    /* Register compatibility layer syscalls */
    int ret = lux9_syscall_register_compat();
    if (ret < 0) {
        fprintf(stderr, "Failed to register compatibility layer\n");
        return -1;
    }
    
    syscall_table_initialized = 1;
    return 0;
}

/* Register a syscall entry */
int lux9_syscall_register(const struct lux9_syscall_entry *entry) {
    if (!entry || entry->syscall_number > LUX9_SYSCALL_MAX) {
        return -1;
    }
    
    if (syscall_table[entry->syscall_number].handler != NULL) {
        fprintf(stderr, "Syscall %u already registered\n", entry->syscall_number);
        return -1;
    }
    
    syscall_table[entry->syscall_number] = *entry;
    registered_syscalls++;
    
    return 0;
}

/* Unregister a syscall entry */
int lux9_syscall_unregister(uint32_t syscall_number) {
    if (syscall_number > LUX9_SYSCALL_MAX) {
        return -1;
    }
    
    if (syscall_table[syscall_number].handler == NULL) {
        return -1;  /* Not registered */
    }
    
    memset(&syscall_table[syscall_number], 0, sizeof(struct lux9_syscall_entry));
    registered_syscalls--;
    
    return 0;
}

/* Get syscall handler */
lux9_syscall_handler_t lux9_syscall_get_handler(uint32_t syscall_number) {
    if (syscall_number > LUX9_SYSCALL_MAX) {
        return NULL;
    }
    
    return syscall_table[syscall_number].handler;
}

/* Get syscall name */
const char *lux9_syscall_get_name(uint32_t syscall_number) {
    if (syscall_number > LUX9_SYSCALL_MAX) {
        return "INVALID";
    }
    
    const char *name = syscall_table[syscall_number].syscall_name;
    if (!name) {
        /* Generate name for compatibility layer syscalls */
        if (LUX9_SYSCALL_IS_COMPAT(syscall_number)) {
            return plan9_syscall_get_name(syscall_number);
        }
        return "UNREGISTERED";
    }
    
    return name;
}

/* ============================================================================
 * CORE SYSCALL DISPATCHER
 * ============================================================================
 */

/* Main syscall dispatcher */
int lux9_syscall_dispatch(uint32_t syscall_number, uintptr_t args[], 
                         size_t arg_count, uintptr_t *result) {
    syscall_stats.total_calls++;
    
    /* Validate syscall number */
    if (!lux9_syscall_is_valid_fast(syscall_number)) {
        syscall_stats.invalid_calls++;
        return -LUX9_EINVAL;
    }
    
    /* Update statistics */
    if (LUX9_SYSCALL_IS_COMPAT(syscall_number)) {
        syscall_stats.compat_calls++;
    } else if (LUX9_SYSCALL_IS_NATIVE(syscall_number)) {
        syscall_stats.native_calls++;
    } else if (LUX9_SYSCALL_IS_EXTENDED(syscall_number)) {
        syscall_stats.extended_calls++;
    } else if (LUX9_SYSCALL_IS_SUBSYSTEM(syscall_number)) {
        syscall_stats.subsystem_calls++;
    }
    
    /* Check compatibility layer first */
    if (LUX9_SYSCALL_IS_COMPAT(syscall_number)) {
        return lux9_compat_dispatch(syscall_number, args, arg_count, result);
    }
    
    /* Get handler from table */
    lux9_syscall_handler_t handler = lux9_syscall_get_handler(syscall_number);
    if (!handler) {
        return -LUX9_ENOSYS;
    }
    
    /* Call handler */
    int ret = handler(args, arg_count, result);
    return ret;
}

/* Fast path dispatcher for common syscalls */
int lux9_syscall_dispatch_fast(uint32_t syscall_number, uintptr_t args[], 
                               size_t arg_count, uintptr_t *result) {
    /* Fast path for Plan 9 compatibility layer (most common) */
    if (LUX9_SYSCALL_IS_COMPAT(syscall_number) && syscall_number <= 53) {
        return lux9_compat_dispatch_fast(syscall_number, args, arg_count, result);
    }
    
    /* Fall back to normal dispatcher */
    return lux9_syscall_dispatch(syscall_number, args, arg_count, result);
}

/* ============================================================================
 * COMPATIBILITY LAYER DISPATCH
 * ============================================================================
 */

/* Register compatibility layer syscalls */
int lux9_syscall_register_compat(void) {
    /* Plan 9 compatibility layer registration */
    extern int plan9_syscall_register_all(void);
    int ret = plan9_syscall_register_all();
    if (ret < 0) {
        return -1;
    }
    
    /* Lux9 native compatibility layer registration */
    extern int lux9_native_syscall_register_all(void);
    ret = lux9_native_syscall_register_all();
    if (ret < 0) {
        return -1;
    }
    
    return 0;
}

/* Dispatch to compatibility layer */
int lux9_compat_dispatch(uint32_t syscall_number, uintptr_t args[],
                        size_t arg_count, uintptr_t *result) {
    /* Plan 9 compatibility layer */
    if (LUX9_SYSCALL_IS_COMPAT(syscall_number)) {
        return plan9_syscall_dispatch(syscall_number, args, arg_count, result);
    }
    
    return -LUX9_ENOSYS;
}

/* Fast path for Plan 9 compatibility */
int lux9_compat_dispatch_fast(uint32_t syscall_number, uintptr_t args[],
                              size_t arg_count, uintptr_t *result) {
    /* Handle common Plan 9 syscalls directly */
    switch (syscall_number) {
        case LUX9_SYS_READ:
            /* Fast path for read */
            return sys_read_impl(args[0], (void *)args[1], args[2], (uintptr_t *)result);
        
        case LUX9_SYS_WRITE:
            /* Fast path for write */
            return sys_write_impl(args[0], (const void *)args[1], args[2], (uintptr_t *)result);
        
        case LUX9_SYS_OPEN:
            /* Fast path for open */
            return sys_open_impl((const char *)args[0], args[1], (int *)result);
        
        case LUX9_SYS_CLOSE:
            /* Fast path for close */
            return sys_close_impl(args[0], result);
        
        default:
            /* Fall back to slow path */
            break;
    }
    
    return plan9_syscall_dispatch(syscall_number, args, arg_count, result);
}

/* ============================================================================
 * VALIDATION FUNCTIONS
 * ============================================================================
 */

/* Validate syscall number */
int lux9_syscall_validate_number(uint32_t syscall_number) {
    return lux9_syscall_is_valid_fast(syscall_number);
}

/* Check if syscall is registered */
int lux9_syscall_is_registered(uint32_t syscall_number) {
    if (syscall_number > LUX9_SYSCALL_MAX) {
        return 0;
    }
    
    return syscall_table[syscall_number].handler != NULL;
}

/* Get subsystem name */
const char *lux9_syscall_get_subsystem_name(uint32_t syscall_number) {
    if (syscall_number > LUX9_SYSCALL_MAX) {
        return "INVALID";
    }
    
    const char *name = syscall_table[syscall_number].description;
    if (name) {
        return name;
    }
    
    /* Fall back to compatibility layer */
    if (LUX9_SYSCALL_IS_COMPAT(syscall_number)) {
        return "Plan9-Compatible";
    }
    
    return "Unknown";
}

/* ============================================================================
 * STATISTICS AND DEBUGGING
 * ============================================================================
 */

/* Get syscall statistics */
int lux9_syscall_get_stats(uint32_t syscall_number, void *stats) {
    if (!stats) {
        return -LUX9_EINVAL;
    }
    
    if (syscall_number == (uint32_t)-1) {
        /* Return global statistics */
        memcpy(stats, &syscall_stats, sizeof(syscall_stats));
        return 0;
    }
    
    if (syscall_number > LUX9_SYSCALL_MAX) {
        return -LUX9_EINVAL;
    }
    
    /* Return per-syscall statistics */
    struct lux9_syscall_entry *entry = &syscall_table[syscall_number];
    memcpy(stats, &entry->flags, sizeof(uint32_t));  /* Reuse flags field for stats */
    
    return 0;
}

/* Print syscall table */
void lux9_syscall_print_table(void) {
    printf("Lux9 Syscall Table:\n");
    printf("===================\n");
    printf("Registered syscalls: %u\n", registered_syscalls);
    printf("\n");
    
    /* Print statistics */
    printf("Statistics:\n");
    printf("  Total calls:     %lu\n", syscall_stats.total_calls);
    printf("  Plan 9 compat:   %lu\n", syscall_stats.compat_calls);
    printf("  Lux9 native:     %lu\n", syscall_stats.native_calls);
    printf("  Extended:        %lu\n", syscall_stats.extended_calls);
    printf("  Subsystem:       %lu\n", syscall_stats.subsystem_calls);
    printf("  Invalid:         %lu\n", syscall_stats.invalid_calls);
    printf("\n");
    
    /* Print registered syscalls */
    printf("Registered syscalls:\n");
    for (uint32_t i = 0; i <= LUX9_SYSCALL_MAX; i++) {
        if (syscall_table[i].handler != NULL) {
            printf("  %-3u: %-20s [%s]\n", 
                   i, 
                   syscall_table[i].syscall_name ?: "UNNAMED",
                   lux9_syscall_get_category(i));
        }
    }
}

/* Reset statistics */
void lux9_syscall_reset_stats(void) {
    memset(&syscall_stats, 0, sizeof(syscall_stats));
}

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================
 */

/* Initialize syscall system */
int lux9_syscall_init(void) {
    int ret = lux9_syscall_table_init();
    if (ret < 0) {
        return -1;
    }
    
    return 0;
}

/* Cleanup syscall system */
void lux9_syscall_fini(void) {
    if (!syscall_table_initialized) {
        return;
    }
    
    memset(syscall_table, 0, sizeof(syscall_table));
    memset(&syscall_stats, 0, sizeof(syscall_stats));
    syscall_table_initialized = 0;
    registered_syscalls = 0;
}

/* Module initialization */
__attribute__((constructor))
static void lux9_syscall_module_init(void) {
    lux9_syscall_init();
}

/* Module cleanup */
__attribute__((destructor))
static void lux9_syscall_module_fini(void) {
    lux9_syscall_fini();
}

/* ============================================================================
 * PLATFORM-SPECIFIC OPTIMIZATIONS
 * ============================================================================
 */

#ifdef __x86_64__
/* x86-64 specific fast syscall entry */
int lux9_syscall_fastcall(uint64_t syscall_number, uint64_t a0, uint64_t a1, 
                         uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5,
                         uint64_t *result) {
    uintptr_t args[6] = {a0, a1, a2, a3, a4, a5};
    return lux9_syscall_dispatch_fast(syscall_number, args, 6, result);
}
#elif defined(__aarch64__)
/* ARM64 specific fast syscall entry */
int lux9_syscall_fastcall(uint64_t syscall_number, uint64_t a0, uint64_t a1,
                         uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5,
                         uint64_t *result) {
    uintptr_t args[6] = {a0, a1, a2, a3, a4, a5};
    return lux9_syscall_dispatch_fast(syscall_number, args, 6, result);
}
#endif

/* ============================================================================
 * ERROR CODES
 * ============================================================================
 */

/* Unified error codes */
#define LUX9_SUCCESS         0
#define LUX9_EINVAL         -1  /* Invalid argument */
#define LUX9_ENOSYS         -2  /* Syscall not implemented */
#define LUX9_EACCES         -3  /* Permission denied */
#define LUX9_EFAULT         -4  /* Bad address */
#define LUX9_EBUSY          -5  /* Resource busy */
#define LUX9_ENOMEM         -6  /* Out of memory */
#define LUX9_EIO           -7   /* I/O error */

/* Convert to platform error codes */
int lux9_syscall_to_errno(int lux9_error) {
    switch (lux9_error) {
        case LUX9_SUCCESS: return 0;
        case LUX9_EINVAL: return EINVAL;
        case LUX9_ENOSYS: return ENOSYS;
        case LUX9_EACCES: return EACCES;
        case LUX9_EFAULT: return EFAULT;
        case LUX9_EBUSY: return EBUSY;
        case LUX9_ENOMEM: return ENOMEM;
        case LUX9_EIO: return EIO;
        default: return EINVAL;
    }
}

/* Convert from platform error codes */
int lux9_errno_to_syscall(int errno_val) {
    switch (errno_val) {
        case 0: return LUX9_SUCCESS;
        case EINVAL: return LUX9_EINVAL;
        case ENOSYS: return LUX9_ENOSYS;
        case EACCES: return LUX9_EACCES;
        case EFAULT: return LUX9_EFAULT;
        case EBUSY: return LUX9_EBUSY;
        case ENOMEM: return LUX9_ENOMEM;
        case EIO: return LUX9_EIO;
        default: return LUX9_EINVAL;
    }
}
