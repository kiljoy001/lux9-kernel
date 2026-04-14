/*
 * Lux9 Unified System Call Interface - Kernel Integration
 * 
 * This header integrates the unified syscall system into the kernel.
 * It resolves conflicts between Plan 9, WASM, and Pebble syscalls.
 * 
 * Version: 1.0
 * Last Updated: 2026-01-12
 */

#ifndef _LUX9_SYSCALL_UNIFIED_H_
#define _LUX9_SYSCALL_UNIFIED_H_

#include "u.h"
#include "fcall.h"

/*
 * ============================================================================
 * UNIFIED SYSCALL ARCHITECTURE
 * ============================================================================
 * 
 * CRITICAL CONFLICT RESOLUTION:
 * - WASM syscalls 63-65 conflicted with Pebble arena syscalls
 * - Solution: Move WASM syscalls to new range 160-162
 * - All legacy constants work through compatibility macros
 * 
 * Numbering Scheme:
 * - 0-99:    Plan 9 compatibility (direct mapping)
 * - 160-162: WASM runtime (was 63-65, now 160-162)
 * - 170-175: Pebble arena memory management
 * - 180-185: Exchange IPC system
 * - 186+:    Extended operations
 */

// Architecture constants for syscall classification
#define LUX9_SYSCALL_ARCH_MASK    0xF0000000  // Bits 28-31: Architecture
#define LUX9_SYSCALL_SUBSYS_MASK  0x0F000000  // Bits 24-27: Subsystem  
#define LUX9_SYSCALL_FUNC_MASK    0x00FFFFFF  // Bits 0-23: Function

// Architecture namespaces
#define LUX9_ARCH_PLAN9           0x10000000  // Plan 9 compatibility
#define LUX9_ARCH_LUX9_NATIVE     0x20000000  // Lux9 native operations
#define LUX9_ARCH_RUMP            0x30000000  // RUMP/NetBSD compatibility

// Subsystem namespaces
#define LUX9_SUBSYS_CORE          0x01000000  // Core file/proc operations
#define LUX9_SUBSYS_WASM          0x02000000  // WebAssembly runtime
#define LUX9_SUBSYS_PEBBLE        0x03000000  // Pebble arena memory
#define LUX9_SUBSYS_EXCHANGE      0x04000000  // Exchange IPC system

/*
 * ============================================================================
 * UNIFIED SYSCALL DEFINITIONS
 * ============================================================================
 * 
 * CRITICAL: WASM syscalls moved from 63-65 to 160-162 to resolve conflicts
 */

// Plan 9 Compatibility Layer (0-99)
// Direct mapping to existing Plan 9 syscalls for compatibility
#define LUX9_SYS_OPEN             (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 14)
#define LUX9_SYS_CLOSE            (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 2)
#define LUX9_SYS_READ             (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 15)
#define LUX9_SYS_WRITE            (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 20)
#define LUX9_SYS_CREATE           (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 22)
#define LUX9_SYS_RFORK            (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 19)
#define LUX9_SYS_PIPE             (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 21)
#define LUX9_SYS_EXEC             (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 7)
#define LUX9_SYS_WAIT             (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 166)

// Lux9 Native Operations
// WASM Runtime (160-162) - RESOLVED CONFLICT: moved from 63-65
#define LUX9_SYS_WASM_COMPILE     (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_WASM | 160)
#define LUX9_SYS_WASM_EXECUTE     (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_WASM | 161)
#define LUX9_SYS_WASM_DESTROY     (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_WASM | 162)

// Pebble Arena (170-175)
#define LUX9_SYS_PEBBLE_BLACK_ALLOC   (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_PEBBLE | 170)
#define LUX9_SYS_PEBBLE_BLACK_FREE    (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_PEBBLE | 171)
#define LUX9_SYS_PEBBLE_WHITE_ISSUE  (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_PEBBLE | 172)
#define LUX9_SYS_PEBBLE_WHITE_VERIFY (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_PEBBLE | 173)
#define LUX9_SYS_PEBBLE_RED_COPY     (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_PEBBLE | 174)
#define LUX9_SYS_PEBBLE_BLUE_DISCARD (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_PEBBLE | 175)

// Exchange IPC (180-185)
#define LUX9_SYS_EXCHANGE_ALLOC       (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_EXCHANGE | 180)
#define LUX9_SYS_EXCHANGE_FREE        (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_EXCHANGE | 181)
#define LUX9_SYS_EXCHANGE_PUBLISH    (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_EXCHANGE | 182)
#define LUX9_SYS_EXCHANGE_SUBSCRIBE  (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_EXCHANGE | 183)
#define LUX9_SYS_EXCHANGE_UNSUBSCRIBE (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_EXCHANGE | 184)
#define LUX9_SYS_EXCHANGE_RECEIVE    (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_EXCHANGE | 185)

// Process and Memory Management
#define LUX9_SYS_GETPID2             (LUX9_ARCH_LUX9_NATIVE | LUX9_SUBSYS_CORE | 186)
#define LUX9_SYS_BRK                (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 55)
#define LUX9_SYS_NSEC               (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 53)
#define LUX9_SYS_MOUNT              (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 46)
#define LUX9_SYS_SEEK               (LUX9_ARCH_PLAN9 | LUX9_SUBSYS_CORE | 39)

/*
 * ============================================================================
 * BACKWARD COMPATIBILITY LAYER
 * ============================================================================
 * 
 * These macros ensure existing code continues to work during migration
 */

// CRITICAL: WASM syscall compatibility - map old numbers to new unified numbers
#define SYS_WASM_COMPILE     LUX9_SYS_WASM_COMPILE     // 63 → 160
#define SYS_WASM_EXECUTE     LUX9_SYS_WASM_EXECUTE     // 64 → 161  
#define SYS_WASM_DESTROY     LUX9_SYS_WASM_DESTROY     // 65 → 162

// Plan 9 syscall compatibility
#define OPEN                 LUX9_SYS_OPEN
#define CLOSE                LUX9_SYS_CLOSE
#define READ                 LUX9_SYS_READ
#define WRITE                LUX9_SYS_WRITE
#define CREATE               LUX9_SYS_CREATE
#define RFORK                LUX9_SYS_RFORK
#define PIPE                 LUX9_SYS_PIPE
#define EXEC                 LUX9_SYS_EXEC
#define WAIT                 LUX9_SYS_WAIT

/*
 * ============================================================================
 * UTILITY MACROS
 * ============================================================================
 * 
 * Helper functions for syscall classification and validation
 */

// Architecture classification
#define LUX9_IS_PLAN9_SYSCALL(x)      (((x) & LUX9_SYSCALL_ARCH_MASK) == LUX9_ARCH_PLAN9)
#define LUX9_IS_LUX9_NATIVE(x)        (((x) & LUX9_SYSCALL_ARCH_MASK) == LUX9_ARCH_LUX9_NATIVE)
#define LUX9_IS_RUMP_SYSCALL(x)       (((x) & LUX9_SYSCALL_ARCH_MASK) == LUX9_ARCH_RUMP)

// Subsystem identification
#define LUX9_GET_SUBSYSTEM(x)         (((x) & LUX9_SYSCALL_SUBSYS_MASK) >> 24)
#define LUX9_GET_FUNCTION(x)           ((x) & LUX9_SYSCALL_FUNC_MASK)

// Syscall validation
#define LUX9_SYSCALL_IS_VALID(x)      ((x) <= LUX9_SYS_MAX)
#define LUX9_SYSCALL_IS_WASM(x)       (LUX9_GET_SUBSYSTEM(x) == 2)  // 0x02000000 >> 24 = 2
#define LUX9_SYSCALL_IS_PEBBLE(x)     (LUX9_GET_SUBSYSTEM(x) == 3)  // 0x03000000 >> 24 = 3
#define LUX9_SYSCALL_IS_EXCHANGE(x)   (LUX9_GET_SUBSYSTEM(x) == 4)  // 0x04000000 >> 24 = 4

// Maximum syscall number for table bounds checking
#define LUX9_SYS_MAX                   999

/*
 * ============================================================================
 * KERNEL INTEGRATION FUNCTIONS
 * ============================================================================
 */

// Syscall dispatch function declaration
int lux9_syscall_dispatch(uint syscall_num, uintptr args[], int argc, uintptr *result);

// Syscall validation
int lux9_syscall_validate(uint syscall_num);

// Architecture detection
int lux9_syscall_get_arch(uint syscall_num);
int lux9_syscall_get_subsys(uint syscall_num);

// Compatibility layer for old syscall numbers
uint lux9_compat_map_legacy(uint legacy_num);

#endif /* _LUX9_SYSCALL_UNIFIED_H_ */