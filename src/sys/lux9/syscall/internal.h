/*
 * Internal definitions for Lux9 Syscall System
 * 
 * This header contains internal definitions not meant for public use.
 */

#ifndef _LUX9_SYSCALL_INTERNAL_H_
#define _LUX9_SYSCALL_INTERNAL_H_

#include "lux9_syscall.h"
#include <stddef.h>

/*
 * ============================================================================
 * INTERNAL MACROS AND CONSTANTS
 * ============================================================================
 */

/* Maximum number of syscalls in table */
#define LUX9_SYSCALL_MAX           4096

/* Error codes */
#define LUX9_E_SUCCESS            0
#define LUX9_E_INVALID_ARGS       -1
#define LUX9_E_NOT_FOUND          -2
#define LUX9_E_ALREADY_EXISTS     -3
#define LUX9_E_NO_MEMORY          -4
#define LUX9_E_NOT_IMPLEMENTED    -5
#define LUX9_E_PERMISSION_DENIED  -6

/* Compatibility layer offsets */
#define LUX9_PLAN9_OFFSET          0
#define LUX9_LUX9_OFFSET          100
#define LUX9_EXTENDED_OFFSET      200
#define LUX9_MEMORY_OFFSET       1000

/*
 * ============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================
 */

/* Syscall statistics structure */
struct lux9_syscall_stats {
    uint64_t call_count;
    uint64_t error_count;
    uint64_t total_time_ns;
    uint32_t min_time_ns;
    uint32_t max_time_ns;
    uint32_t avg_time_ns;
};

/* Syscall handler info */
struct lux9_syscall_info {
    uint32_t syscall_number;
    const char *name;
    const char *description;
    uint32_t flags;
    uint32_t subsystem_id;
    int (*handler)(uintptr_t args[], size_t arg_count, uintptr_t *result);
    struct lux9_syscall_stats stats;
};

/*
 * ============================================================================
 * INTERNAL FUNCTION DECLARATIONS
 * ============================================================================
 */

/* Syscall table management */
int lux9_syscall_table_init(void);
int lux9_syscall_register_entry(const struct lux9_syscall_info *entry);
int lux9_syscall_unregister_entry(uint32_t syscall_number);
struct lux9_syscall_info *lux9_syscall_get_info(uint32_t syscall_number);

/* Compatibility layer functions */
int plan9_syscall_register_all(void);
int plan9_syscall_dispatch(uint32_t syscall_number, uintptr_t args[],
                          size_t arg_count, uintptr_t *result);
const char *plan9_syscall_get_name(uint32_t syscall_number);

int lux9_native_syscall_register_all(void);

/* Handler implementations */
int sys_read_impl(int fd, void *buf, size_t count, uintptr_t *result);
int sys_write_impl(int fd, const void *buf, size_t count, uintptr_t *result);
int sys_open_impl(const char *pathname, int flags, int *result);
int sys_close_impl(int fd, int *result);
int sys_pipe_impl(int *fds, int *result);

/* Utility functions */
int lux9_syscall_validate_args(uint32_t syscall_number, uintptr_t args[], size_t arg_count);
void lux9_syscall_update_stats(uint32_t syscall_number, uint32_t time_ns, int error);
const char *lux9_syscall_error_string(int error_code);

/* Debugging and tracing */
#ifdef LUX9_SYSCALL_DEBUG
    #define LUX9_SYSCALL_TRACE(args) do { printf args; } while(0)
#else
    #define LUX9_SYSCALL_TRACE(args) do { } while(0)
#endif

/*
 * ============================================================================
 * INLINE HELPER FUNCTIONS
 * ============================================================================
 */

/* Fast syscall number validation */
static inline int lux9_syscall_number_is_valid(uint32_t syscall_number) {
    return syscall_number < LUX9_SYSCALL_MAX;
}

/* Get syscall category from number */
static inline uint32_t lux9_syscall_get_category(uint32_t syscall_number) {
    if (syscall_number < LUX9_LUX9_OFFSET) {
        return LUX9_SYSCALL_COMPAT;
    } else if (syscall_number < LUX9_EXTENDED_OFFSET) {
        return LUX9_SYSCALL_NATIVE;
    } else if (syscall_number < LUX9_MEMORY_OFFSET) {
        return LUX9_SYSCALL_EXTENDED;
    } else {
        return LUX9_SYSCALL_SUBSYSTEM;
    }
}

/* Check if syscall needs compatibility translation */
static inline int lux9_syscall_needs_translation(uint32_t syscall_number) {
    /* Currently only Plan 9 compatibility needs translation */
    return syscall_number < LUX9_LUX9_OFFSET;
}

/* Get subsystem name from ID */
static inline const char *lux9_syscall_subsystem_name(uint32_t subsystem_id) {
    switch (subsystem_id) {
        case 'P9  ': return "Plan 9";
        case 'LUX9': return "Lux9";
        case 'EXT ': return "Extended";
        case 'MEM ': return "Memory";
        case 'NET ': return "Network";
        case 'SEC ': return "Security";
        default: return "Unknown";
    }
}

#endif /* _LUX9_SYSCALL_INTERNAL_H_ */
