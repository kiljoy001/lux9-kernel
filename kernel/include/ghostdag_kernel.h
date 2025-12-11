/*
 * Lux9 GHOSTDAG Kernel
 *
 * GHOSTDAG consensus for 9P message ordering.
 * Provides total ordering of all kernel operations without locks.
 *
 * Based on PHANTOM GHOSTDAG adapted for microkernel 9P.
 */

#ifndef _GHOSTDAG_KERNEL_H_
#define _GHOSTDAG_KERNEL_H_

/* Forward declarations */
struct Proc;
struct Fcall;

/*
 * GHOSTDAG Configuration
 */
#define GHOSTDAG_K_PARAMETER 3   /* k-cluster parameter */
#define GHOSTDAG_MAX_ANTICONE 10 /* Maximum anticone size */
#define GHOSTDAG_MAX_PARENTS 8   /* Max parents per message */
#define GHOSTDAG_GENESIS_ID 0    /* Genesis message ID */
#define GHOSTDAG_MAX_DAGS 64     /* Max number of DAG instances */

/*
 * GHOSTDAG Message Colors
 */
#define GHOSTDAG_COLOR_RED 0
#define GHOSTDAG_COLOR_BLUE 1

/*
 * GHOSTDAG Message States
 */
#define GHOSTDAG_STATE_PENDING 0
#define GHOSTDAG_STATE_ORDERED 1
#define GHOSTDAG_STATE_DELIVERED 2
#define GHOSTDAG_STATE_COMPLETE 3

/*
 * GHOSTDAG Message Types
 */
#define GHOSTDAG_MSG_9P 0
#define GHOSTDAG_MSG_RAW 1

/*
 * GHOSTDAG Message Payload
 */
typedef struct GhostPayload {
  int type;
  union {
    Fcall *fcall;
    struct {
      void *data;
      ulong len;
    } raw;
  };
} GhostPayload;

/*
 * GHOSTDAG Message Metadata
 * Attached to each 9P Fcall or raw data for consensus tracking
 */
typedef struct GhostMsg {
  /* Message identity */
  uint gm_id;          /* Unique message ID */
  uvlong gm_timestamp; /* Creation timestamp */

  /* The payload */
  GhostPayload gm_payload;

  Proc *gm_caller;   /* Calling process */
  char gm_path[256]; /* Target path */

  /* DAG relationships */
  uint gm_parent_count;
  uint gm_parents[GHOSTDAG_MAX_PARENTS];

  /* Consensus state */
  uchar gm_color; /* BLUE or RED */
  uchar gm_state; /* Pending/Ordered/Delivered */
  ushort gm_anticone_size;

  /* Ordering */
  uvlong gm_global_seq;   /* Global sequence number */
  uvlong gm_ordered_time; /* When ordering determined */

  /* Linked list */
  struct GhostMsg *gm_next;
  struct GhostMsg *gm_prev;
} GhostMsg;

/*
 * GHOSTDAG DAG Structure
 */
typedef struct GhostDAG {
  int gd_id; /* DAG Instance ID */

  /* Message queue */
  GhostMsg *gd_head;
  GhostMsg *gd_tail;
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
  Rendez gd_rendez;

  /* State */
  int gd_initialized;
} GhostDAG;

/*
 * Global GHOSTDAG instance (System DAG, ID 0)
 */
extern GhostDAG *ghostdag;

/*
 * Core API
 */

/* Initialize GHOSTDAG subsystem (called by kernel main) */
void ghostdag_init(uint k_param);

/* Create a new dynamic DAG instance */
GhostDAG *ghostdag_create_instance(uint k_param);

/* Destroy a DAG instance */
void ghostdag_destroy_instance(GhostDAG *dag);

/* Get DAG instance by ID */
GhostDAG *ghostdag_get(int id);

/* Submit 9P message for ordering - REPLACES p9_route() */
int ghostdag_submit(GhostDAG *dag, Proc *caller, Fcall *t, char *path);

/* Submit raw data for ordering */
int ghostdag_submit_raw(GhostDAG *dag, Proc *caller, void *data, ulong len);

/* Get next ordered message ready for delivery */
GhostMsg *ghostdag_next(GhostDAG *dag);

/* Complete message and remove from DAG */
void ghostdag_complete(GhostDAG *dag, GhostMsg *msg);

/*
 * Ordering API
 */

/* Compute anticone for message */
int ghostdag_anticone(GhostDAG *dag, GhostMsg *msg);

/* Determine color (BLUE if anticone <= k) */
int ghostdag_color(GhostDAG *dag, GhostMsg *msg);

/* Check if message can be delivered */
int ghostdag_can_deliver(GhostDAG *dag, GhostMsg *msg);

/*
 * Processing
 */

/* Process one ordered message - called from scheduler */
int ghostdag_process_one(GhostDAG *dag);

/* Process all ready messages */
void ghostdag_process_all(GhostDAG *dag);

/*
 * Statistics
 */
void ghostdag_stats(GhostDAG *dag, uvlong *total, uvlong *blue, uvlong *red);

/*
 * Convenience macros
 */
#define ghostdag_is_blue(m) ((m)->gm_color == GHOSTDAG_COLOR_BLUE)
#define ghostdag_is_ordered(m) ((m)->gm_state >= GHOSTDAG_STATE_ORDERED)

/*
 * State type for external compatibility
 */
typedef GhostDAG ghostdag_state_t;

/* Create/destroy for CLR compatibility */
ghostdag_state_t *ghostdag_state_create(uint k_param);
uint ghostdag_add_message(ghostdag_state_t *state, Proc *p, Fcall *t,
                          char *path);

#endif /* _GHOSTDAG_KERNEL_H_ */
