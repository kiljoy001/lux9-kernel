/*
 * CLR Channel System - Zero-Copy Pebble-Backed Channels
 *
 * Channels use exchange pages for zero-copy inter-tasklet communication.
 * Both sender and receiver map the same physical page.
 */

#ifndef CLR_CHANNEL_H
#define CLR_CHANNEL_H

#include "clr_tasklet.h"

#define CHANNEL_MAGIC 0x4348414E /* "CHAN" */
#define CHANNEL_DATA_SIZE 3968   /* 4KB - 128 bytes header */

/*
 * ChannelPage - 4KB exchange page layout
 * Uses standard C types for header portability
 */
typedef struct ChannelPage ChannelPage;
struct ChannelPage {
  /* Header - 64 bytes */
  unsigned int magic;
  unsigned int id;
  unsigned int head;      /* Read index */
  unsigned int tail;      /* Write index */
  unsigned int capacity;  /* Max elements */
  unsigned int elem_size; /* Bytes per element */
  unsigned int count;     /* Current element count */
  unsigned int flags;
  unsigned char lock_data[32]; /* Lock storage */

  /* Wait queues - 64 bytes */
  TaskletSlot *send_waiters;
  TaskletSlot *recv_waiters;
  unsigned char reserved[48];

  /* Ring buffer data - rest of page */
  unsigned char data[CHANNEL_DATA_SIZE];
};

/*
 * ClrChannel - Channel handle
 */
typedef struct ClrChannel ClrChannel;
struct ClrChannel {
  unsigned long id;
  void *exchange;    /* ExchangeHandle - opaque */
  void *cap;         /* UserCapability - opaque */
  ChannelPage *page; /* Mapped channel page */
  ClrChannel *next;
  int closed;
};

/* Channel API */
void clr_channel_init(void);
ClrChannel *clr_channel_create(unsigned long elem_size, unsigned long capacity);
int clr_channel_send(ClrChannel *ch, void *data, unsigned long size);
int clr_channel_recv(ClrChannel *ch, void *data, unsigned long size);
int clr_channel_trysend(ClrChannel *ch, void *data, unsigned long size);
int clr_channel_tryrecv(ClrChannel *ch, void *data, unsigned long size);
void clr_channel_close(ClrChannel *ch);
void clr_channel_stats(unsigned long *total, unsigned long *sent,
                       unsigned long *recv);

#endif /* CLR_CHANNEL_H */
