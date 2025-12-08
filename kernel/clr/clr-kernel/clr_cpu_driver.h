/*
 * CLR CPU Driver - Barrelfish-inspired per-CPU kernel
 * Each CPU runs independent CLR kernel instance
 */

#ifndef CLR_CPU_DRIVER_H
#define CLR_CPU_DRIVER_H

#include "clr_kernel_architecture.h"
#include <stdatomic.h>

/* ========== Barrelfish Lesson: CPU Driver Model ========== */

/* Per-CPU kernel instance (no shared state) */
typedef struct cpu_driver {
    /* CPU identification */
    uint32_t cpu_id;
    uint32_t numa_node;
    
    /* Local CLR kernel instance */
    clr_kernel_system_t *local_kernel;
    
    /* Local tasklet scheduler */
    struct {
        clr_tasklet_t **run_queue;
        size_t queue_size;
        size_t queue_head;
        atomic_bool needs_reschedule;
    } scheduler;
    
    /* Inter-CPU message channels (no shared memory) */
    struct {
        void *send_buffer;      /* Outgoing messages */
        void *recv_buffer;      /* Incoming messages */
        size_t buffer_size;
        atomic_uint send_seq;   /* Sequence numbers */
        atomic_uint recv_seq;
    } ipc;
    
    /* Local memory management */
    struct {
        void *local_heap;       /* CPU-local heap */
        size_t heap_size;
        size_t heap_used;
        uint64_t *page_tables;  /* CPU-local page tables */
    } memory;
    
    /* Performance counters */
    struct {
        uint64_t messages_sent;
        uint64_t messages_received;
        uint64_t cache_misses;
        uint64_t tlb_flushes;
    } stats;
} cpu_driver_t;

/* ========== Barrelfish Lesson: Monitor/System Process ========== */

/* System-wide coordinator (runs on BSP) */
typedef struct monitor_process {
    /* CPU driver registry */
    cpu_driver_t *cpu_drivers[256];  /* Max 256 CPUs */
    uint32_t num_cpus;
    
    /* Global namespace (replicated, not shared) */
    struct {
        char **names;
        tasklet_id_t *tasklets;
        size_t count;
    } namespace;
    
    /* Capability management */
    struct {
        uint64_t *capability_db;     /* Who can do what */
        size_t db_size;
    } capabilities;
    
    /* Resource allocation */
    struct {
        uint64_t memory_map[1024];    /* Physical memory regions */
        uint32_t cpu_assignments[256]; /* Which tasklet on which CPU */
    } resources;
} monitor_process_t;

/* ========== Barrelfish Lesson: Dispatcher (User-level scheduling) ========== */

typedef struct dispatcher {
    /* User-level thread scheduler */
    tasklet_id_t current_tasklet;
    clr_tasklet_t **tasklet_queue;
    
    /* Upcall entry points */
    void (*yield_handler)(void);
    void (*timer_handler)(void);
    void (*fault_handler)(uint64_t fault_addr);
    
    /* Time slice management */
    uint64_t time_slice_start;
    uint64_t time_slice_end;
    
    /* CPU affinity */
    uint32_t pinned_cpu;
    bool can_migrate;
} dispatcher_t;

/* ========== Inter-Core Communication Protocol ========== */

typedef enum {
    MSG_TASKLET_MIGRATE,    /* Move tasklet to different CPU */
    MSG_MEMORY_GRANT,       /* Grant memory capability */
    MSG_CHANNEL_CREATE,     /* Create inter-tasklet channel */
    MSG_CONSENSUS_VOTE,     /* GHOSTDAG consensus message */
    MSG_CACHE_INVALIDATE,   /* TLB/cache coherency */
} ipc_msg_type_t;

typedef struct inter_core_message {
    ipc_msg_type_t type;
    uint32_t from_cpu;
    uint32_t to_cpu;
    uint64_t timestamp;      /* For ordering */
    
    union {
        struct {
            tasklet_id_t tasklet;
            uint32_t target_cpu;
        } migrate;
        
        struct {
            uint64_t phys_addr;
            size_t size;
            uint32_t permissions;
        } memory;
        
        struct {
            uint32_t dag_node;
            bool vote;  /* BLUE/RED */
        } consensus;
    } payload;
} inter_core_message_t;

/* ========== Operations ========== */

/* Initialize CPU driver on specific core */
cpu_driver_t* cpu_driver_init(uint32_t cpu_id, size_t local_heap_size);

/* Start CPU driver main loop */
void cpu_driver_run(cpu_driver_t *driver);

/* Send message to another CPU (async, non-blocking) */
int cpu_driver_send_message(cpu_driver_t *driver, 
                            uint32_t target_cpu,
                            inter_core_message_t *msg);

/* Receive messages from other CPUs */
int cpu_driver_receive_messages(cpu_driver_t *driver,
                                inter_core_message_t *msgs,
                                size_t max_msgs);

/* Migrate tasklet to different CPU */
int cpu_driver_migrate_tasklet(cpu_driver_t *driver,
                               tasklet_id_t tasklet,
                               uint32_t target_cpu);

/* ========== Barrelfish Lesson: Hardware Abstraction ========== */

/* Platform-specific operations */
typedef struct platform_ops {
    /* Inter-processor interrupts */
    void (*send_ipi)(uint32_t target_cpu, uint32_t vector);
    void (*ipi_handler)(uint32_t vector);
    
    /* Cache management */
    void (*flush_cache_line)(void *addr);
    void (*flush_tlb_entry)(uint64_t vaddr);
    
    /* Memory barriers */
    void (*memory_barrier)(void);
    void (*write_barrier)(void);
    void (*read_barrier)(void);
    
    /* NUMA operations */
    uint32_t (*get_numa_node)(uint32_t cpu_id);
    uint64_t (*alloc_numa_memory)(uint32_t node, size_t size);
} platform_ops_t;

/* ========== Barrelfish Lesson: Heterogeneous Support ========== */

typedef enum {
    CPU_TYPE_X86_64,
    CPU_TYPE_ARM64,
    CPU_TYPE_GPU,      /* Future: GPU compute */
    CPU_TYPE_FPGA,     /* Future: FPGA acceleration */
} cpu_type_t;

typedef struct heterogeneous_cpu {
    cpu_type_t type;
    uint32_t capabilities;  /* What this CPU can do */
    
    /* Type-specific operations */
    union {
        struct {
            uint64_t (*rdtsc)(void);
            void (*cpuid)(uint32_t *eax, uint32_t *ebx, 
                         uint32_t *ecx, uint32_t *edx);
        } x86_64;
        
        struct {
            uint64_t (*read_cntpct)(void);
        } arm64;
    } ops;
} heterogeneous_cpu_t;

/* ========== Key Barrelfish Insights Applied ========== */

/*
 * 1. NO SHARED STATE: Each CPU has completely independent kernel
 * 2. MESSAGE PASSING ONLY: All coordination via explicit messages
 * 3. REPLICATION: Namespace, capabilities replicated per CPU
 * 4. AGREEMENT PROTOCOLS: Use GHOSTDAG for consensus
 * 5. HARDWARE NEUTRAL: Abstract all hardware specifics
 * 6. USER-LEVEL SCHEDULING: Dispatchers handle threading
 * 7. CAPABILITY-BASED: All resources accessed via capabilities
 * 8. HETEROGENEOUS READY: Support different CPU types
 */

#endif /* CLR_CPU_DRIVER_H */