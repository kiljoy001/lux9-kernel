#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* Limine HHDM offset - all physical memory mapped at PA + this offset */
extern uintptr limine_hhdm_offset;

/* Flag to indicate xinit() has completed (for early-boot allocators) */
int xinit_done = 0;

/* -------------------------------------------------------------------------
 * XALLOC configuration
 * -------------------------------------------------------------------------
 * INITIAL_NHOLE – number of static Hole descriptors (kept in the Xalloc
 *                 struct).  This value must match the size of the static array.
 * DYNAMIC_NHOLE – number of descriptors to allocate at once when the static
 *                 pool runs out.
 * Nhole        – alias for INITIAL_NHOLE used by the rest of the file.
 * -------------------------------------------------------------------------
 */
enum
{
	INITIAL_NHOLE	= 128,
	DYNAMIC_NHOLE	= 256,
	Nhole		= INITIAL_NHOLE,	/* static hole descriptor count */
	Magichole	= 0x484F4C45,		/* HOLE */
};

typedef struct Hole Hole;
typedef struct Xalloc Xalloc;
typedef struct Xhdr Xhdr;

struct Hole
{
	uintptr	addr;
	uintptr	size;
	uintptr	top;
	Hole*	link;
};

struct Xhdr
{
	ulong	size;
	ulong	magix;
	char	data[];
};

struct Xalloc
{
	Lock	lk;  /* Named lock member instead of anonymous */
	Hole	hole[Nhole];
	Hole*	flist;
	Hole*	table;
};

static Xalloc	xlists;

/* TEST 2A: Memory Allocation Tracking */
