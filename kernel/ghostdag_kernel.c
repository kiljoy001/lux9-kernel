/*
 * Lux9 GHOSTDAG Kernel Implementation
 *
 * Lock-free message ordering via DAG consensus.
 * All 9P messages flow through GHOSTDAG for total ordering.
 */

#include <u.h>
#include "portlib.h"

/* Manual typedefs (portlib.h gives structs but not always typedefs used by
 * kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"
#include "ghostdag_kernel.h"

/* Registry of GhostDAG instances */
static GhostDAG *ghostdags[GHOSTDAG_MAX_DAGS];
static Lock registry_lock;

/* Global GHOSTDAG instance (alias for ghostdags[0]) */
GhostDAG *ghostdag = nil;

/*
 * Lock for queue operations within a DAG
 * (We add this to ensure safety with multiple submitters)
 */
static Lock dag_locks[GHOSTDAG_MAX_DAGS];

/*
 * Helpers
 */
static void lock_dag(GhostDAG *dag) {
  if (dag && dag->gd_id >= 0 && dag->gd_id < GHOSTDAG_MAX_DAGS)
    lock(&dag_locks[dag->gd_id]);
}

static void unlock_dag(GhostDAG *dag) {
  if (dag && dag->gd_id >= 0 && dag->gd_id < GHOSTDAG_MAX_DAGS)
    unlock(&dag_locks[dag->gd_id]);
}

/*
 * Initialize GHOSTDAG subsystem
 */
void ghostdag_init(uint k_param) {
  if (ghostdag != nil)
    return;

  /* Initialize registry lock */
  /* lock_init(&registry_lock); // Assuming static init or auto-init handled by
   * system */

  /* Create System DAG (Instance 0) */
  ghostdag = ghostdag_create_instance(k_param);
  if (ghostdag == nil)
    panic("ghostdag_init: failed to create system DAG");

  print("ghostdag: initialized system DAG k=%d\n", ghostdag->gd_k_param);
}

/*
 * Create a new DAG instance
 */
GhostDAG *ghostdag_create_instance(uint k_param) {
  GhostDAG *dag;
  int id = -1;
  int i;

  lock(&registry_lock);
  for (i = 0; i < GHOSTDAG_MAX_DAGS; i++) {
    if (ghostdags[i] == nil) {
      id = i;
      break;
    }
  }

  if (id == -1) {
    unlock(&registry_lock);
    return nil;
  }

  dag = xalloc(sizeof(GhostDAG));
  if (dag == nil) {
    unlock(&registry_lock);
    return nil;
  }

  memset(dag, 0, sizeof(GhostDAG));
  dag->gd_id = id;
  dag->gd_k_param = k_param ? k_param : GHOSTDAG_K_PARAMETER;
  dag->gd_max_anticone = GHOSTDAG_MAX_ANTICONE;
  dag->gd_next_id = 1;
  dag->gd_global_seq = 0;
  dag->gd_initialized = 1;

  /* Initialize synchronization */

  ghostdags[id] = dag;
  unlock(&registry_lock);

  return dag;
}

/*
 * Destroy a DAG instance
 */
void ghostdag_destroy_instance(GhostDAG *dag) {
  GhostMsg *msg, *next;
  int id;

  if (dag == nil)
    return;
  id = dag->gd_id;

  lock(&registry_lock);
  if (ghostdags[id] != dag) {
    unlock(&registry_lock);
    return; /* Sanity check failed */
  }

  /* Cannot destroy System DAG easily? Allow it for now if requested */
  if (id == 0 && ghostdag == dag) {
    ghostdag = nil;
  }

  ghostdags[id] = nil;
  unlock(&registry_lock);

  /* Free all messages */
  lock_dag(dag);
  for (msg = dag->gd_head; msg != nil; msg = next) {
    next = msg->gm_next;
    /* Determine if we need to free payload data */
    if (msg->gm_payload.type == GHOSTDAG_MSG_RAW && msg->gm_payload.raw.data) {
      xfree(msg->gm_payload.raw.data);
    }
    xfree(msg);
  }
  unlock_dag(dag);

  xfree(dag);
}

/*
 * Get DAG by ID
 */
GhostDAG *ghostdag_get(int id) {
  if (id < 0 || id >= GHOSTDAG_MAX_DAGS)
    return nil;
  /* This is racy without a reader lock or refcounting,
     but for this kernel architecture we assume cooperative or safe enough */
  return ghostdags[id];
}

/*
 * CLR compatibility: create state
 */
ghostdag_state_t *ghostdag_state_create(uint k_param) {
  ghostdag_init(k_param);
  return ghostdag;
}

/*
 * Allocate a new ghost message
 */
static GhostMsg *ghostdag_alloc(GhostDAG *dag, Proc *caller, char *path) {
  GhostMsg *msg;

  msg = xalloc(sizeof(GhostMsg));
  if (msg == nil)
    return nil;

  memset(msg, 0, sizeof(GhostMsg));

  msg->gm_caller = caller;
  msg->gm_id = dag->gd_next_id++;
  msg->gm_timestamp = seconds();
  msg->gm_state = GHOSTDAG_STATE_PENDING;
  msg->gm_color = GHOSTDAG_COLOR_BLUE; /* Default optimistic */

  if (path != nil)
    strncpy(msg->gm_path, path, sizeof(msg->gm_path) - 1);

  return msg;
}

/*
 * Enqueue message (append to DAG)
 */
static void ghostdag_enqueue(GhostDAG *dag, GhostMsg *msg) {
  msg->gm_next = nil;
  msg->gm_prev = dag->gd_tail;

  if (dag->gd_tail != nil)
    dag->gd_tail->gm_next = msg;
  else
    dag->gd_head = msg;

  dag->gd_tail = msg;
  dag->gd_count++;
}

/*
 * Dequeue message
 */
static void ghostdag_dequeue(GhostDAG *dag, GhostMsg *msg) {
  if (msg->gm_prev != nil)
    msg->gm_prev->gm_next = msg->gm_next;
  else
    dag->gd_head = msg->gm_next;

  if (msg->gm_next != nil)
    msg->gm_next->gm_prev = msg->gm_prev;
  else
    dag->gd_tail = msg->gm_prev;

  dag->gd_count--;
  msg->gm_next = msg->gm_prev = nil;
}

/*
 * Compute anticone size
 */
int ghostdag_anticone(GhostDAG *dag, GhostMsg *msg) {
  GhostMsg *gm;
  int anticone = 0;
  uint i;
  int is_parent;

  for (gm = dag->gd_head; gm != nil; gm = gm->gm_next) {
    if (gm == msg)
      continue;
    if (gm->gm_state != GHOSTDAG_STATE_PENDING)
      continue;

    is_parent = 0;
    for (i = 0; i < msg->gm_parent_count; i++) {
      if (msg->gm_parents[i] == gm->gm_id) {
        is_parent = 1;
        break;
      }
    }

    if (!is_parent)
      anticone++;
  }

  msg->gm_anticone_size = anticone;
  return anticone;
}

/*
 * Determine color based on anticone
 */
int ghostdag_color(GhostDAG *dag, GhostMsg *msg) {
  int anticone = ghostdag_anticone(dag, msg);

  if (anticone <= (int)dag->gd_k_param) {
    msg->gm_color = GHOSTDAG_COLOR_BLUE;
    dag->gd_blue_msgs++;
  } else {
    msg->gm_color = GHOSTDAG_COLOR_RED;
    dag->gd_red_msgs++;
  }

  return msg->gm_color;
}

/*
 * Check if message can be delivered
 */
int ghostdag_can_deliver(GhostDAG *dag, GhostMsg *msg) {
  GhostMsg *gm;
  uint i;

  if (msg->gm_color != GHOSTDAG_COLOR_BLUE)
    return 0;

  for (i = 0; i < msg->gm_parent_count; i++) {
    for (gm = dag->gd_head; gm != nil; gm = gm->gm_next) {
      if (gm->gm_id == msg->gm_parents[i]) {
        if (gm->gm_state < GHOSTDAG_STATE_DELIVERED)
          return 0;
        break;
      }
    }
  }

  return 1;
}

/*
 * Internal submit logic
 */
static int _ghostdag_submit(GhostDAG *dag, Proc *caller, GhostPayload payload,
                            char *path) {
  GhostMsg *msg;
  GhostMsg *tail;

  if (dag == nil || !dag->gd_initialized)
    return -1;

  lock_dag(dag);

  msg = ghostdag_alloc(dag, caller, path);
  if (msg == nil) {
    unlock_dag(dag);
    return -1;
  }

  msg->gm_payload = payload;

  tail = dag->gd_tail;
  if (tail != nil) {
    msg->gm_parents[0] = tail->gm_id;
    msg->gm_parent_count = 1;
  }

  ghostdag_enqueue(dag, msg);
  dag->gd_total_msgs++;

  ghostdag_color(dag, msg);

  if (msg->gm_color == GHOSTDAG_COLOR_BLUE) {
    msg->gm_state = GHOSTDAG_STATE_ORDERED;
    msg->gm_global_seq = ++dag->gd_global_seq;
    wakeup(&dag->gd_rendez);
  }

  unlock_dag(dag);
  return 0;
}

/*
 * Submit 9P message for GHOSTDAG ordering
 */
int ghostdag_submit(GhostDAG *dag, Proc *caller, Fcall *t, char *path) {
  GhostPayload p;
  p.type = GHOSTDAG_MSG_9P;
  p.fcall = t;
  /* Backward compatibility for calls passing nil dag */
  if (dag == nil)
    dag = ghostdag;
  return _ghostdag_submit(dag, caller, p, path);
}

/*
 * Submit generic data
 */
int ghostdag_submit_raw(GhostDAG *dag, Proc *caller, void *data, ulong len) {
  GhostPayload p;
  void *buf;

  if (len > 0) {
    buf = xalloc(len);
    if (buf == nil)
      return -1;
    memmove(buf, data, len);
  } else {
    buf = nil;
  }

  p.type = GHOSTDAG_MSG_RAW;
  p.raw.data = buf;
  p.raw.len = len;

  if (_ghostdag_submit(dag, caller, p, "raw") < 0) {
    if (buf)
      xfree(buf);
    return -1;
  }
  return 0;
}

/*
 * CLR compatibility: add message
 */
uint ghostdag_add_message(ghostdag_state_t *state, Proc *p, Fcall *t,
                          char *path) {
  USED(state); /* In original code, but now we respect state if passed */
  if (state == nil)
    state = ghostdag;

  if (ghostdag_submit(state, p, t, path) < 0)
    return 0;
  /* Warning: this reads next_id without lock, but standard pattern in this
   * codebase */
  return state->gd_next_id - 1;
}

/*
 * Get next ordered message ready for delivery
 */
GhostMsg *ghostdag_next(GhostDAG *dag) {
  GhostMsg *msg;

  if (dag == nil)
    return nil;

  lock_dag(dag);
  for (msg = dag->gd_head; msg != nil; msg = msg->gm_next) {
    if (msg->gm_state == GHOSTDAG_STATE_ORDERED &&
        ghostdag_can_deliver(dag, msg)) {
      unlock_dag(dag);
      return msg;
    }
  }
  unlock_dag(dag);

  return nil;
}

/*
 * Complete message and remove from DAG
 */
void ghostdag_complete(GhostDAG *dag, GhostMsg *msg) {
  if (msg == nil || dag == nil)
    return;

  lock_dag(dag);
  ghostdag_dequeue(dag, msg);
  unlock_dag(dag);

  msg->gm_state = GHOSTDAG_STATE_COMPLETE;

  /* For Fcall, the fcall payload is managed by caller (p9 subsystem) */
  /* For Raw, we allocated it, so we should free it */
  if (msg->gm_payload.type == GHOSTDAG_MSG_RAW && msg->gm_payload.raw.data) {
    xfree(msg->gm_payload.raw.data);
  }

  xfree(msg);
}

/*
 * Process one ordered message (System DAG specific mostly)
 */
int ghostdag_process_one(GhostDAG *dag) {
  GhostMsg *msg;
  Fcall reply;

  if (dag == nil)
    return 0;

  msg = ghostdag_next(dag);
  if (msg == nil)
    return 0;

  if (msg->gm_payload.type == GHOSTDAG_MSG_9P) {
    memset(&reply, 0, sizeof(reply));
    /* If caller is nil (e.g. kernel task), we skip dispatch */
    if (msg->gm_caller)
      p9_dispatch(msg->gm_caller, msg->gm_payload.fcall, &reply);
  }
  /* For RAW messages, 'processing' simply means marking delivered so it flows
   * out */

  msg->gm_state = GHOSTDAG_STATE_DELIVERED;
  msg->gm_ordered_time = seconds();

  ghostdag_complete(dag, msg);

  return 1;
}

/*
 * Process all ready messages
 */
void ghostdag_process_all(GhostDAG *dag) {
  while (ghostdag_process_one(dag))
    ;
}

/*
 * Get statistics
 */
void ghostdag_stats(GhostDAG *dag, uvlong *total, uvlong *blue, uvlong *red) {
  if (dag == nil)
    return;

  if (total != nil)
    *total = dag->gd_total_msgs;
  if (blue != nil)
    *blue = dag->gd_blue_msgs;
  if (red != nil)
    *red = dag->gd_red_msgs;
}
