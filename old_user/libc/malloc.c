#include <u.h>
#include <libc.h>
#include <pool.h>

static void*
sbrk_alloc(ulong n)
{
	return sbrk(n);
}

static int
sbrk_merge(void *a, void *b)
{
	return 0; /* sbrk chunks are usually contiguous but pool expects mergeable arenas. Simplification: don't merge arenas yet. */
}

static void
pool_lock(Pool *p)
{
	/* Single threaded userspace for now, or implement spinlock */
}

static void
pool_unlock(Pool *p)
{
}

static void
pool_print(Pool *p, char *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	vfprint(2, fmt, v); /* print to stderr */
	va_end(v);
}

static void
pool_panic(Pool *p, char *msg)
{
	print("pool panic: %s\n", msg);
	abort();
}

static Pool mainpool = {
	.name = "main",
	.maxsize = 1024*1024*1024, /* 1GB limit */
	.minblock = 8,
	.quantum = 8,
	.alloc = sbrk_alloc,
	.merge = sbrk_merge,
	.lock = pool_lock,
	.unlock = pool_unlock,
	.print = pool_print,
	.panic = pool_panic,
};

void*
malloc(ulong n)
{
	return poolalloc(&mainpool, n);
}

void
free(void *v)
{
	poolfree(&mainpool, v);
}

void*
realloc(void *v, ulong n)
{
	return poolrealloc(&mainpool, v, n);
}

void*
calloc(ulong n, ulong sz)
{
	void *v;
	ulong total = n * sz;
	v = malloc(total);
	if(v)
		memset(v, 0, total);
	return v;
}

ulong
msize(void *v)
{
	return poolmsize(&mainpool, v);
}
