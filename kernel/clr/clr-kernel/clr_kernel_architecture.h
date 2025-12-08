/*
 * CLR Kernel Architecture - Formally Verified Design
 * Based on proven Coq specifications
 *
 * DEEP PEBBLE INTEGRATION:
 * - All CLR memory via pebble black-white-red
 * - Reference counting via white tokens (no GC sweep needed)
 * - Speculative execution via red-blue shadows
 * - Zero-copy IPC via exchange pages
 */

#ifndef CLR_KERNEL_ARCHITECTURE_H
#define CLR_KERNEL_ARCHITECTURE_H

#include <stdint.h>
#include <stdbool.h>
#include "../clr-implementation/clr_runtime.h"
#include "../ipc/ipc_kmsg.h"
#include "../ipc/ghostdag_kernel.h"
#include "../ipc/fsm_packet.h"
#include "../../include/pebble.h"
#include "../../include/exchange.h"
#include "clr_pebble_integration.h"

#define CLR_DEFAULT_HEAP_SIZE (32 * 1024 * 1024) // 32MB for CLR managed heap
#define CLR_DEFAULT_DAG_K 8                     // Default k-parameter for GHOSTDAG

/* ========== CLR Tasklet Definitions ========== */

typedef uint32_t tasklet_id_t;
typedef uint32_t channel_id_t;

/* CLR Tasklet - matches Coq CLRTasklet record */
/* NOW USES PEBBLE FOR ALL MEMORY */
typedef struct clr_tasklet {
    tasklet_id_t id;

    /* Pebble-backed CLR state (replaces old clr_state_t) */
    struct clr_pebble_state *state;   /* Pebble-backed execution state */

    channel_id_t channel;         /* Inter-tasklet communication channel */
    uint32_t dag_node_id;         /* Associated GHOSTDAG node */

    /* FSM state for tasklet lifecycle */
    enum {
        TASKLET_IDLE,
        TASKLET_RECEIVING,
        TASKLET_EXECUTING,
        TASKLET_SENDING,
        TASKLET_BLOCKED
    } fsm_state;

    /* Memory constraints */
    size_t max_stack_size;
    size_t max_heap_size;

    /* Security context */
    struct {
        uint32_t capabilities;    /* Capability bits */
        tasklet_id_t parent_id;   /* Parent tasklet for hierarchy */
    } security;

    /* Intrusive list for tasklet management */
    struct clr_tasklet *next;
    struct clr_tasklet *prev;
} clr_tasklet_t;

/* ========== Channel Communication ========== */

/* Tasklet message - NOW USES EXCHANGE PAGES FOR ZERO-COPY */
typedef struct tasklet_message {
    tasklet_id_t from;
    tasklet_id_t to;

    /* Zero-copy via exchange + pebble white token */
    struct clr_object *payload_obj;   /* CLR object being transferred */
    PebbleWhite *white_token;         /* White token for receiver */
    ExchangeHandle *exchange_handles; /* Physical page handles */
    size_t npages;                    /* Number of pages */

    uint32_t dag_id;              /* GHOSTDAG message ID for ordering */

    /* FSM packet integration */
    fsm_packet_t *fsm_packet;     /* Optional FSM packet wrapper */

    /* Intrusive list */
    struct tasklet_message *next;
    struct tasklet_message *prev;
} tasklet_message_t;

/* Channel structure for inter-tasklet communication */
/* NOW USES INTRUSIVE LISTS */
typedef struct tasklet_channel {
    channel_id_t id;
    tasklet_id_t sender;
    tasklet_id_t receiver;

    /* Message queue with GHOSTDAG ordering (intrusive list) */
    tasklet_message_t *queue_head;
    tasklet_message_t *queue_tail;
    size_t count;

    /* Synchronization */
    bool is_blocking;
    bool is_ready;

    /* Intrusive list for channel management */
    struct tasklet_channel *next;
    struct tasklet_channel *prev;
} tasklet_channel_t;

/* ========== CLR Kernel System ========== */

typedef struct clr_kernel_system {
    /* Tasklet management (intrusive lists) */
    clr_tasklet_t *tasklets_head;
    clr_tasklet_t *tasklets_tail;
    size_t tasklet_count;
    tasklet_id_t next_tasklet_id;

    /* Channel management (intrusive lists) */
    tasklet_channel_t *channels_head;
    tasklet_channel_t *channels_tail;
    size_t channel_count;
    channel_id_t next_channel_id;

    /* GHOSTDAG consensus state */
    ghostdag_state_t *dag_state;

    /* FSM packet router */
    fsm_packet_router_t *fsm_router;

    /* Pebble state for kernel-level allocations */
    PebbleState *kernel_pebble;

    /* Statistics */
    struct {
        uint64_t messages_sent;
        uint64_t messages_received;
        uint64_t tasklets_created;
        uint64_t channels_created;
        uint64_t zero_copy_transfers;
    } stats;
} clr_kernel_system_t;

/* ========== Kernel Operations (Verified) ========== */

/* Initialize CLR kernel system */
clr_kernel_system_t* clr_kernel_init(size_t heap_size, uint32_t k_parameter);

/* Create new tasklet */
tasklet_id_t clr_kernel_create_tasklet(
    clr_kernel_system_t *sys,
    size_t stack_size,
    size_t heap_size
);

/* Create communication channel */
channel_id_t clr_kernel_create_channel(
    clr_kernel_system_t *sys,
    tasklet_id_t sender,
    tasklet_id_t receiver,
    bool blocking
);

/* Execute CLR instruction in tasklet context */
clr_result_t clr_kernel_execute_instruction(
    clr_kernel_system_t *sys,
    tasklet_id_t tasklet,
    const clr_instruction_t *instruction
);

/* Send message through channel (respects GHOSTDAG ordering) */
clr_result_t clr_kernel_send_message(
    clr_kernel_system_t *sys,
    channel_id_t channel,
    const clr_value_t *values,
    size_t count
);

/* Receive message from channel (respects GHOSTDAG ordering) */
clr_result_t clr_kernel_receive_message(
    clr_kernel_system_t *sys,
    channel_id_t channel,
    clr_value_t *values,
    size_t *count
);

/* ========== FSM State Transitions ========== */

/* Transition tasklet FSM state */
bool clr_kernel_tasklet_transition(
    clr_tasklet_t *tasklet,
    int new_state
);

/* Check if tasklet can make progress */
bool clr_kernel_tasklet_can_progress(
    const clr_kernel_system_t *sys,
    tasklet_id_t tasklet
);

/* ========== Safety Properties (From Proofs) ========== */

/* Verify tasklet isolation */
bool clr_kernel_verify_isolation(const clr_kernel_system_t *sys);

/* Verify message ordering via GHOSTDAG */
bool clr_kernel_verify_message_ordering(const clr_kernel_system_t *sys);

/* Verify no deadlocks in FSM */
bool clr_kernel_verify_deadlock_freedom(const clr_kernel_system_t *sys);

/* ========== Integration with GNU Mach ========== */

/* Convert Mach IPC message to CLR tasklet message */
tasklet_message_t* clr_kernel_from_mach_msg(
    const struct ipc_kmsg *kmsg,
    clr_kernel_system_t *sys
);

/* Convert CLR tasklet message to Mach IPC message */
struct ipc_kmsg* clr_kernel_to_mach_msg(
    const tasklet_message_t *msg,
    clr_kernel_system_t *sys
);

/* ========== Bootstrap ========== */

/* Bootstrap CLR kernel from multiboot info */
void clr_kernel_bootstrap(
    void *multiboot_info,
    size_t memory_size
);

/* Main kernel loop */
void clr_kernel_main_loop(clr_kernel_system_t *sys);

#endif /* CLR_KERNEL_ARCHITECTURE_H */