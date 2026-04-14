#include <u.h>
#include <libc.h>
#include <p9ring.h>

static void
p9ring_fence(void)
{
	__asm__ volatile("mfence" ::: "memory");
}

int
p9ring_batch_init(void *page, u64int seqno)
{
	P9BatchHeader *hdr;

	if(page == nil)
		return -1;

	hdr = (P9BatchHeader*)page;
	hdr->magic = P9BATCH_MAGIC;
	hdr->nonce = seqno;
	hdr->num_messages = 0;
	hdr->used_bytes = sizeof(P9BatchHeader);
	return 0;
}

int
p9ring_batch_add(void *page, u16int len, const void *msg)
{
	P9BatchHeader *hdr;
	uchar *buf;
	ulong used;

	if(page == nil || msg == nil || len == 0)
		return -1;

	hdr = (P9BatchHeader*)page;
	if(hdr->magic != P9BATCH_MAGIC)
		return -1;

	used = hdr->used_bytes;
	if(used + 2 + len > P9RING_PAGE_SIZE)
		return -1;

	buf = (uchar*)page;
	buf[used] = (uchar)(len & 0xFF);
	buf[used + 1] = (uchar)((len >> 8) & 0xFF);
	memmove(buf + used + 2, msg, len);

	hdr->used_bytes = used + 2 + len;
	hdr->num_messages++;
	return 0;
}

int
p9ring_submit(P9RingChannel *chan, void *page)
{
	u32int head, tail, mask, size;

	if(chan == nil || page == nil)
		return -1;

	mask = chan->submission.mask;
	if(mask == 0)
		return -1;

	head = chan->submission.head;
	tail = chan->submission.tail;
	size = mask + 1;
	if((tail - head) >= size)
		return -1;

	chan->submission.pages[tail & mask] = (u64int)(uintptr)page;
	p9ring_fence();
	chan->submission.tail = tail + 1;
	return 0;
}

int
p9ring_complete(P9RingChannel *chan, void **page)
{
	u32int head, tail, mask;

	if(chan == nil || page == nil)
		return -1;

	head = chan->completion.head;
	tail = chan->completion.tail;
	if(head == tail)
		return 0;

	mask = chan->completion.mask;
	p9ring_fence();
	*page = (void*)(uintptr)chan->completion.pages[head & mask];
	chan->completion.head = head + 1;
	return 1;
}

int
p9ring_kick(int fd)
{
	char buf = 'k';

	return write(fd, &buf, 1);
}

int
p9ring_kick_ipc(int fd)
{
	static char cmd[] = "kickipc";

	return write(fd, cmd, sizeof(cmd) - 1);
}
