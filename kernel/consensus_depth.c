/*
 * Lux9 Selective Consensus Depth Implementation
 *
 * Operation classification and depth-based routing for MSGORD.
 */

#include "portlib.h"
#include "u.h"

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "consensus_depth.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "msgord.h"

/* Global rollback registry */
RollbackRegistry *global_rollback_registry = nil;
static RollbackRegistry _global_registry;
static Lock registry_lock;

/*
 * Path Classification Helpers
 */
static int path_starts_with(char *path, char *prefix) {
  if (path == nil || prefix == nil)
    return 0;
  return strncmp(path, prefix, strlen(prefix)) == 0;
}

/*
 * Classify operation based on Fcall type
 */
static OperationType classify_by_fcall_type(Fcall *t) {
  if (t == nil)
    return OP_TYPE_UNKNOWN;

  switch (t->type) {
  /* Metadata operations - typically local */
  case Tversion:
  case Tauth:
  case Tattach:
  case Tflush:
  case Tclunk:
    return OP_TYPE_LOCAL_FILE;

  /* Walk is typically local lookup */
  case Twalk:
    return OP_TYPE_LOCAL_FILE;

  /* Open/Create may need more consensus */
  case Topen:
  case Tcreate:
    return OP_TYPE_LOCAL_FILE;

  /* Read/Write depend on path */
  case Tread:
  case Twrite:
    return OP_TYPE_LOCAL_FILE; /* Refined by path checks in get_operation_type
                                */

  /* Remove may need consensus */
  case Tremove:
    return OP_TYPE_LOCAL_FILE;

  /* Stat typically local */
  case Tstat:
  case Twstat:
    return OP_TYPE_LOCAL_FILE;

  default:
    return OP_TYPE_UNKNOWN;
  }
}

/*
 * Check if operation is local memory (no consensus needed)
 */
int is_local_memory_operation(Fcall *t) {
  if (t == nil)
    return 0;

  /* Version and auth are always local */
  if (t->type == Tversion || t->type == Tauth)
    return 1;

  /* Flush is local cancellation */
  if (t->type == Tflush)
    return 1;

  /* Clunk is local cleanup */
  if (t->type == Tclunk)
    return 1;

  return 0;
}

/*
 * Check if operation is IPC between processes
 */
int is_ipc_between_processes(Fcall *t, char *path) {
  if (path == nil)
    return 0;

  /* /srv is service registry - IPC */
  if (path_starts_with(path, "/srv"))
    return 1;

  /* /proc writes to other processes */
  if (path_starts_with(path, "/proc") && t != nil && t->type == Twrite)
    return 1;

  /* Pipe operations */
  if (path_starts_with(path, "/pipe"))
    return 1;

  return 0;
}

/*
 * Check if operation is device access
 */
int is_device_operation(Fcall *t, char *path) {
  USED(t);
  if (path == nil)
    return 0;

  /* /dev is device namespace */
  if (path_starts_with(path, "/dev"))
    return 1;

  return 0;
}

/*
 * Check if operation is security critical
 */
int is_security_critical_operation(Fcall *t, char *path) {
  if (path == nil)
    return 0;

  /* Key operations */
  if (path_starts_with(path, "/mnt/factotum"))
    return 1;

  /* TPM operations */
  if (path_starts_with(path, "/dev/tpm"))
    return 1;

  /* Capability/ledger operations */
  if (path_starts_with(path, "/dev/ledger"))
    return 1;

  /* User database modifications */
  if (path_starts_with(path, "/adm") && t != nil && t->type == Twrite)
    return 1;

  /* Kernel configuration */
  if (path_starts_with(path, "/dev/reboot"))
    return 1;

  return 0;
}

/*
 * Get operation type from Fcall and path
 */
OperationType get_operation_type(Fcall *t, char *path) {
  /* Check security first (highest priority) */
  if (is_security_critical_operation(t, path))
    return OP_TYPE_SECURITY;

  /* Check IPC */
  if (is_ipc_between_processes(t, path))
    return OP_TYPE_IPC;

  /* Check device */
  if (is_device_operation(t, path))
    return OP_TYPE_DEVICE;

  /* Check local memory */
  if (is_local_memory_operation(t))
    return OP_TYPE_LOCAL_MEMORY;

  /* Default to Fcall-based classification */
  return classify_by_fcall_type(t);
}

/*
 * Main classification function
 * Determines consensus depth required for operation.
 */
/*@
  // Assigns consensus depth per proofs/msgord/msgord_consensus_proofs.v
 @*/
ConsensusDepth classify_operation(Fcall *t, char *path) {
  OperationType op_type = get_operation_type(t, path);

  switch (op_type) {
  case OP_TYPE_LOCAL_MEMORY:
    /* No consensus needed for process-local operations */
    return DEPTH_NONE;

  case OP_TYPE_LOCAL_FILE:
    /* Single-node confidence for local file ops */
    return DEPTH_LOCAL;

  case OP_TYPE_DEVICE:
    /* Device access needs local confidence */
    return DEPTH_LOCAL;

  case OP_TYPE_IPC:
    /* IPC needs cluster consensus (multi-process) */
    return DEPTH_CLUSTER;

  case OP_TYPE_NETWORK:
    /* Network needs global consensus */
    return DEPTH_GLOBAL;

  case OP_TYPE_SECURITY:
    /* Security-critical needs full network consensus */
    return DEPTH_GLOBAL;

  case OP_TYPE_CONSENSUS:
    /* Consensus operations need global */
    return DEPTH_GLOBAL;

  default:
    /* Unknown defaults to local (optimistic) */
    return DEPTH_LOCAL;
  }
}

/*
 * Rollback Registry Implementation
 */

void rollback_registry_init(RollbackRegistry *reg, uint max_entries) {
  if (reg == nil)
    return;

  memset(reg, 0, sizeof(RollbackRegistry));
  reg->max_entries = max_entries;

  /* Initialize global registry if this is it */
  if (reg == &_global_registry) {
    global_rollback_registry = reg;
  }
}

/*
 * Allocate rollback entry
 */
static OpRollbackEntry *rollback_alloc(void) {
  OpRollbackEntry *entry = xalloc(sizeof(OpRollbackEntry));
  if (entry == nil)
    return nil;

  memset(entry, 0, sizeof(OpRollbackEntry));
  return entry;
}

/*
 * Register optimistic execution for potential rollback
 */
/*@
  // Registers optimistic execution per proofs/msgord/msgord_consensus_proofs.v
 @*/
OpRollbackEntry *rollback_register(RollbackRegistry *reg, uint op_id,
                                   ConsensusDepth depth, Proc *caller, Fcall *t,
                                   Fcall *r) {
  OpRollbackEntry *entry;

  if (reg == nil)
    reg = global_rollback_registry;
  if (reg == nil)
    return nil;

  /* Check high water mark */
  if (reg->count >= reg->max_entries) {
    /* Force synchronous verification before adding more */
    if (msgord != nil) {
      verify_pending_operations(reg, msgord);
    }
  }

  entry = rollback_alloc();
  if (entry == nil)
    return nil;

  entry->op_id = op_id;
  entry->state = ROLLBACK_PENDING;
  entry->required_depth = depth;
  entry->op_type = (t != nil) ? get_operation_type(t, nil) : OP_TYPE_UNKNOWN;
  entry->caller = caller;
  entry->original_request = t;
  entry->executed_reply = r;
  entry->submit_time = (uvlong)seconds();
  entry->execute_time = entry->submit_time;

  /* Link into registry */
  lock(&registry_lock);
  entry->next = nil;
  entry->prev = reg->tail;
  if (reg->tail != nil)
    reg->tail->next = entry;
  else
    reg->head = entry;
  reg->tail = entry;
  reg->count++;
  reg->total_optimistic++;
  unlock(&registry_lock);

  return entry;
}

/*
 * Find entry by operation ID
 */
OpRollbackEntry *rollback_find(RollbackRegistry *reg, uint op_id) {
  OpRollbackEntry *entry;

  if (reg == nil)
    reg = global_rollback_registry;
  if (reg == nil)
    return nil;

  lock(&registry_lock);
  for (entry = reg->head; entry != nil; entry = entry->next) {
    if (entry->op_id == op_id) {
      unlock(&registry_lock);
      return entry;
    }
  }
  unlock(&registry_lock);

  return nil;
}

/*
 * Remove entry from registry (internal)
 */
static void rollback_unlink(RollbackRegistry *reg, OpRollbackEntry *entry) {
  if (entry->prev != nil)
    entry->prev->next = entry->next;
  else
    reg->head = entry->next;

  if (entry->next != nil)
    entry->next->prev = entry->prev;
  else
    reg->tail = entry->prev;

  reg->count--;
  entry->next = entry->prev = nil;
}

/*
 * Mark operation as verified (commit - no rollback needed)
 */
int rollback_commit(RollbackRegistry *reg, uint op_id) {
  OpRollbackEntry *entry;

  if (reg == nil)
    reg = global_rollback_registry;
  entry = rollback_find(reg, op_id);
  if (entry == nil)
    return -1;

  lock(&registry_lock);
  entry->state = ROLLBACK_COMMITTED;
  entry->verify_time = (uvlong)seconds();
  reg->total_committed++;

  /* Free snapshot if any */
  if (entry->snapshot_data != nil) {
    xfree(entry->snapshot_data);
    entry->snapshot_data = nil;
  }

  /* Remove from registry */
  rollback_unlink(reg, entry);
  unlock(&registry_lock);

  xfree(entry);
  return 0;
}

/*
 * Trigger rollback for failed consensus
 */
int rollback_trigger(RollbackRegistry *reg, uint op_id) {
  OpRollbackEntry *entry;
  int result;

  if (reg == nil)
    reg = global_rollback_registry;
  entry = rollback_find(reg, op_id);
  if (entry == nil)
    return -1;

  lock(&registry_lock);
  entry->state = ROLLBACK_TRIGGERED;
  entry->verify_time = (uvlong)seconds();
  reg->total_rollbacks++;
  unlock(&registry_lock);

  /* Execute the rollback */
  result = rollback_execute(entry);

  lock(&registry_lock);
  entry->state = ROLLBACK_COMPLETE;
  rollback_unlink(reg, entry);
  unlock(&registry_lock);

  /* Free resources */
  if (entry->snapshot_data != nil)
    xfree(entry->snapshot_data);
  xfree(entry);

  return result;
}

/*
 * Execute rollback for a single entry.
 * Strategy: KILL the process. Consensus failure is treated as fatal.
 */
int rollback_execute(OpRollbackEntry *entry) {
  if (entry == nil)
    return -1;

  if (entry->caller != nil) {
    if(getconf("debug.consensus"))
      print("consensus_depth: KILLING pid %lud due to consensus failure\n",
            entry->caller->pid);

    /*
     * Send a fatal note (NDebug) to terminate the process.
     * This is a hard failure - the process attempted an operation
     * that could not achieve consensus (e.g., conflicting writes).
     */
    postnote(entry->caller, 1, "consensus failure", NDebug);
  }

  return 0;
}

/*
 * Clean up completed/committed entries
 */
void rollback_cleanup(RollbackRegistry *reg) {
  OpRollbackEntry *entry, *next;

  if (reg == nil)
    reg = global_rollback_registry;
  if (reg == nil)
    return;

  lock(&registry_lock);
  for (entry = reg->head; entry != nil; entry = next) {
    next = entry->next;

    if (entry->state == ROLLBACK_COMMITTED ||
        entry->state == ROLLBACK_COMPLETE) {
      rollback_unlink(reg, entry);
      if (entry->snapshot_data != nil)
        xfree(entry->snapshot_data);
      xfree(entry);
    }
  }
  unlock(&registry_lock);
}

/*
 * Background Verification
 */

static VerifyCallback verify_cb = nil;

void register_verify_callback(VerifyCallback cb) { verify_cb = cb; }

/*
 * Verify all pending operations against their required depth
 */
void verify_pending_operations(RollbackRegistry *reg, MsgOrd *dag) {
  OpRollbackEntry *entry;
  int confidence;
  int result;

  if (reg == nil)
    reg = global_rollback_registry;
  if (reg == nil || dag == nil)
    return;

  lock(&registry_lock);
  for (entry = reg->head; entry != nil; entry = entry->next) {
    if (entry->state != ROLLBACK_PENDING)
      continue;

    /* Check consensus depth for this operation */
    result = msgord_check_consensus_depth(dag, entry->op_id,
                                          entry->required_depth, &confidence);

    if (result < 0) {
      /* Message not found - may have been completed already */
      continue;
    }

    /* Check if we've reached required confidence (0-100 scale) */
    int verified = 0;
    switch (entry->required_depth) {
    case DEPTH_NONE:
      verified = 1; /* Always verified */
      break;
    case DEPTH_LOCAL:
      verified = (confidence >= 90);
      break;
    case DEPTH_CLUSTER:
      verified = (confidence >= 80);
      break;
    case DEPTH_GLOBAL:
      verified = (confidence >= 95);
      break;
    }

    if (verified) {
      /* Mark as committed */
      unlock(&registry_lock);
      rollback_commit(reg, entry->op_id);
      lock(&registry_lock);

      if (verify_cb != nil)
        verify_cb(entry->op_id, 1, confidence);
    } else {
      /* Check for timeout */
      uvlong now = (uvlong)seconds();
      if (now > entry->submit_time + 30) { /* 30 second timeout */
        unlock(&registry_lock);
        if(getconf("debug.consensus"))
          print("consensus_depth: op %ud timed out (depth=%d conf=%d), "
                "triggering rollback\n",
                entry->op_id, entry->required_depth, confidence);
        rollback_trigger(reg, entry->op_id);
        lock(&registry_lock);
      }
    }
  }
  unlock(&registry_lock);
}

/*
 * Depth-Based Routing
 */

int route_with_depth(MsgOrd *dag, Proc *caller, Fcall *t, Fcall *r, char *path,
                     RollbackRegistry *reg) {
  ConsensusDepth depth = classify_operation(t, path);
  return route_with_explicit_depth(dag, caller, t, r, path, depth, reg);
}

int route_with_explicit_depth(MsgOrd *dag, Proc *caller, Fcall *t, Fcall *r,
                              char *path, ConsensusDepth depth,
                              RollbackRegistry *reg) {
  uint msg_id;
  int result;

  if (dag == nil)
    dag = msgord;
  if (reg == nil)
    reg = global_rollback_registry;

  /* Submit to MSGORD with optimistic execution */
  result = msgord_submit_async_depth(dag, caller, t, r, path, depth, &msg_id);
  if (result < 0)
    return result;

  /* Register for rollback tracking if not DEPTH_NONE */
  if (depth != DEPTH_NONE && reg != nil) {
    rollback_register(reg, msg_id, depth, caller, t, r);
  }

  return 0;
}

/*
 * Module initialization
 */
void consensus_depth_init(void) {
  rollback_registry_init(&_global_registry, 256);
  if(getconf("debug.consensus"))
    print("consensus_depth: initialized with max_entries=%ud\n",
          _global_registry.max_entries);
}
