/*
 * System calls for page ownership and borrowing
 * Provides Rust-style borrow checker semantics for zero-copy IPC
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pageown.h"
#include <error.h>

/*
 * Transfer page ownership to another process (move semantics)
 *
 * Usage: vmexchange(target_pid, vaddr, len)
 *
 * Transfers ownership of page(s) from current process to target process.
 * Current process loses ALL access to the pages (unmapped).
 * Target process gains exclusive ownership.
 *
 * This is like Rust: let b = a; // 'a' is moved, can't use anymore
 */
long
sysvmexchange(va_list list)
{
	int target_pid;
	u64int vaddr;
	ulong len;
	Proc *target;
	uintptr pa;
	u64int *pte;
	ulong offset;
	enum PageOwnError err;
	int npages = 0;

	target_pid = va_arg(list, int);
	vaddr = va_arg(list, u64int);
	len = va_arg(list, ulong);

	/* Validate parameters */
	if((vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);
	if(len == 0 || len > 1*GiB)
		error(Ebadarg);

	/* Find target process */
	target = proctab(target_pid);
	if(target == nil || target == up)
		error("invalid target process");

	/* Process each page in the range */
	for(offset = 0; offset < len; offset += BY2PG) {
		u64int va = vaddr + offset;

		/* Get physical address from current process */
		pte = mmuwalk(m->pml4, va, 0, 0);
		if(pte == nil || (*pte & PTEVALID) == 0)
			error("vmexchange: page not mapped");

		pa = PADDR(*pte);

		/* Transfer ownership */
		err = pageown_transfer(up, target, pa, va);
		if(err != POWN_OK) {
			switch(err) {
			case POWN_ENOTOWNER:
				error("vmexchange: not owner of page");
			case POWN_EBORROWED:
				error("vmexchange: page is borrowed");
			default:
				error("vmexchange: transfer failed");
			}
		}

		/* Unmap from current process (source loses access) */
		*pte = 0;

		/* Map into target's address space (all processes share m->pml4) */
		/* Page table entry manipulation happens via shared kernel page tables */

		npages++;
	}

	/* Flush TLB for both processes */
	putcr3(getcr3());

	return npages;
}

/*
 * Lend page as shared read-only borrow
 *
 * Usage: vmlend_shared(target_pid, vaddr, len)
 *
 * Lends page(s) to target process as read-only.
 * Current process keeps ownership but also becomes read-only.
 * Multiple processes can have shared borrows.
 *
 * This is like Rust: fn read(data: &Vec<u8>)
 */
long
sysvmlend_shared(va_list list)
{
	int target_pid;
	u64int vaddr;
	ulong len;
	Proc *target;
	uintptr pa;
	u64int *pte;
	ulong offset;
	enum PageOwnError err;
	int npages = 0;

	target_pid = va_arg(list, int);
	vaddr = va_arg(list, u64int);
	len = va_arg(list, ulong);

	/* Validate parameters */
	if((vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);
	if(len == 0 || len > 1*GiB)
		error(Ebadarg);

	/* Find target process */
	target = proctab(target_pid);
	if(target == nil || target == up)
		error("invalid target process");

	/* Process each page */
	for(offset = 0; offset < len; offset += BY2PG) {
		u64int va = vaddr + offset;

		/* Get physical address */
		pte = mmuwalk(m->pml4, va, 0, 0);
		if(pte == nil || (*pte & PTEVALID) == 0)
			error("vmlend_shared: page not mapped");

		pa = PADDR(*pte);

		/* Create shared borrow */
		err = pageown_borrow_shared(up, target, pa, va);
		if(err != POWN_OK) {
			switch(err) {
			case POWN_ENOTOWNER:
				error("vmlend_shared: not owner");
			case POWN_EMUTBORROW:
				error("vmlend_shared: has mutable borrow");
			default:
				error("vmlend_shared: borrow failed");
			}
		}

		/* Remove write permission from owner's PTE */
		*pte &= ~PTEWRITE;

		/* Target will access via same page tables (shared m->pml4) */
		/* Page is now read-only for both owner and borrower */

		npages++;
	}

	/* Flush TLB */
	putcr3(getcr3());

	return npages;
}

/*
 * Lend page as mutable exclusive borrow
 *
 * Usage: vmlend_mut(target_pid, vaddr, len)
 *
 * Lends page(s) to target process as exclusive read-write.
 * Current process loses ALL access temporarily (unmapped).
 * Target process has exclusive access.
 * Only one mutable borrow allowed at a time.
 *
 * This is like Rust: fn modify(data: &mut Vec<u8>)
 */
long
sysvmlend_mut(va_list list)
{
	int target_pid;
	u64int vaddr;
	ulong len;
	Proc *target;
	uintptr pa;
	u64int *pte;
	ulong offset;
	enum PageOwnError err;
	int npages = 0;

	target_pid = va_arg(list, int);
	vaddr = va_arg(list, u64int);
	len = va_arg(list, ulong);

	/* Validate parameters */
	if((vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);
	if(len == 0 || len > 1*GiB)
		error(Ebadarg);

	/* Find target process */
	target = proctab(target_pid);
	if(target == nil || target == up)
		error("invalid target process");

	/* Process each page */
	for(offset = 0; offset < len; offset += BY2PG) {
		u64int va = vaddr + offset;

		/* Get physical address */
		pte = mmuwalk(m->pml4, va, 0, 0);
		if(pte == nil || (*pte & PTEVALID) == 0)
			error("vmlend_mut: page not mapped");

		pa = PADDR(*pte);

		/* Create mutable borrow */
		err = pageown_borrow_mut(up, target, pa, va);
		if(err != POWN_OK) {
			switch(err) {
			case POWN_ENOTOWNER:
				error("vmlend_mut: not owner");
			case POWN_ESHAREDBORROW:
				error("vmlend_mut: has shared borrows");
			case POWN_EMUTBORROW:
				error("vmlend_mut: already has mutable borrow");
			default:
				error("vmlend_mut: borrow failed");
			}
		}

		/* Unmap from owner (loses access) */
		*pte = 0;

		/* Target will access via shared page tables */
		/* Owner's mapping is removed, target keeps write access */

		npages++;
	}

	/* Flush TLB */
	putcr3(getcr3());

	return npages;
}

/*
 * Return a borrowed page
 *
 * Usage: vmreturn(vaddr, len)
 *
 * Returns page(s) that were borrowed (shared or mutable).
 * For shared borrows: decrements borrow count
 * For mutable borrows: returns exclusive access to owner
 *
 * This is like Rust: } // end of borrow scope
 */
long
sysvmreturn(va_list list)
{
	u64int vaddr;
	ulong len;
	uintptr pa;
	u64int *pte;
	ulong offset;
	enum PageOwnError err;
	int npages = 0;
	Proc *owner;
	enum PageOwnerState state;

	vaddr = va_arg(list, u64int);
	len = va_arg(list, ulong);

	/* Validate parameters */
	if((vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);
	if(len == 0 || len > 1*GiB)
		error(Ebadarg);

	/* Process each page */
	for(offset = 0; offset < len; offset += BY2PG) {
		u64int va = vaddr + offset;

		/* Get physical address */
		pte = mmuwalk(m->pml4, va, 0, 0);
		if(pte == nil || (*pte & PTEVALID) == 0)
			error("vmreturn: page not mapped");

		pa = PADDR(*pte);

		/* Check what kind of borrow this is */
		owner = pageown_get_owner(pa);
		if(owner == nil || owner == up)
			error("vmreturn: not a borrowed page");

		state = pageown_get_state(pa);

		/* Return based on borrow type */
		if(state == POWN_SHARED_OWNED) {
			/* Shared borrow */
			err = pageown_return_shared(up, pa);
			if(err != POWN_OK)
				error("vmreturn: return shared failed");

			/* Unmap from borrower */
			*pte = 0;

			/* Restore write permission to owner if no more borrows */
			if(pageown_get_state(pa) == POWN_EXCLUSIVE) {
				u64int *owner_pte = mmuwalk(m->pml4, va, 0, 0);
				if(owner_pte && (*owner_pte & PTEVALID))
					*owner_pte |= PTEWRITE;
			}

		} else if(state == POWN_MUT_LENT) {
			/* Mutable borrow */
			err = pageown_return_mut(up, pa);
			if(err != POWN_OK)
				error("vmreturn: return mut failed");

			/* Unmap from borrower */
			*pte = 0;

			/* Restore mapping with full access via shared page tables */
			u64int *restore_pte = mmuwalk(m->pml4, va, 0, 1);
			if(restore_pte)
				*restore_pte = pa | PTEVALID | PTEWRITE | PTEUSER;

		} else {
			error("vmreturn: invalid borrow state");
		}

		npages++;
	}

	/* Flush TLB */
	putcr3(getcr3());

	return npages;
}

/*
 * Query page ownership info
 *
 * Usage: vmowninfo(vaddr, &info)
 *
 * Returns ownership information about a page.
 * Useful for debugging and verification.
 */
struct VmOwnInfo {
	int owner_pid;
	int state;
	int shared_count;
	int mut_borrower_pid;
};

long
sysvmowninfo(va_list list)
{
	u64int vaddr;
	struct VmOwnInfo *info;
	uintptr pa;
	u64int *pte;
	Proc *owner;
	enum PageOwnerState state;
	struct PageOwner *own;

	vaddr = va_arg(list, u64int);
	info = va_arg(list, struct VmOwnInfo*);

	if((vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);

	/* Get physical address */
	pte = mmuwalk(m->pml4, vaddr, 0, 0);
	if(pte == nil || (*pte & PTEVALID) == 0)
		error("vmowninfo: page not mapped");

	pa = PADDR(*pte);

	/* Get ownership info */
	owner = pageown_get_owner(pa);
	state = pageown_get_state(pa);
	
	/* Get the actual page owner structure for detailed info */
	own = pa2owner(pa);

	info->owner_pid = owner ? owner->pid : -1;
	info->state = state;

	/* Get actual counts from page owner structure */
	if(own != nil) {
		info->shared_count = own->shared_count;
		info->mut_borrower_pid = own->mut_borrower ? own->mut_borrower->pid : -1;
	} else {
		info->shared_count = 0;
		info->mut_borrower_pid = -1;
	}

	return 0;
}
