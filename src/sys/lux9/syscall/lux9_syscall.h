/*
 * Lux9 Unified System Call Management
 * 
 * This header provides a unified system call interface that resolves conflicts
 * between Plan 9, Lux9 native, and compatibility layer syscalls.
 * 
 * Core Design Principles:
 * - Hierarchical numbering scheme with subsystem separation
 * - Backward compatibility with all existing code
 * - Clear organization and maintainability
 * - Performance optimization for hot paths
 * 
 * Numbering Scheme:
 * - 0-99:   Plan 9 compatibility (direct mapping)
 * - 100-199: Lux9 native syscalls (WASM, Pebble, Exchange Pool)
 * - 200-299: Extended syscalls (future extensions)
 * - 1000+:   Subsystem-specific (reserved)
 * 
 * Version: 1.0
 * Last Updated: 2026-01-12
 */

#ifndef _LUX9_SYSCALL_H_
#define _LUX9_SYSCALL_H_

#include <sys/types.h>
#include <stdint.h>

/*
 * ============================================================================
 * SYSCALL NUMBERING SCHEME
 * ============================================================================
 * 
 * Each subsystem has dedicated ranges to prevent conflicts:
 * 
 * PLAN9_COMPAT (0-99):
 *   Direct mapping to Plan 9 syscall numbers
 *   Used for compatibility with existing Plan 9 code
 * 
 * LUX9_NATIVE (100-199):
 *   Lux9-specific operations
 *   WASM execution, Pebble memory, Exchange Pool IPC
 * 
 * EXTENDED_SYS (200-299):
 *   Future extensions and new features
 *   Available for custom implementations
 * 
 * SUBSYSTEM_SPECIFIC (1000+):
 *   Reserved for subsystem-specific operations
 *   Memory management, networking, security, etc.
 * 
 * COLLISION RESOLUTION:
 *   Original WASM syscalls 63-65 are now mapped to:
 *   - SYS_WASM_COMPILE:   163 (was 63)
 *   - SYS_WASM_EXECUTE:   164 (was 64) 
 *   - SYS_WASM_DESTROY:   165 (was 65)
 * 
 * Legacy constants remain for backward compatibility
 */

/* ============================================================================
 * PLAN 9 COMPATIBILITY LAYER (0-99)
 * 
 * These maintain exact compatibility with Plan 9 syscall numbers.
 * Direct mapping to /src/libc/9syscall/sys.h
 * ============================================================================
 */

#define LUX9_PLAN9_SYSCALL_BASE     0
#define LUX9_PLAN9_SYSCALL_MAX      99

/* Plan 9 File Operations */
#define LUX9_SYS_RSYNC             0   /* SYSR1 - Restart system call */
#define LUX9_SYS_ERRSTR            1   /* _ERRSTR - Return error string */
#define LUX9_SYS_BIND              2   /* BIND - Bind name to file */
#define LUX9_SYS_CHDIR             3   /* CHDIR - Change working directory */
#define LUX9_SYS_CLOSE             4   /* CLOSE - Close file */
#define LUX9_SYS_DUP               5   /* DUP - Duplicate file descriptor */
#define LUX9_SYS_ALARM             6   /* ALARM - Set alarm clock */
#define LUX9_SYS_EXEC              7   /* EXEC - Execute file */
#define LUX9_SYS_EXITS             8   /* EXITS - Exit process */
#define LUX9_SYS_FSESSION          9   /* _FSESSION - Set foreground session */
#define LUX9_SYS_FAUTH             10  /* FAUTH - Authentication */
#define LUX9_SYS_FSTAT             11  /* _FSTAT - Fstat with stat buffer */
#define LUX9_SYS_SEGBRK            12  /* SEGBRK - Set break point */
#define LUX9_SYS_MOUNT             13  /* _MOUNT - Mount file system */
#define LUX9_SYS_OPEN              14  /* OPEN - Open file */
#define LUX9_SYS_READ              15  /* _READ - Read from file */
#define LUX9_SYS_OSEEK             16  /* OSEEK - Old seek (32-bit) */
#define LUX9_SYS_SLEEP             17  /* SLEEP - Sleep for interval */
#define LUX9_SYS_STAT              18  /* _STAT - Stat with stat buffer */
#define LUX9_SYS_RFORK             19  /* RFORK - Split process */
#define LUX9_SYS_WRITE             20  /* _WRITE - Write to file */
#define LUX9_SYS_PIPE              21  /* PIPE - Create pipe */
#define LUX9_SYS_CREATE            22  /* CREATE - Create file */
#define LUX9_SYS_FD2PATH           23  /* FD2PATH - FD to path */
#define LUX9_SYS_BRK               24  /* BRK_ - Set break */
#define LUX9_SYS_REMOVE            25  /* REMOVE - Remove file */
#define LUX9_SYS_WSTAT             26  /* _WSTAT - Write stat */
#define LUX9_SYS_FWSTAT            27  /* _FWSTAT - File write stat */
#define LUX9_SYS_NOTIFY            28  /* NOTIFY - Notify interrupt */
#define LUX9_SYS_NOTED             29  /* NOTED - Acknowledged interrupt */
#define LUX9_SYS_SEGATTACH         30  /* SEGATTACH - Attach segment */
#define LUX9_SYS_SEGDETACH         31  /* SEGDETACH - Detach segment */
#define LUX9_SYS_SEGFREE           32  /* SEGFREE - Free segment */
#define LUX9_SYS_SEGFLUSH          33  /* SEGFLUSH - Flush segment */
#define LUX9_SYS_RENDEZVOUS        34  /* RENDEZVOUS - Rendezvous */
#define LUX9_SYS_UNMOUNT           35  /* UNMOUNT - Unmount file system */
#define LUX9_SYS_WAIT              36  /* _WAIT - Wait for child */
#define LUX9_SYS_SEMACQUIRE        37  /* SEMACQUIRE - Acquire semaphore */
#define LUX9_SYS_SEMRELEASE        38  /* SEMRELEASE - Release semaphore */
#define LUX9_SYS_SEEK              39  /* SEEK - Set file offset */
#define LUX9_SYS_FVERSION          40  /* FVERSION - File protocol version */
#define LUX9_SYS_ERRSTR            41  /* ERRSTR - Return error string */
#define LUX9_SYS_STAT              42  /* STAT - Stat file */
#define LUX9_SYS_FSTAT             43  /* FSTAT - Fstat file descriptor */
#define LUX9_SYS_WSTAT             44  /* WSTAT - Write file stats */
#define LUX9_SYS_FWSTAT            45  /* FWSTAT - File write stats */
#define LUX9_SYS_MOUNT             46  /* MOUNT - Mount file system */
#define LUX9_SYS_AWAIT             47  /* AWAIT - Wait for interrupt */
#define LUX9_SYS_PREAD             50  /* PREAD - Pre-positioned read */
#define LUX9_SYS_PWRITE            51  /* PWRITE - Pre-positioned write */
#define LUX9_SYS_TSEMACQUIRE       52  /* TSEMACQUIRE - Timed semaphore acquire */
#define LUX9_SYS_NSEC              53  /* _NSEC - Nanosecond time */

/* ============================================================================
 * LUX9 NATIVE SYSCALLS (100-199)
 * 
 * Lux9-specific operations that extend Plan 9 functionality.
 * Includes WASM, Pebble memory management, and Exchange Pool IPC.
 * ============================================================================
 */

#define LUX9_NATIVE_SYSCALL_BASE    100
#define LUX9_NATIVE_SYSCALL_MAX     199

/* WASM Subsystem (avoiding collision with legacy 63-65) */
#define LUX9_SYS_WASM_COMPILE      163 /* Compile WASM module */
#define LUX9_SYS_WASM_EXECUTE      164 /* Execute WASM function */
#define LUX9_SYS_WASM_DESTROY      165 /* Destroy WASM instance */

/* Pebble Memory Management */
#define LUX9_SYS_PEBBLE_ALLOC      166 /* Allocate pebble memory */
#define LUX9_SYS_PEBBLE_FREE       167 /* Free pebble memory */
#define LUX9_SYS_PEBBLE_GET        168 /* Get pebble data */
#define LUX9_SYS_PEBBLE_SET        169 /* Set pebble data */
#define LUX9_SYS_PEBBLE_SYNC       170 /* Synchronize pebble state */

/* Exchange Pool IPC */
#define LUX9_SYS_EXCHANGE_ALLOC    171 /* Allocate exchange capability */
#define LUX9_SYS_EXCHANGE_FREE     172 /* Free exchange capability */
#define LUX9_SYS_EXCHANGE_PUBLISH  173 /* Publish to exchange pool */
#define LUX9_SYS_EXCHANGE_SUBSCRIBE 174 /* Subscribe to exchange topic */
#define LUX9_SYS_EXCHANGE_UNSUBSCRIBE 175 /* Unsubscribe from topic */
#define LUX9_SYS_EXCHANGE_RECEIVE  176 /* Receive from exchange pool */
#define LUX9_SYS_EXCHANGE_SEND     177 /* Send to specific capability */
#define LUX9_SYS_EXCHANGE_ACK      178 /* Acknowledge message */

/* Enhanced Process Management */
#define LUX9_SYS_GETPID2           179 /* Get PID with extended info */
#define LUX9_SYS_RFORK_ENHANCED    180 /* Enhanced RFORK with PTE handling */
#define LUX9_SYS_GETPPID2         181 /* Get parent PID with metadata */

/* Advanced I/O Operations */
#define LUX9_SYS_ASYNC_READ        182 /* Asynchronous read */
#define LUX9_SYS_ASYNC_WRITE       183 /* Asynchronous write */
#define LUX9_SYS_IOVEC_READ        184 /* Scatter-gather read */
#define LUX9_SYS_IOVEC_WRITE       185 /* Scatter-gather write */

/* Security and Capabilities */
#define LUX9_SYS_CAP_CREATE        186 /* Create capability */
#define LUX9_SYS_CAP_DESTROY       187 /* Destroy capability */
#define LUX9_SYS_CAP_VALIDATE      188 /* Validate capability */

/* Memory Management Extensions */
#define LUX9_SYS_MEMINFO           189 /* Get memory information */
#define LUX9_SYS_MADVISE           190 /* Memory advice */

/* Process Control Extensions */
#define LUX9_SYS_SCHED_SET         191 /* Set scheduling parameters */
#define LUX9_SYS_SCHED_GET         192 /* Get scheduling parameters */

/* Notification System */
#define LUX9_SYS_NOTIFY_CREATE     193 /* Create notification */
#define LUX9_SYS_NOTIFY_WAIT       194 /* Wait for notification */
#define LUX9_SYS_NOTIFY_SIGNAL     195 /* Send notification */

/* Time and Synchronization */
#define LUX9_SYS_CLOCK_GETTIME     196 /* Get high-resolution time */
#define LUX9_SYS_CLOCK_SETTIME     197 /* Set high-resolution time */
#define LUX9_SYS_CLOCK_NANOSLEEP   198 /* High-resolution sleep */

/* Future Reserved (199) */
#define LUX9_SYS_FUTURE_199        199 /* Reserved for future use */

/* ============================================================================
 * EXTENDED SYSCALLS (200-299)
 * 
 * Available for custom implementations and future extensions.
 * These are not pre-defined and can be used by subsystems as needed.
 * ============================================================================
 */

#define LUX9_EXTENDED_SYSCALL_BASE  200
#define LUX9_EXTENDED_SYSCALL_MAX   299

/* Virtual Machine Extensions (200-220) */
#define LUX9_SYS_VM_CREATE          200 /* Create VM instance */
#define LUX9_SYS_VM_DESTROY          201 /* Destroy VM instance */
#define LUX9_SYS_VM_EXECUTE          202 /* Execute VM code */
#define LUX9_SYS_VM_SUSPEND          203 /* Suspend VM */
#define LUX9_SYS_VM_RESUME           204 /* Resume VM */

/* Container and Isolation (221-240) */
#define LUX9_SYS_CONTAINER_CREATE   221 /* Create container */
#define LUX9_SYS_CONTAINER_DESTROY  222 /* Destroy container */
#define LUX9_SYS_CONTAINER_ENTER    223 /* Enter container namespace */

/* Network Namespace (241-260) */
#define LUX9_SYS_NETNS_CREATE       241 /* Create network namespace */
#define LUX9_SYS_NETNS_ENTER         242 /* Enter network namespace */

/* File System Extensions (261-280) */
#define LUX9_SYS_FS_SNAPSHOT        261 /* Create file system snapshot */
#define LUX9_SYS_FS_DIFF            262 /* Get file system differences */

/* Database and Storage (281-299) */
#define LUX9_SYS_DB_CREATE          281 /* Create database */
#define LUX9_SYS_DB_DESTROY         282 /* Destroy database */
#define LUX9_SYS_DB_QUERY           283 /* Query database */

/* ============================================================================
 * SUBSYSTEM-SPECIFIC SYSCALLS (1000+)
 * 
 * Reserved for subsystem-specific operations.
 * Each subsystem can define its own range within this space.
 * ============================================================================
 */

#define LUX9_SUBSYSTEM_SYSCALL_BASE 1000

/* Memory Management Subsystem (1000-1099) */
#define LUX9_MEM_SYSCALL_BASE       1000
#define LUX9_MEM_SYSCALL_MAX        1099
#define LUX9_SYS_MMAP               1000 /* Memory map */
#define LUX9_SYS_MUNMAP             1001 /* Unmap memory */
#define LUX9_SYS_MPROTECT           1002 /* Set memory protection */

/* Network Subsystem (1100-1199) */
#define LUX9_NET_SYSCALL_BASE       1100
#define LUX9_NET_SYSCALL_MAX        1199
#define LUX9_SYS_SOCKET_CREATE      1100 /* Create socket */
#define LUX9_SYS_SOCKET_BIND        1101 /* Bind socket */
#define LUX9_SYS_SOCKET_LISTEN      1102 /* Listen on socket */

/* Security Subsystem (1200-1299) */
#define LUX9_SEC_SYSCALL_BASE       1200
#define LUX9_SEC_SYSCALL_MAX        1299
#define LUX9_SYS_AUTH_CREATE        1200 /* Create authentication */
#define LUX9_SYS_AUTH_VALIDATE      1201 /* Validate authentication */

/* Graphics Subsystem (1300-1399) */
#define LUX9_GFX_SYSCALL_BASE       1300
#define LUX9_GFX_SYSCALL_MAX        1399
#define LUX9_SYS_GFX_OPEN           1300 /* Open graphics device */
#define LUX9_SYS_GFX_CLOSE          1301 /* Close graphics device */

/* Audio Subsystem (1400-1499) */
#define LUX9_AUDIO_SYSCALL_BASE     1400
#define LUX9_AUDIO_SYSCALL_MAX      1499
#define LUX9_SYS_AUDIO_OPEN         1400 /* Open audio device */
#define LUX9_SYS_AUDIO_CLOSE        1401 /* Close audio device */

/* ============================================================================
 * BACKWARD COMPATIBILITY LAYERS
 * 
 * These maintain compatibility with existing code by providing
 * both old and new syscall numbers.
 * ============================================================================
 */

/* Plan 9 Compatibility Constants */
#define SYSR1              LUX9_SYS_RSYNC
#define _ERRSTR            LUX9_SYS_ERRSTR
#define BIND               LUX9_SYS_BIND
#define CHDIR              LUX9_SYS_CHDIR
#define CLOSE              LUX9_SYS_CLOSE
#define DUP                LUX9_SYS_DUP
#define ALARM              LUX9_SYS_ALARM
#define EXEC               LUX9_SYS_EXEC
#define EXITS              LUX9_SYS_EXITS
#define _FSESSION          LUX9_SYS_FSESSION
#define FAUTH              LUX9_SYS_FAUTH
#define _FSTAT             LUX9_SYS_FSTAT
#define SEGBRK             LUX9_SYS_SEGBRK
#define _MOUNT             LUX9_SYS_MOUNT
#define OPEN               LUX9_SYS_OPEN
#define _READ              LUX9_SYS_READ
#define OSEEK              LUX9_SYS_OSEEK
#define SLEEP              LUX9_SYS_SLEEP
#define _STAT              LUX9_SYS_STAT
#define RFORK              LUX9_SYS_RFORK
#define _WRITE             LUX9_SYS_WRITE
#define PIPE               LUX9_SYS_PIPE
#define CREATE             LUX9_SYS_CREATE
#define FD2PATH            LUX9_SYS_FD2PATH
#define BRK_               LUX9_SYS_BRK
#define REMOVE             LUX9_SYS_REMOVE
#define _WSTAT             LUX9_SYS_WSTAT
#define _FWSTAT            LUX9_SYS_FWSTAT
#define NOTIFY             LUX9_SYS_NOTIFY
#define NOTED              LUX9_SYS_NOTED
#define SEGATTACH          LUX9_SYS_SEGATTACH
#define SEGDETACH          LUX9_SYS_SEGDETACH
#define SEGFREE            LUX9_SYS_SEGFREE
#define SEGFLUSH           LUX9_SYS_SEGFLUSH
#define RENDEZVOUS         LUX9_SYS_RENDEZVOUS
#define UNMOUNT            LUX9_SYS_UNMOUNT
#define _WAIT              LUX9_SYS_WAIT
#define SEMACQUIRE         LUX9_SYS_SEMACQUIRE
#define SEMRELEASE         LUX9_SYS_SEMRELEASE
#define SEEK               LUX9_SYS_SEEK
#define FVERSION           LUX9_SYS_FVERSION
#define ERRSTR             LUX9_SYS_ERRSTR
#define STAT               LUX9_SYS_STAT
#define FSTAT              LUX9_SYS_FSTAT
#define WSTAT              LUX9_SYS_WSTAT
#define FWSTAT             LUX9_SYS_FWSTAT
#define MOUNT              LUX9_SYS_MOUNT
#define AWAIT              LUX9_SYS_AWAIT
#define PREAD              LUX9_SYS_PREAD
#define PWRITE             LUX9_SYS_PWRITE
#define TSEMACQUIRE        LUX9_SYS_TSEMACQUIRE
#define _NSEC              LUX9_SYS_NSEC

/* Lux9 Legacy Compatibility (Original Numbers) */
#define SYS_WASM_COMPILE      LUX9_SYS_WASM_COMPILE
#define SYS_WASM_EXECUTE      LUX9_SYS_WASM_EXECUTE  
#define SYS_WASM_DESTROY      LUX9_SYS_WASM_DESTROY
#define SYS_GETPID2           LUX9_SYS_GETPID2
#define SYS_EXCHANGE_ALLOC    LUX9_SYS_EXCHANGE_ALLOC
#define SYS_EXCHANGE_FREE     LUX9_SYS_EXCHANGE_FREE
#define SYS_EXCHANGE_PUBLISH  LUX9_SYS_EXCHANGE_PUBLISH
#define SYS_EXCHANGE_SUBSCRIBE LUX9_SYS_EXCHANGE_SUBSCRIBE
#define SYS_EXCHANGE_UNSUBSCRIBE LUX9_SYS_EXCHANGE_UNSUBSCRIBE
#define SYS_EXCHANGE_RECEIVE  LUX9_SYS_EXCHANGE_RECEIVE
#define SYS_WAIT              LUX9_SYS_WAIT

/* ============================================================================
 * SYSCALL ATTRIBUTE MACROS
 * 
 * These help the compiler and tools understand syscall usage patterns.
 * ============================================================================
 */

/* Hot path optimization hint */
#define LUX9_SYSCALL_HOT       __attribute__((hot))
#define LUX9_SYSCALL_COLD      __attribute__((cold))

/* Syscall classification */
#define LUX9_SYSCALL_COMPAT    (1 << 0)  /* Plan 9 compatibility */
#define LUX9_SYSCALL_NATIVE    (1 << 1)  /* Lux9 native */
#define LUX9_SYSCALL_EXTENDED  (1 << 2)  /* Extended functionality */
#define LUX9_SYSCALL_SUBSYSTEM (1 << 3)  /* Subsystem-specific */

/* Performance characteristics */
#define LUX9_SYSCALL_FAST      (1 << 8)  /* Fast syscall (< 1µs) */
#define LUX9_SYSCALL_NORMAL     (1 << 9)  /* Normal speed */
#define LUX9_SYSCALL_SLOW      (1 << 10) /* Slow syscall (> 10µs) */

/* ============================================================================
 * SYSCALL VALIDATION MACROS
 * 
 * These help validate syscall numbers at compile time.
 * ============================================================================
 */

#define LUX9_SYSCALL_IS_COMPAT(n) \
    ((n) >= LUX9_PLAN9_SYSCALL_BASE && (n) <= LUX9_PLAN9_SYSCALL_MAX)

#define LUX9_SYSCALL_IS_NATIVE(n) \
    ((n) >= LUX9_NATIVE_SYSCALL_BASE && (n) <= LUX9_NATIVE_SYSCALL_MAX)

#define LUX9_SYSCALL_IS_EXTENDED(n) \
    ((n) >= LUX9_EXTENDED_SYSCALL_BASE && (n) <= LUX9_EXTENDED_SYSCALL_MAX)

#define LUX9_SYSCALL_IS_SUBSYSTEM(n) \
    ((n) >= LUX9_SUBSYSTEM_SYSCALL_BASE)

#define LUX9_SYSCALL_IS_VALID(n) \
    (LUX9_SYSCALL_IS_COMPAT(n) || LUX9_SYSCALL_IS_NATIVE(n) || \
     LUX9_SYSCALL_IS_EXTENDED(n) || LUX9_SYSCALL_IS_SUBSYSTEM(n))

/* ============================================================================
 * FUNCTION PROTOTYPES
 * 
 * Core syscall dispatch and management functions.
 * ============================================================================
 */

/* Dispatcher function type */
typedef int (*lux9_syscall_handler_t)(uintptr_t args[], size_t arg_count, 
                                    uintptr_t *result);

/* Syscall registration structure */
struct lux9_syscall_entry {
    uint32_t syscall_number;
    const char *syscall_name;
    lux9_syscall_handler_t handler;
    uint32_t flags;
    uint32_t subsystem_id;
    const char *description;
};

/* Core syscall management */
int lux9_syscall_register(const struct lux9_syscall_entry *entry);
int lux9_syscall_unregister(uint32_t syscall_number);
lux9_syscall_handler_t lux9_syscall_get_handler(uint32_t syscall_number);
const char *lux9_syscall_get_name(uint32_t syscall_number);
int lux9_syscall_dispatch(uint32_t syscall_number, uintptr_t args[], 
                         size_t arg_count, uintptr_t *result);

/* Compatibility layer management */
int lux9_compat_add_mapping(uint32_t old_number, uint32_t new_number);
int lux9_compat_remove_mapping(uint32_t old_number);
int lux9_compat_translate(uint32_t old_number, uint32_t *new_number);

/* Subsystem management */
int lux9_subsystem_register(uint32_t base_number, uint32_t count,
                           const char *subsystem_name);
int lux9_subsystem_unregister(uint32_t base_number);
const char *lux9_subsystem_get_name(uint32_t syscall_number);

/* Validation and statistics */
int lux9_syscall_validate_number(uint32_t syscall_number);
int lux9_syscall_get_stats(uint32_t syscall_number, void *stats);
void lux9_syscall_print_table(void);

/* ============================================================================
 * INLINE HELPER FUNCTIONS
 * 
 * Optimized helpers for common operations.
 * ============================================================================
 */

/* Fast syscall number validation */
static inline int lux9_syscall_is_valid_fast(uint32_t syscall_number) {
    return syscall_number <= LUX9_EXTENDED_SYSCALL_MAX || 
           syscall_number >= LUX9_SUBSYSTEM_SYSCALL_BASE;
}

/* Get subsystem ID from syscall number */
static inline uint32_t lux9_syscall_get_subsystem(uint32_t syscall_number) {
    if (LUX9_SYSCALL_IS_COMPAT(syscall_number))
        return 'P9  '; /* Plan 9 */
    if (LUX9_SYSCALL_IS_NATIVE(syscall_number))
        return 'LUX9'; /* Lux9 native */
    if (LUX9_SYSCALL_IS_EXTENDED(syscall_number))
        return 'EXT '; /* Extended */
    if (LUX9_SYSCALL_IS_SUBSYSTEM(syscall_number))
        return 'SUB '; /* Subsystem-specific */
    return 0;
}

/* Get syscall category name */
static inline const char *lux9_syscall_get_category(uint32_t syscall_number) {
    if (LUX9_SYSCALL_IS_COMPAT(syscall_number))
        return "Plan9-Compatible";
    if (LUX9_SYSCALL_IS_NATIVE(syscall_number))
        return "Lux9-Native";
    if (LUX9_SYSCALL_IS_EXTENDED(syscall_number))
        return "Extended";
    if (LUX9_SYSCALL_IS_SUBSYSTEM(syscall_number))
        return "Subsystem-Specific";
    return "Unknown";
}

/* ============================================================================
 * USAGE GUIDELINES
 * 
 * 1. For Plan 9 compatibility: Use LUX9_SYS_* constants
 * 2. For Lux9 native features: Use native syscall numbers (100+)
 * 3. For new features: Use extended range (200+) or subsystem ranges
 * 4. Always validate syscall numbers with LUX9_SYSCALL_IS_VALID()
 * 5. Use the compatibility macros for backward compatibility
 * 
 * Example:
 *   // Old code (still works)
 *   syscall(PREAD, fd, buffer, count, offset);
 *   
 *   // New recommended way
 *   syscall(LUX9_SYS_PREAD, fd, buffer, count, offset);
 *   
 *   // Using compatibility layer
 *   syscall(PREAD, fd, buffer, count, offset);  // Still works!
 * ============================================================================
 */

#endif /* _LUX9_SYSCALL_H_ */
