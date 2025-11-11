/*
 * Exchange page system - clean facade implementation
 * Integrates with borrow checker for safe page exchange
 * Provides Singularity-style exchange heap semantics at page granularity
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "../9front-pc64/fns.h"
#include "pageown.h"
#include "exchange.h"
#include <error.h>
#include "lock_borrow.h"

/* Track prepared pages for cancellation */
struct PreparedPage {
	ExchangeHandle handle;
	uintptr original_vaddr;
	Proc *owner;
	struct PreparedPage *next;
};

static struct PreparedPage *prepared_pages = nil;
static BorrowLock prepared_lock;
static LockDagNode lockdag_exchange_prepared = LOCKDAG_NODE("exchange-prepared");

/* Convert physical address to page frame number */
static inline ulong
pa2pfn(uintptr pa)
{
	return pa >> PGSHIFT;
}

/*
 * Initialize exchange page system
 */
void
exchangeinit(void)
{
	/* Exchange system uses existing pageown infrastructure */
	borrow_lock_init(&prepared_lock, (uintptr)&prepared_lock, &lockdag_exchange_prepared);
	print("exchange: initialized\n");
}

/*
 * Prepare a page for exchange - remove from current process
 * Returns an exchange handle (physical address) that can be passed to another process
 */
ExchangeHandle
exchange_prepare(uintptr vaddr)
{
	u64int *pte;
	uintptr pa;
	enum PageOwnError err;
	struct PreparedPage *pp;
	
	/* Validate virtual address alignment */
	if((vaddr & (BY2PG-1)) != 0)
		return 0;

	/* Get PTE for the virtual address */
	pte = mmuwalk(m->pml4, vaddr, 0, 0);
	if(pte == nil || (*pte & PTEVALID) == 0)
		return 0;

	/* Get physical address */
	pa = PADDR(*pte);

	/* Verify we own this page */
	if(!pageown_is_owned(pa) || pageown_get_owner(pa) != up)
		return 0;

	/* Check if page can be transferred (no active borrows) */
	if(!pageown_can_borrow_mut(pa) && pageown_get_state(pa) != POWN_EXCLUSIVE)
		return 0;

	/* Allocate tracking structure */
	pp = malloc(sizeof(struct PreparedPage));
	if(pp == nil)
		return 0;

	/* Unmap from current process (prepare for transfer) */
	*pte = 0;
	
	/* Flush TLB */
	putcr3(getcr3());

	/* Track the prepared page */
	pp->handle = pa;
	pp->original_vaddr = vaddr;
	pp->owner = up;
	
	borrow_lock(&prepared_lock);
	pp->next = prepared_pages;
	prepared_pages = pp;
	borrow_unlock(&prepared_lock);

	/* Return the physical address as the exchange handle */
	return pa;
}

/*
 * Accept an exchange page into current process
 * Maps the page at the specified virtual address with given permissions
 */
int
exchange_accept(ExchangeHandle handle, uintptr dest_vaddr, int prot)
{
	uintptr pa = handle;
	struct PreparedPage *pp, **prev;
	
	if(handle == 0 || (dest_vaddr & (BY2PG-1)) != 0)
		return EXCHANGE_EINVAL;

	/* Verify this is a valid physical address */
	if(pa2pfn(pa) >= pageownpool.npages)
		return EXCHANGE_ENOTEXCHANGE;

	/* Remove from prepared pages list */
	borrow_lock(&prepared_lock);
	for(prev = &prepared_pages; (pp = *prev) != nil; prev = &pp->next) {
		if(pp->handle == handle) {
			*prev = pp->next;
			free(pp);
			break;
		}
	}
	borrow_unlock(&prepared_lock);

	/* Map the page into current process at the specified address */
	extern void userpmap(uintptr va, uintptr pa, int perms);
	userpmap(dest_vaddr, pa, prot);

	/* Acquire ownership for the current process */
	enum PageOwnError err = pageown_acquire(up, pa, dest_vaddr);
	if(err != POWN_OK) {
		/* If acquire fails, unmap the page */
		u64int *pte = mmuwalk(m->pml4, dest_vaddr, 0, 0);
		if(pte && (*pte & PTEVALID))
			*pte = 0;
		return EXCHANGE_EALREADY;
	}

	return EXCHANGE_OK;
}

/*
 * Cancel an exchange and return page to original owner
 * This undoes a prepare operation
 */
int
exchange_cancel(ExchangeHandle handle)
{
	struct PreparedPage *pp, **prev;
	u64int *pte;
	
	if(handle == 0)
		return EXCHANGE_EINVAL;

	/* Find the prepared page */
	borrow_lock(&prepared_lock);
	for(prev = &prepared_pages; (pp = *prev) != nil; prev = &pp->next) {
		if(pp->handle == handle) {
			*prev = pp->next;
			break;
		}
	}
	borrow_unlock(&prepared_lock);

	if(pp == nil)
		return EXCHANGE_EINVAL;

	/* Remap the page back to the original owner */
	extern void userpmap(uintptr va, uintptr pa, int perms);
	userpmap(pp->original_vaddr, pp->handle, PTEVALID | PTEUSER | PTEWRITE);

	/* Free the tracking structure */
	free(pp);

	return EXCHANGE_OK;
}

/*
 * Transfer a page from one process to another
 * This is the core exchange operation
 */
int
exchange_transfer(Proc *from, Proc *to, ExchangeHandle handle, uintptr to_vaddr)
{
	uintptr pa = handle;
	enum PageOwnError err;
	
	if(from == nil || to == nil || handle == 0 || (to_vaddr & (BY2PG-1)) != 0)
		return EXCHANGE_EINVAL;

	/* Transfer ownership via borrow checker */
	err = pageown_transfer(from, to, pa, to_vaddr);
	if(err != POWN_OK) {
		switch(err) {
		case POWN_ENOTOWNER:
			return EXCHANGE_ENOTOWNER;
		case POWN_EBORROWED:
			return EXCHANGE_EBORROWED;
		default:
			return EXCHANGE_EINVAL;
		}
	}

	/* Map into target process */
	extern void userpmap(uintptr va, uintptr pa, int perms);
	userpmap(to_vaddr, pa, PTEVALID | PTEUSER | PTEWRITE);

	/* Flush TLB */
	putcr3(getcr3());

	return EXCHANGE_OK;
}

/*
 * Query if an exchange handle is valid
 */
int
exchange_is_valid(ExchangeHandle handle)
{
	if(handle == 0)
		return 0;
	
	uintptr pa = handle;
	ulong pfn = pa2pfn(pa);
	
	if(pfn >= pageownpool.npages)
		return 0;
	
	struct PageOwner *own = &pageownpool.pages[pfn];
	return (own->state != POWN_FREE);
}

/*
 * Get the owner of an exchange handle
 */
Proc*
exchange_get_owner(ExchangeHandle handle)
{
	if(handle == 0)
		return nil;
	
	uintptr pa = handle;
	return pageown_get_owner(pa);
}

/*
 * Prepare a range of pages for exchange
 * Returns the number of pages prepared, or negative on error
 */
int
exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles)
{
	ulong offset;
	int npages = 0;
	
	/* Validate parameters */
	if((vaddr & (BY2PG-1)) != 0)
		return -EXCHANGE_EINVAL;
	if(len == 0 || len > 1*GiB)
		return -EXCHANGE_EINVAL;
	if(handles == nil)
		return -EXCHANGE_EINVAL;

	/* Process each page in the range */
	for(offset = 0; offset < len; offset += BY2PG) {
		u64int va = vaddr + offset;
		ExchangeHandle handle = exchange_prepare(va);
		
		if(handle == 0) {
			/* Cancel any previously prepared pages */
			ulong cancel_offset;
			for(cancel_offset = 0; cancel_offset < offset; cancel_offset += BY2PG) {
				exchange_cancel(handles[cancel_offset / BY2PG]);
			}
			return -EXCHANGE_EINVAL;
		}
		
		handles[offset / BY2PG] = handle;
		npages++;
	}

	return npages;
}
