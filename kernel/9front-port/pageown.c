/*
 * Page ownership tracking with Rust-style borrow checker semantics
 * Provides memory safety for zero-copy IPC
 *
 * This is a thin wrapper around the generic borrowchecker that uses
 * physical addresses (pa) as keys.
 *
 * Maintains Plan 9 compatibility by implementing pa2owner as a
 * safe snapshot adapter that converts BorrowOwner data to the
 * legacy PageOwner format on demand.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pageown.h"
#include "borrowchecker.h"

/* Dummy pageownpool for compatibility - not actually used */
struct PageOwnPool pageownpool;

/* Map BorrowError to PageOwnError */
static enum PageOwnError
borrow_to_pageown_error(enum BorrowError err)
{
	switch(err) {
	case BORROW_OK:		return POWN_OK;
	case BORROW_EALREADY:	return POWN_EALREADY;
	case BORROW_ENOTOWNER:	return POWN_ENOTOWNER;
	case BORROW_EBORROWED:	return POWN_EBORROWED;
	case BORROW_EMUTBORROW:	return POWN_EMUTBORROW;
	case BORROW_ESHAREDBORROW: return POWN_ESHAREDBORROW;
	case BORROW_ENOTBORROWED: return POWN_ENOTBORROWED;
	case BORROW_EINVAL:	return POWN_EINVAL;
	case BORROW_ENOMEM:	return POWN_ENOMEM;
	case BORROW_ENOTFOUND:	return POWN_EINVAL;  /* Map to EINVAL */
	default:		return POWN_EINVAL;
	}
}

void
pageowninit(void)
{
	/* Nothing to do - borrowchecker is already initialized */
	memset(&pageownpool, 0, sizeof(pageownpool));
	print("pageown: using borrowchecker for ownership tracking\n");
}

/*
 * Acquire ownership of a page
 * This is like: let x = Page::new(); (exclusive ownership)
 *
 * In HHDM model, vaddr is the HHDM virtual address (pa + hhdm_offset)
 * which is the canonical kernel address for this page.
 */
enum PageOwnError
pageown_acquire(Proc *p, uintptr pa, u64int vaddr)
{
	enum BorrowError err;

	if(p == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	/* Use vaddr (HHDM VA) as key - this is what kernel uses to access page */
	err = borrow_acquire(p, vaddr);
	return borrow_to_pageown_error(err);
}

/*
 * Release ownership of a page
 * This is like: drop(x); (release ownership)
 */
enum PageOwnError
pageown_release(Proc *p, uintptr pa)
{
	enum BorrowError err;
	uintptr hhdm_va;

	if(p == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	hhdm_va = (uintptr)kaddr(pa);
	err = borrow_release(p, hhdm_va);
	return borrow_to_pageown_error(err);
}

/*
 * Transfer ownership from one process to another
 * This is like: let y = x; (move semantics - x is consumed, y gets ownership)
 */
enum PageOwnError
pageown_transfer(Proc *from, Proc *to, uintptr pa, u64int new_vaddr)
{
	enum BorrowError err;
	uintptr hhdm_va;

	(void)new_vaddr;  /* Not needed */

	if(from == nil || to == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	hhdm_va = (uintptr)kaddr(pa);
	err = borrow_transfer(from, to, hhdm_va);
	return borrow_to_pageown_error(err);
}

/*
 * Borrow page as shared read-only
 * This is like: let y = &x; (shared reference)
 */
enum PageOwnError
pageown_borrow_shared(Proc *owner, Proc *borrower, uintptr pa, u64int vaddr)
{
	enum BorrowError err;
	uintptr hhdm_va;

	(void)vaddr;  /* Not needed - we calculate it from pa */

	if(owner == nil || borrower == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	hhdm_va = (uintptr)kaddr(pa);
	err = borrow_borrow_shared(owner, borrower, hhdm_va);
	return borrow_to_pageown_error(err);
}

/*
 * Borrow page as exclusive read-write
 * This is like: let y = &mut x; (mutable reference)
 */
enum PageOwnError
pageown_borrow_mut(Proc *owner, Proc *borrower, uintptr pa, u64int vaddr)
{
	enum BorrowError err;
	uintptr hhdm_va;

	(void)vaddr;  /* Not needed - we calculate it from pa */

	if(owner == nil || borrower == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	hhdm_va = (uintptr)kaddr(pa);
	err = borrow_borrow_mut(owner, borrower, hhdm_va);
	return borrow_to_pageown_error(err);
}

/*
 * Return a shared borrow
 * This is like: drop(y); where y was &x
 */
enum PageOwnError
pageown_return_shared(Proc *borrower, uintptr pa)
{
	enum BorrowError err;
	uintptr hhdm_va;

	if(borrower == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	hhdm_va = (uintptr)kaddr(pa);
	err = borrow_return_shared(borrower, hhdm_va);
	return borrow_to_pageown_error(err);
}

/*
 * Return a mutable borrow
 * This is like: drop(y); where y was &mut x
 */
enum PageOwnError
pageown_return_mut(Proc *borrower, uintptr pa)
{
	enum BorrowError err;
	uintptr hhdm_va;

	if(borrower == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	hhdm_va = (uintptr)kaddr(pa);
	err = borrow_return_mut(borrower, hhdm_va);
	return borrow_to_pageown_error(err);
}

/*
 * Query operations
 */

int
pageown_is_owned(uintptr pa)
{
	uintptr hhdm_va;

	if((pa & (BY2PG-1)) != 0)
		return 0;

	hhdm_va = (uintptr)kaddr(pa);
	return borrow_is_owned(hhdm_va);
}

Proc*
pageown_get_owner(uintptr pa)
{
	uintptr hhdm_va;

	if((pa & (BY2PG-1)) != 0)
		return nil;

	hhdm_va = (uintptr)kaddr(pa);
	return borrow_get_owner(hhdm_va);
}

enum PageOwnerState
pageown_get_state(uintptr pa)
{
	enum BorrowState state;
	uintptr hhdm_va;

	if((pa & (BY2PG-1)) != 0)
		return POWN_FREE;

	hhdm_va = (uintptr)kaddr(pa);
	state = borrow_get_state(hhdm_va);

	/* Map BorrowState to PageOwnerState */
	switch(state) {
	case BORROW_FREE:		return POWN_FREE;
	case BORROW_EXCLUSIVE:		return POWN_EXCLUSIVE;
	case BORROW_SHARED_OWNED:	return POWN_SHARED_OWNED;
	case BORROW_MUT_LENT:		return POWN_MUT_LENT;
	default:			return POWN_FREE;
	}
}

int
pageown_can_borrow_shared(uintptr pa)
{
	uintptr hhdm_va;

	if((pa & (BY2PG-1)) != 0)
		return 0;

	hhdm_va = (uintptr)kaddr(pa);
	return borrow_can_borrow_shared(hhdm_va);
}

int
pageown_can_borrow_mut(uintptr pa)
{
	uintptr hhdm_va;

	if((pa & (BY2PG-1)) != 0)
		return 0;

	hhdm_va = (uintptr)kaddr(pa);
	return borrow_can_borrow_mut(hhdm_va);
}

/*
 * Process cleanup - called when process dies
 */
void
pageown_cleanup_process(Proc *p)
{
	if(p == nil)
		return;
	borrow_cleanup_process(p);
}

/*
 * Statistics and debugging
 */

void
pageown_stats(void)
{
	borrow_stats();
}

void
pageown_dump_page(uintptr pa)
{
	uintptr hhdm_va;

	if((pa & (BY2PG-1)) != 0) {
		print("pageown_dump_page: invalid pa=%#p (not page aligned)\n", pa);
		return;
	}

	hhdm_va = (uintptr)kaddr(pa);
	borrow_dump_resource(hhdm_va);
}

/* --------------------------------------------------------------------
 *  Return a temporary PageOwner populated from the BorrowChecker.
 *  This preserves the historic API and Plan 9 conventions.
 *  Uses the borrowchecker snapshot helper for a consistent view.
 * -------------------------------------------------------------------- */
struct PageOwner*
pa2owner(uintptr pa)
{
	struct BorrowOwner snapshot;
	static struct PageOwner buf[32];  /* Support up to 32 CPUs */
	int cpu;
	struct PageOwner *p;
	uintptr hhdm_va;

	if((pa & (BY2PG-1)) != 0)
		return nil;

	hhdm_va = (uintptr)kaddr(pa);
	if(!borrow_get_owner_snapshot(hhdm_va, &snapshot))
		return nil;

	cpu = 0;
	if(m != nil){
		cpu = m->machno;
		if(cpu < 0 || cpu >= (int)nelem(buf))
			cpu = 0;
	}

	p = &buf[cpu];
	memset(p, 0, sizeof(*p));

	p->owner = snapshot.owner;
	p->state = (enum PageOwnerState)snapshot.state;
	p->shared_count = snapshot.shared_count;
	p->mut_borrower = snapshot.mut_borrower;
	p->acquired_ns = snapshot.acquired_ns;
	p->borrow_deadline_ns = snapshot.borrow_deadline_ns;
	p->owner_vaddr = (u64int)snapshot.key;
	p->pa = pa;
	p->borrow_count = snapshot.borrow_count;

	return p;
}
