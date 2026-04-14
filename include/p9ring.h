#ifndef _P9RING_H_
#define _P9RING_H_

#include <u.h>

enum {
	P9RING_PAGE_SIZE = 4096,
	P9RING_SIZE = 128,
	P9RING_MASK = P9RING_SIZE - 1,
	P9RING_MAGIC = 0x52494E47,
	P9BATCH_MAGIC = 0xB47C4831,
};

typedef struct P9PageRing {
	volatile u32int head;
	volatile u32int tail;
	u32int mask;
	u32int flags;
	u64int pages[P9RING_SIZE];
} P9PageRing;

typedef struct P9RingChannel {
	u32int magic;
	u32int status;
	P9PageRing submission;
	P9PageRing completion;
} P9RingChannel;

typedef struct P9BatchHeader {
	u16int num_messages;
	u16int used_bytes;
	u32int magic;
	u64int nonce;
} P9BatchHeader;

int p9ring_batch_init(void *page, u64int seqno);
int p9ring_batch_add(void *page, u16int len, const void *msg);
int p9ring_submit(P9RingChannel *chan, void *page);
int p9ring_complete(P9RingChannel *chan, void **page);
int p9ring_kick(int fd);
int p9ring_kick_ipc(int fd);

#endif
