/*
 * Lux9 MSGORD Kernel Implementation
 *
 * Lock-free message ordering via DAG consensus.
 * All 9P messages flow through MSGORD for total ordering.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* Manual typedefs */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "9p_router.h"
#include "consensus_depth.h"
#include "msgord.h"
#include "pebble.h"
/* Registry of MsgOrd instances */
static MsgOrd *msgords[MSGORD_MAX_DAGS];
static Lock registry_lock;

/* Global MSGORD instance (alias for msgords[0]) */
MsgOrd *msgord = nil;

/*
 * Lock for queue operations within a DAG
 * (We add this to ensure safety with multiple submitters)
 */
static Lock dag_locks[MSGORD_MAX_DAGS];

/*
 * Helpers
 */
static void lock_dag(MsgOrd *dag) {
  if (dag && dag->gd_id >= 0 && dag->gd_id < MSGORD_MAX_DAGS)
    ilock(&dag_locks[dag->gd_id]);
}

static void unlock_dag(MsgOrd *dag) {
  if (dag && dag->gd_id >= 0 && dag->gd_id < MSGORD_MAX_DAGS)
    iunlock(&dag_locks[dag->gd_id]);
}

/*
 * Initialize MSGORD subsystem
 */
void msgord_init(uint k_param) {
  if (msgord != nil)
    return;

  /* Initialize registry lock */
  /* lock_init(&registry_lock); // Assuming static init or auto-init handled by
   * system */

  /* Create System DAG (Instance 0) */
  msgord = msgord_create_instance(k_param);
  if (msgord == nil)
    panic("msgord_init: failed to create system DAG");

  print("msgord: initialized system DAG k=%d\n", msgord->gd_k_param);
}

/*
 * Create a new DAG instance
 */
MsgOrd *msgord_create_instance(uint k_param) {
  MsgOrd *dag;
  int id = -1;
  int i;

  ilock(&registry_lock);
  for (i = 0; i < MSGORD_MAX_DAGS; i++) {
    if (msgords[i] == nil) {
      id = i;
      break;
    }
  }

  if (id == -1) {
    iunlock(&registry_lock);
    return nil;
  }

  dag = xalloc(sizeof(MsgOrd));
  if (dag == nil) {
    iunlock(&registry_lock);
    return nil;
  }

  memset(dag, 0, sizeof(MsgOrd));
  dag->gd_id = id;
  dag->gd_k_param = k_param ? k_param : MSGORD_K_PARAMETER;
  dag->gd_max_anticone = MSGORD_MAX_ANTICONE;
  dag->gd_next_id = 1;
  dag->gd_global_seq = 0;
  dag->gd_initialized = 1;

  /* Initialize synchronization */

  msgords[id] = dag;
  iunlock(&registry_lock);

  return dag;
}

/*
 * Destroy a DAG instance
 */
void msgord_destroy_instance(MsgOrd *dag) {
  OrdMsg *msg, *next;
  int id;

  if (dag == nil)
    return;
  id = dag->gd_id;

  ilock(&registry_lock);
  if (msgords[id] != dag) {
    iunlock(&registry_lock);
    return; /* Sanity check failed */
  }

  /* Cannot destroy System DAG easily? Allow it for now if requested */
  if (id == 0 && msgord == dag) {
    msgord = nil;
  }

  msgords[id] = nil;
  iunlock(&registry_lock);

  /* Free all messages */
  lock_dag(dag);
  for (msg = dag->gd_head; msg != nil; msg = next) {
    next = msg->gm_next;
    /* Determine if we need to free payload data */
    if (msg->gm_payload.type == MSGORD_MSG_RAW && msg->gm_payload.raw.data) {
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
MsgOrd *msgord_get(int id) {
  if (id < 0 || id >= MSGORD_MAX_DAGS)
    return nil;
  /* This is racy without a reader lock or refcounting,
     but for this kernel architecture we assume cooperative or safe enough */
  return msgords[id];
}

/*
 * CLR compatibility: create state
 */
msgord_state_t *msgord_state_create(uint k_param) {
  msgord_init(k_param);
  return msgord;
}

/*
 * Allocate a new ghost message
 */
static OrdMsg *msgord_alloc(MsgOrd *dag, Proc *caller, char *path) {
  OrdMsg *msg;

  msg = xalloc(sizeof(OrdMsg));
  if (msg == nil)
    return nil;

  memset(msg, 0, sizeof(OrdMsg));

  msg->gm_caller = caller;
  msg->gm_id = dag->gd_next_id++;
  msg->gm_timestamp = seconds();
  msg->gm_state = MSGORD_STATE_PENDING;
  msg->gm_color = MSGORD_COLOR_BLUE; /* Default optimistic */

  if (path != nil)
    strncpy(msg->gm_path, path, sizeof(msg->gm_path) - 1);

  return msg;
}

/*
 * Enqueue message (append to DAG)
 */
static void msgord_enqueue(MsgOrd *dag, OrdMsg *msg) {
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
static void msgord_dequeue(MsgOrd *dag, OrdMsg *msg) {
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
int msgord_anticone(MsgOrd *dag, OrdMsg *msg) {
  /*@
    @ requires dag != \null && msg != \null;
    @ ensures \result >= 0;
    @ ensures msg->gm_anticone_size == \result;
    @ assigns msg->gm_anticone_size;
    @*/
  OrdMsg *gm;
  int anticone = 0;
  uint i;
  int is_parent;

  for (gm = dag->gd_head; gm != nil; gm = gm->gm_next) {
    if (gm == msg)
      continue;
    if (gm->gm_state != MSGORD_STATE_PENDING)
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
int msgord_color(MsgOrd *dag, OrdMsg *msg) {
  /*@
    @ requires dag != \null && msg != \null;
    @ ensures \result == msg->gm_color;
    @ ensures \result == MSGORD_COLOR_BLUE ==> msg->gm_anticone_size <= dag->gd_k_param;
    @ ensures \result == MSGORD_COLOR_RED ==> msg->gm_anticone_size > dag->gd_k_param;
    @ assigns msg->gm_color, msg->gm_anticone_size, dag->gd_blue_msgs, dag->gd_red_msgs;
    @*/
  int anticone = msgord_anticone(dag, msg);

  if (anticone <= (int)dag->gd_k_param) {
    msg->gm_color = MSGORD_COLOR_BLUE;
    dag->gd_blue_msgs++;
  } else {
    msg->gm_color = MSGORD_COLOR_RED;
    dag->gd_red_msgs++;
  }

  return msg->gm_color;
}

/*
 * Check if message can be delivered
 */
int msgord_can_deliver(MsgOrd *dag, OrdMsg *msg) {
  /*@
    @ requires dag != \null && msg != \null;
    @ ensures \result == 1 ==> msg->gm_color == MSGORD_COLOR_BLUE;
    @ assigns \nothing;
    @*/
  OrdMsg *gm;
  uint i;

  if (msg->gm_color != MSGORD_COLOR_BLUE)
    return 0;

  /*
    // Enforces causal DAG ordering per proofs/msgord/msgord_correctness.v
    // All parents must be in DELIVERED state before this message can be delivered.
   */
  for (i = 0; i < msg->gm_parent_count; i++) {
    for (gm = dag->gd_head; gm != nil; gm = gm->gm_next) {
      if (gm->gm_id == msg->gm_parents[i]) {
        if (gm->gm_state < MSGORD_STATE_DELIVERED)
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
static int _msgord_submit(MsgOrd *dag, Proc *caller, OrdPayload payload,
                          char *path, u64int nonce) {
  /*@
    @ requires dag != \null;
    @ ensures \result == 0 ==> dag->gd_total_msgs >= \old(dag->gd_total_msgs);
    @ assigns dag->gd_head, dag->gd_tail, dag->gd_count, dag->gd_total_msgs,
    @         dag->gd_blue_msgs, dag->gd_red_msgs, dag->gd_global_seq, dag->gd_next_id;
    @*/
  OrdMsg *msg;
  OrdMsg *tail;

  if (dag == nil || !dag->gd_initialized)
    return -1;

  /*
   * Kinetic Defense: Congestion Pricing
   * Calculate difficulty based on red message ratio.
   * Only enforce for User Processes (!kp).
   */
  if (caller && !caller->kp && dag->gd_total_msgs > 100) {
    int difficulty = 0;
    ulong ratio_pct = (dag->gd_red_msgs * 100) / dag->gd_total_msgs;

    if (ratio_pct > 10) { /* >10% red messages implies congestion */
      /* Base difficulty on ratio. 10% -> 1, 100% -> 19 */
      difficulty = 1 + ((ratio_pct - 10) / 5);

      /* Verify PoW */
      /* Context: caller PID binds work to the process */
      u64int context = (u64int)caller->pid;

      if (!pow_verify(nonce, context, difficulty)) {
        /* PoW failed or missing */
        return -1;
      }
    }
  }

  lock_dag(dag);

  msg = msgord_alloc(dag, caller, path);
  if (msg == nil) {
    unlock_dag(dag);
    return -1;
  }

  /*
    // Establishes total order (timestamp, id) per proofs/msgord/msgord_correctness.v
    // msg->gm_id is monotonic; msg->gm_timestamp is monotonic.
   */
  msg->gm_payload = payload;

  tail = dag->gd_tail;
  if (tail != nil) {
    msg->gm_parents[0] = tail->gm_id;
    msg->gm_parent_count = 1;
  }

  msgord_enqueue(dag, msg);
  dag->gd_total_msgs++;

  msgord_color(dag, msg);

  if (msg->gm_color == MSGORD_COLOR_BLUE) {
    msg->gm_state = MSGORD_STATE_ORDERED;
    msg->gm_global_seq = ++dag->gd_global_seq;
    wakeup(&dag->gd_rendez);
  } else {
    /* Critical Fix: Fail-Fast on RED (Saturation prevention) */
    /* Remove from DAG immediately */
    msgord_dequeue(dag, msg);
    dag->gd_total_msgs--;
    dag->gd_red_msgs--;

    unlock_dag(dag);
    xfree(msg);
    return -1;
  }

  unlock_dag(dag);
  return 0;
}

/*
 * Submit 9P message for MSGORD ordering
 */
int msgord_submit(MsgOrd *dag, Proc *caller, Fcall *t, char *path, u64int nonce) {
  /*@
    @ requires t != \null;
    @ ensures \result == 0 || \result == -1;
    @*/
  OrdPayload p;
  uint n;
  void *buf;

  /* Calculate size required for serialization */
  n = sizeS2M(t);
  buf = xalloc(n);
  if (buf == nil)
    return -1;

  /* Serialize Fcall into buffer (deep copy) */
  if (convS2M(t, buf, n) != n) {
    xfree(buf);
    return -1;
  }

  p.type = MSGORD_MSG_9P;
  /* Store serialized data in raw part of union */
  p.raw.data = buf;
  p.raw.len = n;

  /* Backward compatibility for calls passing nil dag */
  if (dag == nil)
    dag = msgord;

  if (_msgord_submit(dag, caller, p, path, nonce) < 0) {
    xfree(buf);
    return -1;
  }
  return 0;
}

/*
 * Submit generic data
 */
int msgord_submit_raw(MsgOrd *dag, Proc *caller, void *data, ulong len, u64int nonce) {
  /*@
    @ ensures \result == 0 || \result == -1;
    @*/
  OrdPayload p;
  void *buf;

  if (len > 0) {
    buf = xalloc(len);
    if (buf == nil)
      return -1;
    memmove(buf, data, len);
  } else {
    buf = nil;
  }

  p.type = MSGORD_MSG_RAW;
  p.raw.data = buf;
  p.raw.len = len;

  if (_msgord_submit(dag, caller, p, "raw", nonce) < 0) {
    if (buf)
      xfree(buf);
    return -1;
  }
  return 0;
}

/*
 * CLR compatibility: add message
 */
uint msgord_add_message(msgord_state_t *state, Proc *p, Fcall *t, char *path) {
  USED(state); /* In original code, but now we respect state if passed */
  if (state == nil)
    state = msgord;

  if (msgord_submit(state, p, t, path, 0) < 0)
    return 0;
  /* Warning: this reads next_id without lock, but standard pattern in this
   * codebase */
  return state->gd_next_id - 1;
}

/*
 * Get next ordered message ready for delivery
 */
OrdMsg *msgord_next(MsgOrd *dag) {
  OrdMsg *msg;

  if (dag == nil)
    return nil;

  lock_dag(dag);
  for (msg = dag->gd_head; msg != nil; msg = msg->gm_next) {
    if (msg->gm_state == MSGORD_STATE_ORDERED && msgord_can_deliver(dag, msg)) {
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
void msgord_complete(MsgOrd *dag, OrdMsg *msg) {
  if (msg == nil || dag == nil)
    return;

  lock_dag(dag);
  msgord_dequeue(dag, msg);
  unlock_dag(dag);

  msg->gm_state = MSGORD_STATE_COMPLETE;

  /* Free payload (both 9P and RAW now use deep copy) */
  if ((msg->gm_payload.type == MSGORD_MSG_RAW ||
       msg->gm_payload.type == MSGORD_MSG_9P) &&
      msg->gm_payload.raw.data) {
    xfree(msg->gm_payload.raw.data);
  }

  xfree(msg);
}

/*
 * Process one ordered message (System DAG specific mostly)
 */
int msgord_process_one(MsgOrd *dag) {
  OrdMsg *msg;
  Fcall reply;

  if (dag == nil)
    return 0;

  msg = msgord_next(dag);
  if (msg == nil)
    return 0;

  if (msg->gm_payload.type == MSGORD_MSG_9P) {
    memset(&reply, 0, sizeof(reply));
    /* If caller is nil (e.g. kernel task), we skip dispatch */
    if (msg->gm_caller)
      p9_dispatch(msg->gm_caller, msg->gm_payload.fcall, &reply);
  }
  /* For RAW messages, 'processing' simply means marking delivered so it flows
   * out */

  msg->gm_state = MSGORD_STATE_DELIVERED;
  msg->gm_ordered_time = seconds();

  msgord_complete(dag, msg);

  return 1;
}

/*
 * Process all ready messages
 */
void msgord_process_all(MsgOrd *dag) {
  while (msgord_process_one(dag))
    ;
}

/*
 * Get statistics
 */
void msgord_stats(MsgOrd *dag, uvlong *total, uvlong *blue, uvlong *red) {
  if (dag == nil)
    return;

  if (total != nil)
    *total = dag->gd_total_msgs;
  if (blue != nil)
    *blue = dag->gd_blue_msgs;
  if (red != nil)
    *red = dag->gd_red_msgs;
}

/*
 * Submit 9P message with completion callback
 */
uint msgord_submit_async(MsgOrd *dag, Proc *caller, Fcall *t, char *path,
                         MsgordCallback cb, void *cb_arg, u64int nonce) {
  OrdPayload p;
  OrdMsg *msg;
  OrdMsg *tail;
  uint id;
  uint n;
  void *buf;

  if (dag == nil)
    dag = msgord;
  if (dag == nil || !dag->gd_initialized)
    return 0;

  /*
   * Kinetic Defense: Congestion Pricing (Async)
   */
  if (caller && !caller->kp && dag->gd_total_msgs > 100) {
    int difficulty = 0;
    ulong ratio_pct = (dag->gd_red_msgs * 100) / dag->gd_total_msgs;

    if (ratio_pct > 10) {
      difficulty = 1 + ((ratio_pct - 10) / 5);
      u64int context = (u64int)caller->pid;

      if (!pow_verify(nonce, context, difficulty)) {
        return 0;
      }
    }
  }

  /* Deep copy Fcall */
  n = sizeS2M(t);
  buf = xalloc(n);
  if (buf == nil)
    return 0;

  if (convS2M(t, buf, n) != n) {
    xfree(buf);
    return 0;
  }

  p.type = MSGORD_MSG_9P;
  p.raw.data = buf;
  p.raw.len = n;

  lock_dag(dag);

  msg = msgord_alloc(dag, caller, path);
  if (msg == nil) {
    unlock_dag(dag);
    return 0;
  }

  msg->gm_payload = p;
  msg->gm_callback = cb;
  msg->gm_callback_arg = cb_arg;

  tail = dag->gd_tail;
  if (tail != nil) {
    msg->gm_parents[0] = tail->gm_id;
    msg->gm_parent_count = 1;
  }

  id = msg->gm_id;
  msgord_enqueue(dag, msg);
  dag->gd_total_msgs++;

  msgord_color(dag, msg);

  if (msg->gm_color == MSGORD_COLOR_BLUE) {
    msg->gm_state = MSGORD_STATE_ORDERED;
    msg->gm_global_seq = ++dag->gd_global_seq;
    wakeup(&dag->gd_rendez);
  } else {
    /* Critical Fix: Fail-Fast on RED (Saturation prevention) */
    msgord_dequeue(dag, msg);
    dag->gd_total_msgs--;
    dag->gd_red_msgs--;

    unlock_dag(dag);
    xfree(msg);
    return 0; /* Error */
  }

  unlock_dag(dag);
  return id;
}

/*
 * Find message by ID
 */
OrdMsg *msgord_find_by_id(MsgOrd *dag, uint id) {
  OrdMsg *msg;

  if (dag == nil)
    dag = msgord;
  if (dag == nil)
    return nil;

  lock_dag(dag);
  for (msg = dag->gd_head; msg != nil; msg = msg->gm_next) {
    if (msg->gm_id == id) {
      unlock_dag(dag);
      return msg;
    }
  }
  unlock_dag(dag);
  return nil;
}

/*
 * Set callback on existing message
 */
void msgord_set_callback(OrdMsg *msg, MsgordCallback cb, void *cb_arg) {
  if (msg == nil)
    return;
  msg->gm_callback = cb;
  msg->gm_callback_arg = cb_arg;
}

/*
 * Fire callbacks for all ready messages
 * Returns number of callbacks fired
 */
int msgord_fire_completions(MsgOrd *dag) {
  OrdMsg *msg, *next;
  int fired = 0;
  Fcall t, reply;

  if (dag == nil)
    dag = msgord;
  if (dag == nil)
    return 0;

  lock_dag(dag);
  for (msg = dag->gd_head; msg != nil; msg = next) {
    next = msg->gm_next;

    if (msg->gm_state != MSGORD_STATE_ORDERED)
      continue;
    if (!msgord_can_deliver(dag, msg))
      continue;

    /* Process the message */
    if (msg->gm_payload.type == MSGORD_MSG_9P && msg->gm_caller) {
      if (convM2S(msg->gm_payload.raw.data, msg->gm_payload.raw.len, &t) ==
          msg->gm_payload.raw.len) {
        memset(&reply, 0, sizeof(reply));
        unlock_dag(dag);
        p9_dispatch(msg->gm_caller, &t, &reply);
        lock_dag(dag);
      }
    }

    msg->gm_state = MSGORD_STATE_DELIVERED;
    msg->gm_ordered_time = seconds();

    /* Fire callback if registered */
    if (msg->gm_callback != nil) {
      unlock_dag(dag);
      msg->gm_callback(msg, MSGORD_CB_SUCCESS, msg->gm_callback_arg);
      lock_dag(dag);
      fired++;
    }

    /* Remove from DAG */
    msgord_dequeue(dag, msg);
    msg->gm_state = MSGORD_STATE_COMPLETE;

    /* Free payload (both 9P and RAW now use deep copy) */
    if ((msg->gm_payload.type == MSGORD_MSG_RAW ||
         msg->gm_payload.type == MSGORD_MSG_9P) &&
        msg->gm_payload.raw.data) {
      xfree(msg->gm_payload.raw.data);
    }
    xfree(msg);
  }
  unlock_dag(dag);

  return fired;
}

/*
 * Check consensus depth for an operation
 * Returns: 0 on success, -1 if message not found
 * confidence_out is 0-100 scale (no SSE/float in kernel)
 */
int msgord_check_consensus_depth(MsgOrd *dag, uint op_id, int required_depth,
                                 int *confidence_out) {
  OrdMsg *msg;
  int confidence;

  if (dag == nil)
    dag = msgord;
  if (dag == nil)
    return -1;

  msg = msgord_find_by_id(dag, op_id);
  if (msg == nil)
    return -1;

  /* Calculate confidence based on message state and consensus depth */
  if (msg->gm_state >= MSGORD_STATE_DELIVERED) {
    confidence = 100;
  } else if (msg->gm_state >= MSGORD_STATE_ORDERED) {
    /* Ordered but not yet delivered - high confidence */
    confidence = 95;
  } else if (msg->gm_color == MSGORD_COLOR_BLUE) {
    /* Blue (optimistic) - moderate confidence based on anticone */
    confidence = 80 - (int)msg->gm_anticone_size * 5;
    if (confidence < 50)
      confidence = 50;
  } else {
    /* Red (pessimistic) - lower confidence */
    confidence = 30;
  }

  /* Adjust for required depth */
  USED(required_depth); /* May adjust confidence threshold in future */

  if (confidence_out != nil)
    *confidence_out = confidence;

  return 0;
}

/*
 * Submit async with depth parameter (alternative signature for
 * consensus_depth.c) Returns: 0 on success, -1 on error
 */
int msgord_submit_async_depth(MsgOrd *dag, Proc *caller, void *t, void *r,
                              char *path, int depth, uint *msg_id_out, u64int nonce) {
  Fcall *fcall_t = (Fcall *)t;
  uint msg_id;

  USED(r); /* Reply will be filled by MSGORD processing */
  USED(
      depth); /* Depth is used for rollback registration in consensus_depth.c */

  if (dag == nil)
    dag = msgord;
  if (dag == nil || fcall_t == nil)
    return -1;

  /* Submit via existing async mechanism */
  msg_id = msgord_submit_async(dag, caller, fcall_t, path, nil, nil, nonce);
  if (msg_id == 0)
    return -1;

  if (msg_id_out != nil)
    *msg_id_out = msg_id;

  return 0;
}
