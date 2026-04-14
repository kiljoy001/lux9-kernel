/*
 * GNU Mach GHOSTDAG Kernel Integration
 * Copyright (c) 2025 GHOSTDAG IPC Consensus System
 * 
 * GHOSTDAG consensus algorithm adapted for GNU Mach microkernel IPC.
 * Provides total ordering of IPC messages using DAG-based consensus
 * without proof-of-work, optimized for microkernel message passing.
 *
 * Based on "PHANTOM GHOSTDAG: A Scalable Generalization of Nakamoto Consensus"
 * Adapted for microkernel constraints and real-time requirements.
 */

#ifndef _IPC_GHOSTDAG_KERNEL_H_
#define _IPC_GHOSTDAG_KERNEL_H_

#include <mach/kern_return.h>
#include <mach/message.h>
#include <kern/lock.h>
#include <kern/kalloc.h>
#include <ipc/ipc_types.h>

/* Forward declarations to avoid circular dependencies */
struct ipc_kmsg;
struct ipc_mqueue;

/*
 * GHOSTDAG Configuration for GNU Mach
 * Tuned for microkernel constraints and 5-message queue limits
 */
#define GHOSTDAG_K_PARAMETER        3     /* k-cluster parameter */
#define GHOSTDAG_MAX_ANTICONE      10     /* Maximum anticone size */
#define GHOSTDAG_MAX_MESSAGES     65536   /* Unlimited for practical purposes */
#define GHOSTDAG_GENESIS_ID         0     /* Genesis message ID */
#define GHOSTDAG_BATCH_SIZE         4     /* Respect 5-message queue limit */

/*
 * GHOSTDAG Message Metadata
 * Added to each ipc_kmsg to track consensus state
 */
typedef struct ghostdag_msg_meta {
    /* Message identity */
    uint32_t        gm_id;              /* Unique message ID */
    uint64_t        gm_timestamp;       /* Message timestamp (μs) */
    mach_port_t     gm_source_port;     /* Source port for ordering */
    
    /* DAG relationships */
    uint32_t        gm_parent_count;    /* Number of parent messages */
    uint32_t        gm_parents[8];      /* Parent message IDs (limited for kernel) */
    
    /* Consensus state */
    uint8_t         gm_color;           /* BLUE=1, RED=0 */
    uint8_t         gm_processed;       /* Processing state */
    uint16_t        gm_anticone_size;   /* Size of anticone set */
    
    /* FSM state for pruning */
    uint8_t         gm_fsm_state;       /* FSM lifecycle state */
    uint8_t         gm_reference_count; /* Reference count for safe pruning */
    boolean_t       gm_can_prune;       /* Ready for pruning flag */
    
    /* Performance tracking */
    uint64_t        gm_created_time;    /* When message was created */
    uint64_t        gm_ordered_time;    /* When consensus was reached */
    
} *ghostdag_msg_meta_t;

#define GHOSTDAG_COLOR_RED     0
#define GHOSTDAG_COLOR_BLUE    1

#define GHOSTDAG_STATE_PENDING    0
#define GHOSTDAG_STATE_ORDERED    1
#define GHOSTDAG_STATE_DELIVERED  2
#define GHOSTDAG_STATE_TERMINATED 3  /* FSM terminal state - can be pruned */

/* FSM states for progressive message lifecycle */
#define FSM_IPC_NEW         0
#define FSM_IPC_READY       1  
#define FSM_IPC_RUNNING     2
#define FSM_IPC_BLOCKED     3
#define FSM_IPC_PROCESSING  4
#define FSM_IPC_COMPLETE    5
#define FSM_IPC_TERMINATED  6

/*
 * GHOSTDAG DAG Structure
 * Kernel-optimized for minimal memory usage
 */
typedef struct ghostdag_dag {
    decl_simple_lock_data(, gd_lock)    /* DAG access lock */
    
    /* Message storage */
    uint32_t        gd_message_count;    /* Current message count */
    uint32_t        gd_next_id;          /* Next message ID to assign */
    uint32_t        gd_genesis_id;       /* Genesis message ID */
    
    /* Topological ordering */
    uint32_t        gd_ordered_count;    /* Messages in topological order */
    uint32_t        *gd_topological_order;  /* Dynamic allocation */
    
    /* Performance optimization */
    uint64_t        gd_last_recompute;   /* Last full recompute time */
    uint8_t         **gd_reachability_cache;  /* Dynamic allocation */
    
    /* Statistics */
    uint64_t        gd_total_messages;   /* Total messages processed */
    uint64_t        gd_blue_messages;    /* Blue messages count */
    uint64_t        gd_red_messages;     /* Red messages count */
    uint64_t        gd_avg_anticone;     /* Average anticone size */
    uint64_t        gd_pruned_count;     /* Messages pruned via FSM GC */
    uint32_t        gd_active_count;     /* Currently active messages */
    
} *ghostdag_dag_t;

/*
 * Global GHOSTDAG State
 * Single DAG instance for the entire kernel
 */
extern struct ghostdag_dag *kernel_ghostdag;

/*
 * GHOSTDAG Kernel Functions
 */

/* Initialize GHOSTDAG subsystem */
extern kern_return_t ghostdag_kernel_init(void);

/* Add message to DAG and determine ordering */
extern kern_return_t ghostdag_add_message(
    struct ipc_kmsg        *kmsg,
    ghostdag_msg_meta_t     *meta_out);

/* Get next message in topological order */
extern kern_return_t ghostdag_get_next_ordered(
    uint32_t               *msg_id_out);

/* Check if message should be delivered (respects 5-message limit) */
extern boolean_t ghostdag_can_deliver_batch(
    uint32_t               batch_size);

/* Update DAG with message relationships */
extern kern_return_t ghostdag_update_dag(
    uint32_t               msg_id,
    uint32_t               *parents,
    uint32_t               parent_count);

/* Compute anticone for message */
extern kern_return_t ghostdag_compute_anticone(
    uint32_t               msg_id,
    uint32_t               *anticone_out,
    uint32_t               *anticone_size);

/* Determine if message is BLUE or RED */
extern uint8_t ghostdag_determine_color(
    uint32_t               msg_id,
    uint32_t               *anticone,
    uint32_t               anticone_size);

/* Get GHOSTDAG statistics */
extern kern_return_t ghostdag_get_stats(
    uint64_t               *total_messages,
    uint64_t               *blue_messages,
    uint64_t               *red_messages,
    uint64_t               *avg_anticone);

/*
 * Integration with GNU Mach IPC
 */

/* Hook into ipc_mqueue_send() */
extern mach_msg_return_t ghostdag_mqueue_send(
    struct ipc_kmsg        *kmsg,
    mach_msg_option_t      option,
    mach_msg_timeout_t     timeout);

/* Hook into ipc_kmsg allocation */
extern kern_return_t ghostdag_kmsg_init(
    struct ipc_kmsg        *kmsg);

/* Hook into ipc_kmsg deallocation */
extern void ghostdag_kmsg_cleanup(
    struct ipc_kmsg        *kmsg);

/*
 * Debug and Testing Support
 */
#if MACH_KDB
extern void ghostdag_debug_print_dag(void);
extern void ghostdag_debug_print_message(uint32_t msg_id);
extern void ghostdag_debug_verify_dag(void);
#endif

/*
 * Additional Function Declarations
 */

/* FSM state management */
extern void ghostdag_progress_fsm_state(struct ipc_kmsg *kmsg);
extern void ghostdag_fsm_garbage_collect(ghostdag_dag_t dag);

/* Message tracking */
extern void ghostdag_message_delivered(uint64_t msg_id);

/* Shared memory registration */
extern void ghostdag_register_shm(vm_offset_t addr, vm_size_t size);

/* Debug helpers */
extern void ghostdag_debug_print_dag(void);
extern void ghostdag_debug_check_consistency(void);

/*
 * Performance Macros
 * Optimized for kernel use
 */
#define ghostdag_lock(dag)      simple_lock(&(dag)->gd_lock)
#define ghostdag_unlock(dag)    simple_unlock(&(dag)->gd_lock)

#define ghostdag_msg_is_blue(meta)   ((meta)->gm_color == GHOSTDAG_COLOR_BLUE)
#define ghostdag_msg_is_red(meta)    ((meta)->gm_color == GHOSTDAG_COLOR_RED)

#define ghostdag_msg_is_ordered(meta) ((meta)->gm_processed >= GHOSTDAG_STATE_ORDERED)

/*
 * Error Codes
 */
#define GHOSTDAG_SUCCESS             KERN_SUCCESS
#define GHOSTDAG_INVALID_MESSAGE     KERN_INVALID_ARGUMENT
#define GHOSTDAG_DAG_FULL           KERN_RESOURCE_SHORTAGE
#define GHOSTDAG_ANTICONE_TOO_LARGE KERN_FAILURE
#define GHOSTDAG_REACHABILITY_ERROR KERN_FAILURE

#endif /* _IPC_GHOSTDAG_KERNEL_H_ */