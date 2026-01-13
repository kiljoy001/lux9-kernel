/*
 * RUMP/NetBSD Compatibility Layer
 * 
 * This header provides compatibility with RUMP/NetBSD syscall definitions.
 * RUMP uses a different syscall numbering scheme that maps to actual
 * NetBSD syscalls.
 * 
 * Since RUMP syscalls are typically implemented through a hypervisor or
 * compatibility layer, we provide a mapping to the unified scheme.
 */

#ifndef _LUX9_SYSCALL_RUMP_COMPAT_H_
#define _LUX9_SYSCALL_RUMP_COMPAT_H_

#include "lux9_syscall.h"

/*
 * ============================================================================
 * RUMP SYSCALL MAPPING STRATEGY
 * 
 * RUMP syscalls are hypervisor-based and map to actual NetBSD syscalls.
 * We provide a translation layer that:
 * 1. Accepts RUMP syscall numbers
 * 2. Translates to unified numbering scheme
 * 3. Handles any necessary conversion
 * ============================================================================
 */

/* RUMP syscall base offsets for different categories */
#define LUX9_RUMP_FILE_BASE        3000  /* File I/O operations */
#define LUX9_RUMP_PROCESS_BASE     3100  /* Process management */
#define LUX9_RUMP_NETWORK_BASE     3200  /* Network operations */
#define LUX9_RUMP_IPC_BASE         3300  /* IPC operations */
#define LUX9_RUMP_MISC_BASE        3900  /* Miscellaneous */

/*
 * ============================================================================
 * COMMON RUMP SYSCALLS
 * 
 * These are the most commonly used RUMP syscalls mapped to unified scheme.
 * ============================================================================
 */

/* File I/O (3000-3099) */
#define LUX9_RUMP_SYS_READ         (LUX9_RUMP_FILE_BASE + 0)   /* Read */
#define LUX9_RUMP_SYS_WRITE        (LUX9_RUMP_FILE_BASE + 1)  /* Write */
#define LUX9_RUMP_SYS_OPEN         (LUX9_RUMP_FILE_BASE + 2)  /* Open */
#define LUX9_RUMP_SYS_CLOSE        (LUX9_RUMP_FILE_BASE + 3) /* Close */
#define LUX9_RUMP_SYS_LSEEK        (LUX9_RUMP_FILE_BASE + 4) /* Seek */
#define LUX9_RUMP_SYS_PREAD        (LUX9_RUMP_FILE_BASE + 5)  /* Pre-positioned read */
#define LUX9_RUMP_SYS_PWRITE       (LUX9_RUMP_FILE_BASE + 6)  /* Pre-positioned write */

/* Process Management (3100-3199) */
#define LUX9_RUMP_SYS_GETPID       (LUX9_RUMP_PROCESS_BASE + 0)  /* Get PID */
#define LUX9_RUMP_SYS_GETPPID      (LUX9_RUMP_PROCESS_BASE + 1) /* Get parent PID */
#define LUX9_RUMP_SYS_FORK         (LUX9_RUMP_PROCESS_BASE + 2)  /* Fork process */
#define LUX9_RUMP_SYS_WAIT         (LUX9_RUMP_PROCESS_BASE + 3)  /* Wait for child */
#define LUX9_RUMP_SYS_EXIT         (LUX9_RUMP_PROCESS_BASE + 4)  /* Exit process */

/* Network Operations (3200-3299) */
#define LUX9_RUMP_SYS_SOCKET       (LUX9_RUMP_NETWORK_BASE + 0)  /* Create socket */
#define LUX9_RUMP_SYS_BIND         (LUX9_RUMP_NETWORK_BASE + 1)  /* Bind socket */
#define LUX9_RUMP_SYS_CONNECT      (LUX9_RUMP_NETWORK_BASE + 2)  /* Connect socket */
#define LUX9_RUMP_SYS_LISTEN       (LUX9_RUMP_NETWORK_BASE + 3) /* Listen on socket */
#define LUX9_RUMP_SYS_ACCEPT       (LUX9_RUMP_NETWORK_BASE + 4)  /* Accept connection */
#define LUX9_RUMP_SYS_SEND         (LUX9_RUMP_NETWORK_BASE + 5)  /* Send data */
#define LUX9_RUMP_SYS_RECV         (LUX9_RUMP_NETWORK_BASE + 6)  /* Receive data */

/* IPC Operations (3300-3399) */
#define LUX9_RUMP_SYS_SEMGET       (LUX9_RUMP_IPC_BASE + 0)  /* Get semaphore */
#define LUX9_RUMP_SYS_SEMOP        (LUX9_RUMP_IPC_BASE + 1)  /* Semaphore operation */
#define LUX9_RUMP_SYS_SEMCTL       (LUX9_RUMP_IPC_BASE + 2)  /* Semaphore control */
#define LUX9_RUMP_SYS_SHMGET       (LUX9_RUMP_IPC_BASE + 3)  /* Get shared memory */
#define LUX9_RUMP_SYS_SHMAT        (LUX9_RUMP_IPC_BASE + 4)  /* Attach shared memory */
#define LUX9_RUMP_SYS_SHMDT        (LUX9_RUMP_IPC_BASE + 5)  /* Detach shared memory */

/* Miscellaneous (3900-3999) */
#define LUX9_RUMP_SYS_MMAP         (LUX9_RUMP_MISC_BASE + 0)  /* Memory map */
#define LUX9_RUMP_SYS_MUNMAP       (LUX9_RUMP_MISC_BASE + 1)  /* Unmap memory */
#define LUX9_RUMP_SYS_MPROTECT     (LUX9_RUMP_MISC_BASE + 2)  /* Set protection */
#define LUX9_RUMP_SYS_FCNTL        (LUX9_RUMP_MISC_BASE + 3)  /* File control */
#define LUX9_RUMP_SYS_IOCTL        (LUX9_RUMP_MISC_BASE + 4)  /* I/O control */

/*
 * ============================================================================
 * RUMP COMPATIBILITY MACROS
 * 
 * These allow existing RUMP code to work with the unified scheme.
 * ============================================================================
 */

/* Direct syscall number mapping */
#define RUMP_SYS_READ              LUX9_RUMP_SYS_READ
#define RUMP_SYS_WRITE             LUX9_RUMP_SYS_WRITE
#define RUMP_SYS_OPEN              LUX9_RUMP_SYS_OPEN
#define RUMP_SYS_CLOSE             LUX9_RUMP_SYS_CLOSE
#define RUMP_SYS_GETPID            LUX9_RUMP_SYS_GETPID
#define RUMP_SYS_SOCKET            LUX9_RUMP_SYS_SOCKET
#define RUMP_SYS_BIND              LUX9_RUMP_SYS_BIND
#define RUMP_SYS_CONNECT           LUX9_RUMP_SYS_CONNECT
#define RUMP_SYS_LISTEN            LUX9_RUMP_SYS_LISTEN
#define RUMP_SYS_ACCEPT            LUX9_RUMP_SYS_ACCEPT

/*
 * ============================================================================
 * RUMP SYSCALL VALIDATION
 * 
 * Functions to validate and handle RUMP syscalls.
 * ============================================================================
 */

/* Check if syscall number is a RUMP syscall */
static inline int lux9_rump_syscall_is_valid(uint32_t syscall_num) {
    return (syscall_num >= LUX9_RUMP_FILE_BASE && 
            syscall_num <= LUX9_RUMP_MISC_BASE + 99);
}

/* Get RUMP subsystem from syscall number */
static inline const char *lux9_rump_syscall_subsystem(uint32_t syscall_num) {
    if (syscall_num >= LUX9_RUMP_FILE_BASE && 
        syscall_num < LUX9_RUMP_FILE_BASE + 100) {
        return "File I/O";
    }
    if (syscall_num >= LUX9_RUMP_PROCESS_BASE && 
        syscall_num < LUX9_RUMP_PROCESS_BASE + 100) {
        return "Process Management";
    }
    if (syscall_num >= LUX9_RUMP_NETWORK_BASE && 
        syscall_num < LUX9_RUMP_NETWORK_BASE + 100) {
        return "Network";
    }
    if (syscall_num >= LUX9_RUMP_IPC_BASE && 
        syscall_num < LUX9_RUMP_IPC_BASE + 100) {
        return "IPC";
    }
    if (syscall_num >= LUX9_RUMP_MISC_BASE && 
        syscall_num < LUX9_RUMP_MISC_BASE + 100) {
        return "Miscellaneous";
    }
    return "Unknown";
}

/*
 * ============================================================================
 * RUMP TRANSLATION LAYER
 * 
 * Functions to translate between RUMP and unified syscall numbers.
 * ============================================================================
 */

/* Translate RUMP syscall number to unified number */
int rump_syscall_translate(uint32_t rump_num, uint32_t *unified_num);

/* Translate unified syscall number to RUMP number */
int rump_syscall_untranslate(uint32_t unified_num, uint32_t *rump_num);

/* Check if a syscall needs RUMP translation */
int rump_syscall_needs_translation(uint32_t syscall_num);

/*
 * ============================================================================
 * RUMP HYPERVISOR INTERFACE
 * 
 * Functions for handling RUMP hypervisor calls.
 * ============================================================================
 */

/* Initialize RUMP compatibility layer */
int rump_compat_init(void);

/* Clean up RUMP compatibility layer */
void rump_compat_fini(void);

/* Handle RUMP syscall through hypervisor */
int rump_syscall_hypervisor_call(uint32_t syscall_num, void *args, void *result);

/* Check if hypervisor is available */
int rump_hypervisor_available(void);

/*
 * ============================================================================
 * PERFORMANCE OPTIMIZATION FOR RUMP
 * 
 * Fast path for RUMP syscalls.
 * ============================================================================
 */

/* Fast RUMP syscall check */
static inline int rump_syscall_is_fast(uint32_t syscall_num) {
    /* Fast RUMP syscalls: basic file I/O and process ops */
    return (syscall_num >= LUX9_RUMP_FILE_BASE && 
            syscall_num <= LUX9_RUMP_FILE_BASE + 10) ||
           (syscall_num >= LUX9_RUMP_PROCESS_BASE && 
            syscall_num <= LUX9_RUMP_PROCESS_BASE + 5);
}

/* RUMP syscall performance categorization */
static inline const char *rump_syscall_perf_category(uint32_t syscall_num) {
    if (rump_syscall_is_fast(syscall_num))
        return "Fast";
    if (syscall_num >= LUX9_RUMP_NETWORK_BASE && 
        syscall_num < LUX9_RUMP_NETWORK_BASE + 100) {
        return "Network-I/O";
    }
    if (syscall_num >= LUX9_RUMP_IPC_BASE && 
        syscall_num < LUX9_RUMP_IPC_BASE + 100) {
        return "IPC";
    }
    return "Slow";
}

/*
 * ============================================================================
 * RUMP ERROR HANDLING
 * 
 * Functions to handle RUMP-specific errors.
 * ============================================================================
 */

/* RUMP error codes */
#define LUX9_RUMP_SUCCESS        0
#define LUX9_RUMP_ERROR         -1
#define LUX9_RUMP_NOT_SUPPORTED -2
#define LUX9_RUMP_HYPERVISOR_DOWN -3
#define LUX9_RUMP_INVALID_ARGS  -4

/* Get last RUMP error */
int rump_syscall_get_error(void);

/* Set RUMP error */
void rump_syscall_set_error(int error_code);

/* Convert RUMP error to unified error */
int rump_error_to_unified(int rump_error);

/*
 * ============================================================================
 * RUMP DEBUGGING SUPPORT
 * 
 * Functions for debugging RUMP syscall issues.
 * ============================================================================
 */

/* Enable RUMP syscall tracing */
void rump_syscall_trace_enable(void);
void rump_syscall_trace_disable(void);

/* Check if RUMP tracing is enabled */
int rump_syscall_trace_is_enabled(void);

/* Print RUMP syscall statistics */
void rump_syscall_print_stats(void);

/* Validate RUMP syscall arguments */
int rump_syscall_validate_args(uint32_t syscall_num, void *args);

/*
 * ============================================================================
 * USAGE GUIDELINES FOR RUMP COMPATIBILITY
 * 
 * 1. RUMP syscalls are automatically translated to unified scheme
 * 2. Use RUMP_SYS_* macros for compatibility
 * 3. RUMP hypervisor must be available for full functionality
 * 4. Some RUMP syscalls may not be available (LUX9_RUMP_NOT_SUPPORTED)
 * 5. Use rump_syscall_needs_translation() to check if translation is needed
 * 
 * Example:
 *   // RUMP code
 *   fd = rump_syscall(RUMP_SYS_OPEN, "file.txt", O_RDONLY);
 *   
 *   // This is automatically translated to unified scheme
 *   // No changes needed in user code!
 * ============================================================================
 */

#endif /* _LUX9_SYSCALL_RUMP_COMPAT_H_ */
