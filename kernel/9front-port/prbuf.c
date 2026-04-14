/*
 * Pointer Ring Buffer for Lock-Free Message Passing
 *
 * Replaces linked-list queues with atomic ring buffer operations
 * to eliminate deadlocks in the kernel print system.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>

/*
 * Message types for structured output
 */
enum {
	MSG_STANDARD = 0,
	MSG_DEBUG,
	MSG_CRITICAL,
	MSG_STRUCTURED,
	MSG_GRAPHICS,
};

/*
 * Print message structure
 * Stored in pre-allocated pools to avoid allocation during print
 */
typedef struct PrintMessage PrintMessage;
struct PrintMessage {
	uint	type;
	uint	len;
	uint	flags;
	uint	refcnt;
	char	content[512];	/* inline buffer for small messages */
	char	*overflow;	/* for messages > 512 bytes */
};

/*
 * Message pool sizes
 */
enum {
	POOL_SMALL = 64,	/* 64 small message slots */
	POOL_MEDIUM = 32,	/* 32 medium message slots */
	POOL_LARGE = 16,	/* 16 large message slots */
	MSG_INLINE_MAX = 512,
};

/*
 * Pointer ring buffer structure
 */
enum {
	PRBUF_SIZE = 256,	/* must be power of 2 */
	PRBUF_MASK = PRBUF_SIZE - 1,
};

typedef struct PRBuf PRBuf;
struct PRBuf {
	PrintMessage	*slots[PRBUF_SIZE];
	volatile uint	head;		/* producer writes here */
	volatile uint	tail;		/* consumer reads here */
	uvlong	produced;	/* total messages produced */
	uvlong	consumed;	/* total messages consumed */
	uvlong	dropped;	/* messages dropped (buffer full) */
};

/*
 * Message pool for allocation
 */
typedef struct MsgPool MsgPool;
struct MsgPool {
	PrintMessage	*msgs;
	uint		size;
	volatile uint	head;	/* free list head */
	Lock		lock;	/* only for allocation, not deallocation */
};

/*
 * Global print system state
 */
static struct {
	PRBuf		ringbuf;	/* main ring buffer */
	MsgPool		smallpool;	/* small message pool */
	MsgPool		medpool;	/* medium message pool */
	Rendez		consumer;	/* consumer wakeup */
	int		initialized;
	int		consumer_active;
} printsys;

enum {
	KprintQLen = 64,
};

typedef struct {
	Rendez	r;
	Lock	lock;
	struct {
		PrintMessage *msg;
		uint	offset;
	} queue[KprintQLen];
	uint	head;
	uint	tail;
	int	active;
} KprintSub;

static KprintSub kprintsub;

static void	kprint_deliver(PrintMessage*);
static int	kprint_has_data(void*);

/*
 * Initialize message pool
 */
static void
poolinit(MsgPool *p, uint size)
{
	int i;

	p->msgs = malloc(size * sizeof(PrintMessage));
	if(p->msgs == nil)
		panic("poolinit: out of memory");

	p->size = size;
	p->head = 0;
	memset(&p->lock, 0, sizeof(Lock));

	/* initialize all messages as free */
	for(i = 0; i < size; i++){
		p->msgs[i].refcnt = 0;
		p->msgs[i].overflow = nil;
	}
}

/*
 * Allocate message from pool
 * Returns nil if pool exhausted
 */
static PrintMessage*
msgalloc(MsgPool *p)
{
	PrintMessage *m;
	uint i, start;

	/* simple linear scan for free slot */
	lock(&p->lock);
	start = p->head;
	for(i = 0; i < p->size; i++){
		uint idx = (start + i) & (p->size - 1);
		m = &p->msgs[idx];
		if(m->refcnt == 0){
			m->refcnt = 1;
			m->overflow = nil;
			m->len = 0;
			p->head = (idx + 1) & (p->size - 1);
			unlock(&p->lock);
			return m;
		}
	}
	unlock(&p->lock);
	return nil;
}

/*
 * Free message back to pool
 */
static void
msgfree(PrintMessage *m)
{
	if(m == nil)
		return;

	if(m->overflow != nil){
		free(m->overflow);
		m->overflow = nil;
	}
	m->refcnt = 0;	/* atomic, allows reuse */
}

static void
msgretain(PrintMessage *m)
{
	__sync_add_and_fetch(&m->refcnt, 1);
}

static void
msgrelease(PrintMessage *m)
{
	if(__sync_sub_and_fetch(&m->refcnt, 1) == 0)
		msgfree(m);
}

/*
 * Initialize the print system
 */
void
prbuf_init(void)
{
	if(printsys.initialized)
		return;

	memset(&printsys.ringbuf, 0, sizeof(PRBuf));
	poolinit(&printsys.smallpool, POOL_SMALL);
	poolinit(&printsys.medpool, POOL_MEDIUM);

	printsys.initialized = 1;
}

/*
 * Create a print message
 * Tries small pool first, then medium pool
 */
PrintMessage*
prbuf_msg_create(char *content, uint len, uint type)
{
	PrintMessage *m;

	if(!printsys.initialized)
		prbuf_init();

	/* try small pool first */
	m = msgalloc(&printsys.smallpool);
	if(m == nil)
		m = msgalloc(&printsys.medpool);
	if(m == nil){
		printsys.ringbuf.dropped++;
		return nil;
	}

	m->type = type;
	m->flags = 0;

	if(len <= MSG_INLINE_MAX){
		memmove(m->content, content, len);
		m->len = len;
	} else {
		/* allocate overflow buffer */
		m->overflow = malloc(len);
		if(m->overflow == nil){
			msgfree(m);
			printsys.ringbuf.dropped++;
			return nil;
		}
		memmove(m->overflow, content, len);
		m->len = len;
	}

	return m;
}

/*
 * Add message pointer to ring buffer
 * Lock-free producer operation
 * Returns 1 on success, 0 if buffer full
 */
int
prbuf_produce(PrintMessage *m)
{
	PRBuf *rb = &printsys.ringbuf;
	uint head, tail, next;

	if(m == nil)
		return 0;

	head = rb->head;
	tail = rb->tail;
	next = (head + 1) & PRBUF_MASK;

	/* check if buffer full */
	if(next == tail){
		rb->dropped++;
		msgfree(m);
		return 0;
	}

	/* store pointer */
	rb->slots[head] = m;

	/* memory barrier before updating head */
	coherence();

	rb->head = next;
	rb->produced++;

	/* wake consumer if sleeping */
	if(printsys.consumer_active)
		wakeup(&printsys.consumer);

	return 1;
}

/*
 * Get message from ring buffer
 * Lock-free consumer operation
 * Returns nil if buffer empty
 */
PrintMessage*
prbuf_consume(void)
{
	PRBuf *rb = &printsys.ringbuf;
	PrintMessage *m;
	uint head, tail;

	tail = rb->tail;
	head = rb->head;

	/* check if buffer empty */
	if(tail == head)
		return nil;

	/* get pointer */
	m = rb->slots[tail];

	/* memory barrier before updating tail */
	coherence();

	rb->tail = (tail + 1) & PRBUF_MASK;
	rb->consumed++;

	return m;
}

/*
 * Get message content pointer
 */
char*
prbuf_msg_content(PrintMessage *m)
{
	if(m == nil)
		return nil;
	if(m->overflow != nil)
		return m->overflow;
	return m->content;
}

/*
 * Get message length
 */
uint
prbuf_msg_len(PrintMessage *m)
{
	if(m == nil)
		return 0;
	return m->len;
}

/*
 * Release message after consumption
 */
void
prbuf_msg_release(PrintMessage *m)
{
	msgrelease(m);
}

/*
 * Check if buffer has messages (for sleep condition)
 */
static int
has_data(void *arg)
{
	PRBuf *rb;
	USED(arg);
	rb = &printsys.ringbuf;
	return rb->head != rb->tail;
}

/*
 * Check if buffer has messages
 */
int
prbuf_has_data(void)
{
	PRBuf *rb = &printsys.ringbuf;
	return rb->head != rb->tail;
}

/*
 * Get buffer statistics
 */
void
prbuf_stats(uvlong *produced, uvlong *consumed, uvlong *dropped)
{
	PRBuf *rb = &printsys.ringbuf;
	if(produced) *produced = rb->produced;
	if(consumed) *consumed = rb->consumed;
	if(dropped) *dropped = rb->dropped;
}

/*
 * Consumer thread entry point
 * Processes messages and outputs to framebuffer/serial
 */
void
prbuf_consumer_thread(void *arg)
{
	PrintMessage *m;
	char *content;
	uint len;

	USED(arg);
	printsys.consumer_active = 1;

	for(;;){
		/* process all available messages */
		while((m = prbuf_consume()) != nil){
			content = prbuf_msg_content(m);
			len = prbuf_msg_len(m);

			kprint_deliver(m);

			/* output to console */
			if(screenputs != nil)
				screenputs(content, len);

			/* output to serial */
			if(consuart != nil)
				uartputs(content, len);

			prbuf_msg_release(m);
		}

		/* sleep until more messages available */
		sleep(&printsys.consumer, has_data, nil);
	}
}

/*
 * Start the consumer thread
 */
void
prbuf_start_consumer(void)
{
	if(!printsys.initialized)
		prbuf_init();

	kproc("prbuf-consumer", prbuf_consumer_thread, nil);
}

/*
 * Convenience function: format and queue a message
 * Used to replace print system internals
 */
int
prbuf_print(char *buf, int len)
{
	PrintMessage *m;

	if(len <= 0)
		return 0;

	m = prbuf_msg_create(buf, len, MSG_STANDARD);
	if(m == nil)
		return 0;

	if(!prbuf_produce(m))
		return 0;

	return len;
}

void
prbuf_kprint_open(void)
{
	lock(&kprintsub.lock);
	kprintsub.active = 1;
	kprintsub.head = kprintsub.tail = 0;
	unlock(&kprintsub.lock);
}

void
prbuf_kprint_close(void)
{
	PrintMessage *m;

	lock(&kprintsub.lock);
	kprintsub.active = 0;
	while(kprintsub.head != kprintsub.tail){
		m = kprintsub.queue[kprintsub.head].msg;
		kprintsub.queue[kprintsub.head].msg = nil;
		kprintsub.queue[kprintsub.head].offset = 0;
		kprintsub.head = (kprintsub.head + 1) % KprintQLen;
		if(m != nil)
			prbuf_msg_release(m);
	}
	unlock(&kprintsub.lock);
	wakeup(&kprintsub.r);
}

long
prbuf_kprint_read(void *buf, long n)
{
	PrintMessage *m;
	int copied;

	if(n <= 0)
		return 0;

	lock(&kprintsub.lock);
	for(;;){
		if(kprintsub.head != kprintsub.tail)
			break;
		if(!kprintsub.active){
			unlock(&kprintsub.lock);
			return 0;
		}
		unlock(&kprintsub.lock);
		sleep(&kprintsub.r, kprint_has_data, nil);
		lock(&kprintsub.lock);
	}
	m = kprintsub.queue[kprintsub.head].msg;
	copied = 0;
	if(m != nil){
		uint avail;
		char *data;

		data = prbuf_msg_content(m);
		data += kprintsub.queue[kprintsub.head].offset;
		avail = prbuf_msg_len(m) - kprintsub.queue[kprintsub.head].offset;
		copied = avail;
		if(copied > n)
			copied = n;
		memmove(buf, data, copied);
		kprintsub.queue[kprintsub.head].offset += copied;
		if(kprintsub.queue[kprintsub.head].offset >= prbuf_msg_len(m)){
			kprintsub.queue[kprintsub.head].msg = nil;
			kprintsub.queue[kprintsub.head].offset = 0;
			kprintsub.head = (kprintsub.head + 1) % KprintQLen;
			prbuf_msg_release(m);
		}
	} else {
		kprintsub.head = (kprintsub.head + 1) % KprintQLen;
	}
	unlock(&kprintsub.lock);

	return copied;
}

static void
kprint_deliver(PrintMessage *m)
{
	uint next;

	if(!kprintsub.active)
		return;

	lock(&kprintsub.lock);
	if(!kprintsub.active){
		unlock(&kprintsub.lock);
		return;
	}

	next = (kprintsub.tail + 1) % KprintQLen;
	if(next == kprintsub.head){
		PrintMessage *drop = kprintsub.queue[kprintsub.head].msg;
		kprintsub.queue[kprintsub.head].msg = nil;
		kprintsub.queue[kprintsub.head].offset = 0;
		kprintsub.head = (kprintsub.head + 1) % KprintQLen;
		if(drop != nil)
			prbuf_msg_release(drop);
	}
	msgretain(m);
	kprintsub.queue[kprintsub.tail].msg = m;
	kprintsub.queue[kprintsub.tail].offset = 0;
	kprintsub.tail = next;
	unlock(&kprintsub.lock);

	wakeup(&kprintsub.r);
}

static int
kprint_has_data(void *arg)
{
	USED(arg);
	return kprintsub.head != kprintsub.tail || !kprintsub.active;
}

int
prbuf_ready(void)
{
	return printsys.consumer_active;
}
