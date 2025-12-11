/*
 * Domain FSM GHOSTDAG Integration
 * 
 * Preserves and enhances GHOSTDAG consensus ordering within the FSM 
 * modernization, ensuring all domain operations maintain total ordering
 * and distributed consensus across the 8-domain system.
 */

#ifndef _KERN_DOMAIN_FSM_GHOSTDAG_INTEGRATION_H_
#define _KERN_DOMAIN_FSM_GHOSTDAG_INTEGRATION_H_

#include "domain_fsm_modernization.h"
#include "../ipc/ghostdag_kernel.h"
#include "../ipc/fsm_packet.h"

/* ========== GHOSTDAG-Enhanced FSM Packets ========== */

/*
 * Every FSM packet sent between domains includes GHOSTDAG metadata
 * for total ordering and consensus participation
 */
typedef struct domain_fsm_ghostdag_packet {
    struct fsm_packet base_packet;
    
    /* GHOSTDAG consensus metadata */
    struct ghostdag_msg_meta ghostdag_meta;
    
    /* Domain consensus participation */
    uint32_t consensus_operation_id;    /* Unique operation ID */
    uint32_t participating_domains;     /* Bitmask of domains that must vote */
    uint32_t votes_received;            /* Current vote count */
    uint32_t votes_required;            /* Required votes for consensus */
    
    /* Ordering guarantees */
    uint64_t global_sequence_number;    /* Global ordering across all domains */
    uint32_t domain_sequence_number;    /* Per-domain ordering */
    uint64_t causal_timestamp;          /* Vector clock for causality */
    
    /* Consensus state */
    uint8_t consensus_phase;            /* PROPOSE, VOTE, COMMIT, ABORT */
    uint8_t consensus_decision;         /* APPROVE, REJECT, TIMEOUT */
    boolean_t requires_consensus;       /* Does this operation need consensus? */
    
} domain_fsm_ghostdag_packet_t;

/* Consensus phases */
#define CONSENSUS_PHASE_PROPOSE     1
#define CONSENSUS_PHASE_VOTE        2
#define CONSENSUS_PHASE_COMMIT      3
#define CONSENSUS_PHASE_ABORT       4

/* Consensus decisions */
#define CONSENSUS_DECISION_APPROVE  1
#define CONSENSUS_DECISION_REJECT   0
#define CONSENSUS_DECISION_TIMEOUT  2

/* ========== Domain Consensus Coordinator ========== */

typedef struct domain_consensus_coordinator {
    /* GHOSTDAG integration */
    struct ghostdag_dag *global_dag;
    
    /* Active consensus operations */
    struct {
        uint32_t operation_id;
        uint32_t participating_domains;
        uint32_t votes[MAX_DOMAIN_KERNELS];
        uint64_t start_time;
        uint64_t timeout_time;
        boolean_t completed;
    } active_operations[256];
    
    uint32_t active_operation_count;
    uint32_t next_operation_id;
    
    /* Global ordering */
    uint64_t global_sequence_counter;
    uint32_t domain_sequence_counters[MAX_DOMAIN_KERNELS];
    
    /* Performance tracking */
    uint64_t total_consensus_operations;
    uint64_t successful_consensus_operations;
    uint64_t average_consensus_latency_ns;
    uint64_t ghostdag_messages_processed;
    
} domain_consensus_coordinator_t;

/* ========== GHOSTDAG Operations Types ========== */

/*
 * Define which domain operations require GHOSTDAG consensus
 */
typedef enum {
    /* Memory operations requiring consensus */
    GHOSTDAG_OP_CROSS_DOMAIN_ALLOC,
    GHOSTDAG_OP_SHARED_OBJECT_CREATE,
    GHOSTDAG_OP_GARBAGE_COLLECT_GLOBAL,
    
    /* Execution operations requiring consensus */
    GHOSTDAG_OP_CROSS_DOMAIN_CALL,
    GHOSTDAG_OP_EXCEPTION_PROPAGATION,
    GHOSTDAG_OP_CODE_MIGRATION,
    
    /* System operations requiring consensus */
    GHOSTDAG_OP_DOMAIN_SHUTDOWN,
    GHOSTDAG_OP_LOAD_BALANCING_DECISION,
    GHOSTDAG_OP_FAULT_RECOVERY,
    
    /* Resource operations requiring consensus */
    GHOSTDAG_OP_RESOURCE_ALLOCATION,
    GHOSTDAG_OP_PRIORITY_ESCALATION,
    GHOSTDAG_OP_DEADLOCK_RESOLUTION,
    
} ghostdag_operation_type_t;

/* ========== Enhanced FSM Packet Router with GHOSTDAG ========== */

typedef struct domain_fsm_ghostdag_router {
    /* Standard FSM routing */
    domain_fsm_registry_t *fsm_registry;
    
    /* GHOSTDAG consensus integration */
    domain_consensus_coordinator_t *consensus_coordinator;
    
    /* Packet ordering and delivery */
    kern_return_t (*route_with_consensus)(domain_fsm_ghostdag_packet_t *packet);
    kern_return_t (*ensure_causal_order)(domain_fsm_ghostdag_packet_t *packet);
    kern_return_t (*coordinate_consensus)(ghostdag_operation_type_t op_type, 
                                          uint32_t target_domains);
    
    /* GHOSTDAG DAG maintenance */
    kern_return_t (*add_packet_to_dag)(domain_fsm_ghostdag_packet_t *packet);
    kern_return_t (*get_next_deliverable)(domain_fsm_ghostdag_packet_t **packet);
    
} domain_fsm_ghostdag_router_t;

/* ========== API Functions ========== */

/* Initialize GHOSTDAG-enhanced FSM system */
kern_return_t domain_fsm_ghostdag_init(void);

/* Send packet with GHOSTDAG consensus integration */
kern_return_t domain_fsm_send_with_consensus(
    uint32_t target_domain,
    struct fsm_packet *packet,
    ghostdag_operation_type_t op_type,
    boolean_t requires_consensus);

/* Coordinate consensus across domains for an operation */
kern_return_t domain_fsm_coordinate_consensus(
    ghostdag_operation_type_t op_type,
    uint32_t participating_domains,
    const uint8_t *operation_data,
    uint32_t data_length);

/* Vote on a consensus operation */
kern_return_t domain_fsm_consensus_vote(
    uint32_t operation_id,
    uint32_t domain_id,
    uint8_t vote_decision);

/* Get next packet in GHOSTDAG total order */
kern_return_t domain_fsm_get_next_ordered_packet(
    domain_fsm_ghostdag_packet_t **packet_out);

/* Check if packet can be delivered (respects GHOSTDAG ordering) */
boolean_t domain_fsm_can_deliver_packet(
    domain_fsm_ghostdag_packet_t *packet);

/* Update GHOSTDAG relationships for packet */
kern_return_t domain_fsm_update_packet_dag(
    domain_fsm_ghostdag_packet_t *packet,
    uint32_t *parent_packet_ids,
    uint32_t parent_count);

/* Get GHOSTDAG consensus statistics */
kern_return_t domain_fsm_get_consensus_stats(
    uint64_t *total_operations,
    uint64_t *successful_operations,
    uint64_t *average_latency_ns,
    uint64_t *ghostdag_efficiency);

/* ========== Consensus Decision Functions ========== */

/* Determine if operation requires consensus */
boolean_t domain_fsm_requires_consensus(
    uint32_t source_domain,
    uint32_t target_domain,
    uint32_t operation_command);

/* Calculate participating domains for consensus */
uint32_t domain_fsm_calculate_participants(
    ghostdag_operation_type_t op_type,
    uint32_t source_domain,
    uint32_t target_domain);

/* Apply consensus decision to operation */
kern_return_t domain_fsm_apply_consensus_decision(
    uint32_t operation_id,
    uint8_t decision,
    uint32_t participating_domains);

/* ========== GHOSTDAG Ordering Preservation ========== */

/*
 * Key insight: FSM packets enhance GHOSTDAG rather than replace it
 * 
 * 1. Every FSM packet gets a GHOSTDAG message ID
 * 2. Packet routing respects GHOSTDAG total ordering  
 * 3. Cross-domain operations participate in GHOSTDAG consensus
 * 4. Causal relationships maintained via packet dependencies
 * 5. Domain failures handled via GHOSTDAG fault tolerance
 */

/* Vector clock for causal ordering */
typedef struct domain_vector_clock {
    uint64_t domain_clocks[MAX_DOMAIN_KERNELS];
    uint64_t global_clock;
} domain_vector_clock_t;

/* Update vector clock for packet */
kern_return_t domain_fsm_update_vector_clock(
    domain_vector_clock_t *clock,
    uint32_t domain_id,
    domain_fsm_ghostdag_packet_t *packet);

/* Check if packet respects causal ordering */
boolean_t domain_fsm_respects_causality(
    domain_fsm_ghostdag_packet_t *packet,
    domain_vector_clock_t *current_clock);

/* ========== Performance Optimizations ========== */

/*
 * Optimizations that preserve GHOSTDAG guarantees:
 * 
 * 1. Batch consensus operations for related packets
 * 2. Pipeline consensus phases across domains  
 * 3. Cache GHOSTDAG reachability for frequent operations
 * 4. Prune completed operations from DAG using FSM state
 * 5. Use domain locality to reduce consensus overhead
 */

/* Batch consensus for multiple related operations */
kern_return_t domain_fsm_batch_consensus(
    ghostdag_operation_type_t *op_types,
    domain_fsm_ghostdag_packet_t **packets,
    uint32_t batch_size);

/* Pipeline consensus across multiple domains */
kern_return_t domain_fsm_pipeline_consensus(
    uint32_t operation_id,
    uint32_t *domain_pipeline,
    uint32_t pipeline_length);

#endif /* _KERN_DOMAIN_FSM_GHOSTDAG_INTEGRATION_H_ */