#ifndef POOL_TYPES_H
#define POOL_TYPES_H

#include <u.h>

/* Forward declaration of Pool (defined in pool.h) */
typedef struct Pool Pool;

/* Forward declarations */
typedef struct Alloc Alloc;
typedef struct Arena Arena;
typedef struct Bhdr Bhdr;
typedef struct Btail Btail;
typedef struct Free Free;

/* Block header - all blocks start with this */
struct Bhdr {
	ulong magic;
	ulong size;
};

/* Macro for embedding Bhdr in structures */
#define EMBED_BHDR \
	union { \
		struct { \
			ulong magic; \
			ulong size; \
		}; \
		Bhdr bhdr; \
	}

/* Block tail - all blocks end with this */
struct Btail {
	uchar magic0;
	uchar datasize[2];
	uchar magic1;
	ulong size; /* same as Bhdr->size */
};

/* Free block - has header, tail, and linkage for splay tree */
struct Free {
	EMBED_BHDR;
	Free *left;
	Free *right;
	Free *next;
	Free *prev;
};

/* Allocated block - has header and tail */
struct Alloc {
	EMBED_BHDR;
};

/* Arena - large region subdivided into blocks */
struct Arena {
	EMBED_BHDR;
	Arena *aup;
	Arena *down;
	ulong asize;
	ulong pad; /* to a multiple of 8 bytes */
};

/* Magic values */
enum {
	NOT_MAGIC = 0xdeadfa11,
	DEAD_MAGIC = 0xdeaddead,
	FREE_MAGIC = 0xBA5EBA11,
	ALLOC_MAGIC = 0x0A110C09,
	UNALLOC_MAGIC = 0xCAB00D1F,
	ARENA_MAGIC = 0xC0A1E5CF,
	ARENATAIL_MAGIC = 0xEC5E1A0D,
	ALIGN_MAGIC = 0xA1F1D1C1,
	FLOATING_MAGIC = 0xCBCBCBCB, /* temporarily neither allocated nor in the free tree */
};

/* Tail magic bytes */
enum { TAIL_MAGIC0 = 0xBE, TAIL_MAGIC1 = 0xEF };

/* Minimum block size */
enum { MINBLOCKSIZE = sizeof(Free) + sizeof(Btail) };

/* Data magic for overflow detection */
extern uchar datamagic[4];

/* Poison value for debug */
#define Poison ((void *)-0x35014542) /* cafebabe */

/* Utility macros for block navigation */
#define B2NB(b) ((Bhdr *)((uchar *)(b) + (b)->size))
#define B2T(b) ((Btail *)((uchar *)(b) + (b)->size - sizeof(Btail)))
#define B2PT(b) ((Btail *)((uchar *)(b) - sizeof(Btail)))
#define T2HDR(t) ((Bhdr *)((uchar *)(t) + sizeof(Btail) - (t)->size))

/* Arena macros */
#define A2TB(a) ((Bhdr *)((uchar *)(a) + (a)->asize - sizeof(Bhdr)))
#define A2B(a) B2NB(a)

/* Data conversion macros */
#define _B2D(a) ((void *)((uchar *)a + sizeof(Bhdr)))
#define _D2B(v) ((Alloc *)((uchar *)v - sizeof(Bhdr)))

/* Short (2-byte) packing macros */
#define SHORT(x) (((x)[0] << 8) | (x)[1])
#define PSHORT(p, x) (((uchar *)(p))[0] = ((x) >> 8) & 0xFF, ((uchar *)(p))[1] = (x) & 0xFF)

/* Debug macros */
#define antagonism if (!(p->flags & POOL_ANTAGONISM)) { } else
#define paranoia if (!(p->flags & POOL_PARANOIA)) { } else
#define verbosity if (!(p->flags & POOL_VERBOSITY)) { } else

#define DPRINT if (!(p->flags & POOL_DEBUGGING)) { } else p->print
#define LOG if (!(p->flags & POOL_LOGGING)) { } else p->print

#endif /* POOL_TYPES_H */
