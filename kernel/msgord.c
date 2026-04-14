/*
 * Lux9 MSGORD Kernel Implementation
 *
 * Conflict-frontier message ordering via bounded DAG admission.
 * Conflicting operations serialize on resource tips; unrelated ones stay
 * concurrent.
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

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

static MsgOrd *msgord_alloc_struct(void) {
  return xallocz(sizeof(MsgOrd), 1);
}

static void msgord_init_struct(MsgOrd *dag, int id, uint k_param) {
  dag->gd_id = id;
  dag->gd_k_param = k_param ? k_param : MSGORD_K_PARAMETER;
  dag->gd_max_anticone = MSGORD_MAX_ANTICONE;
  dag->gd_next_id = 1;
  dag->gd_global_seq = 0;
  dag->gd_initialized = 1;
}

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
 * Conflict-frontier helpers
 *
 * Instead of chaining every message on the global tail, derive parents from
 * the latest accepted tips for the resources an operation touches. This keeps
 * unrelated services concurrent while still serializing conflicting operations.
 */
#define MSGORD_HASH_OFFSET 1469598103934665603ULL
#define MSGORD_HASH_PRIME 1099511628211ULL
#define MSGORD_KEY_ROOT 0x0100000000000000ULL
#define MSGORD_KEY_PATH 0x0200000000000000ULL
#define MSGORD_KEY_PARENT 0x0300000000000000ULL
#define MSGORD_KEY_FID 0x0400000000000000ULL
#define MSGORD_KEY_OP 0x0500000000000000ULL
#define MSGORD_KEY_EXCHANGE 0x0600000000000000ULL

static uvlong msgord_hash_bytes(uvlong seed, char *s, int n) {
  uvlong h;
  int i;

  h = seed ? seed : MSGORD_HASH_OFFSET;
  if (s == nil || n <= 0)
    return h;

  for (i = 0; i < n; i++) {
    h ^= (uchar)s[i];
    h *= MSGORD_HASH_PRIME;
  }

  return h;
}

static uvlong msgord_hash_string(uvlong seed, char *s) {
  uvlong h;

  h = seed ? seed : MSGORD_HASH_OFFSET;
  if (s == nil)
    return h;

  while (*s != 0) {
    h ^= (uchar)*s++;
    h *= MSGORD_HASH_PRIME;
  }

  return h;
}

static uvlong msgord_hash_u32(uvlong seed, u32int v) {
  char buf[BIT32SZ];

  PBIT32(buf, v);
  return msgord_hash_bytes(seed, buf, sizeof(buf));
}

static int msgord_add_resource_key(uvlong *keys, int nkeys, uvlong key) {
  /*@
    @ requires \valid(keys + (0 .. MSGORD_MAX_RESOURCE_KEYS-1));
    @ requires 0 <= nkeys <= MSGORD_MAX_RESOURCE_KEYS;
    @ assigns keys[0 .. MSGORD_MAX_RESOURCE_KEYS-1];
    @ ensures nkeys <= \result <= MSGORD_MAX_RESOURCE_KEYS;
    @*/
  int i;

  if (key == 0)
    return nkeys;

  for (i = 0; i < nkeys; i++) {
    if (keys[i] == key)
      return nkeys;
  }

  if (nkeys < MSGORD_MAX_RESOURCE_KEYS)
    keys[nkeys++] = key;
  return nkeys;
}

static int msgord_add_parent_id(OrdMsg *msg, uint parent_id) {
  /*@
    @ requires msg != \null;
    @ assigns msg->gm_parent_count,
    @         msg->gm_parents[0 .. MSGORD_MAX_PARENTS-1];
    @ ensures \result == 0 || \result == 1;
    @ ensures 0 <= msg->gm_parent_count <= MSGORD_MAX_PARENTS;
    @*/
  uint i;

  if (parent_id == 0)
    return 0;

  for (i = 0; i < msg->gm_parent_count; i++) {
    if (msg->gm_parents[i] == parent_id)
      return 0;
  }

  if (msg->gm_parent_count >= MSGORD_MAX_PARENTS)
    return 0;

  msg->gm_parents[msg->gm_parent_count++] = parent_id;
  return 1;
}

static int msgord_messages_conflict(OrdMsg *a, OrdMsg *b) {
  /*@
    @ requires a != \null && b != \null;
    @ assigns \nothing;
    @ ensures \result == 0 || \result == 1;
    @ ensures a->gm_resource_count == 0 || b->gm_resource_count == 0 ==> \result == 1;
    @*/
  uint i, j;

  if (a->gm_resource_count == 0 || b->gm_resource_count == 0)
    return 1;

  for (i = 0; i < a->gm_resource_count; i++) {
    for (j = 0; j < b->gm_resource_count; j++) {
      if (a->gm_resource_keys[i] == b->gm_resource_keys[j])
        return 1;
    }
  }

  return 0;
}

void msgord_spec_init(MsgOrdSpec *spec) {
  /*@
    @ requires spec == \null || \valid(spec);
    @ assigns spec == \null ? \nothing : *spec;
    @ ensures spec == \null || spec->resource_count == 0;
    @*/
  if (spec == nil)
    return;
  memset(spec, 0, sizeof(*spec));
}

int msgord_spec_add(MsgOrdSpec *spec, uvlong key) {
  /*@
    @ requires spec == \null || \valid(spec);
    @ assigns spec == \null ? \nothing :
    @         spec->resource_count,
    @         spec->resource_keys[0 .. MSGORD_MAX_RESOURCE_KEYS-1];
    @ ensures \result == 0 || \result == 1;
    @ ensures spec == \null ==> \result == 0;
    @ ensures spec != \null ==> 0 <= spec->resource_count <= MSGORD_MAX_RESOURCE_KEYS;
    @ ensures spec != \null && \result == 1 ==> spec->resource_count >= \old(spec->resource_count);
    @*/
  int nkeys;

  if (spec == nil)
    return 0;
  nkeys = msgord_add_resource_key(spec->resource_keys, spec->resource_count,
                                  key);
  if (nkeys == spec->resource_count)
    return 0;
  spec->resource_count = (uchar)nkeys;
  return 1;
}

uvlong msgord_key_op(uchar op_type) {
  /*@
    @ assigns \nothing;
    @*/
  return msgord_hash_u32(MSGORD_KEY_OP, op_type);
}

uvlong msgord_key_fid(u32int fid) {
  /*@
    @ assigns \nothing;
    @ ensures fid == NOFID ==> \result == 0;
    @*/
  if (fid == NOFID)
    return 0;
  return msgord_hash_u32(MSGORD_KEY_FID, fid);
}

uvlong msgord_key_root(char *path) {
  /*@
    @ assigns \nothing;
    @ ensures path == \null ==> \result == 0;
    @*/
  char *start, *slash;

  if (path == nil || *path == 0)
    return 0;

  start = path;
  while (*start == '/')
    start++;

  slash = start;
  while (*slash != 0 && *slash != '/')
    slash++;

  if (slash <= start)
    return 0;
  return msgord_hash_bytes(MSGORD_KEY_ROOT, start, (int)(slash - start));
}

uvlong msgord_key_path(char *path) {
  /*@
    @ assigns \nothing;
    @ ensures path == \null ==> \result == 0;
    @*/
  if (path == nil || *path == 0)
    return 0;
  return msgord_hash_string(MSGORD_KEY_PATH, path);
}

uvlong msgord_key_parent(char *path) {
  /*@
    @ assigns \nothing;
    @ ensures path == \null ==> \result == 0;
    @*/
  char *path_end;

  if (path == nil || *path == 0)
    return 0;

  path_end = path + strlen(path);
  while (path_end > path && path_end[-1] == '/')
    path_end--;
  while (path_end > path && path_end[-1] != '/')
    path_end--;
  if (path_end <= path)
    return 0;
  return msgord_hash_bytes(MSGORD_KEY_PARENT, path, (int)(path_end - path));
}

uvlong msgord_key_exchange(const ExchangeHandle *handle) {
  /*@
    @ requires handle == \null || \valid_read(handle);
    @ assigns \nothing;
    @ ensures handle == \null ==> \result == 0;
    @*/
  uvlong h;

  if (handle == nil)
    return 0;

  h = msgord_hash_bytes(MSGORD_KEY_EXCHANGE, (char *)handle->hash,
                        BLIND_LEDGER_CAP_SIZE);
  h = msgord_hash_u32(h, handle->type);
  return h;
}

static int msgord_tip_probe(MsgOrd *dag, uvlong key) {
  /*@
    @ requires dag != \null;
    @ assigns \nothing;
    @ ensures -1 <= \result < MSGORD_TIP_SLOTS;
    @*/
  uint i, slot;

  slot = (uint)(key % MSGORD_TIP_SLOTS);
  for (i = 0; i < MSGORD_TIP_SLOTS; i++) {
    uint idx = (slot + i) % MSGORD_TIP_SLOTS;
    if (!dag->gd_tip_used[idx] || dag->gd_tip_keys[idx] == key)
      return (int)idx;
  }
  return -1;
}

static uint msgord_tip_lookup(MsgOrd *dag, uvlong key) {
  /*@
    @ requires dag != \null;
    @ assigns \nothing;
    @*/
  int slot;

  slot = msgord_tip_probe(dag, key);
  if (slot < 0)
    return 0;
  if (!dag->gd_tip_used[slot] || dag->gd_tip_keys[slot] != key)
    return 0;
  return dag->gd_tip_msgs[slot];
}

static int msgord_tip_update(MsgOrd *dag, uvlong key, uint msg_id) {
  /*@
    @ requires dag != \null;
    @ assigns dag->gd_tip_used[0 .. MSGORD_TIP_SLOTS-1],
    @         dag->gd_tip_keys[0 .. MSGORD_TIP_SLOTS-1],
    @         dag->gd_tip_msgs[0 .. MSGORD_TIP_SLOTS-1];
    @ ensures \result == 0 || \result == -1;
    @*/
  int slot;

  slot = msgord_tip_probe(dag, key);
  if (slot < 0)
    return -1;
  dag->gd_tip_used[slot] = 1;
  dag->gd_tip_keys[slot] = key;
  dag->gd_tip_msgs[slot] = msg_id;
  return 0;
}

static int msgord_index_probe(MsgOrd *dag, uint id) {
  /*@
    @ requires dag != \null;
    @ assigns \nothing;
    @ ensures -1 <= \result < MSGORD_ID_SLOTS;
    @*/
  uint i, slot;

  slot = id % MSGORD_ID_SLOTS;
  for (i = 0; i < MSGORD_ID_SLOTS; i++) {
    uint idx = (slot + i) % MSGORD_ID_SLOTS;
    if (!dag->gd_index_used[idx] || dag->gd_index_ids[idx] == id)
      return (int)idx;
  }
  return -1;
}

static void msgord_index_insert(MsgOrd *dag, OrdMsg *msg) {
  /*@
    @ requires dag != \null && msg != \null;
    @ assigns dag->gd_index_used[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_ids[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_msgs[0 .. MSGORD_ID_SLOTS-1];
    @*/
  int slot;

  slot = msgord_index_probe(dag, msg->gm_id);
  if (slot < 0)
    return;
  dag->gd_index_used[slot] = 1;
  dag->gd_index_ids[slot] = msg->gm_id;
  dag->gd_index_msgs[slot] = msg;
}

static void msgord_index_remove(MsgOrd *dag, uint id) {
  /*@
    @ requires dag != \null;
    @ assigns dag->gd_index_used[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_ids[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_msgs[0 .. MSGORD_ID_SLOTS-1];
    @*/
  int slot;
  uint idx;
  OrdMsg *msg;
  uint msg_id;

  slot = msgord_index_probe(dag, id);
  if (slot < 0)
    return;
  if (!dag->gd_index_used[slot] || dag->gd_index_ids[slot] != id)
    return;
  dag->gd_index_used[slot] = 0;
  dag->gd_index_ids[slot] = 0;
  dag->gd_index_msgs[slot] = nil;

  /*
   * Repair the probe cluster behind the removed slot so lookups still reach
   * later entries that hashed into the same run.
   */
  for (idx = ((uint)slot + 1) % MSGORD_ID_SLOTS; dag->gd_index_used[idx];
       idx = (idx + 1) % MSGORD_ID_SLOTS) {
    msg_id = dag->gd_index_ids[idx];
    msg = dag->gd_index_msgs[idx];
    dag->gd_index_used[idx] = 0;
    dag->gd_index_ids[idx] = 0;
    dag->gd_index_msgs[idx] = nil;
    if (msg != nil) {
      int reinsert = msgord_index_probe(dag, msg_id);
      if (reinsert >= 0) {
        dag->gd_index_used[reinsert] = 1;
        dag->gd_index_ids[reinsert] = msg_id;
        dag->gd_index_msgs[reinsert] = msg;
      }
    }
  }
}

static OrdMsg *msgord_index_lookup(MsgOrd *dag, uint id) {
  /*@
    @ requires dag != \null;
    @ assigns \nothing;
    @ ensures \result == \null || \valid(\result);
    @ ensures \result == \null || \result->gm_id == id;
    @*/
  int slot;

  slot = msgord_index_probe(dag, id);
  if (slot < 0)
    return nil;
  if (!dag->gd_index_used[slot] || dag->gd_index_ids[slot] != id)
    return nil;
  return dag->gd_index_msgs[slot];
}

static int msgord_select_parents(MsgOrd *dag, OrdMsg *msg,
                                 const MsgOrdSpec *spec) {
  /*@
    @ requires dag != \null && msg != \null;
    @ requires spec == \null || \valid_read(spec);
    @ assigns msg->gm_resource_count,
    @         msg->gm_resource_keys[0 .. MSGORD_MAX_RESOURCE_KEYS-1],
    @         msg->gm_parent_count,
    @         msg->gm_parents[0 .. MSGORD_MAX_PARENTS-1];
    @ ensures \result == msg->gm_resource_count;
    @ ensures 0 <= \result <= MSGORD_MAX_RESOURCE_KEYS;
    @ ensures 0 <= msg->gm_parent_count <= MSGORD_MAX_PARENTS;
    @*/
  uint parent_id;
  int nkeys;
  uint i;

  msg->gm_resource_count = 0;
  if (spec != nil) {
    for (i = 0; i < spec->resource_count && i < MSGORD_MAX_RESOURCE_KEYS; i++) {
      nkeys = msgord_add_resource_key(msg->gm_resource_keys,
                                      msg->gm_resource_count,
                                      spec->resource_keys[i]);
      if (nkeys > msg->gm_resource_count)
        msg->gm_resource_count = (uchar)nkeys;
    }
  }

  if (dag->gd_barrier_tip != 0)
    msgord_add_parent_id(msg, dag->gd_barrier_tip);

  if (msg->gm_resource_count == 0 && dag->gd_global_tip != 0)
    msgord_add_parent_id(msg, dag->gd_global_tip);

  if (dag->gd_tip_overflow && dag->gd_global_tip != 0)
    msgord_add_parent_id(msg, dag->gd_global_tip);

  for (i = 0; i < msg->gm_resource_count && msg->gm_parent_count < MSGORD_MAX_PARENTS;
       i++) {
    parent_id = msgord_tip_lookup(dag, msg->gm_resource_keys[i]);
    if (parent_id == 0 || parent_id == msg->gm_id)
      continue;
    msgord_add_parent_id(msg, parent_id);
  }

  return msg->gm_resource_count;
}

static void msgord_publish_tips(MsgOrd *dag, OrdMsg *msg) {
  /*@
    @ requires dag != \null && msg != \null;
    @ assigns dag->gd_global_tip,
    @         dag->gd_barrier_tip,
    @         dag->gd_tip_overflow,
    @         dag->gd_tip_used[0 .. MSGORD_TIP_SLOTS-1],
    @         dag->gd_tip_keys[0 .. MSGORD_TIP_SLOTS-1],
    @         dag->gd_tip_msgs[0 .. MSGORD_TIP_SLOTS-1];
    @ ensures dag->gd_global_tip == msg->gm_id;
    @ ensures msg->gm_resource_count == 0 ==> dag->gd_barrier_tip == msg->gm_id;
    @ ensures msg->gm_resource_count != 0 ==> dag->gd_barrier_tip == \old(dag->gd_barrier_tip);
    @*/
  uint i;

  dag->gd_global_tip = msg->gm_id;
  if (msg->gm_resource_count == 0)
    dag->gd_barrier_tip = msg->gm_id;

  for (i = 0; i < msg->gm_resource_count; i++) {
    if (msgord_tip_update(dag, msg->gm_resource_keys[i], msg->gm_id) < 0)
      dag->gd_tip_overflow = 1;
  }
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

  dag = msgord_alloc_struct();
  if (dag == nil) {
    iunlock(&registry_lock);
    return nil;
  }

  msgord_init_struct(dag, id, k_param);

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
  OrdMsg *msg = xallocz(sizeof(OrdMsg), 1);
  if (msg == nil)
    return nil;

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
static void msgord_enqueue_unsafe(MsgOrd *dag, OrdMsg *msg) {
  msg->gm_next = nil;
  msg->gm_prev = dag->gd_tail;

  if (dag->gd_tail != nil)
    dag->gd_tail->gm_next = msg;
  else
    dag->gd_head = msg;

  dag->gd_tail = msg;
  dag->gd_count++;
}

static void msgord_enqueue(MsgOrd *dag, OrdMsg *msg) {
  if (dag == nil || msg == nil)
    return;
  msgord_enqueue_unsafe(dag, msg);
}

/*
 * Dequeue message
 */
static void msgord_dequeue_unsafe(MsgOrd *dag, OrdMsg *msg) {
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

static void msgord_dequeue(MsgOrd *dag, OrdMsg *msg) {
  if (dag == nil || msg == nil)
    return;
  msgord_dequeue_unsafe(dag, msg);
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
    if (!msgord_messages_conflict(msg, gm))
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
    @ ensures \result == MSGORD_COLOR_BLUE ==> msg->gm_anticone_size <=
    dag->gd_k_param;
    @ ensures \result == MSGORD_COLOR_RED ==> msg->gm_anticone_size >
    dag->gd_k_param;
    @ assigns msg->gm_color, msg->gm_anticone_size, dag->gd_blue_msgs,
    dag->gd_red_msgs;
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
  uint i;

  if (msg->gm_color != MSGORD_COLOR_BLUE)
    return 0;

  /*
    // Enforces causal DAG ordering per proofs/msgord/msgord_correctness.v
    // All parents must be in DELIVERED state before this message can be
    delivered.
  */
  for (i = 0; i < msg->gm_parent_count; i++) {
    OrdMsg *parent = msgord_index_lookup(dag, msg->gm_parents[i]);
    if (parent != nil && parent->gm_state < MSGORD_STATE_DELIVERED)
      return 0;
  }

  return 1;
}

/*
 * Internal submit logic
 */
static int _msgord_submit(MsgOrd *dag, Proc *caller, OrdPayload payload,
                          char *path, const MsgOrdSpec *spec,
                          u64int nonce) {
  uint id;
  /*@
    @ requires dag != \null;
    @ ensures \result > 0 ==> dag->gd_total_msgs == \old(dag->gd_total_msgs) + 1;
    @ ensures \result > 0 ==> dag->gd_global_seq == \old(dag->gd_global_seq) + 1;
    @ ensures \result > 0 ==> dag->gd_global_tip != 0;
    @ ensures \result < 0 ==> dag->gd_total_msgs == \old(dag->gd_total_msgs);
    @ ensures \result < 0 ==> dag->gd_global_seq == \old(dag->gd_global_seq);
    @ assigns dag->gd_head, dag->gd_tail, dag->gd_count, dag->gd_total_msgs,
    @         dag->gd_blue_msgs, dag->gd_red_msgs, dag->gd_global_seq,
    @         dag->gd_global_tip, dag->gd_barrier_tip, dag->gd_next_id,
    @         dag->gd_tip_overflow,
    @         dag->gd_tip_used[0 .. MSGORD_TIP_SLOTS-1],
    @         dag->gd_tip_keys[0 .. MSGORD_TIP_SLOTS-1],
    @         dag->gd_tip_msgs[0 .. MSGORD_TIP_SLOTS-1],
    @         dag->gd_index_used[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_ids[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_msgs[0 .. MSGORD_ID_SLOTS-1];
    @*/
  OrdMsg *msg;

  if (dag == nil || !dag->gd_initialized)
    return -1;

  /*
   * Kinetic Defense: Congestion Pricing
   * Calculate difficulty based on red message ratio.
   * Only enforce for User Processes (!kp).
   */
  if (caller && !caller->kp && dag->gd_total_msgs > 100) {
    ulong ratio_pct = (dag->gd_red_msgs * 100) / dag->gd_total_msgs;
    int difficulty = pow_calculate_difficulty(POW_OP_MSGORD, ratio_pct);

    if (difficulty > 0) {
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
   // Establishes total order (timestamp, id) per
    proofs/msgord/msgord_correctness.v
    // msg->gm_id is monotonic; msg->gm_timestamp is monotonic.
   */
  msg->gm_payload = payload;
  msgord_select_parents(dag, msg, spec);

  msgord_enqueue(dag, msg);
  msgord_index_insert(dag, msg);
  dag->gd_total_msgs++;

  msgord_color(dag, msg);

  if (msg->gm_color == MSGORD_COLOR_BLUE) {
    msg->gm_state = MSGORD_STATE_ORDERED;
    msg->gm_global_seq = ++dag->gd_global_seq;
    msgord_publish_tips(dag, msg);
    wakeup(&dag->gd_rendez);
  } else {
    /* Critical Fix: Fail-Fast on RED (Saturation prevention) */
    /* Remove from DAG immediately */
    msgord_dequeue(dag, msg);
    msgord_index_remove(dag, msg->gm_id);
    dag->gd_total_msgs--;
    dag->gd_red_msgs--;

    unlock_dag(dag);
    xfree(msg);
    return -1;
  }

  id = msg->gm_id;
  unlock_dag(dag);
  return (int)id;
}

/*
 * Submit 9P message for MSGORD ordering
 */
int msgord_submit(MsgOrd *dag, Proc *caller, Fcall *t, char *path,
                  const MsgOrdSpec *spec, u64int nonce) {
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

  if (_msgord_submit(dag, caller, p, path, spec, nonce) < 0) {
    xfree(buf);
    return -1;
  }
  return 0;
}

/*
 * Submit message using exchange page
 */
uint msgord_submit_exchange(MsgOrd *dag, Proc *caller, ExchangeHandle handle,
                            ulong offset, ulong len, char *path,
                            const MsgOrdSpec *spec, u64int nonce) {
  /*@
    @ ensures dag != \null && \result > 0 ==> dag->gd_global_tip != 0;
    @*/
  OrdPayload p;

  p.type = MSGORD_MSG_EXCHANGE;
  p.exchange.handle = handle;
  p.exchange.offset = offset;
  p.exchange.len = len;

  if (dag == nil)
    dag = msgord;

  int ret = _msgord_submit(dag, caller, p, path, spec, nonce);
  return (ret < 0) ? 0 : (uint)ret;
}

/*
 * Submit generic data
 */
int msgord_submit_raw(MsgOrd *dag, Proc *caller, void *data, ulong len,
                      const MsgOrdSpec *spec, u64int nonce) {
  /*@
    @ requires len == 0 || data != \null;
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

  if (_msgord_submit(dag, caller, p, "raw", spec, nonce) < 0) {
    if (buf)
      xfree(buf);
    return -1;
  }
  return 0;
}

/*
 * CLR compatibility: add message
 */
uint msgord_add_message(msgord_state_t *state, Proc *p, Fcall *t, char *path,
                        const MsgOrdSpec *spec) {
  /*@
    @ requires t != \null;
    @ ensures \result == 0 || \result > 0;
    @*/
  USED(state); /* In original code, but now we respect state if passed */
  if (state == nil)
    state = msgord;

  if (msgord_submit(state, p, t, path, spec, 0) < 0)
    return 0;
  /* Warning: this reads next_id without lock, but standard pattern in this
   * codebase */
  return state->gd_next_id - 1;
}

/*
 * Get next ordered message ready for delivery
 */
OrdMsg *msgord_next(MsgOrd *dag) {
  /*@
    @ requires dag == \null || \valid(dag);
    @ assigns \nothing;
    @ ensures \result == \null || \valid(\result);
    @ ensures \result == \null || \result->gm_state == MSGORD_STATE_ORDERED;
    @*/
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
  /*@
    @ requires dag != \null && msg != \null;
    @ assigns dag->gd_head, dag->gd_tail, dag->gd_count,
    @         dag->gd_index_used[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_ids[0 .. MSGORD_ID_SLOTS-1],
    @         dag->gd_index_msgs[0 .. MSGORD_ID_SLOTS-1];
    @*/
  if (msg == nil || dag == nil)
    return;

  lock_dag(dag);
  msgord_dequeue(dag, msg);
  msgord_index_remove(dag, msg->gm_id);
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
  Fcall t;
  Fcall reply;

  if (dag == nil)
    return 0;

  msg = msgord_next(dag);
  if (msg == nil)
    return 0;

  if (msg->gm_payload.type == MSGORD_MSG_9P) {
    t = (Fcall){0};
    reply = (Fcall){0};
    /* If caller is nil (e.g. kernel task), we skip dispatch */
    if (msg->gm_caller &&
        convM2S(msg->gm_payload.raw.data, msg->gm_payload.raw.len, &t) ==
            msg->gm_payload.raw.len)
      p9_dispatch(msg->gm_caller, &t, &reply);
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
  /*@
    @ requires dag == \null || \valid(dag);
    @ requires total == \null || \valid(total);
    @ requires blue == \null || \valid(blue);
    @ requires red == \null || \valid(red);
    @ assigns total == \null ? \nothing : *total,
    @         blue == \null ? \nothing : *blue,
    @         red == \null ? \nothing : *red;
    @ ensures dag != \null && total != \null ==> *total == dag->gd_total_msgs;
    @ ensures dag != \null && blue != \null ==> *blue == dag->gd_blue_msgs;
    @ ensures dag != \null && red != \null ==> *red == dag->gd_red_msgs;
    @*/
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
                         const MsgOrdSpec *spec, MsgordCallback cb,
                         void *cb_arg, u64int nonce) {
  /*@
    @ requires t != \null;
    @ ensures dag != \null && \result > 0 ==> dag->gd_global_tip != 0;
    @*/
  OrdPayload p;
  OrdMsg *msg;
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
    ulong ratio_pct = (dag->gd_red_msgs * 100) / dag->gd_total_msgs;
    int difficulty = pow_calculate_difficulty(POW_OP_MSGORD, ratio_pct);

    if (difficulty > 0) {
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
  msgord_select_parents(dag, msg, spec);

  id = msg->gm_id;
  msgord_enqueue(dag, msg);
  msgord_index_insert(dag, msg);
  dag->gd_total_msgs++;

  msgord_color(dag, msg);

  if (msg->gm_color == MSGORD_COLOR_BLUE) {
    msg->gm_state = MSGORD_STATE_ORDERED;
    msg->gm_global_seq = ++dag->gd_global_seq;
    msgord_publish_tips(dag, msg);
    wakeup(&dag->gd_rendez);
  } else {
    /* Critical Fix: Fail-Fast on RED (Saturation prevention) */
    msgord_dequeue(dag, msg);
    msgord_index_remove(dag, msg->gm_id);
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
  /*@
    @ requires dag == \null || \valid(dag);
    @ assigns \nothing;
    @ ensures \result == \null || \valid(\result);
    @ ensures \result == \null || \result->gm_id == id;
    @*/
  OrdMsg *msg;

  if (dag == nil)
    dag = msgord;
  if (dag == nil)
    return nil;

  lock_dag(dag);
  msg = msgord_index_lookup(dag, id);
  unlock_dag(dag);
  return msg;
}

/*
 * Set callback on existing message
 */
void msgord_set_callback(OrdMsg *msg, MsgordCallback cb, void *cb_arg) {
  /*@
    @ requires msg == \null || \valid(msg);
    @ assigns msg == \null ? \nothing : msg->gm_callback,
    @         msg == \null ? \nothing : msg->gm_callback_arg;
    @ ensures msg == \null || msg->gm_callback == cb;
    @ ensures msg == \null || msg->gm_callback_arg == cb_arg;
    @*/
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
  /*@
    @ requires dag == \null || \valid(dag);
    @ assigns \everything;
    @ ensures \result >= 0;
    @*/
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
        reply = (Fcall){0};
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
    msgord_index_remove(dag, msg->gm_id);
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
  /*@
    @ requires dag == \null || \valid(dag);
    @ requires confidence_out == \null || \valid(confidence_out);
    @ assigns confidence_out == \null ? \nothing : *confidence_out;
    @ ensures \result == 0 || \result == -1;
    @ ensures \result == 0 && confidence_out != \null ==>
    @         0 <= *confidence_out <= 100;
    @*/
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
                              char *path, const MsgOrdSpec *spec, int depth,
                              uint *msg_id_out, u64int nonce) {
  /*@
    @ requires t != \null;
    @ requires msg_id_out == \null || \valid(msg_id_out);
    @ assigns msg_id_out == \null ? \nothing : *msg_id_out;
    @ ensures \result == 0 || \result == -1;
    @ ensures \result == 0 && msg_id_out != \null ==> *msg_id_out > 0;
    @*/
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
  msg_id = msgord_submit_async(dag, caller, fcall_t, path, spec, nil, nil,
                               nonce);
  if (msg_id == 0)
    return -1;

  if (msg_id_out != nil)
    *msg_id_out = msg_id;

  return 0;
}
