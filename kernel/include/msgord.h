/*
 * Lux9 MSGORD Kernel
 *
 * MSGORD consensus for 9P message ordering.
 * Provides total ordering of all kernel operations without locks.
 *
 * Based on PHANTOM MSGORD adapted for microkernel 9P.
 */

#ifndef _MSGORD_KERNEL_H_
#define _MSGORD_KERNEL_H_

/* Forward declarations */
struct Proc;
struct Fcall;
struct Rendez;
typedef struct Proc Proc;
typedef struct Fcall Fcall;
typedef struct Rendez Rendez;

/*
 * MSGORD Configuration
 */
#define MSGORD_K_PARAMETER 3   /* k-cluster parameter */
#define MSGORD_MAX_ANTICONE 10 /* Maximum anticone size */
#define MSGORD_MAX_PARENTS 8   /* Max parents per message */
#define MSGORD_GENESIS_ID 0    /* Genesis message ID */
#define MSGORD_MAX_DAGS 64     /* Max number of DAG instances */

/*
 * MSGORD Message Colors
 */
#define MSGORD_COLOR_RED 0
#define MSGORD_COLOR_BLUE 1

/*
 * MSGORD Message States
 */
#define MSGORD_STATE_PENDING 0
#define MSGORD_STATE_ORDERED 1
#define MSGORD_STATE_DELIVERED 2
#define MSGORD_STATE_COMPLETE 3

/*
 * MSGORD Message Types
 */
#define MSGORD_MSG_9P 0
#define MSGORD_MSG_RAW 1

/*
 * MSGORD Message Payload
 */
typedef struct OrdPayload {
  int type;
  union {
    Fcall *fcall;
    struct {
      void *data;
      ulong len;
    } raw;
  };
} OrdPayload;

/*
 * MSGORD Message Metadata
 * Attached to each 9P Fcall or raw data for consensus tracking
 */
typedef struct OrdMsg {
  /* Message identity */
  uint gm_id;          /* Unique message ID */
  uvlong gm_timestamp; /* Creation timestamp */

  /* The payload */
  OrdPayload gm_payload;

  Proc *gm_caller;   /* Calling process */
  char gm_path[256]; /* Target path */

  /* DAG relationships */
  uint gm_parent_count;
  uint gm_parents[MSGORD_MAX_PARENTS];

  /* Consensus state */
  uchar gm_color; /* BLUE or RED */
  uchar gm_state; /* Pending/Ordered/Delivered */
  ushort gm_anticone_size;

  /* Ordering */
  uvlong gm_global_seq;   /* Global sequence number */
  uvlong gm_ordered_time; /* When ordering determined */

  /* Completion callback for async operations */
  void (*gm_callback)(struct OrdMsg *, int status, void *arg);
  void *gm_callback_arg;

  /* Linked list */
  struct OrdMsg *gm_next;
  struct OrdMsg *gm_prev;
} OrdMsg;

/*
 * MSGORD DAG Structure
 */
typedef struct MsgOrd {
  int gd_id; /* DAG Instance ID */

  /* Message queue */
  OrdMsg *gd_head;
  OrdMsg *gd_tail;
  uint gd_count;

  /* ID generation */
  uint gd_next_id;
  uvlong gd_global_seq;

  /* Parameters */
  uint gd_k_param;
  uint gd_max_anticone;

  /* Statistics */
  uvlong gd_total_msgs;
  uvlong gd_blue_msgs;
  uvlong gd_red_msgs;
  uvlong gd_avg_latency;

  /* Synchronization for readers */
  Rendez *gd_rendez;

  /* State */
  int gd_initialized;
} MsgOrd;

/*
 * Global MSGORD instance (System DAG, ID 0)
 */
extern MsgOrd *msgord;

/*
 * Core API
 */

/* Initialize MSGORD subsystem (called by kernel main) */
void msgord_init(uint k_param);

/* Create a new dynamic DAG instance */
MsgOrd *msgord_create_instance(uint k_param);

/* Destroy a DAG instance */
void msgord_destroy_instance(MsgOrd *dag);

/* Get DAG instance by ID */
MsgOrd *msgord_get(int id);

/* Submit 9P message for ordering - REPLACES p9_route() */
int msgord_submit(MsgOrd *dag, Proc *caller, Fcall *t, char *path);

/* Submit raw data for ordering */
int msgord_submit_raw(MsgOrd *dag, Proc *caller, void *data, ulong len);

/* Get next ordered message ready for delivery */
OrdMsg *msgord_next(MsgOrd *dag);

/* Complete message and remove from DAG */
void msgord_complete(MsgOrd *dag, OrdMsg *msg);

/*
 * Ordering API
 */

/* Compute anticone for message */
int msgord_anticone(MsgOrd *dag, OrdMsg *msg);

/* Determine color (BLUE if anticone <= k) */
int msgord_color(MsgOrd *dag, OrdMsg *msg);

/* Check if message can be delivered */
int msgord_can_deliver(MsgOrd *dag, OrdMsg *msg);

/*
 * Processing
 */

/* Process one ordered message - called from scheduler */
int msgord_process_one(MsgOrd *dag);

/* Process all ready messages */
void msgord_process_all(MsgOrd *dag);

/*
 * Statistics
 */
void msgord_stats(MsgOrd *dag, uvlong *total, uvlong *blue, uvlong *red);

/*
 * Convenience macros
 */
#define msgord_is_blue(m) ((m)->gm_color == MSGORD_COLOR_BLUE)
#define msgord_is_ordered(m) ((m)->gm_state >= MSGORD_STATE_ORDERED)

/*
 * State type for external compatibility
 */
typedef MsgOrd msgord_state_t;

/* Create/destroy for CLR compatibility */
msgord_state_t *msgord_state_create(uint k_param);
uint msgord_add_message(msgord_state_t *state, Proc *p, Fcall *t,
                          char *path);

/*
 * Completion Callback API
 */
typedef void (*MsgordCallback)(OrdMsg *msg, int status, void *arg);

/* Submit 9P message with completion callback */
uint msgord_submit_async(MsgOrd *dag, Proc *caller, Fcall *t, char *path,
                           MsgordCallback cb, void *cb_arg);

/* Find message by ID */
OrdMsg *msgord_find_by_id(MsgOrd *dag, uint id);

/* Set callback on existing message */
void msgord_set_callback(OrdMsg *msg, MsgordCallback cb, void *cb_arg);

/* Fire callbacks for all ready messages (call from scheduler) */
int msgord_fire_completions(MsgOrd *dag);

/* Callback status codes */
#define MSGORD_CB_SUCCESS 0
#define MSGORD_CB_TIMEOUT 1
#define MSGORD_CB_ROLLBACK 2
#define MSGORD_CB_ERROR 3

/*
 * Consensus Depth Integration (for consensus_depth.c)
 * Note: ConsensusDepth is defined as enum in consensus_depth.h
 * Functions here use int for compatibility when consensus_depth.h is not
 * included
 */

/* Check consensus depth for an operation - confidence is 0-100 scale
 * depth argument is ConsensusDepth enum value (compatible with int) */
int msgord_check_consensus_depth(MsgOrd *dag, uint op_id,
                                   int required_depth, int *confidence_out);

/* Submit async with depth parameter (alternative signature for
 * consensus_depth.c)
 * t and r are Fcall* but declared as void* for header independence */
int msgord_submit_async_depth(MsgOrd *dag, Proc *caller, void *t, void *r,
                                char *path, int depth, uint *msg_id_out);

/* Macro alias for backwards compatibility */
#define msgord_submit_async_ex msgord_submit_async_depth

#endif /* _MSGORD_KERNEL_H_ */
