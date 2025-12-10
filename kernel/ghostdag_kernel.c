/*
 * Lux9 GHOSTDAG Kernel Implementation
 *
 * Lock-free message ordering via DAG consensus.
 * All 9P messages flow through GHOSTDAG for total ordering.
 */

#include <u.h>
#include "portlib.h"

/* Manual typedefs (portlib.h gives structs but not always typedefs used by kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "9p_router.h"
#include "ghostdag_kernel.h"

/* Global GHOSTDAG instance */
GhostDAG *ghostdag = nil;

/*
 * Initialize GHOSTDAG subsystem
 */
void ghostdag_init(uint k_param) {
  if (ghostdag != nil)
    return;

  ghostdag = xalloc(sizeof(GhostDAG));
  if (ghostdag == nil)
    panic("ghostdag_init: out of memory");

  memset(ghostdag, 0, sizeof(GhostDAG));
  ghostdag->gd_k_param = k_param ? k_param : GHOSTDAG_K_PARAMETER;
  ghostdag->gd_max_anticone = GHOSTDAG_MAX_ANTICONE;
  ghostdag->gd_next_id = 1;
  ghostdag->gd_global_seq = 0;
  ghostdag->gd_initialized = 1;

  print("ghostdag: initialized k=%d\n", ghostdag->gd_k_param);
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
static GhostMsg *ghostdag_alloc(Proc *caller, Fcall *t, char *path) {
  GhostMsg *msg;

  msg = xalloc(sizeof(GhostMsg));
  if (msg == nil)
    return nil;

  memset(msg, 0, sizeof(GhostMsg));

  msg->gm_fcall = t;
  msg->gm_caller = caller;
  msg->gm_id = ghostdag->gd_next_id++;
  msg->gm_timestamp = seconds();
  msg->gm_state = GHOSTDAG_STATE_PENDING;
  msg->gm_color = GHOSTDAG_COLOR_BLUE;

  if (path != nil)
    strncpy(msg->gm_path, path, sizeof(msg->gm_path) - 1);

  return msg;
}

/*
 * Enqueue message (append to DAG)
 */
static void ghostdag_enqueue(GhostMsg *msg) {
  msg->gm_next = nil;
  msg->gm_prev = ghostdag->gd_tail;

  if (ghostdag->gd_tail != nil)
    ghostdag->gd_tail->gm_next = msg;
  else
    ghostdag->gd_head = msg;

  ghostdag->gd_tail = msg;
  ghostdag->gd_count++;
}

/*
 * Dequeue message
 */
static void ghostdag_dequeue(GhostMsg *msg) {
  if (msg->gm_prev != nil)
    msg->gm_prev->gm_next = msg->gm_next;
  else
    ghostdag->gd_head = msg->gm_next;

  if (msg->gm_next != nil)
    msg->gm_next->gm_prev = msg->gm_prev;
  else
    ghostdag->gd_tail = msg->gm_prev;

  ghostdag->gd_count--;
  msg->gm_next = msg->gm_prev = nil;
}

/*
 * Compute anticone size
 */
int ghostdag_anticone(GhostMsg *msg) {
  GhostMsg *m;
  int anticone = 0;
  uint i;
  int is_parent;

  for (m = ghostdag->gd_head; m != nil; m = m->gm_next) {
    if (m == msg)
      continue;
    if (m->gm_state != GHOSTDAG_STATE_PENDING)
      continue;

    is_parent = 0;
    for (i = 0; i < msg->gm_parent_count; i++) {
      if (msg->gm_parents[i] == m->gm_id) {
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
int ghostdag_color(GhostMsg *msg) {
  int anticone = ghostdag_anticone(msg);

  if (anticone <= (int)ghostdag->gd_k_param) {
    msg->gm_color = GHOSTDAG_COLOR_BLUE;
    ghostdag->gd_blue_msgs++;
  } else {
    msg->gm_color = GHOSTDAG_COLOR_RED;
    ghostdag->gd_red_msgs++;
  }

  return msg->gm_color;
}

/*
 * Check if message can be delivered
 */
int ghostdag_can_deliver(GhostMsg *msg) {
  GhostMsg *m;
  uint i;

  if (msg->gm_color != GHOSTDAG_COLOR_BLUE)
    return 0;

  for (i = 0; i < msg->gm_parent_count; i++) {
    for (m = ghostdag->gd_head; m != nil; m = m->gm_next) {
      if (m->gm_id == msg->gm_parents[i]) {
        if (m->gm_state < GHOSTDAG_STATE_DELIVERED)
          return 0;
        break;
      }
    }
  }

  return 1;
}

/*
 * Submit 9P message for GHOSTDAG ordering
 */
int ghostdag_submit(Proc *caller, Fcall *t, char *path) {
  GhostMsg *msg;
  GhostMsg *tail;

  if (ghostdag == nil || !ghostdag->gd_initialized)
    return -1;

  msg = ghostdag_alloc(caller, t, path);
  if (msg == nil)
    return -1;

  tail = ghostdag->gd_tail;
  if (tail != nil) {
    msg->gm_parents[0] = tail->gm_id;
    msg->gm_parent_count = 1;
  }

  ghostdag_enqueue(msg);
  ghostdag->gd_total_msgs++;

  ghostdag_color(msg);

  if (msg->gm_color == GHOSTDAG_COLOR_BLUE) {
    msg->gm_state = GHOSTDAG_STATE_ORDERED;
    msg->gm_global_seq = ++ghostdag->gd_global_seq;
  }

  return 0;
}

/*
 * CLR compatibility: add message
 */
uint ghostdag_add_message(ghostdag_state_t *state, Proc *p, Fcall *t,
                          char *path) {
  USED(state);
  if (ghostdag_submit(p, t, path) < 0)
    return 0;
  return ghostdag->gd_next_id - 1;
}

/*
 * Get next ordered message ready for delivery
 */
GhostMsg *ghostdag_next(void) {
  GhostMsg *msg;

  if (ghostdag == nil)
    return nil;

  for (msg = ghostdag->gd_head; msg != nil; msg = msg->gm_next) {
    if (msg->gm_state == GHOSTDAG_STATE_ORDERED && ghostdag_can_deliver(msg))
      return msg;
  }

  return nil;
}

/*
 * Complete message and remove from DAG
 */
void ghostdag_complete(GhostMsg *msg) {
  if (msg == nil)
    return;

  ghostdag_dequeue(msg);
  msg->gm_state = GHOSTDAG_STATE_COMPLETE;
  xfree(msg);
}

/*
 * Process one ordered message
 */
int ghostdag_process_one(void) {
  GhostMsg *msg;
  Fcall reply;

  if (ghostdag == nil)
    return 0;

  msg = ghostdag_next();
  if (msg == nil)
    return 0;

  memset(&reply, 0, sizeof(reply));
  p9_dispatch(msg->gm_caller, msg->gm_fcall, &reply);

  msg->gm_state = GHOSTDAG_STATE_DELIVERED;
  msg->gm_ordered_time = seconds();

  ghostdag_complete(msg);

  return 1;
}

/*
 * Process all ready messages
 */
void ghostdag_process_all(void) {
  while (ghostdag_process_one())
    ;
}

/*
 * Get statistics
 */
void ghostdag_stats(uvlong *total, uvlong *blue, uvlong *red) {
  if (ghostdag == nil)
    return;

  if (total != nil)
    *total = ghostdag->gd_total_msgs;
  if (blue != nil)
    *blue = ghostdag->gd_blue_msgs;
  if (red != nil)
    *red = ghostdag->gd_red_msgs;
}
