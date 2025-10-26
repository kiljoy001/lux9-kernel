/*
 * Rust-style borrow checker for VM page ownership
 * Provides memory safety for zero-copy IPC
 */

#pragma once

/* Page ownership states - based on Rust borrow semantics */
enum PageOwnerState {
	POWN_FREE = 0,          /* Page is unowned (in free pool) */
	POWN_EXCLUSIVE,         /* Owned exclusively by one process (moved) */
	POWN_SHARED_OWNED,      /* Owner has page, but lent as shared (&) */
	POWN_MUT_LENT,          /* Owner lent page as mutable (&mut), blocked */
};

/* Per-page ownership tracking descriptor */
struct PageOwner {
	/* Core ownership */
	Proc	*owner;			/* Original owner process */
	enum PageOwnerState state;	/* Current ownership state */

	/* Borrow tracking */
	int	shared_count;		/* Number of shared borrows (&) */
	Proc	*mut_borrower;		/* Exclusive mutable borrower (&mut) */
	
	/* Track individual shared borrowers for cleanup */
	#define MAX_SHARED_BORROWS 16
	Proc	*shared_borrowers[MAX_SHARED_BORROWS];	/* Processes with shared borrows */
	int	shared_borrower_count;			/* Current count */

	/* Lifetime tracking */
	uvlong	acquired_ns;		/* When ownership was acquired */
	uvlong	borrow_deadline_ns;	/* Automatic return time (0 = never) */

	/* Page table linkage */
	u64int	owner_vaddr;		/* Virtual address in owner's space */
	u64int	*owner_pte;		/* PTE in owner's page table */

	/* Physical page info */
	uintptr	pa;			/* Physical address of page */

	/* Debugging */
	ulong	transfer_count;		/* Number of ownership transfers */
	ulong	borrow_count;		/* Total times borrowed */
};

/* Page ownership pool - one entry per physical page */
struct PageOwnPool {
	Lock lk;  /* Named lock instead of anonymous */
	struct PageOwner *pages;	/* Array indexed by page frame number */
	ulong	npages;			/* Total number of pages */
	ulong	nowned;			/* Pages currently owned */
	ulong	nshared;		/* Pages with shared borrows */
	ulong	nmut;			/* Pages with mutable borrows */
};

/* Error codes for ownership operations */
enum PageOwnError {
	POWN_OK = 0,
	POWN_EALREADY,		/* Already owned */
	POWN_ENOTOWNER,		/* Not the owner */
	POWN_EBORROWED,		/* Can't modify - has borrows */
	POWN_EMUTBORROW,	/* Can't borrow - already &mut */
	POWN_ESHAREDBORROW,	/* Can't borrow &mut - has & */
	POWN_ENOTBORROWED,	/* Not borrowed, can't return */
	POWN_EINVAL,		/* Invalid parameters */
	POWN_ENOMEM,		/* Out of memory */
};

/* Global page ownership pool */
extern struct PageOwnPool pageownpool;

/* Core ownership operations */
void	pageowninit(void);
enum PageOwnError pageown_acquire(Proc *p, uintptr pa, u64int vaddr);
enum PageOwnError pageown_release(Proc *p, uintptr pa);
enum PageOwnError pageown_transfer(Proc *from, Proc *to, uintptr pa, u64int new_vaddr);

/* Borrow operations (Rust-style) */
enum PageOwnError pageown_borrow_shared(Proc *owner, Proc *borrower, uintptr pa, u64int vaddr);
enum PageOwnError pageown_borrow_mut(Proc *owner, Proc *borrower, uintptr pa, u64int vaddr);
enum PageOwnError pageown_return_shared(Proc *borrower, uintptr pa);
enum PageOwnError pageown_return_mut(Proc *borrower, uintptr pa);

/* Query operations */
int	pageown_is_owned(uintptr pa);
Proc*	pageown_get_owner(uintptr pa);
enum PageOwnerState pageown_get_state(uintptr pa);
int	pageown_can_borrow_shared(uintptr pa);
int	pageown_can_borrow_mut(uintptr pa);

/* Process cleanup - called when process dies */
void	pageown_cleanup_process(Proc *p);

/* Get page owner descriptor for physical address */
struct PageOwner* pa2owner(uintptr pa);

/* Statistics and debugging */
void	pageown_stats(void);
void	pageown_dump_page(uintptr pa);
