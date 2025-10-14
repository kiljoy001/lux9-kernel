/*
 * Unified borrow checker for kernel primitives
 * Provides Rust-style ownership and borrowing for locks, memory, I/O, etc.
 */

#pragma once

/* Borrow states - based on Rust borrow semantics */
enum BorrowState {
	BORROW_FREE = 0,          /* Resource is unowned */
	BORROW_EXCLUSIVE,         /* Owned exclusively by one process */
	BORROW_SHARED_OWNED,      /* Owner has resource, but lent as shared */
	BORROW_MUT_LENT,          /* Owner lent resource as mutable, blocked */
};

/* Per-resource ownership tracking descriptor */
struct BorrowOwner {
	/* Resource identification */
	uintptr	key;			/* Unique key for the resource (e.g., address) */

	/* Core ownership */
	Proc	*owner;			/* Original owner process */
	enum BorrowState state;		/* Current ownership state */

	/* Borrow tracking */
	int	shared_count;		/* Number of shared borrows (&) */
	Proc	*mut_borrower;		/* Exclusive mutable borrower (&mut) */

	/* Lifetime tracking */
	uvlong	acquired_ns;		/* When ownership was acquired */
	uvlong	borrow_deadline_ns;	/* Automatic return time (0 = never) */

	/* Debugging */
	ulong	borrow_count;		/* Total times borrowed */

	struct BorrowOwner *next;	/* For hash table chaining */
};

/* Hash table bucket for BorrowOwner */
struct BorrowBucket {
	struct BorrowOwner *head;
};

/* Borrow pool - hash table of resources */
struct BorrowPool {
	Lock lock;
	struct BorrowBucket *owners;	/* Hash table buckets */
	ulong	nbuckets;		/* Number of hash buckets */
	ulong	nowners;		/* Total number of owned resources */
	ulong	nshared;		/* Resources with shared borrows */
	ulong	nmut;			/* Resources with mutable borrows */
};

/* Error codes for borrow operations */
enum BorrowError {
	BORROW_OK = 0,
	BORROW_EALREADY,		/* Already owned */
	BORROW_ENOTOWNER,		/* Not the owner */
	BORROW_EBORROWED,		/* Can't modify - has borrows */
	BORROW_EMUTBORROW,		/* Can't borrow - already &mut */
	BORROW_ESHAREDBORROW,		/* Can't borrow &mut - has & */
	BORROW_ENOTBORROWED,		/* Not borrowed, can't return */
	BORROW_EINVAL,			/* Invalid parameters */
	BORROW_ENOMEM,			/* Out of memory */
	BORROW_ENOTFOUND,		/* Resource not found */
};

/* Global borrow pool */
extern struct BorrowPool borrowpool;

/* Core ownership operations */
void	borrowinit(void);
enum BorrowError borrow_acquire(Proc *p, uintptr key);
enum BorrowError borrow_release(Proc *p, uintptr key);
enum BorrowError borrow_transfer(Proc *from, Proc *to, uintptr key);

/* Borrow operations (Rust-style) */
enum BorrowError borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_return_shared(Proc *borrower, uintptr key);
enum BorrowError borrow_return_mut(Proc *borrower, uintptr key);

/* Query operations */
int	borrow_is_owned(uintptr key);
Proc*	borrow_get_owner(uintptr key);
enum BorrowState borrow_get_state(uintptr key);
int	borrow_can_borrow_shared(uintptr key);
int	borrow_can_borrow_mut(uintptr key);

/* Process cleanup - called when process dies */
void	borrow_cleanup_process(Proc *p);

/* Statistics and debugging */
void	borrow_stats(void);
void	borrow_dump_resource(uintptr key);

/* Hash function for keys */
ulong	borrow_hash(uintptr key);