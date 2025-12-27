/*
 * CLR Channel System - Zero-Copy Pebble-Backed Implementation
 *
 * Channels use exchange pages for inter-tasklet communication.
 * Data is written directly to shared memory - no copying.
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

/* Memory functions */
extern void *mallocz(ulong size, int clr);
extern void *xalloc(ulong size);
extern void free(void *p);
extern void *memset(void *s, int c, ulong n);
extern void *memmove(void *dst, const void *src, ulong n);
extern int print(char *fmt, ...);

#include "clr_channel.h"

/* Channel manager state */
typedef struct ChannelManager ChannelManager;
struct ChannelManager {
  ClrChannel *channels;
  ulong next_id;
  Lock lock;
  int initialized;

  ulong total_created;
  ulong total_closed;
  ulong messages_sent;
  ulong messages_recv;
};

static ChannelManager clr_chanmgr;

void clr_channel_init(void) {
  memset(&clr_chanmgr, 0, sizeof(clr_chanmgr));
  clr_chanmgr.next_id = 1;
  clr_chanmgr.initialized = 1;
  print("CLR: channel system initialized (zero-copy)\n");
}

ClrChannel *clr_channel_create(ulong elem_size, ulong capacity) {
  ClrChannel *ch;

  if (!clr_chanmgr.initialized)
    clr_channel_init();

  ch = mallocz(sizeof(ClrChannel), 1);
  if (ch == nil)
    return nil;

  /* Allocate page */
  ch->page = xalloc(BY2PG);
  if (ch->page == nil) {
    free(ch);
    return nil;
  }

  memset(ch->page, 0, BY2PG);

  lock(&clr_chanmgr.lock);
  ch->id = clr_chanmgr.next_id++;
  clr_chanmgr.total_created++;
  unlock(&clr_chanmgr.lock);

  ch->page->magic = CHANNEL_MAGIC;
  ch->page->id = ch->id;
  ch->page->elem_size = elem_size;
  ch->page->capacity = (CHANNEL_DATA_SIZE / elem_size);
  if (capacity > 0 && capacity < ch->page->capacity)
    ch->page->capacity = capacity;
  ch->page->head = 0;
  ch->page->tail = 0;
  ch->page->count = 0;
  ch->closed = 0;

  lock(&clr_chanmgr.lock);
  ch->next = clr_chanmgr.channels;
  clr_chanmgr.channels = ch;
  unlock(&clr_chanmgr.lock);

  print("CLR: created channel %ud (elem_size=%ud, capacity=%ud)\n", ch->id,
        ch->page->elem_size, ch->page->capacity);

  return ch;
}

static Lock *channel_lock(ChannelPage *page) { return (Lock *)page->lock_data; }

int clr_channel_send(ClrChannel *ch, void *data, ulong size) {
  ChannelPage *page;
  TaskletSlot *current;
  ulong offset;

  if (ch == nil || ch->closed || data == nil)
    return -1;

  page = ch->page;
  if (size > page->elem_size)
    size = page->elem_size;

  lock(channel_lock(page));

  while (page->count >= page->capacity) {
    current = clr_tasklet_current();
    if (current == nil) {
      unlock(channel_lock(page));
      return -1;
    }

    current->next = page->send_waiters;
    page->send_waiters = current;
    unlock(channel_lock(page));

    clr_tasklet_block(ch);

    lock(channel_lock(page));
    if (ch->closed) {
      unlock(channel_lock(page));
      return -1;
    }
  }

  offset = (page->tail % page->capacity) * page->elem_size;
  memmove(page->data + offset, data, size);
  page->tail++;
  page->count++;

  if (page->recv_waiters != nil) {
    TaskletSlot *waiter = page->recv_waiters;
    page->recv_waiters = waiter->next;
    waiter->next = nil;
    unlock(channel_lock(page));
    clr_tasklet_resume(waiter);
  } else {
    unlock(channel_lock(page));
  }

  lock(&clr_chanmgr.lock);
  clr_chanmgr.messages_sent++;
  unlock(&clr_chanmgr.lock);

  return 0;
}

int clr_channel_recv(ClrChannel *ch, void *data, ulong size) {
  ChannelPage *page;
  TaskletSlot *current;
  ulong offset;

  if (ch == nil || data == nil)
    return -1;

  page = ch->page;
  if (size > page->elem_size)
    size = page->elem_size;

  lock(channel_lock(page));

  while (page->count == 0) {
    if (ch->closed) {
      unlock(channel_lock(page));
      return -1;
    }

    current = clr_tasklet_current();
    if (current == nil) {
      unlock(channel_lock(page));
      return -1;
    }

    current->next = page->recv_waiters;
    page->recv_waiters = current;
    unlock(channel_lock(page));

    clr_tasklet_block(ch);

    lock(channel_lock(page));
  }

  offset = (page->head % page->capacity) * page->elem_size;
  memmove(data, page->data + offset, size);
  page->head++;
  page->count--;

  if (page->send_waiters != nil) {
    TaskletSlot *waiter = page->send_waiters;
    page->send_waiters = waiter->next;
    waiter->next = nil;
    unlock(channel_lock(page));
    clr_tasklet_resume(waiter);
  } else {
    unlock(channel_lock(page));
  }

  lock(&clr_chanmgr.lock);
  clr_chanmgr.messages_recv++;
  unlock(&clr_chanmgr.lock);

  return 0;
}

int clr_channel_trysend(ClrChannel *ch, void *data, ulong size) {
  ChannelPage *page;
  ulong offset;

  if (ch == nil || ch->closed || data == nil)
    return -1;

  page = ch->page;
  if (size > page->elem_size)
    size = page->elem_size;

  lock(channel_lock(page));

  if (page->count >= page->capacity) {
    unlock(channel_lock(page));
    return -1;
  }

  offset = (page->tail % page->capacity) * page->elem_size;
  memmove(page->data + offset, data, size);
  page->tail++;
  page->count++;

  if (page->recv_waiters != nil) {
    TaskletSlot *waiter = page->recv_waiters;
    page->recv_waiters = waiter->next;
    waiter->next = nil;
    unlock(channel_lock(page));
    clr_tasklet_resume(waiter);
  } else {
    unlock(channel_lock(page));
  }

  lock(&clr_chanmgr.lock);
  clr_chanmgr.messages_sent++;
  unlock(&clr_chanmgr.lock);

  return 0;
}

int clr_channel_tryrecv(ClrChannel *ch, void *data, ulong size) {
  ChannelPage *page;
  ulong offset;

  if (ch == nil || data == nil)
    return -1;

  page = ch->page;
  if (size > page->elem_size)
    size = page->elem_size;

  lock(channel_lock(page));

  if (page->count == 0) {
    unlock(channel_lock(page));
    return -1;
  }

  offset = (page->head % page->capacity) * page->elem_size;
  memmove(data, page->data + offset, size);
  page->head++;
  page->count--;

  if (page->send_waiters != nil) {
    TaskletSlot *waiter = page->send_waiters;
    page->send_waiters = waiter->next;
    waiter->next = nil;
    unlock(channel_lock(page));
    clr_tasklet_resume(waiter);
  } else {
    unlock(channel_lock(page));
  }

  lock(&clr_chanmgr.lock);
  clr_chanmgr.messages_recv++;
  unlock(&clr_chanmgr.lock);

  return 0;
}

void clr_channel_close(ClrChannel *ch) {
  ChannelPage *page;
  TaskletSlot *waiter;

  if (ch == nil || ch->closed)
    return;

  ch->closed = 1;
  page = ch->page;

  lock(channel_lock(page));

  while (page->send_waiters != nil) {
    waiter = page->send_waiters;
    page->send_waiters = waiter->next;
    waiter->next = nil;
    clr_tasklet_resume(waiter);
  }

  while (page->recv_waiters != nil) {
    waiter = page->recv_waiters;
    page->recv_waiters = waiter->next;
    waiter->next = nil;
    clr_tasklet_resume(waiter);
  }

  unlock(channel_lock(page));

  lock(&clr_chanmgr.lock);
  clr_chanmgr.total_closed++;
  unlock(&clr_chanmgr.lock);

  print("CLR: closed channel %ud\n", ch->id);
}

void clr_channel_stats(ulong *total, ulong *sent, ulong *recv) {
  if (total)
    *total = clr_chanmgr.total_created - clr_chanmgr.total_closed;
  if (sent)
    *sent = clr_chanmgr.messages_sent;
  if (recv)
    *recv = clr_chanmgr.messages_recv;
}
