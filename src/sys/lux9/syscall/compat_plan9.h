/*
 * Plan 9 Compatibility Layer
 * 
 * This header provides full backward compatibility with Plan 9 syscall
 * definitions from /src/libc/9syscall/sys.h
 * 
 * It maps Plan 9 syscalls to the unified numbering scheme while maintaining
 * exact compatibility with existing Plan 9 code.
 */

#ifndef _LUX9_SYSCALL_PLAN9_COMPAT_H_
#define _LUX9_SYSCALL_PLAN9_COMPAT_H_

#include "lux9_syscall.h"

/*
 * ============================================================================
 * PLAN 9 SYSCALL NUMBER TRANSLATION
 * 
 * Direct mapping from Plan 9 syscall numbers to unified scheme.
 * These maintain exact numeric compatibility for existing code.
 * ============================================================================
 */

/* Plan 9 syscall numbers (direct mapping) */
#define SYSR1         LUX9_SYS_RSYNC
#define _ERRSTR       LUX9_SYS_ERRSTR
#define BIND          LUX9_SYS_BIND
#define CHDIR         LUX9_SYS_CHDIR
#define CLOSE         LUX9_SYS_CLOSE
#define DUP           LUX9_SYS_DUP
#define ALARM         LUX9_SYS_ALARM
#define EXEC          LUX9_SYS_EXEC
#define EXITS         LUX9_SYS_EXITS
#define _FSESSION     LUX9_SYS_FSESSION
#define FAUTH         LUX9_SYS_FAUTH
#define _FSTAT        LUX9_SYS_FSTAT
#define SEGBRK        LUX9_SYS_SEGBRK
#define _MOUNT        LUX9_SYS_MOUNT
#define OPEN          LUX9_SYS_OPEN
#define _READ         LUX9_SYS_READ
#define OSEEK         LUX9_SYS_OSEEK
#define SLEEP         LUX9_SYS_SLEEP
#define _STAT         LUX9_SYS_STAT
#define RFORK         LUX9_SYS_RFORK
#define _WRITE        LUX9_SYS_WRITE
#define PIPE          LUX9_SYS_PIPE
#define CREATE        LUX9_SYS_CREATE
#define FD2PATH       LUX9_SYS_FD2PATH
#define BRK_          LUX9_SYS_BRK
#define REMOVE        LUX9_SYS_REMOVE
#define _WSTAT        LUX9_SYS_WSTAT
#define _FWSTAT       LUX9_SYS_FWSTAT
#define NOTIFY        LUX9_SYS_NOTIFY
#define NOTED         LUX9_SYS_NOTED
#define SEGATTACH     LUX9_SYS_SEGATTACH
#define SEGDETACH     LUX9_SYS_SEGDETACH
#define SEGFREE       LUX9_SYS_SEGFREE
#define SEGFLUSH      LUX9_SYS_SEGFLUSH
#define RENDEZVOUS    LUX9_SYS_RENDEZVOUS
#define UNMOUNT       LUX9_SYS_UNMOUNT
#define _WAIT         LUX9_SYS_WAIT
#define SEMACQUIRE    LUX9_SYS_SEMACQUIRE
#define SEMRELEASE    LUX9_SYS_SEMRELEASE
#define SEEK          LUX9_SYS_SEEK
#define FVERSION      LUX9_SYS_FVERSION
#define ERRSTR        LUX9_SYS_ERRSTR
#define STAT          LUX9_SYS_STAT
#define FSTAT         LUX9_SYS_FSTAT
#define WSTAT         LUX9_SYS_WSTAT
#define FWSTAT        LUX9_SYS_FWSTAT
#define MOUNT         LUX9_SYS_MOUNT
#define AWAIT         LUX9_SYS_AWAIT
#define PREAD         LUX9_SYS_PREAD
#define PWRITE        LUX9_SYS_PWRITE
#define TSEMACQUIRE   LUX9_SYS_TSEMACQUIRE
#define _NSEC         LUX9_SYS_NSEC

/*
 * ============================================================================
 * PLAN 9 TYPE DEFINITIONS
 * 
 * Compatible type definitions for Plan 9 syscall handling.
 * ============================================================================
 */

/* Plan 9 syscall return type */
typedef intptr_t plan9_syscall_result_t;

/* Plan 9 argument structure */
struct plan9_syscall_args {
    uintptr_t args[6];  /* Maximum 6 arguments for Plan 9 syscalls */
    size_t arg_count;
};

/* Plan 9 syscall entry point */
typedef plan9_syscall_result_t (*plan9_syscall_handler_t)(
    struct plan9_syscall_args *args);

/*
 * ============================================================================
 * PLAN 9 DISPATCHER INTERFACE
 * 
 * Functions for handling Plan 9 compatibility.
 * ============================================================================
 */

/* Register Plan 9 syscall handler */
int plan9_syscall_register(uint32_t syscall_num, plan9_syscall_handler_t handler);

/* Handle Plan 9 syscall dispatch */
int plan9_syscall_dispatch(uint32_t syscall_num, struct plan9_syscall_args *args,
                         plan9_syscall_result_t *result);

/* Validate Plan 9 syscall number */
int plan9_syscall_is_valid(uint32_t syscall_num);

/* Get Plan 9 syscall name */
const char *plan9_syscall_get_name(uint32_t syscall_num);

/* ============================================================================
 * PERFORMANCE OPTIMIZATION
 * 
 * Fast path for Plan 9 compatibility layer.
 * ============================================================================
 */

/* Fast Plan 9 syscall validation */
static inline int plan9_syscall_is_fast_path(uint32_t syscall_num) {
    return syscall_num <= 53;  /* Most commonly used Plan 9 syscalls */
}

/* Plan 9 syscall categorization */
static inline const char *plan9_syscall_category(uint32_t syscall_num) {
    if (syscall_num <= 10) return "File I/O";
    if (syscall_num <= 20) return "Process Management";
    if (syscall_num <= 30) return "Memory Management";
    if (syscall_num <= 40) return "File System";
    if (syscall_num <= 50) return "Advanced I/O";
    return "Specialized";
}

#endif /* _LUX9_SYSCALL_PLAN9_COMPAT_H_ */
