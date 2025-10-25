/*
 * Page ownership tracking with Rust-style borrow checker semantics
 * Provides memory safety for zero-copy IPC
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pageown.h"

struct PageOwnPool pageownpool;

/* Convert physical address to page frame number */
static inline ulong
pa2pfn(uintptr pa)
{
	return pa >> PGSHIFT;
}

/* Get page owner descriptor for physical address */
struct PageOwner*
pa2owner(uintptr pa)
{
	ulong pfn = pa2pfn(pa);
	if(pfn >= pageownpool.npages)
		return nil;
	return &pageownpool.pages[pfn];
}

void
pageowninit(void)
{
	ulong npages;
	ulong i;

	/* Calculate total number of physical pages */
	extern Conf conf;
	npages = 0;
	for(i = 0; i < nelem(conf.mem); i++) {
		npages += conf.mem[i].npage;
		/* Only print first few entries to avoid verbose output */
		if(i < 4) {
			print("pageown: conf.mem[%lud].base = %#p, .npage = %lud\n", i, conf.mem[i].base, conf.mem[i].npage);
		} else if(i == 4) {
			print("pageown: ... (showing first 4 entries only)\n");
		}
	}
	
	print("pageown: total npages = %lud\n", npages);
	
	/* Check for unreasonably large npages */
	if (npages > 1024*1024) {  /* More than 1 million pages */
		print("pageown: unreasonably large npages = %lud\n", npages);
		pageownpool.npages = 0;
		pageownpool.nowned = 0;
		pageownpool.nshared = 0;
		pageownpool.nmut = 0;
		pageownpool.pages = nil;
		return;
	}

	/* Check if we have any pages to manage */
	if(npages == 0) {
		pageownpool.npages = 0;
		pageownpool.nowned = 0;
		pageownpool.nshared = 0;
		pageownpool.nmut = 0;
		pageownpool.pages = nil;
		return;
	}

	/* Calculate required memory size */
	ulong size = npages * sizeof(struct PageOwner);
	print("pageown: allocating %lud bytes for %lud pages\n", size, npages);
	
	/* Check for unreasonably large allocation */
	if (size > 128*1024*1024) {  /* More than 128MB */
		print("pageown: unreasonably large allocation = %lud bytes\n", size);
		pageownpool.npages = 0;
		pageownpool.nowned = 0;
		pageownpool.nshared = 0;
		pageownpool.nmut = 0;
		pageownpool.pages = nil;
		return;
	}

	/* Allocate page owner array */
	pageownpool.pages = xalloc(size);
	if(pageownpool.pages == nil) {
		/* Fallback - this should not happen in normal operation */
		pageownpool.npages = 0;
		pageownpool.nowned = 0;
		pageownpool.nshared = 0;
		pageownpool.nmut = 0;
		print("pageown: failed to allocate page owner array (size: %lud)\n", size);
		return;
	}

	/* Allocate page owner array */
	pageownpool.pages = xalloc(size);
	if(pageownpool.pages == nil) {
		/* Fallback - this should not happen in normal operation */
		pageownpool.npages = 0;
		pageownpool.nowned = 0;
		pageownpool.nshared = 0;
		pageownpool.nmut = 0;
		print("pageown: failed to allocate page owner array (size: %lud)\n", size);
		return;
	}

	/* Initialize all pages to FREE state */
	for(i = 0; i < npages; i++) {
		pageownpool.pages[i].owner = nil;
		pageownpool.pages[i].state = POWN_FREE;
		pageownpool.pages[i].shared_count = 0;
		pageownpool.pages[i].shared_borrower_count = 0;
		pageownpool.pages[i].mut_borrower = nil;
		pageownpool.pages[i].acquired_ns = 0;
		pageownpool.pages[i].borrow_deadline_ns = 0;
		pageownpool.pages[i].owner_vaddr = 0;
		pageownpool.pages[i].owner_pte = nil;
		pageownpool.pages[i].pa = i << PGSHIFT;
		pageownpool.pages[i].transfer_count = 0;
		pageownpool.pages[i].borrow_count = 0;
	}

	pageownpool.npages = npages;
	pageownpool.nowned = 0;
	pageownpool.nshared = 0;
	pageownpool.nmut = 0;

	print("pageown: initialized with %lud pages\n", npages);
}

/*
 * Acquire ownership of a page
 * This is like: let x = Page::new(); (exclusive ownership)
 */
enum PageOwnError
pageown_acquire(Proc *p, uintptr pa, u64int vaddr)
{
	struct PageOwner *own;

	if(p == nil || (pa & (BY2PG-1)) != 0)
		return POWN_EINVAL;

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		iunlock(&pageownpool);
		return POWN_EINVAL;
	}

	/* Check if already owned */
	if(own->state != POWN_FREE) {
		iunlock(&pageownpool);
		return POWN_EALREADY;
	}

	/* Acquire exclusive ownership */
	own->owner = p;
	own->state = POWN_EXCLUSIVE;
	own->owner_vaddr = vaddr;
	own->acquired_ns = todget(nil, nil);
	pageownpool.nowned++;

	iunlock(&pageownpool);
	return POWN_OK;
}

/*
 * Release ownership of a page
 * This is like: drop(x); (move to free pool)
 */
enum PageOwnError
pageown_release(Proc *p, uintptr pa)
{
	struct PageOwner *own;

	if(p == nil)
		return POWN_EINVAL;

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		iunlock(&pageownpool);
		return POWN_EINVAL;
	}

	/* Must be the owner */
	if(own->owner != p) {
		iunlock(&pageownpool);
		return POWN_ENOTOWNER;
	}

	/* Can't release if borrowed */
	if(own->shared_count > 0 || own->mut_borrower != nil) {
		iunlock(&pageownpool);
		return POWN_EBORROWED;
	}

	/* Release ownership */
	own->owner = nil;
	own->state = POWN_FREE;
	own->owner_vaddr = 0;
	own->owner_pte = nil;
	pageownpool.nowned--;

	iunlock(&pageownpool);
	return POWN_OK;
}

/*
 * Transfer ownership from one process to another
 * This is like: let b = a; (move semantics, 'a' is invalidated)
 */
enum PageOwnError
pageown_transfer(Proc *from, Proc *to, uintptr pa, u64int new_vaddr)
{
	struct PageOwner *own;

	if(from == nil || to == nil)
		return POWN_EINVAL;

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		iunlock(&pageownpool);
		return POWN_EINVAL;
	}

	/* Must be the current owner */
	if(own->owner != from) {
		iunlock(&pageownpool);
		return POWN_ENOTOWNER;
	}

	/* Can't transfer if borrowed */
	if(own->shared_count > 0 || own->mut_borrower != nil) {
		iunlock(&pageownpool);
		return POWN_EBORROWED;
	}

	/* Transfer ownership */
	own->owner = to;
	own->owner_vaddr = new_vaddr;
	own->acquired_ns = todget(nil, nil);
	own->transfer_count++;

	/* State remains POWN_EXCLUSIVE */

	iunlock(&pageownpool);
	return POWN_OK;
}

/*
 * Borrow page as shared (read-only)
 * This is like: fn read(data: &Vec<u8>)
 * Rules:
 * - Multiple shared borrows allowed
 * - No mutable borrows allowed while shared borrows exist
 */
enum PageOwnError
pageown_borrow_shared(Proc *owner, Proc *borrower, uintptr pa, u64int vaddr)
{
	struct PageOwner *own;

	if(owner == nil || borrower == nil)
		return POWN_EINVAL;

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		iunlock(&pageownpool);
		return POWN_EINVAL;
	}

	/* Must be owned by the specified owner */
	if(own->owner != owner) {
		iunlock(&pageownpool);
		return POWN_ENOTOWNER;
	}

	/* Can't borrow shared if there's a mutable borrow */
	if(own->mut_borrower != nil) {
		iunlock(&pageownpool);
		return POWN_EMUTBORROW;
	}

	/* Check if we have space for another shared borrower */
	if(own->shared_borrower_count >= MAX_SHARED_BORROWS) {
		iunlock(&pageownpool);
		return POWN_ENOMEM;
	}

	/* Add shared borrow */
	own->shared_borrowers[own->shared_borrower_count] = borrower;
	own->shared_borrower_count++;
	own->shared_count++;
	own->borrow_count++;

	/* Update state if this is the first borrow */
	if(own->state == POWN_EXCLUSIVE)
		own->state = POWN_SHARED_OWNED;

	if(own->shared_count == 1)
		pageownpool.nshared++;

	iunlock(&pageownpool);
	return POWN_OK;
}

/*
 * Borrow page as mutable (exclusive read-write)
 * This is like: fn modify(data: &mut Vec<u8>)
 * Rules:
 * - Only one mutable borrow allowed
 * - No other borrows (shared or mutable) allowed
 * - Original owner loses access temporarily
 */
enum PageOwnError
pageown_borrow_mut(Proc *owner, Proc *borrower, uintptr pa, u64int vaddr)
{
	struct PageOwner *own;

	if(owner == nil || borrower == nil)
		return POWN_EINVAL;

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		iunlock(&pageownpool);
		return POWN_EINVAL;
	}

	/* Must be owned by the specified owner */
	if(own->owner != owner) {
		iunlock(&pageownpool);
		return POWN_ENOTOWNER;
	}

	/* Can't borrow mutably if any borrows exist */
	if(own->shared_count > 0) {
		iunlock(&pageownpool);
		return POWN_ESHAREDBORROW;
	}

	if(own->mut_borrower != nil) {
		iunlock(&pageownpool);
		return POWN_EMUTBORROW;
	}

	/* Add mutable borrow */
	own->mut_borrower = borrower;
	own->state = POWN_MUT_LENT;
	own->borrow_count++;
	pageownpool.nmut++;

	iunlock(&pageownpool);
	return POWN_OK;
}

/*
 * Return a shared borrow
 * This is like: } (end of borrow scope for &)
 */
enum PageOwnError
pageown_return_shared(Proc *borrower, uintptr pa)
{
	struct PageOwner *own;

	if(borrower == nil)
		return POWN_EINVAL;

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		iunlock(&pageownpool);
		return POWN_EINVAL;
	}

	/* Find and remove this borrower */
	int found = 0;
	for(int i = 0; i < own->shared_borrower_count; i++) {
		if(own->shared_borrowers[i] == borrower) {
			/* Remove this borrower by shifting others down */
			for(int j = i; j < own->shared_borrower_count - 1; j++) {
				own->shared_borrowers[j] = own->shared_borrowers[j + 1];
			}
			own->shared_borrower_count--;
			own->shared_count--;
			found = 1;
			break;
		}
	}

	if(!found) {
		iunlock(&pageownpool);
		return POWN_ENOTBORROWED;
	}

	/* If no more shared borrows, revert to exclusive */
	if(own->shared_count == 0) {
		own->state = POWN_EXCLUSIVE;
		pageownpool.nshared--;
	}

	iunlock(&pageownpool);
	return POWN_OK;
}

/*
 * Return a mutable borrow
 * This is like: } (end of borrow scope for &mut)
 */
enum PageOwnError
pageown_return_mut(Proc *borrower, uintptr pa)
{
	struct PageOwner *own;

	if(borrower == nil)
		return POWN_EINVAL;

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		iunlock(&pageownpool);
		return POWN_EINVAL;
	}

	/* Must be the mutable borrower */
	if(own->mut_borrower != borrower) {
		iunlock(&pageownpool);
		return POWN_ENOTBORROWED;
	}

	/* Return the mutable borrow */
	own->mut_borrower = nil;
	own->state = POWN_EXCLUSIVE;
	pageownpool.nmut--;

	iunlock(&pageownpool);
	return POWN_OK;
}

/* Query functions */

int
pageown_is_owned(uintptr pa)
{
	struct PageOwner *own;
	int owned;

	ilock(&pageownpool);
	own = pa2owner(pa);
	owned = (own != nil && own->state != POWN_FREE);
	iunlock(&pageownpool);

	return owned;
}

Proc*
pageown_get_owner(uintptr pa)
{
	struct PageOwner *own;
	Proc *owner;

	ilock(&pageownpool);
	own = pa2owner(pa);
	owner = (own != nil) ? own->owner : nil;
	iunlock(&pageownpool);

	return owner;
}

enum PageOwnerState
pageown_get_state(uintptr pa)
{
	struct PageOwner *own;
	enum PageOwnerState state;

	ilock(&pageownpool);
	own = pa2owner(pa);
	state = (own != nil) ? own->state : POWN_FREE;
	iunlock(&pageownpool);

	return state;
}

int
pageown_can_borrow_shared(uintptr pa)
{
	struct PageOwner *own;
	int can;

	ilock(&pageownpool);
	own = pa2owner(pa);
	can = (own != nil && own->state != POWN_FREE && own->mut_borrower == nil);
	iunlock(&pageownpool);

	return can;
}

int
pageown_can_borrow_mut(uintptr pa)
{
	struct PageOwner *own;
	int can;

	ilock(&pageownpool);
	own = pa2owner(pa);
	can = (own != nil && own->state != POWN_FREE &&
	       own->shared_count == 0 && own->mut_borrower == nil);
	iunlock(&pageownpool);

	return can;
}

/*
 * Cleanup all pages owned or borrowed by a process
 * Called when process dies - implements "drop" semantics
 */
void
pageown_cleanup_process(Proc *p)
{
	ulong i;
	struct PageOwner *own;
	int cleaned = 0;

	if(p == nil)
		return;

	ilock(&pageownpool);

	/* Scan all pages looking for this process */
	for(i = 0; i < pageownpool.npages; i++) {
		own = &pageownpool.pages[i];

		/* Release owned pages */
		if(own->owner == p) {
			/* Force release even if borrowed - process is dying */
			own->owner = nil;
			own->state = POWN_FREE;
			own->shared_count = 0;
			own->shared_borrower_count = 0;
			own->mut_borrower = nil;
			own->owner_vaddr = 0;
			own->owner_pte = nil;
			pageownpool.nowned--;
			cleaned++;
		}

		/* Remove mutable borrows by this process */
		if(own->mut_borrower == p) {
			own->mut_borrower = nil;
			if(own->state == POWN_MUT_LENT)
				own->state = POWN_EXCLUSIVE;
			pageownpool.nmut--;
			cleaned++;
		}

		/* Remove shared borrows by this process */
		for(int j = 0; j < own->shared_borrower_count; j++) {
			if(own->shared_borrowers[j] == p) {
				/* Remove this borrower by shifting others down */
				for(int k = j; k < own->shared_borrower_count - 1; k++) {
					own->shared_borrowers[k] = own->shared_borrowers[k + 1];
				}
				own->shared_borrower_count--;
				own->shared_count--;
				if(own->shared_count == 0 && own->state == POWN_SHARED_OWNED) {
					own->state = POWN_EXCLUSIVE;
					pageownpool.nshared--;
				}
				cleaned++;
				break; /* Each process can only borrow a page once */
			}
		}
	}

	iunlock(&pageownpool);

	if(cleaned > 0)
		print("pageown: cleaned %d pages for pid %d\n", cleaned, p->pid);
}

/* Statistics */

void
pageown_stats(void)
{
	ilock(&pageownpool);
	print("Page Ownership Statistics:\n");
	print("  Total pages:   %lud\n", pageownpool.npages);
	print("  Owned:         %lud\n", pageownpool.nowned);
	print("  Shared borrows: %lud\n", pageownpool.nshared);
	print("  Mut borrows:   %lud\n", pageownpool.nmut);
	iunlock(&pageownpool);
}

void
pageown_dump_page(uintptr pa)
{
	struct PageOwner *own;
	char *statestr[] = {
		"FREE",
		"EXCLUSIVE",
		"SHARED_OWNED",
		"MUT_LENT",
	};

	ilock(&pageownpool);
	own = pa2owner(pa);
	if(own == nil) {
		print("Invalid physical address: %#p\n", pa);
		iunlock(&pageownpool);
		return;
	}

	print("Page %#p (PFN %lud):\n", pa, pa2pfn(pa));
	print("  State:          %s\n", statestr[own->state]);
	print("  Owner:          %s (pid %d)\n",
		own->owner ? own->owner->text : "none",
		own->owner ? own->owner->pid : -1);
	print("  Shared borrows: %d\n", own->shared_count);
	print("  Mut borrower:   %s (pid %d)\n",
		own->mut_borrower ? own->mut_borrower->text : "none",
		own->mut_borrower ? own->mut_borrower->pid : -1);
	print("  Transfers:      %lud\n", own->transfer_count);
	print("  Total borrows:  %lud\n", own->borrow_count);

	iunlock(&pageownpool);
}
