/*
 * Memory Management Subsystem Syscalls
 * 
 * This header defines syscalls for the memory management subsystem.
 * Range: 1000-1099
 */

#ifndef _LUX9_SYSCALL_SUBSYS_MEMORY_H_
#define _LUX9_SYSCALL_SUBSYS_MEMORY_H_

#include "lux9_syscall.h"

/*
 * ============================================================================
 * MEMORY MANAGEMENT SYSCALLS (1000-1099)
 * 
 * Provides comprehensive memory management functionality including:
 * - Memory mapping and protection
 * - Heap management
 * - Shared memory
 * - Memory-mapped I/O
 * - NUMA-aware allocation
 * ============================================================================
 */

/* Basic Memory Operations (1000-1019) */
#define LUX9_SYS_MMAP              1000  /* Memory map */
#define LUX9_SYS_MUNMAP             1001  /* Unmap memory */
#define LUX9_SYS_MPROTECT           1002  /* Set memory protection */
#define LUX9_SYS_MLOCK              1003  /* Lock memory */
#define LUX9_SYS_MUNLOCK           1004  /* Unlock memory */
#define LUX9_SYS_MLOCKALL           1005  /* Lock all memory */
#define LUX9_SYS_MUNLOCKALL         1006  /* Unlock all memory */
#define LUX9_SYS_MSYNC              1007  /* Synchronize memory */
#define LUX9_SYS_MREMAP             1008  /* Remap memory */
#define LUX9_SYS_MADVISE            1009  /* Memory advice */
#define LUX9_SYS_MINCORE            1010  /* Check if memory is resident */
#define LUX9_SYS_MALLOC             1011  /* Allocate memory */
#define LUX9_SYS_FREE               1012  /* Free memory */
#define LUX9_SYS_REALLOC            1013  /* Reallocate memory */
#define LUX9_SYS_MEMALIGN           1014  /* Aligned memory allocation */
#define LUX9_SYS_VALLOC             1015  /* Virtual memory allocation */
#define LUX9_SYS_PALLOC             1016  /* Physical allocation */
#define LUX9_SYS_PFREE              1017  /* Physical free */

/* Advanced Memory Features (1020-1039) */
#define LUX9_SYS_MEMINFO_GET        1020  /* Get memory info */
#define LUX9_SYS_NUMA_INFO          1021  /* Get NUMA information */
#define LUX9_SYS_NUMA_SET_POLICY    1022  /* Set NUMA policy */
#define LUX9_SYS_NUMA_GET_POLICY    1023  /* Get NUMA policy */
#define LUX9_SYS_NUMA_MOVE_TASK      1024  /* Move task to NUMA node */
#define LUX9_SYS_NUMA_ALLOC_LOCAL   1025  /* Allocate local memory */
#define LUX9_SYS_NUMA_ALLOC_PREFERRED 1026 /* Allocate preferred node */
#define LUX9_SYS_NUMA_ALLOC_BIND     1027 /* Allocate on specific nodes */
#define LUX9_SYS_NUMA_ALLOC_INTERLEAVE 1028 /* Allocate interleaved */

/* Memory Protection (1040-1059) */
#define LUX9_SYS_MPROTECT_EXEC       1040  /* Set execute protection */
#define LUX9_SYS_MPROTECT_READ       1041  /* Set read protection */
#define LUX9_SYS_MPROTECT_WRITE      1042  /* Set write protection */
#define LUX9_SYS_MPROTECT_NO_ACCESS  1043  /* Set no access */
#define LUX9_SYS_MPROTECT_SECURE     1044  /* Set secure protection */
#define LUX9_SYS_MPROTECT_CHECK      1045  /* Check protection flags */

/* Shared Memory (1060-1079) */
#define LUX9_SYS_SHM_CREATE          1060  /* Create shared memory */
#define LUX9_SYS_SHM_ATTACH          1061  /* Attach shared memory */
#define LUX9_SYS_SHM_DETACH          1062  /* Detach shared memory */
#define LUX9_SYS_SHM_DESTROY         1063  /* Destroy shared memory */
#define LUX9_SYS_SHM_GET_INFO        1064  /* Get shared memory info */
#define LUX9_SYS_SHM_SET_PERMS       1065  /* Set permissions */
#define LUX9_SYS_SHM_SNAPSHOT        1066  /* Snapshot shared memory */
#define LUX9_SYS_SHM_RESTORE         1067  /* Restore shared memory */
#define LUX9_SYS_SHM_PERSISTENT       1068  /* Make persistent */
#define LUX9_SYS_SHM_VOLATILE        1069  /* Make volatile */

/* Memory-Mapped I/O (1080-1099) */
#define LUX9_SYS_MMIO_MAP            1080  /* Map device memory */
#define LUX9_SYS_MMIO_UNMAP          1081  /* Unmap device memory */
#define LUX9_SYS_MMIO_SYNC           1082  /* Sync device memory */
#define LUX9_SYS_MMIO_CACHE_CTL      1083  /* Cache control */
#define LUX9_SYS_MMIO_READ           1084  /* Read device memory */
#define LUX9_SYS_MMIO_WRITE          1085  /* Write device memory */
#define LUX9_SYS_MMIO_FLAGS_GET      1086  /* Get MMIO flags */
#define LUX9_SYS_MMIO_FLAGS_SET     1087  /* Set MMIO flags */
#define LUX9_SYS_MMIO_PERF_CTL       1088  /* Performance control */

/*
 * ============================================================================
 * MEMORY SYSCALL VALIDATION
 * ============================================================================
 */

/* Validate memory syscall number */
static inline int lux9_memory_syscall_is_valid(uint32_t syscall_num) {
    return (syscall_num >= LUX9_MEM_SYSCALL_BASE && 
            syscall_num <= LUX9_MEM_SYSCALL_MAX);
}

/* Check if syscall is basic memory operation */
static inline int lux9_memory_syscall_is_basic(uint32_t syscall_num) {
    return (syscall_num >= LUX9_MEM_SYSCALL_BASE && 
            syscall_num < LUX9_MEM_SYSCALL_BASE + 20);
}

/* Check if syscall is advanced feature */
static inline int lux9_memory_syscall_is_advanced(uint32_t syscall_num) {
    return (syscall_num >= LUX9_MEM_SYSCALL_BASE + 20 && 
            syscall_num < LUX9_MEM_SYSCALL_BASE + 40);
}

/* Check if syscall is NUMA-related */
static inline int lux9_memory_syscall_is_numa(uint32_t syscall_num) {
    return (syscall_num >= LUX9_MEM_SYSCALL_BASE + 20 && 
            syscall_num <= LUX9_MEM_SYSCALL_BASE + 29);
}

/*
 * ============================================================================
 * MEMORY SYSCALL CATEGORIZATION
 * ============================================================================
 */

static inline const char *lux9_memory_syscall_category(uint32_t syscall_num) {
    if (syscall_num < LUX9_MEM_SYSCALL_BASE + 20)
        return "Basic Operations";
    if (syscall_num < LUX9_MEM_SYSCALL_BASE + 40)
        return "Advanced Features";
    if (syscall_num < LUX9_MEM_SYSCALL_BASE + 60)
        return "Protection";
    if (syscall_num < LUX9_MEM_SYSCALL_BASE + 80)
        return "Shared Memory";
    return "Memory-Mapped I/O";
}

/*
 * ============================================================================
 * MEMORY PERFORMANCE CHARACTERISTICS
 * ============================================================================
 */

static inline int lux9_memory_syscall_is_fast(uint32_t syscall_num) {
    /* Fast syscalls: basic malloc/free, mmap/munmap */
    return (syscall_num == LUX9_SYS_MALLOC ||
            syscall_num == LUX9_SYS_FREE ||
            syscall_num == LUX9_SYS_MMAP ||
            syscall_num == LUX9_SYS_MUNMAP);
}

static inline int lux9_memory_syscall_is_slow(uint32_t syscall_num) {
    /* Slow syscalls: NUMA operations, shared memory, MMIO */
    return (syscall_num >= LUX9_MEM_SYSCALL_BASE + 20 &&
            syscall_num <= LUX9_MEM_SYSCALL_MAX);
}

/*
 * ============================================================================
 * MEMORY SYSCALL HELPERS
 * ============================================================================
 */

/* Get memory syscall subsystem */
static inline uint32_t lux9_memory_syscall_get_subsystem(uint32_t syscall_num) {
    return 'MEM ';
}

/* Get memory operation type */
static inline const char *lux9_memory_syscall_operation(uint32_t syscall_num) {
    switch (syscall_num) {
        case LUX9_SYS_MMAP: return "Memory Map";
        case LUX9_SYS_MUNMAP: return "Memory Unmap";
        case LUX9_SYS_MPROTECT: return "Set Protection";
        case LUX9_SYS_MALLOC: return "Allocate";
        case LUX9_SYS_FREE: return "Free";
        case LUX9_SYS_REALLOC: return "Reallocate";
        case LUX9_SYS_SHM_CREATE: return "Create Shared Memory";
        case LUX9_SYS_SHM_ATTACH: return "Attach Shared Memory";
        case LUX9_SYS_MMIO_MAP: return "Map Device Memory";
        default: return "Memory Operation";
    }
}

/*
 * ============================================================================
 * MEMORY SYSCALL ARGUMENT STRUCTURES
 * 
 * Standard argument structures for memory syscalls.
 * ============================================================================
 */

/* Basic allocation structure */
struct lux9_mem_alloc_args {
    uintptr_t size;           /* Size to allocate */
    uintptr_t alignment;      /* Alignment requirement */
    uintptr_t flags;          /* Allocation flags */
    uintptr_t numa_node;      /* Preferred NUMA node */
};

/* Memory mapping structure */
struct lux9_mem_mmap_args {
    uintptr_t addr;           /* Starting address */
    uintptr_t length;         /* Length of mapping */
    uintptr_t protection;     /* Protection flags */
    uintptr_t flags;          /* Mapping flags */
    uintptr_t fd;             /* File descriptor */
    uintptr_t offset;         /* File offset */
    uintptr_t numa_policy;    /* NUMA policy */
};

/* Protection structure */
struct lux9_mem_protect_args {
    uintptr_t addr;           /* Starting address */
    uintptr_t length;         /* Length */
    uintptr_t protection;     /* New protection */
    uintptr_t flags;          /* Protection flags */
};

/* NUMA policy structure */
struct lux9_numa_policy_args {
    uintptr_t policy;         /* NUMA policy */
    uintptr_t nodes;          /* Node bitmask */
    uintptr_t node_count;     /* Number of nodes */
};

/*
 * ============================================================================
 * USAGE GUIDELINES FOR MEMORY SYSCALLS
 * 
 * 1. Use LUX9_SYS_M* for memory management operations
 * 2. NUMA-aware applications should use NUMA-specific syscalls
 * 3. Shared memory operations provide persistence options
 * 4. Memory-mapped I/O provides direct device access
 * 5. Always check return values for allocation failures
 * 
 * Example:
 *   // Allocate 4KB with NUMA awareness
 *   struct lux9_mem_alloc_args args = {4096, 0, 0, 1};
 *   void *ptr = syscall(LUX9_SYS_MALLOC, &args);
 *   
 *   // Map file into memory
 *   struct lux9_mem_mmap_args mmap_args = {
 *       0, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0, NUMA_LOCAL
 *   };
 *   void *mapped = syscall(LUX9_SYS_MMAP, &mmap_args);
 * ============================================================================
 */

#endif /* _LUX9_SYSCALL_SUBSYS_MEMORY_H_ */
