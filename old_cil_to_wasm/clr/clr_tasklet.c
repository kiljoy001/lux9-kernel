/*
 * CLR Tasklet System - Memory-Efficient Pooled Implementation
 *
 * Tasklets are packed into exchange pages (16 slots × 256B = 4KB).
 * This provides efficient memory usage and exchange page integration.
 */

/* Manual Plan 9 Types (avoiding include maze) */
#define _U_H_
#define nil ((void *)0)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#define USED(x)                                                                \
  if (x) {                                                                     \
  }
#define BY2PG 4096

/* Minimal Lock type */
typedef struct Lock Lock;
struct Lock {
  int key;
};
static inline void lock(Lock *l) { (void)l; }
static inline void unlock(Lock *l) { (void)l; }

/* Memory functions - declared to avoid implicit function warnings */
extern void *mallocz(ulong size, int clr);
extern void *xalloc(ulong size);
extern void free(void *p);
extern void *memset(void *s, int c, ulong n);
extern void *memmove(void *dst, const void *src, ulong n);
extern int print(char *fmt, ...);

#include "clr_tasklet.h"

/* Tasklet scheduler state */
typedef struct TaskletScheduler TaskletScheduler;
struct TaskletScheduler {
  TaskletPool *pools;
  TaskletSlot *run_queue_head;
  TaskletSlot *run_queue_tail;
  TaskletSlot *current;
  ulong next_id;
  ulong next_pool_id;
  Lock lock;
  int initialized;

  ulong total_created;
  ulong total_destroyed;
  ulong pools_allocated;
};

static TaskletScheduler clr_sched;

/* Allocate a new tasklet pool (one 4KB page) */
static TaskletPool *pool_alloc(void) {
  TaskletPool *pool;
  int i;

  pool = mallocz(sizeof(TaskletPool), 1);
  if (pool == nil)
    return nil;

  /* Use xalloc for page allocation */
  pool->slots = xalloc(BY2PG);
  if (pool->slots == nil) {
    free(pool);
    return nil;
  }

  memset(pool->slots, 0, BY2PG);

  lock(&clr_sched.lock);
  pool->pool_id = clr_sched.next_pool_id++;
  unlock(&clr_sched.lock);

  pool->freemask = 0xFFFF; /* All 16 slots free */

  for (i = 0; i < TASKLET_SLOTS_PER_PAGE; i++) {
    pool->slots[i].magic = TASKLET_MAGIC;
    pool->slots[i].state = TASKLET_FREE;
    pool->slots[i].pool_index = pool->pool_id;
    pool->slots[i].slot_index = i;
  }

  clr_sched.pools_allocated++;

  return pool;
}

/* Find a free slot in a pool */
static TaskletSlot *pool_alloc_slot(TaskletPool *pool) {
  unsigned int mask, bit;
  int i;
  Lock *plock = (Lock *)pool->lock;

  lock(plock);
  mask = pool->freemask;
  if (mask == 0) {
    unlock(plock);
    return nil;
  }

  for (i = 0; i < TASKLET_SLOTS_PER_PAGE; i++) {
    bit = 1U << i;
    if (mask & bit) {
      pool->freemask &= ~bit;
      unlock(plock);
      return &pool->slots[i];
    }
  }

  unlock(plock);
  return nil;
}

/* Free a slot back to its pool */
static void pool_free_slot(TaskletSlot *slot) {
  TaskletPool *pool;
  unsigned int bit;
  Lock *plock;

  if (slot == nil || slot->magic != TASKLET_MAGIC)
    return;

  for (pool = clr_sched.pools; pool != nil; pool = pool->next) {
    if (pool->pool_id == slot->pool_index)
      break;
  }

  if (pool == nil)
    return;

  slot->state = TASKLET_FREE;
  slot->id = 0;
  slot->next = nil;
  slot->blocked_on = nil;

  if (slot->overflow_heap != nil) {
    free(slot->overflow_heap);
    slot->overflow_heap = nil;
  }

  bit = 1U << slot->slot_index;
  plock = (Lock *)pool->lock;
  lock(plock);
  pool->freemask |= bit;
  unlock(plock);
}

void clr_tasklet_init(void) {
  memset(&clr_sched, 0, sizeof(clr_sched));
  clr_sched.next_id = 1;
  clr_sched.next_pool_id = 1;
  clr_sched.initialized = 1;
  print("CLR: tasklet scheduler initialized (pooled design)\n");
}

TaskletSlot *clr_tasklet_create(ulong assembly_id, ulong method_token,
                                int priority) {
  TaskletPool *pool;
  TaskletSlot *slot = nil;

  if (!clr_sched.initialized)
    clr_tasklet_init();

  for (pool = clr_sched.pools; pool != nil; pool = pool->next) {
    slot = pool_alloc_slot(pool);
    if (slot != nil)
      break;
  }

  if (slot == nil) {
    pool = pool_alloc();
    if (pool == nil)
      return nil;

    lock(&clr_sched.lock);
    pool->next = clr_sched.pools;
    clr_sched.pools = pool;
    unlock(&clr_sched.lock);

    slot = pool_alloc_slot(pool);
    if (slot == nil)
      return nil;
  }

  lock(&clr_sched.lock);
  slot->id = clr_sched.next_id++;
  clr_sched.total_created++;
  unlock(&clr_sched.lock);

  slot->state = TASKLET_CREATED;
  slot->assembly_id = assembly_id;
  slot->method_token = method_token;
  slot->priority = priority;
  slot->ip = 0;
  slot->stack_top = 0;
  slot->locals_count = 0;
  slot->flags = 0;
  slot->next = nil;
  slot->blocked_on = nil;
  slot->overflow_heap = nil;

  print("CLR: created tasklet %ud in pool %ud slot %ud\n", slot->id,
        slot->pool_index, slot->slot_index);

  return slot;
}

void clr_tasklet_ready(TaskletSlot *t) {
  if (t == nil || t->state == TASKLET_DONE || t->state == TASKLET_FREE)
    return;

  t->state = TASKLET_READY;
  t->next = nil;

  lock(&clr_sched.lock);
  if (clr_sched.run_queue_tail == nil) {
    clr_sched.run_queue_head = t;
    clr_sched.run_queue_tail = t;
  } else {
    clr_sched.run_queue_tail->next = t;
    clr_sched.run_queue_tail = t;
  }
  unlock(&clr_sched.lock);
}

static TaskletSlot *dequeue(void) {
  TaskletSlot *t;

  lock(&clr_sched.lock);
  t = clr_sched.run_queue_head;
  if (t != nil) {
    clr_sched.run_queue_head = t->next;
    if (clr_sched.run_queue_head == nil)
      clr_sched.run_queue_tail = nil;
    t->next = nil;
  }
  unlock(&clr_sched.lock);

  return t;
}

void clr_tasklet_yield(void) {
  TaskletSlot *current = clr_sched.current;

  if (current == nil)
    return;

  if (current->state == TASKLET_RUNNING) {
    current->state = TASKLET_READY;
    clr_tasklet_ready(current);
  }

  clr_tasklet_schedule();
}

void clr_tasklet_block(void *channel) {
  TaskletSlot *current = clr_sched.current;

  if (current == nil)
    return;

  current->state = TASKLET_BLOCKED;
  current->blocked_on = channel;

  clr_tasklet_schedule();
}

void clr_tasklet_resume(TaskletSlot *t) {
  if (t == nil || t->state != TASKLET_BLOCKED)
    return;

  t->blocked_on = nil;
  clr_tasklet_ready(t);
}

void clr_tasklet_schedule(void) {
  TaskletSlot *next;

  next = dequeue();
  if (next == nil) {
    clr_sched.current = nil;
    return;
  }

  clr_sched.current = next;
  next->state = TASKLET_RUNNING;
}

TaskletSlot *clr_tasklet_current(void) { return clr_sched.current; }

void clr_tasklet_destroy(TaskletSlot *t) {
  if (t == nil)
    return;

  lock(&clr_sched.lock);
  clr_sched.total_destroyed++;
  unlock(&clr_sched.lock);

  pool_free_slot(t);
}

int clr_tasklet_push(TaskletSlot *t, ulong value) {
  if (t == nil)
    return -1;

  if (t->stack_top >= TASKLET_INLINE_STACK)
    return -1;

  t->eval_stack[t->stack_top++] = value;
  return 0;
}

ulong clr_tasklet_pop(TaskletSlot *t) {
  if (t == nil || t->stack_top <= 0)
    return 0;

  return t->eval_stack[--t->stack_top];
}

int clr_tasklet_setlocal(TaskletSlot *t, int index, ulong value) {
  if (t == nil || index < 0)
    return -1;

  if (index >= TASKLET_INLINE_LOCALS)
    return -1;

  t->locals[index] = value;
  if (index >= t->locals_count)
    t->locals_count = index + 1;

  return 0;
}

ulong clr_tasklet_getlocal(TaskletSlot *t, int index) {
  if (t == nil || index < 0 || index >= t->locals_count)
    return 0;

  return t->locals[index];
}

void clr_tasklet_stats(ulong *total, ulong *ready, ulong *running,
                       ulong *pools) {
  TaskletSlot *t;
  ulong count = 0;

  lock(&clr_sched.lock);
  for (t = clr_sched.run_queue_head; t != nil; t = t->next)
    count++;
  unlock(&clr_sched.lock);

  if (total)
    *total = clr_sched.total_created - clr_sched.total_destroyed;
  if (ready)
    *ready = count;
  if (running)
    *running = (clr_sched.current != nil) ? 1 : 0;
  if (pools)
    *pools = clr_sched.pools_allocated;
}
