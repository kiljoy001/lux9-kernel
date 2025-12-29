#include "libexchange.h"
#include "lib9p.h"
#include <libc.h>

/* Minimal libc functions needed */
extern void *memmove(void *dst, const void *src, ulong n);
extern void *memset(void *s, int c, ulong n);
extern int strlen(const char *s);

/* Simple string to integer conversion */
static int
atoi(const char *s)
{
	int n = 0;
	int neg = 0;

	if (*s == '-') {
		neg = 1;
		s++;
	}

	while (*s >= '0' && *s <= '9') {
		n = n * 10 + (*s - '0');
		s++;
	}

	return neg ? -n : n;
}

/* Simple integer to string conversion */
static int
itoa(int n, char *buf, int size)
{
	int i = 0;
	int neg = 0;
	char tmp[32];

	if (n < 0) {
		neg = 1;
		n = -n;
	}

	if (n == 0) {
		tmp[i++] = '0';
	} else {
		while (n > 0) {
			tmp[i++] = '0' + (n % 10);
			n /= 10;
		}
	}

	if (neg)
		tmp[i++] = '-';

	/* Reverse into output buffer */
	int j;
	for (j = 0; j < i && j < size - 1; j++)
		buf[j] = tmp[i - 1 - j];
	buf[j] = '\0';

	return j;
}

/* Simple strcat */
static char*
strcat(char *dst, const char *src)
{
	char *d = dst;
	while (*d)
		d++;
	while ((*d++ = *src++))
		;
	return dst;
}

/* Simple strcmp */
static int
strcmp(const char *s1, const char *s2)
{
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* Initialize exchange pool and attach to kernel endpoint */
ExchPool*
exch_pool_init(int pool_size)
{
	static ExchPool pool_storage;
	ExchPool *pool = &pool_storage;
	char path[64];
	char buf[64];
	int n, chan_id;

	/* Initialize pool structure */
	memset(pool, 0, sizeof(ExchPool));
	pool->pool_size = pool_size;

	/* Open #X/clone to allocate channel */
	pool->ctl_fd = p9_open("#X/clone", ORDWR);
	if (pool->ctl_fd < 0)
		goto error;

	/* Read channel ID */
	n = p9_read(pool->ctl_fd, buf, sizeof(buf) - 1);
	if (n <= 0)
		goto error;
	buf[n] = '\0';
	chan_id = atoi(buf);
	pool->chan_id = chan_id;

	/* Close clone fd, open actual control fd */
	p9_close(pool->ctl_fd);

	/* Build path to channel directory */
	memset(path, 0, sizeof(path));
	strcpy(path, "#X/");
	itoa(chan_id, path + strlen(path), sizeof(path) - strlen(path));
	strcat(path, "/ctl");

	/* Open channel control */
	pool->ctl_fd = p9_open(path, OWRITE);
	if (pool->ctl_fd < 0)
		goto error;

	/* Attach to kernel endpoint */
	n = p9_write(pool->ctl_fd, "attach kernel", 13);
	if (n < 0)
		goto error;

	/* Set pool size if requested */
	if (pool_size > 0) {
		char cmd[64];
		strcpy(cmd, "poolsize ");
		itoa(pool_size, cmd + strlen(cmd), sizeof(cmd) - strlen(cmd));
		n = p9_write(pool->ctl_fd, cmd, strlen(cmd));
		if (n < 0)
			goto error;
	}

	/* Open pool file descriptor */
	memset(path, 0, sizeof(path));
	strcpy(path, "#X/");
	itoa(chan_id, path + strlen(path), sizeof(path) - strlen(path));
	strcat(path, "/pool");
	pool->pool_fd = p9_open(path, ORDWR);
	if (pool->pool_fd < 0)
		goto error;

	/* Open and map ring buffer control page */
	memset(path, 0, sizeof(path));
	strcpy(path, "#X/");
	itoa(chan_id, path + strlen(path), sizeof(path) - strlen(path));
	strcat(path, "/ring");

	int ring_fd = p9_open(path, ORDWR);
	if (ring_fd < 0)
		goto error;

	/* Read ring capability */
	UserCapability ring_cap;
	n = p9_read(ring_fd, &ring_cap, sizeof(UserCapability));
	if (n < sizeof(UserCapability)) {
		p9_close(ring_fd);
		goto error;
	}

	/* Map ring buffer using segattach syscall
	 * For now, we'll use a simple approach: assume kernel provides fixed mapping
	 * Full implementation would use Tsyssegattach or similar
	 *
	 * TODO: Implement proper capability-based mapping via syscall
	 * For Phase 7, we'll use legacy fixed mapping or stub
	 */
	pool->ring = 0;  /* TODO: Map ring buffer */

	p9_close(ring_fd);

	return pool;

error:
	if (pool->ctl_fd >= 0)
		p9_close(pool->ctl_fd);
	if (pool->pool_fd >= 0)
		p9_close(pool->pool_fd);
	return 0;
}

/* Allocate exchange page from pool */
int
exch_alloc(ExchPool *pool, UserCapability *out_cap)
{
	int n;

	if (pool == 0 || out_cap == 0)
		return -1;

	/* Read capability from pool */
	n = p9_read(pool->pool_fd, out_cap, sizeof(UserCapability));
	if (n < sizeof(UserCapability))
		return -1;

	return 0;
}

/* Map exchange page by capability
 * TODO: Implement proper capability-based mapping
 * For now, returns stub
 */
void*
exch_map(const UserCapability *cap)
{
	/* TODO: Implement Tsyssegattach or similar for capability-based mapping
	 * This requires kernel support for mapping by capability hash
	 * For Phase 7, this is a stub
	 */
	(void)cap;
	return 0;
}

/* Unmap exchange page */
int
exch_unmap(void *addr, ulong size)
{
	/* TODO: Implement Tsyssegdetach or munmap */
	(void)addr;
	(void)size;
	return 0;
}

/* Submit exchange page UUID to ring buffer */
int
exch_submit(ExchPool *pool, const uuid_t *page_uuid)
{
	u32int tail, next_tail;

	if (pool == 0 || pool->ring == 0 || page_uuid == 0)
		return -1;

	/* Get current tail */
	tail = pool->ring->submission.tail;
	next_tail = (tail + 1) & pool->ring->submission.mask;

	/* Check if ring is full */
	if (next_tail == pool->ring->submission.head)
		return -1;  /* Ring full */

	/* Copy UUID to ring */
	memmove(&pool->ring->submission.uuids[tail], page_uuid, sizeof(uuid_t));

	/* Update tail (release to kernel) */
	pool->ring->submission.tail = next_tail;
	pool->submits++;

	return 0;
}

/* Ring the doorbell to notify kernel */
void
exch_doorbell(void)
{
	/* Issue syscall instruction to trigger kernel processing */
	asm volatile("syscall" : : : "memory", "rcx", "r11");
}

/* Wait for completion UUID from ring buffer */
int
exch_wait(ExchPool *pool, uuid_t *out_uuid)
{
	u32int head;

	if (pool == 0 || pool->ring == 0 || out_uuid == 0)
		return -1;

	/* Spin until completion available */
	while (pool->ring->completion.head == pool->ring->completion.tail)
		;  /* TODO: Could use pause or yield here */

	/* Get current head */
	head = pool->ring->completion.head;

	/* Copy UUID from ring */
	memmove(out_uuid, &pool->ring->completion.uuids[head], sizeof(uuid_t));

	/* Update head (release to kernel) */
	pool->ring->completion.head = (head + 1) & pool->ring->completion.mask;
	pool->completions++;

	return 0;
}

/* Free exchange page back to pool */
int
exch_free(ExchPool *pool, const UserCapability *cap)
{
	int n;

	if (pool == 0 || cap == 0)
		return -1;

	/* Write capability back to pool */
	n = p9_write(pool->pool_fd, (void*)cap, sizeof(UserCapability));
	if (n < sizeof(UserCapability))
		return -1;

	return 0;
}

/* Cleanup and destroy exchange pool */
void
exch_pool_destroy(ExchPool *pool)
{
	if (pool == 0)
		return;

	if (pool->ctl_fd >= 0)
		p9_close(pool->ctl_fd);
	if (pool->pool_fd >= 0)
		p9_close(pool->pool_fd);

	/* TODO: Unmap ring buffer */

	/* TODO: Free pool structure (requires sbrk down) */
}
