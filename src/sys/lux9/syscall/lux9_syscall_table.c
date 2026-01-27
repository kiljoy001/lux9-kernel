/*
 * Lux9 System Call Table
 * 
 * This module defines the complete syscall table mapping syscall numbers
 * to handlers and metadata.
 */

#include "lux9_syscall.h"
#include "compat_plan9.h"
#include "compat_lux9_native.h"
#include <stdio.h>

/* ============================================================================
 * SYSCALL TABLE DEFINITION
 * ============================================================================
 */

/* Plan 9 compatibility syscall entries */
#define SYSCALL_ENTRY(num, name, handler, flags) \
    [num] = { \
        .syscall_number = num, \
        .syscall_name = #name, \
        .handler = handler, \
        .flags = flags, \
        .subsystem_id = 'P9  ', \
        .description = "Plan 9 Compatible" \
    }

/* Lux9 native syscall entries */
#define LUX9_SYSCALL_ENTRY(num, name, handler, flags, desc) \
    [num] = { \
        .syscall_number = num, \
        .syscall_name = #name, \
        .handler = handler, \
        .flags = flags, \
        .subsystem_id = 'LUX9', \
        .description = desc \
    }

/* Extended syscall entries */
#define EXT_SYSCALL_ENTRY(num, name, handler, flags, desc) \
    [num] = { \
        .syscall_number = num, \
        .syscall_name = #name, \
        .handler = handler, \
        .flags = flags, \
        .subsystem_id = 'EXT ', \
        .description = desc \
    }

/* ============================================================================
 * SYSCALL HANDLER DECLARATIONS
 * 
 * Forward declarations of syscall handler functions.
 * ============================================================================
 */

/* Plan 9 compatibility handlers */
int plan9_read_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_write_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_open_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_close_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_pipe_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_fork_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_wait_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_exec_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_exit_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_dup_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_bind_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_mount_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_unmount_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_seek_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_stat_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_fstat_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int plan9_brk_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);

/* Lux9 native handlers */
int lux9_wasm_compile_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_wasm_execute_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_wasm_destroy_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_exchange_alloc_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_exchange_free_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_exchange_publish_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_exchange_subscribe_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_exchange_receive_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_pebble_alloc_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_pebble_free_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_getpid2_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);

/* Extended handlers */
int lux9_vm_create_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_container_create_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);

/* Memory subsystem handlers */
int lux9_mmap_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_munmap_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_mprotect_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_malloc_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);
int lux9_free_handler(uintptr_t args[], size_t arg_count, uintptr_t *result);

/* ============================================================================
 * COMPLETE SYSCALL TABLE
 * ============================================================================
 */

struct lux9_syscall_entry lux9_syscall_table[LUX9_SYSCALL_MAX + 1] = {
    /* Plan 9 Compatibility Layer (0-99) */
    SYSCALL_ENTRY(0,  RSYNC,          plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Restart system call"),
    SYSCALL_ENTRY(1,  ERRSTR,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Return error string"),
    SYSCALL_ENTRY(2,  BIND,          plan9_bind_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Bind name to file"),
    SYSCALL_ENTRY(3,  CHDIR,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Change directory"),
    SYSCALL_ENTRY(4,  CLOSE,         plan9_close_handler,   LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Close file"),
    SYSCALL_ENTRY(5,  DUP,           plan9_dup_handler,     LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Duplicate file descriptor"),
    SYSCALL_ENTRY(6,  ALARM,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Set alarm"),
    SYSCALL_ENTRY(7,  EXEC,          plan9_exec_handler,     LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_SLOW,   "Execute file"),
    SYSCALL_ENTRY(8,  EXITS,         plan9_exit_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Exit process"),
    SYSCALL_ENTRY(9,  FSESSION,      plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Set session"),
    SYSCALL_ENTRY(10, FAUTH,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Authentication"),
    SYSCALL_ENTRY(11, FSTAT,         plan9_fstat_handler,   LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Fstat with buffer"),
    SYSCALL_ENTRY(12, SEGBRK,        plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Set break"),
    SYSCALL_ENTRY(13, MOUNT,         plan9_mount_handler,   LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_SLOW,   "Mount filesystem"),
    SYSCALL_ENTRY(14, OPEN,          plan9_open_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Open file"),
    SYSCALL_ENTRY(15, READ,          plan9_read_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Read from file"),
    SYSCALL_ENTRY(16, OSEEK,         plan9_seek_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Old seek"),
    SYSCALL_ENTRY(17, SLEEP,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Sleep"),
    SYSCALL_ENTRY(18, STAT,          plan9_stat_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Stat file"),
    SYSCALL_ENTRY(19, RFORK,         plan9_fork_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_SLOW,   "Fork process"),
    SYSCALL_ENTRY(20, WRITE,         plan9_write_handler,   LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Write to file"),
    SYSCALL_ENTRY(21, PIPE,          plan9_pipe_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Create pipe"),
    SYSCALL_ENTRY(22, CREATE,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Create file"),
    SYSCALL_ENTRY(23, FD2PATH,       plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "FD to path"),
    SYSCALL_ENTRY(24, BRK,           plan9_brk_handler,      LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Set break"),
    SYSCALL_ENTRY(25, REMOVE,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Remove file"),
    SYSCALL_ENTRY(26, WSTAT,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Write stat"),
    SYSCALL_ENTRY(27, FWSTAT,        plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "File write stat"),
    SYSCALL_ENTRY(28, NOTIFY,        plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Notify"),
    SYSCALL_ENTRY(29, NOTED,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Noted"),
    SYSCALL_ENTRY(30, SEGATTACH,     plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Attach segment"),
    SYSCALL_ENTRY(31, SEGDETACH,     plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Detach segment"),
    SYSCALL_ENTRY(32, SEGFREE,       plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Free segment"),
    SYSCALL_ENTRY(33, SEGFLUSH,      plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Flush segment"),
    SYSCALL_ENTRY(34, RENDEZVOUS,    plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Rendezvous"),
    SYSCALL_ENTRY(35, UNMOUNT,       plan9_unmount_handler, LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Unmount filesystem"),
    SYSCALL_ENTRY(36, WAIT,          plan9_wait_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Wait for child"),
    SYSCALL_ENTRY(37, SEMACQUIRE,    plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Acquire semaphore"),
    SYSCALL_ENTRY(38, SEMRELEASE,    plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Release semaphore"),
    SYSCALL_ENTRY(39, SEEK,          plan9_seek_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Set offset"),
    SYSCALL_ENTRY(40, FVERSION,      plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "File version"),
    SYSCALL_ENTRY(41, ERRSTR,        plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Error string"),
    SYSCALL_ENTRY(42, STAT,          plan9_stat_handler,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Stat"),
    SYSCALL_ENTRY(43, FSTAT,         plan9_fstat_handler,   LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Fstat"),
    SYSCALL_ENTRY(44, WSTAT,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Write stat"),
    SYSCALL_ENTRY(45, FWSTAT,        plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "File write stat"),
    SYSCALL_ENTRY(46, MOUNT,         plan9_mount_handler,   LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_SLOW,   "Mount"),
    SYSCALL_ENTRY(47, AWAIT,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NORMAL, "Await"),
    SYSCALL_ENTRY(50, PREAD,         plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Pre-positioned read"),
    SYSCALL_ENTRY(51, PWRITE,        plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Pre-positioned write"),
    SYSCALL_ENTRY(52, TSEMACQUIRE,   plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Timed semaphore"),
    SYSCALL_ENTRY(53, NSEC,          plan9_syscall_null,    LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_FAST,   "Nanosecond time"),

    /* Lux9 Native Layer (100-199) */
    LUX9_SYSCALL_ENTRY(163, WASM_COMPILE,     lux9_wasm_compile_handler,   LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_NORMAL, "WASM Module Compilation"),
    LUX9_SYSCALL_ENTRY(164, WASM_EXECUTE,     lux9_wasm_execute_handler,   LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_FAST,   "WASM Function Execution"),
    LUX9_SYSCALL_ENTRY(165, WASM_DESTROY,     lux9_wasm_destroy_handler,   LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_FAST,   "WASM Instance Cleanup"),
    LUX9_SYSCALL_ENTRY(166, PEBBLE_ALLOC,     lux9_pebble_alloc_handler,   LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_FAST,   "Pebble Memory Allocation"),
    LUX9_SYSCALL_ENTRY(167, PEBBLE_FREE,      lux9_pebble_free_handler,    LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_FAST,   "Pebble Memory Deallocation"),
    LUX9_SYSCALL_ENTRY(171, EXCHANGE_ALLOC,   lux9_exchange_alloc_handler, LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_NORMAL, "Exchange Capability Allocation"),
    LUX9_SYSCALL_ENTRY(172, EXCHANGE_FREE,    lux9_exchange_free_handler,  LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_FAST,   "Exchange Capability Free"),
    LUX9_SYSCALL_ENTRY(173, EXCHANGE_PUBLISH, lux9_exchange_publish_handler, LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_SLOW, "Exchange Message Publish"),
    LUX9_SYSCALL_ENTRY(174, EXCHANGE_SUBSCRIBE, lux9_exchange_subscribe_handler, LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_NORMAL, "Exchange Subscribe"),
    LUX9_SYSCALL_ENTRY(176, EXCHANGE_RECEIVE, lux9_exchange_receive_handler, LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_SLOW, "Exchange Message Receive"),
    LUX9_SYSCALL_ENTRY(179, GETPID2,         lux9_getpid2_handler,        LUX9_SYSCALL_NATIVE | LUX9_SYSCALL_FAST,   "Enhanced PID Information"),

    /* Extended Layer (200-299) - Examples */
    EXT_SYSCALL_ENTRY(200, VM_CREATE,        lux9_vm_create_handler,        LUX9_SYSCALL_EXTENDED | LUX9_SYSCALL_SLOW,  "Virtual Machine Creation"),
    EXT_SYSCALL_ENTRY(221, CONTAINER_CREATE,  lux9_container_create_handler, LUX9_SYSCALL_EXTENDED | LUX9_SYSCALL_SLOW,  "Container Creation"),

    /* Memory Subsystem (1000-1099) */
    [1000] = {  /* LUX9_SYS_MMAP */ 
        .syscall_number = 1000,
        .syscall_name = "MMAP",
        .handler = lux9_mmap_handler,
        .flags = LUX9_SYSCALL_SUBSYSTEM | LUX9_SYSCALL_NORMAL,
        .subsystem_id = 'MEM ',
        .description = "Memory Mapping"
    },
    [1001] = {  /* LUX9_SYS_MUNMAP */
        .syscall_number = 1001,
        .syscall_name = "MUNMAP",
        .handler = lux9_munmap_handler,
        .flags = LUX9_SYSCALL_SUBSYSTEM | LUX9_SYSCALL_FAST,
        .subsystem_id = 'MEM ',
        .description = "Memory Unmapping"
    },
    [1002] = {  /* LUX9_SYS_MPROTECT */
        .syscall_number = 1002,
        .syscall_name = "MPROTECT",
        .handler = lux9_mprotect_handler,
        .flags = LUX9_SYSCALL_SUBSYSTEM | LUX9_SYSCALL_FAST,
        .subsystem_id = 'MEM ',
        .description = "Memory Protection"
    },
    [1011] = {  /* LUX9_SYS_MALLOC */
        .syscall_number = 1011,
        .syscall_name = "MALLOC",
        .handler = lux9_malloc_handler,
        .flags = LUX9_SYSCALL_SUBSYSTEM | LUX9_SYSCALL_FAST,
        .subsystem_id = 'MEM ',
        .description = "Memory Allocation"
    },
    [1012] = {  /* LUX9_SYS_FREE */
        .syscall_number = 1012,
        .syscall_name = "FREE",
        .handler = lux9_free_handler,
        .flags = LUX9_SYSCALL_SUBSYSTEM | LUX9_SYSCALL_FAST,
        .subsystem_id = 'MEM ',
        .description = "Memory Deallocation"
    },
};

/* ============================================================================
 * SYSCALL TABLE UTILITIES
 * ============================================================================
 */

/* Get syscall entry by number */
struct lux9_syscall_entry *lux9_syscall_get_entry(uint32_t syscall_number) {
    if (syscall_number > LUX9_SYSCALL_MAX) {
        return NULL;
    }
    return &lux9_syscall_table[syscall_number];
}

/* Check if syscall is registered */
int lux9_syscall_is_registered(uint32_t syscall_number) {
    struct lux9_syscall_entry *entry = lux9_syscall_get_entry(syscall_number);
    return entry && entry->handler != NULL;
}

/* Register all syscalls in table */
int lux9_syscall_register_table(void) {
    int registered = 0;
    
    for (uint32_t i = 0; i <= LUX9_SYSCALL_MAX; i++) {
        if (lux9_syscall_table[i].handler != NULL) {
            if (lux9_syscall_register(&lux9_syscall_table[i]) == 0) {
                registered++;
            }
        }
    }
    
    return registered;
}

/* Unregister all syscalls in table */
void lux9_syscall_unregister_table(void) {
    for (uint32_t i = 0; i <= LUX9_SYSCALL_MAX; i++) {
        if (lux9_syscall_table[i].handler != NULL) {
            lux9_syscall_unregister(i);
        }
    }
}

/* Find syscall by name */
struct lux9_syscall_entry *lux9_syscall_find_by_name(const char *name) {
    if (!name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i <= LUX9_SYSCALL_MAX; i++) {
        if (lux9_syscall_table[i].handler != NULL &&
            lux9_syscall_table[i].syscall_name != NULL &&
            strcmp(lux9_syscall_table[i].syscall_name, name) == 0) {
            return &lux9_syscall_table[i];
        }
    }
    
    return NULL;
}

/* Get syscall statistics */
int lux9_syscall_table_get_stats(uint64_t *total_registered, 
                                 uint64_t *compat_count,
                                 uint64_t *native_count,
                                 uint64_t *extended_count,
                                 uint64_t *subsystem_count) {
    if (!total_registered) {
        return -1;
    }
    
    *total_registered = 0;
    *compat_count = 0;
    *native_count = 0;
    *extended_count = 0;
    *subsystem_count = 0;
    
    for (uint32_t i = 0; i <= LUX9_SYSCALL_MAX; i++) {
        if (lux9_syscall_table[i].handler != NULL) {
            (*total_registered)++;
            
            if (lux9_syscall_table[i].flags & LUX9_SYSCALL_COMPAT) {
                (*compat_count)++;
            }
            if (lux9_syscall_table[i].flags & LUX9_SYSCALL_NATIVE) {
                (*native_count)++;
            }
            if (lux9_syscall_table[i].flags & LUX9_SYSCALL_EXTENDED) {
                (*extended_count)++;
            }
            if (lux9_syscall_table[i].flags & LUX9_SYSCALL_SUBSYSTEM) {
                (*subsystem_count)++;
            }
        }
    }
    
    return 0;
}

/* Print syscall table summary */
void lux9_syscall_table_print_summary(void) {
    uint64_t total, compat, native, extended, subsystem;
    
    if (lux9_syscall_table_get_stats(&total, &compat, &native, &extended, &subsystem) == 0) {
        printf("Lux9 Syscall Table Summary:\n");
        printf("==========================\n");
        printf("Total registered:    %lu\n", total);
        printf("Plan 9 compatible:   %lu\n", compat);
        printf("Lux9 native:         %lu\n", native);
        printf("Extended:            %lu\n", extended);
        printf("Subsystem:           %lu\n", subsystem);
        printf("\n");
    }
}

/* Validate syscall table */
int lux9_syscall_table_validate(void) {
    int errors = 0;
    
    for (uint32_t i = 0; i <= LUX9_SYSCALL_MAX; i++) {
        struct lux9_syscall_entry *entry = &lux9_syscall_table[i];
        
        /* Skip empty entries */
        if (entry->handler == NULL) {
            continue;
        }
        
        /* Check syscall number matches index */
        if (entry->syscall_number != i) {
            fprintf(stderr, "Syscall %u: number mismatch (%u)\n", i, entry->syscall_number);
            errors++;
        }
        
        /* Check for valid flags */
        if (!(entry->flags & (LUX9_SYSCALL_COMPAT | LUX9_SYSCALL_NATIVE | 
                             LUX9_SYSCALL_EXTENDED | LUX9_SYSCALL_SUBSYSTEM))) {
            fprintf(stderr, "Syscall %u (%s): no category flags set\n", 
                    i, entry->syscall_name ?: "UNKNOWN");
            errors++;
        }
        
        /* Check subsystem ID consistency */
        if ((entry->flags & LUX9_SYSCALL_COMPAT) && entry->subsystem_id != 'P9  ') {
            fprintf(stderr, "Syscall %u (%s): Plan 9 compat with wrong subsystem ID\n",
                    i, entry->syscall_name ?: "UNKNOWN");
            errors++;
        }
        
        if ((entry->flags & LUX9_SYSCALL_NATIVE) && entry->subsystem_id != 'LUX9') {
            fprintf(stderr, "Syscall %u (%s): Lux9 native with wrong subsystem ID\n",
                    i, entry->syscall_name ?: "UNKNOWN");
            errors++;
        }
    }
    
    return errors == 0 ? 0 : -1;
}
