/*
 * Lux9 Selective Consensus Depth
 *
 * Operation classification and depth-based routing for MSGORD.
 * Implements "all nodes, all the way down" with variable depth.
 */

#ifndef _CONSENSUS_DEPTH_H_
#define _CONSENSUS_DEPTH_H_

#include "msgord.h"

/* Forward declarations */
struct Proc;
struct Fcall;
typedef struct Fcall Fcall;

/*
 * Operation Types for Classification
 */
typedef enum {
  OP_TYPE_UNKNOWN = 0,
  OP_TYPE_LOCAL_MEMORY, /* Process-local memory operations */
  OP_TYPE_LOCAL_FILE,   /* Local file read/write */
  OP_TYPE_IPC,          /* Inter-process communication */
  OP_TYPE_DEVICE,       /* Device access */
  OP_TYPE_NETWORK,      /* Network operations */
  OP_TYPE_SECURITY,     /* Security-critical operations */
  OP_TYPE_CONSENSUS,    /* Consensus-related operations */
} OperationType;

/*
 * Consensus Depth Levels
 */
typedef enum {
  DEPTH_NONE = 0, /* No consensus needed - local/immediate */
  DEPTH_LOCAL,    /* Local node only */
  DEPTH_CLUSTER,  /* Cluster-level consensus */
  DEPTH_GLOBAL,   /* Full global consensus */
} ConsensusDepth;

/*
 * Rollback State for Optimistic Execution
 */
typedef enum {
  ROLLBACK_NONE = 0,  /* No rollback needed */
  ROLLBACK_PENDING,   /* Awaiting consensus verification */
  ROLLBACK_TRIGGERED, /* Rollback in progress */
  ROLLBACK_COMPLETE,  /* Rollback finished */
  ROLLBACK_COMMITTED, /* Successfully committed (no rollback) */
} RollbackState;

/*
 * Operation Rollback Entry
 * Tracks optimistically executed operations for potential rollback.
 */
typedef struct OpRollbackEntry {
  uint op_id;                    /* MSGORD message ID */
  RollbackState state;           /* Current rollback state */
  ConsensusDepth required_depth; /* Required consensus depth */
  OperationType op_type;         /* Classification of operation */

  /* Rollback data */
  void *snapshot_data; /* Pre-execution state snapshot */
  ulong snapshot_size; /* Size of snapshot data */

  /* Fcall context for rollback */
  struct Proc *caller;     /* Calling process */
  Fcall *original_request; /* Original request */
  Fcall *executed_reply;   /* Reply that was sent */

  /* Timing */
  uvlong submit_time;  /* When operation was submitted */
  uvlong execute_time; /* When optimistic execution occurred */
  uvlong verify_time;  /* When consensus verified (or failed) */

  /* Linked list for pending rollbacks */
  struct OpRollbackEntry *next;
  struct OpRollbackEntry *prev;
} OpRollbackEntry;

/*
 * Rollback Registry
 * Tracks all pending optimistic executions awaiting verification.
 */
typedef struct RollbackRegistry {
  OpRollbackEntry *head;
  OpRollbackEntry *tail;
  uint count;
  uint max_entries; /* High water mark before forced sync */

  /* Statistics */
  uvlong total_optimistic; /* Total optimistic executions */
  uvlong total_committed;  /* Successfully committed */
  uvlong total_rollbacks;  /* Rollbacks triggered */
} RollbackRegistry;

/*
 * Operation Classification API
 */

/* Classify operation and determine required consensus depth */
ConsensusDepth classify_operation(Fcall *t, char *path);

/* Classification helpers */
int is_local_memory_operation(Fcall *t);
int is_ipc_between_processes(Fcall *t, char *path);
int is_device_operation(Fcall *t, char *path);
int is_security_critical_operation(Fcall *t, char *path);

/* Get operation type */
OperationType get_operation_type(Fcall *t, char *path);

/*
 * Rollback Management API
 */

/* Initialize rollback registry */
void rollback_registry_init(RollbackRegistry *reg, uint max_entries);

/* Register optimistic execution for potential rollback */
OpRollbackEntry *rollback_register(RollbackRegistry *reg, uint op_id,
                                   ConsensusDepth depth, struct Proc *caller,
                                   Fcall *t, Fcall *r);

/* Mark operation as verified (commit) */
int rollback_commit(RollbackRegistry *reg, uint op_id);

/* Trigger rollback for failed consensus */
int rollback_trigger(RollbackRegistry *reg, uint op_id);

/* Execute pending rollback */
int rollback_execute(OpRollbackEntry *entry);

/* Get entry by operation ID */
OpRollbackEntry *rollback_find(RollbackRegistry *reg, uint op_id);

/* Clean up completed entries */
void rollback_cleanup(RollbackRegistry *reg);

/*
 * Background Verification API
 */

/* Check all pending operations against their required depth */
void verify_pending_operations(RollbackRegistry *reg, MsgOrd *dag);

/* Callback type for verification completion */
typedef void (*VerifyCallback)(uint op_id, int success, int confidence);

/* Register verification callback */
void register_verify_callback(VerifyCallback cb);

/*
 * Depth-Based Routing API
 */

/* Route operation with automatic depth classification */
int route_with_depth(MsgOrd *dag, struct Proc *caller, Fcall *t, Fcall *r,
                     char *path, RollbackRegistry *reg);

/* Route with explicit depth override */
int route_with_explicit_depth(MsgOrd *dag, struct Proc *caller, Fcall *t,
                              Fcall *r, char *path, ConsensusDepth depth,
                              RollbackRegistry *reg);

/*
 * Global rollback registry
 */
extern RollbackRegistry *global_rollback_registry;

#endif /* _CONSENSUS_DEPTH_H_ */
