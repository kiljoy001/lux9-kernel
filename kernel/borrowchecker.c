/*
 * Unified borrow checker for kernel primitives
 * Provides Rust-style ownership and borrowing for locks, memory, I/O, etc.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"

/* External declarations for CR3 switch memory system checks */
extern uintptr saved_limine_hhdm_offset;
extern struct MemoryCoordination mem_coord;
extern struct BorrowPool borrowpool;
extern int xinit_done;  /* Defined in xalloc.c, set after xinit() completes */
#include "fns.h"
#include "borrowchecker.h"
#include "lock_dag.h"

/* Global borrow pool */
struct BorrowPool borrowpool;

/**
 * Initialize the global borrow pool and allocate its bucket table using the bootstrap allocator.
 *
 * Sets the pool to 1024 buckets, allocates the bucket array via bootstrap_alloc, initializes all bucket
 * heads to nil, and resets the owner, shared, and mutable borrow counters to zero. Panics if allocation fails.
 */
void
borrowinit(void)
{
	ulong i;
	
	lockdag_init();
	/* Always use bootstrap allocator for early boot - no xinit dependency */
	borrowpool.nbuckets = 1024; /* Arbitrary size, can be tuned */
	borrowpool.owners = bootstrap_alloc(borrowpool.nbuckets * sizeof(struct BorrowBucket));
	if (borrowpool.owners == nil) {
		panic("borrowinit: failed to allocate hash table");
	}
	print("borrowinit: using bootstrap_alloc hash table (%lu buckets)\n", borrowpool.nbuckets);

	for (i = 0; i < borrowpool.nbuckets; i++) {
		borrowpool.owners[i].head = nil;
	}

	borrowpool.nowners = 0;
	borrowpool.nshared = 0;
	borrowpool.nmut = 0;
}

/* Hash function for uintptr keys */
ulong
borrow_hash(uintptr key)
{
	if(borrowpool.nbuckets == 0 || borrowpool.owners == nil)
		panic("borrow_hash: borrowinit not called");
	/* Simple hash: use the lower bits */
	return key % borrowpool.nbuckets;
}

/* Find BorrowOwner for a key */
static struct BorrowOwner*
find_owner(uintptr key)
{
	ulong hash = borrow_hash(key);
	struct BorrowOwner *owner = borrowpool.owners[hash].head;

	while (owner != nil) {
		if (owner->key == key) {
			return owner;
		}
		owner = owner->next;
	}
	return nil;
}

/**
 * Create and insert a new BorrowOwner for the given resource key.
 *
 * The new BorrowOwner is allocated using the bootstrap allocator, initialized with
 * default fields (no owner, state BORROW_FREE, system_owner OWNER_BOOTLOADER,
 * is_system_owned == 0, zeroed counters and lists), and prepended into the
 * borrow pool bucket computed from `key`.
 *
 * @param key Resource identifier used as the borrow-owner lookup key.
 * @returns Pointer to the newly created BorrowOwner, or `nil` if allocation failed.
 */
static struct BorrowOwner*
create_owner(uintptr key)
{
	ulong hash = borrow_hash(key);
	struct BorrowOwner *owner;

	/* Always use bootstrap allocator for early boot */
	owner = bootstrap_alloc(sizeof(struct BorrowOwner));
	if (owner == nil) {
		print("create_owner: bootstrap_alloc failed for BorrowOwner\n");
		return nil;
	}

	owner->key = key;
	owner->owner = nil;
	owner->state = BORROW_FREE;
	owner->system_owner = OWNER_BOOTLOADER;  /* Default to bootloader ownership */
	owner->is_system_owned = 0;  /* Not system owned by default */
	owner->shared_count = 0;
	owner->shared_list = nil;  /* Initialize shared borrower list */
	owner->mut_borrower = nil;
	owner->acquired_ns = 0;
	owner->borrow_deadline_ns = 0;
	owner->borrow_count = 0;
	owner->next = borrowpool.owners[hash].head;
	owner->alloc_source = ALLOC_BOOTSTRAP;  /* Track allocation source */
	borrowpool.owners[hash].head = owner;

	borrowpool.nowners++;
	return owner;
}

/* Acquire ownership of a resource */
enum BorrowError
borrow_acquire(Proc *p, uintptr key)
{
	struct BorrowOwner *owner;

	if (p == nil) {
		return BORROW_EINVAL;
	}

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	if (owner == nil) {
		owner = create_owner(key);
		if (owner == nil) {
			iunlock(&borrowpool.lock);
			return BORROW_ENOMEM;
		}
	}

	if (owner->state != BORROW_FREE) {
		iunlock(&borrowpool.lock);
		return BORROW_EALREADY;
	}

	owner->owner = p;
	owner->state = BORROW_EXCLUSIVE;
	owner->acquired_ns = todget(nil, nil);
	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/**
 * Release ownership of a resource identified by `key` held by process `p`.
 *
 * Validates that `p` is the current owner and that there are no active shared or
 * mutable borrows; on success clears ownership, marks the resource free, removes
 * the owner entry from the hash table, and frees that entry only if `xinit_done`
 * indicates post-initialization.
 *
 * @param p Process attempting the release; must be non-nil and the current owner.
 * @param key Resource key whose ownership is being released.
 * @returns BORROW_OK on success.
 * @returns BORROW_EINVAL if `p` is nil.
 * @returns BORROW_ENOTFOUND if no owner entry exists for `key`.
 * @returns BORROW_ENOTOWNER if `p` is not the recorded owner of the resource.
 * @returns BORROW_EBORROWED if there are active shared or mutable borrows that prevent release.
 */
enum BorrowError
borrow_release(Proc *p, uintptr key)
{
	struct BorrowOwner *owner, *prev;
	ulong hash;

	if (p == nil) {
		return BORROW_EINVAL;
	}

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	if (owner == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (owner->owner != p) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTOWNER;
	}

	if (owner->shared_count > 0 || owner->shared_list != nil || owner->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EBORROWED;
	}

	owner->owner = nil;
	owner->state = BORROW_FREE;
	borrowpool.nowners--;

	/* FIX MEMORY LEAK: Remove and free the owner entry from hash table */
	hash = borrow_hash(key);
	prev = nil;
	for (owner = borrowpool.owners[hash].head; owner != nil; owner = owner->next) {
		if (owner->key == key) {
		if (prev == nil) {
			borrowpool.owners[hash].head = owner->next;
		} else {
			prev->next = owner->next;
		}
		/* Don't free bootstrap-allocated memory, only free xalloc'd memory */
		if (owner->alloc_source == ALLOC_XALLOC) {
			xfree(owner);
		}
		break;
		}
		prev = owner;
	}

	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/* Transfer ownership from one process to another */
enum BorrowError
borrow_transfer(Proc *from, Proc *to, uintptr key)
{
	struct BorrowOwner *owner;

	if (from == nil || to == nil) {
		return BORROW_EINVAL;
	}

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	if (owner == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (owner->owner != from) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTOWNER;
	}

	if (owner->shared_count > 0 || owner->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EBORROWED;
	}

	owner->owner = to;
	owner->acquired_ns = todget(nil, nil);
	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/**
 * Grant a shared borrow of the identified resource from an owner to a borrower.
 *
 * Adds the borrower to the owner's shared-borrow list and updates owner/resource
 * state and global counters when the operation succeeds.
 *
 * @param owner The current owning process of the resource.
 * @param borrower The process to receive the shared borrow.
 * @param key Identifier for the resource to be borrowed.
 * @returns BORROW_OK on success.
 * @returns BORROW_EINVAL if `owner` or `borrower` is NULL.
 * @returns BORROW_ENOTFOUND if no owner entry exists for `key`.
 * @returns BORROW_ENOTOWNER if `owner` is not the recorded owner of the resource.
 * @returns BORROW_EMUTBORROW if a mutable borrow is active for the resource.
 * @returns BORROW_EALREADY if `borrower` already holds a shared borrow for the resource.
 * @returns BORROW_ENOMEM if allocation of the shared-borrow record fails.
 */
enum BorrowError
borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key)
{
	struct BorrowOwner *own;
	struct SharedBorrower *sb;

	if (owner == nil || borrower == nil) {
		return BORROW_EINVAL;
	}

	ilock(&borrowpool.lock);
	own = find_owner(key);
	if (own == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (own->owner != owner) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTOWNER;
	}

	if (own->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EMUTBORROW;
	}

	/* Check if this borrower already has a shared borrow */
	for (sb = own->shared_list; sb != nil; sb = sb->next) {
		if (sb->proc == borrower) {
			iunlock(&borrowpool.lock);
			return BORROW_EALREADY;  /* Already has a shared borrow */
		}
	}

	/* Allocate new shared borrower node */
	sb = bootstrap_alloc(sizeof(struct SharedBorrower));
	if (sb == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOMEM;
	}

	/* Add to front of shared list */
	sb->proc = borrower;
	sb->next = own->shared_list;
	sb->alloc_source = ALLOC_BOOTSTRAP;  /* Track allocation source */
	own->shared_list = sb;

	own->shared_count++;
	own->borrow_count++;
	if (own->state == BORROW_EXCLUSIVE) {
		own->state = BORROW_SHARED_OWNED;
	}
	if (own->shared_count == 1) {
		borrowpool.nshared++;
	}

	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/* Borrow resource as mutable */
enum BorrowError
borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key)
{
	struct BorrowOwner *own;

	if (owner == nil || borrower == nil) {
		return BORROW_EINVAL;
	}

	ilock(&borrowpool.lock);
	own = find_owner(key);
	if (own == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (own->owner != owner) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTOWNER;
	}

	if (own->shared_count > 0) {
		iunlock(&borrowpool.lock);
		return BORROW_ESHAREDBORROW;
	}

	if (own->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EMUTBORROW;
	}

	own->mut_borrower = borrower;
	own->state = BORROW_MUT_LENT;
	own->borrow_count++;
	borrowpool.nmut++;

	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/**
 * Release a shared borrow held by a process for a given resource key.
 *
 * Removes the borrower from the resource's shared-borrow list, decrements
 * shared counters, updates the resource state when no shared borrowers
 * remain, and frees borrow bookkeeping memory when permitted.
 *
 * @param borrower Process returning the shared borrow; must not be nil.
 * @param key Resource identifier whose shared borrow is being returned.
 * @returns BORROW_OK on successful return;
 *          BORROW_EINVAL if `borrower` is nil;
 *          BORROW_ENOTFOUND if no owner exists for `key`;
 *          BORROW_ENOTBORROWED if the borrower does not hold a shared borrow. 
 */
enum BorrowError
borrow_return_shared(Proc *borrower, uintptr key)
{
	struct BorrowOwner *own;
	struct SharedBorrower *sb, *prev;

	if (borrower == nil) {
		return BORROW_EINVAL;
	}

	ilock(&borrowpool.lock);
	own = find_owner(key);
	if (own == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (own->shared_count == 0 || own->shared_list == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTBORROWED;
	}

	/* Find and remove borrower from shared list */
	prev = nil;
	for (sb = own->shared_list; sb != nil; sb = sb->next) {
		if (sb->proc == borrower) {
			/* Found it - remove from list */
			if (prev == nil) {
				own->shared_list = sb->next;
			} else {
				prev->next = sb->next;
			}
			/* Only free if allocated with xalloc after xinit */
					if (sb->alloc_source == ALLOC_XALLOC) {
						xfree(sb);
					}

			own->shared_count--;
			if (own->shared_count == 0) {
				own->state = BORROW_EXCLUSIVE;
				borrowpool.nshared--;
			}

			iunlock(&borrowpool.lock);
			return BORROW_OK;
		}
		prev = sb;
	}

	/* Borrower not found in list */
	iunlock(&borrowpool.lock);
	return BORROW_ENOTBORROWED;
}

/* Return a mutable borrow */
enum BorrowError
borrow_return_mut(Proc *borrower, uintptr key)
{
	struct BorrowOwner *own;

	if (borrower == nil) {
		return BORROW_EINVAL;
	}

	ilock(&borrowpool.lock);
	own = find_owner(key);
	if (own == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (own->mut_borrower != borrower) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTBORROWED;
	}

	own->mut_borrower = nil;
	own->state = BORROW_EXCLUSIVE;
	borrowpool.nmut--;

	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/* Query functions */
int
borrow_is_owned(uintptr key)
{
	struct BorrowOwner *owner;
	int owned;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	owned = (owner != nil && owner->state != BORROW_FREE);
	iunlock(&borrowpool.lock);

	return owned;
}

Proc*
borrow_get_owner(uintptr key)
{
	struct BorrowOwner *owner;
	Proc *p;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	p = (owner != nil) ? owner->owner : nil;
	iunlock(&borrowpool.lock);

	return p;
}

int
borrow_get_owner_snapshot(uintptr key, struct BorrowOwner *out)
{
	struct BorrowOwner *owner;
	int ok = 0;

	if(out == nil)
		return 0;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	if(owner != nil){
		*out = *owner;
		out->next = nil;
		ok = 1;
	}
	iunlock(&borrowpool.lock);

	return ok;
}

enum BorrowState
borrow_get_state(uintptr key)
{
	struct BorrowOwner *owner;
	enum BorrowState state;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	state = (owner != nil) ? owner->state : BORROW_FREE;
	iunlock(&borrowpool.lock);

	return state;
}

int
borrow_can_borrow_shared(uintptr key)
{
	struct BorrowOwner *owner;
	int can;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	can = (owner != nil && owner->state != BORROW_FREE && owner->mut_borrower == nil);
	iunlock(&borrowpool.lock);

	return can;
}

int
borrow_can_borrow_mut(uintptr key)
{
	struct BorrowOwner *owner;
	int can;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	can = (owner != nil && owner->state != BORROW_FREE &&
	       owner->shared_count == 0 && owner->mut_borrower == nil);
	iunlock(&borrowpool.lock);

	return can;
}

/**
 * Release and remove all borrow/bookkeeping state associated with a process.
 *
 * Scans the global borrow pool and: force-releases resources owned by the process, clears
 * shared and mutable borrows held by the process, removes the process from other owners'
 * shared lists, and removes owner entries that become unused.
 *
 * Memory allocated for SharedBorrower and BorrowOwner nodes is freed when the global
 * xinit_done flag indicates full initialization; otherwise nodes are only detached but not freed.
 *
 * @param p Pointer to the process whose borrow-related state should be cleaned; no action is taken if `p` is `nil`.
 */
void
borrow_cleanup_process(Proc *p)
{
	ulong i;
	struct BorrowOwner *owner, *prev, *next;
	struct SharedBorrower *sb, *sb_next;
	int cleaned = 0;

	if (p == nil) {
		return;
	}

	ilock(&borrowpool.lock);

	for (i = 0; i < borrowpool.nbuckets; i++) {
		prev = nil;
		owner = borrowpool.owners[i].head;
		while (owner != nil) {
			next = owner->next;

			if (owner->owner == p) {
				/* Force release - free all shared borrowers */
				for (sb = owner->shared_list; sb != nil; sb = sb_next) {
					sb_next = sb->next;
					if (xinit_done) {
						xfree(sb);
					}
				}
				owner->shared_list = nil;
				owner->owner = nil;
				owner->state = BORROW_FREE;
				owner->shared_count = 0;
				owner->mut_borrower = nil;
				borrowpool.nowners--;
				cleaned++;
			}

			if (owner->mut_borrower == p) {
				owner->mut_borrower = nil;
				if (owner->state == BORROW_MUT_LENT) {
					owner->state = BORROW_EXCLUSIVE;
				}
				borrowpool.nmut--;
				cleaned++;
			}

			/* Remove this process from shared borrower list if present */
			struct SharedBorrower *sb_prev = nil;
			for (sb = owner->shared_list; sb != nil; sb = sb_next) {
				sb_next = sb->next;
				if (sb->proc == p) {
					/* Remove from list */
					if (sb_prev == nil) {
						owner->shared_list = sb_next;
					} else {
						sb_prev->next = sb_next;
					}
					if (sb->alloc_source == ALLOC_XALLOC) {
						xfree(sb);
					}
					owner->shared_count--;
					if (owner->shared_count == 0 && owner->state == BORROW_SHARED_OWNED) {
						owner->state = BORROW_EXCLUSIVE;
						borrowpool.nshared--;
					}
					cleaned++;
				} else {
					sb_prev = sb;
				}
			}

			/* Remove from list if freed and no borrows */
			if (owner->state == BORROW_FREE && owner->shared_count == 0 &&
			    owner->shared_list == nil && owner->mut_borrower == nil) {
				if (prev == nil) {
					borrowpool.owners[i].head = next;
				} else {
					prev->next = next;
				}
				/* Don't free bootstrap-allocated memory, only free xalloc'd memory */
				if (owner->alloc_source == ALLOC_XALLOC) {
					xfree(owner);
				}
			} else {
				prev = owner;
			}

			owner = next;
		}
	}

	iunlock(&borrowpool.lock);

	if (cleaned > 0) {
		print("borrow: cleaned %d resources for pid %d\n", cleaned, p->pid);
	}
}

/* Statistics */
void
borrow_stats(void)
{
	ilock(&borrowpool.lock);
	print("Borrow Checker Statistics:\n");
	print("  Total owners:  %%lud\n", borrowpool.nowners);
	print("  Shared borrows: %%lud\n", borrowpool.nshared);
	print("  Mut borrows:   %%lud\n", borrowpool.nmut);
	iunlock(&borrowpool.lock);
}

/* Dump resource info */
void
borrow_dump_resource(uintptr key)
{
	struct BorrowOwner *owner;
	char *statestr[] = {
		"FREE",
		"EXCLUSIVE",
		"SHARED_OWNED",
		"MUT_LENT",
	};

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	if (owner == nil) {
		print("Resource %%#p not found\n", key);
		iunlock(&borrowpool.lock);
		return;
	}

	print("Resource %%#p:\n", key);
	print("  State:          %s\n", statestr[owner->state]);
	print("  Owner:          %s (pid %d)\n",
		owner->owner ? owner->owner->text : "none",
		owner->owner ? owner->owner->pid : -1);
	print("  Shared borrows: %d\n", owner->shared_count);
	print("  Mut borrower:   %s (pid %d)\n",
		owner->mut_borrower ? owner->mut_borrower->text : "none",
		owner->mut_borrower ? owner->mut_borrower->pid : -1);
	print("  Total borrows:  %%lud\n", owner->borrow_count);

	iunlock(&borrowpool.lock);
}

/* ========== System-Level Ownership Functions ========== */

/**
 * Acquire system-level exclusive ownership of the resource identified by `key`.
 *
 * Marks the resource as owned by `owner` and sets its state to exclusive.
 * If ownership is recorded after system initialization completes, records the acquisition timestamp; before initialization the timestamp is set to zero.
 *
 * @param key Identifier for the resource (e.g., physical page frame or resource key).
 * @param owner System owner to assign to the resource.
 * @returns BORROW_OK on success;
 *          BORROW_ENOMEM if an owner entry could not be allocated;
 *          BORROW_EALREADY if the resource is not free.
 */
enum BorrowError
borrow_acquire_system(uintptr key, enum BorrowSystemOwner owner)
{
	struct BorrowOwner *own;

	ilock(&borrowpool.lock);
	own = find_owner(key);
	if (own == nil) {
		own = create_owner(key);
		if (own == nil) {
			iunlock(&borrowpool.lock);
			return BORROW_ENOMEM;
		}
	}

	if (own->state != BORROW_FREE) {
		iunlock(&borrowpool.lock);
		return BORROW_EALREADY;
	}

	own->system_owner = owner;
	own->is_system_owned = 1;
	own->state = BORROW_EXCLUSIVE;
	/* Skip timestamp during early boot (before xinit) to avoid timer init */
	if (xinit_done) {
		own->acquired_ns = todget(nil, nil);
	} else {
		own->acquired_ns = 0;
	}
	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/**
 * Release system-level ownership for the resource identified by `key`.
 *
 * Clears the resource's system ownership and marks it free; if removal conditions are met
 * the owner entry is also removed from the pool. Fails if the resource does not exist,
 * is not owned by `owner`, or has active shared or mutable borrows.
 *
 * @param key Resource identifier (key) whose system ownership should be released.
 * @param owner System owner expected to currently hold ownership for the resource.
 * @returns BORROW_OK on success.
 * @returns BORROW_ENOTFOUND if no owner entry exists for `key`.
 * @returns BORROW_ENOTOWNER if `key` is not owned by the specified `owner`.
 * @returns BORROW_EBORROWED if the resource has active shared or mutable borrows.
 */
enum BorrowError
borrow_release_system(uintptr key, enum BorrowSystemOwner owner)
{
	struct BorrowOwner *own, *prev;
	ulong hash;

	ilock(&borrowpool.lock);
	own = find_owner(key);
	if (own == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (!own->is_system_owned || own->system_owner != owner) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTOWNER;
	}

	if (own->shared_count > 0 || own->shared_list != nil || own->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EBORROWED;
	}

	own->is_system_owned = 0;
	own->state = BORROW_FREE;
	borrowpool.nowners--;

	/* Remove and free the owner entry from hash table */
	hash = borrow_hash(key);
	prev = nil;
	for (own = borrowpool.owners[hash].head; own != nil; own = own->next) {
		if (own->key == key) {
			if (prev == nil) {
				borrowpool.owners[hash].head = own->next;
			} else {
				prev->next = own->next;
			}
			if (own->alloc_source == ALLOC_XALLOC) {
				xfree(own);
			}
			break;
		}
		prev = own;
	}

	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/**
 * Transfer system-level ownership of a resource identified by `key` from one system owner to another.
 *
 * Updates the resource's system owner and records a new acquisition timestamp when the system is initialized;
 * if the system is not yet initialized (`xinit_done` is false) the timestamp is set to 0.
 *
 * @param from The current system owner expected to hold ownership of the resource.
 * @param to The system owner to transfer ownership to.
 * @param key The resource identifier whose system ownership will be transferred.
 * @returns BORROW_OK on success.
 * @returns BORROW_ENOTFOUND if the resource is not tracked.
 * @returns BORROW_ENOTOWNER if the resource is not system-owned by `from`.
 * @returns BORROW_EBORROWED if the resource has active shared or mutable borrows and cannot be transferred.
 */
enum BorrowError
borrow_transfer_system(enum BorrowSystemOwner from, enum BorrowSystemOwner to, uintptr key)
{
	struct BorrowOwner *owner;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	if (owner == nil) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTFOUND;
	}

	if (!owner->is_system_owned || owner->system_owner != from) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTOWNER;
	}

	if (owner->shared_count > 0 || owner->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EBORROWED;
	}

	owner->system_owner = to;
	/* Skip timestamp during early boot (before xinit) to avoid timer init */
	if (xinit_done) {
		owner->acquired_ns = todget(nil, nil);
	} else {
		owner->acquired_ns = 0;
	}
	iunlock(&borrowpool.lock);
	return BORROW_OK;
}

/* Get system owner of a resource */
enum BorrowSystemOwner
borrow_get_system_owner(uintptr key)
{
	struct BorrowOwner *owner;
	enum BorrowSystemOwner sys_owner;

	ilock(&borrowpool.lock);
	owner = find_owner(key);
	if (owner == nil || !owner->is_system_owned) {
		sys_owner = OWNER_BOOTLOADER;  /* Default */
	} else {
		sys_owner = owner->system_owner;
	}
	iunlock(&borrowpool.lock);

	return sys_owner;
}

/* Check if resource is owned by a specific system */
int
borrow_is_owned_by_system(uintptr key, enum BorrowSystemOwner owner)
{
	struct BorrowOwner *own;
	int owned;

	ilock(&borrowpool.lock);
	own = find_owner(key);
	owned = (own != nil && own->is_system_owned && own->system_owner == owner);
	iunlock(&borrowpool.lock);

	return owned;
}

/* ========== Range-Based Memory Tracking ========== */

/* Global range list for memory tracking */
static struct MemoryRange *range_list = nil;
static Lock range_lock;

/**
 * Initialize the memory range tracking subsystem.
 *
 * Prepares internal state used to record and manage ownership of memory ranges
 * during bootstrap and runtime.
 */
void
memory_range_init(void)
{
	print("memory_range_init: initialized\n");
}

/**
 * Add a physical memory range to the tracked memory ranges and associate it with a system owner.
 *
 * The range is identified by the provided start and end physical addresses and will be recorded
 * in the global memory range list. Allocation for the internal tracking entry is performed
 * automatically: before xinit completes a bootstrap allocation is used, and after xinit the
 * entry is allocated with the regular allocator.
 *
 * @param start Start physical address of the range.
 * @param end End physical address of the range.
 * @param owner System owner to associate with this memory range.
 */
void
memory_range_add(uintptr start, uintptr end, enum BorrowSystemOwner owner)
{
	/* Always use dynamic allocation */
	memory_range_add_discovered(start, end, owner);
}

/**
 * Add a memory ownership range to the global list, allocating the tracking entry.
 *
 * This creates a MemoryRange for [start, end) owned by `owner` and prepends it
 * to the global range_list. Allocation is performed with xalloc when the system
 * has completed initialization (xinit_done true) or with bootstrap_alloc
 * during early bootstrap. If allocation fails the function logs an error and
 * returns without modifying the list.
 *
 * @param start Start physical address of the range (inclusive).
 * @param end End physical address of the range (exclusive).
 * @param owner The system owner to associate with the range.
 */
void
memory_range_add_discovered(uintptr start, uintptr end, enum BorrowSystemOwner owner)
{
	struct MemoryRange *range;

	/* Dynamically allocate range entry */
	if (xinit_done) {
		range = xalloc(sizeof(struct MemoryRange));
	} else {
		range = bootstrap_alloc(sizeof(struct MemoryRange));
	}
	if (range == nil) {
		print("memory_range_add_discovered: allocation failed for range [%#p-%#p]\n",
		      start, end);
		return;
	}

	ilock(&range_lock);

	range->start = start;
	range->end = end;
	range->owner = owner;

	/* Add to front of list */
	range->next = range_list;
	range_list = range;

	iunlock(&range_lock);

	print("memory_range_add_discovered: [%#p-%#p] owner=%d (dynamic)\n", start, end, owner);
}

/**
 * Remove a tracked memory range that exactly matches the provided bounds.
 *
 * Searches the registered memory ranges for an entry whose start and end match
 * the given values; if found, removes it from the global list. If the entry
 * was allocated after system initialization, its storage is freed. If no
 * matching entry exists the function returns without modifying the list.
 *
 * @param start Starting address of the range to remove (same value used when the range was added).
 * @param end Ending address of the range to remove (same value used when the range was added).
 */
void
memory_range_remove(uintptr start, uintptr end)
{
	struct MemoryRange *range, *prev;

	ilock(&range_lock);

	prev = nil;
	for (range = range_list; range != nil; range = range->next) {
		if (range->start == start && range->end == end) {
			/* Found it - remove from list */
			if (prev == nil) {
				range_list = range->next;
			} else {
				prev->next = range->next;
			}

			/* Free if dynamically allocated (after xinit) */
			if (xinit_done) {
				xfree(range);
			}

			iunlock(&range_lock);
			print("memory_range_remove: [%#p-%#p] removed\n", start, end);
			return;
		}
		prev = range;
	}

	iunlock(&range_lock);
	print("memory_range_remove: [%#p-%#p] not found\n", start, end);
}

/**
 * Print a debug listing of all tracked memory ranges and their owners.
 *
 * Iterates the global memory range list and prints each range's start and end
 * addresses, owner (BOOTLOADER or KERNEL), and size, followed by a total count
 * and the current allocation mode indicator.
 */
void
memory_range_dump(void)
{
	struct MemoryRange *range;
	int count = 0;

	ilock(&range_lock);

	print("=== Memory Range Tracking ===\n");
	for (range = range_list; range != nil; range = range->next) {
		print("  [%#p-%#p] owner=%s size=%#p\n",
		      range->start, range->end,
		      range->owner == OWNER_BOOTLOADER ? "BOOTLOADER" : "KERNEL",
		      range->end - range->start);
		count++;
	}
	print("Total: %d ranges\n", count);
	print("Mode: BOOTSTRAP ALLOCATION (early boot)\n");

	iunlock(&range_lock);
}

/**
 * Estimate how many additional memory ranges can be added to the tracking list.
 *
 * Before the allocator initialization completes (xinit not done), this returns a
 * conservative capacity based on the bootstrap allocator. After initialization it
 * returns a very large value representing essentially unlimited capacity.
 *
 * @returns An approximate number of additional ranges that can be added:
 *          1000 before xinit is complete, 999999 after xinit.
 */
int
memory_range_capacity(void)
{
	/* During early boot, limited by bootstrap pool, after xinit unlimited */
	if (!xinit_done) {
		/* Bootstrap allocation is limited */
		return 1000;  /* Conservative estimate */
	} else {
		/* After xinit, limited only by available memory */
		return 999999;  /* Effectively unlimited */
	}
}

/* Get owner of an address */
enum BorrowSystemOwner
memory_range_get_owner(uintptr addr)
{
	struct MemoryRange *range;
	enum BorrowSystemOwner owner;

	ilock(&range_lock);

	/* Search for containing range */
	for (range = range_list; range != nil; range = range->next) {
		if (addr >= range->start && addr < range->end) {
			owner = range->owner;
			iunlock(&range_lock);
			return owner;
		}
	}

	iunlock(&range_lock);

	/* Not in any tracked range - assume bootloader for now */
	return OWNER_BOOTLOADER;
}

/* Check if requester can access an address */
int
memory_range_check_access(uintptr addr, enum BorrowSystemOwner requester)
{
	enum BorrowSystemOwner owner = memory_range_get_owner(addr);

	/* Owner can always access */
	if (owner == requester)
		return 1;

	/* For now, deny cross-owner access */
	return 0;
}

/* ========== Per-Page Tracking (Runtime) ========== */

/**
 * Acquire system ownership for a physical address range, performed page-by-page.
 *
 * Attempts to acquire ownership for each 4KB page in the half-open range
 * [start_pa, start_pa + size). This function must be called only after full
 * runtime initialization (xinit); it returns an error and performs no changes
 * if called during early boot. On failure for any page, previously acquired
 * pages in the range are released (rollback).
 *
 * @param start_pa Starting physical address of the range (bytes).
 * @param size Size of the range in bytes; the range is processed in 4KB pages.
 * @param owner System owner to assign to each page.
 * @returns BORROW_OK on success.
 *          BORROW_EINVAL if called before runtime initialization.
 *          BORROW_EALREADY if a page was already owned (treated as success for that page).
 *          Other BorrowError values if acquisition fails for a page; in that case
 *          previously acquired pages are released and the error is returned.
 */
enum BorrowError
borrow_acquire_range_phys(uintptr start_pa, usize size, enum BorrowSystemOwner owner)
{
	uintptr pa;
	enum BorrowError err;

	/* IMPORTANT: Only use this after xinit() for runtime tracking!
	 * For boot coordination, use memory_range_add() instead */
	if (!xinit_done) {
		print("borrow_acquire_range_phys: ERROR - called during early boot\n");
		print("  Use memory_range_add() for boot coordination instead\n");
		return BORROW_EINVAL;
	}

	/* Acquire ownership for each 4KB page in the range */
	for (pa = start_pa; pa < start_pa + size; pa += 0x1000) {
		err = borrow_acquire_system(pa, owner);
		if (err != BORROW_OK && err != BORROW_EALREADY) {
			/* Rollback on error */
			while (pa > start_pa) {
				pa -= 0x1000;
				borrow_release_system(pa, owner);
			}
			return err;
		}
	}

	return BORROW_OK;
}

/**
 * Determine whether every page in a physical address range is owned by a given system owner.
 *
 * Checks ownership per 4KB page across the range [start_pa, start_pa + size). Before kernel
 * initialization (when xinit_done is false) the function consults the bootstrap memory range
 * tracking; after initialization it checks runtime per-page system ownership.
 *
 * @param start_pa Starting physical address of the range (inclusive).
 * @param size Size of the range in bytes; the check is performed per 4KB page over the range.
 * @param owner System owner to verify for each page in the range.
 * @returns `1` if every page in the specified range is owned by `owner`, `0` otherwise.
 */
int
borrow_range_owned_by_system(uintptr start_pa, usize size, enum BorrowSystemOwner owner)
{
	uintptr pa;

	/* During early boot, check range tracking instead */
	if (!xinit_done) {
		/* Check if entire range has same owner */
		for (pa = start_pa; pa < start_pa + size; pa += 0x1000) {
			if (memory_range_get_owner(pa) != owner)
				return 0;
		}
		return 1;
	}

	/* Runtime: check page-by-page */
	for (pa = start_pa; pa < start_pa + size; pa += 0x1000) {
		if (!borrow_is_owned_by_system(pa, owner))
			return 0;
	}

	return 1;
}

/**
 * Determine whether a system owner may access an entire physical address range.
 *
 * Checks access at 4KB page granularity. Before full kernel initialization (when
 * xinit_done is false) the function consults the boot-time memory range tracker;
 * after initialization it verifies per-page system ownership.
 *
 * @param start_pa Starting physical address of the range.
 * @param size Size of the range in bytes.
 * @param requester System owner requesting access.
 * @returns `1` if `requester` is allowed to access every 4KB page in the range,
 *          `0` otherwise.
 */
int
borrow_can_access_range_phys(uintptr start_pa, usize size, enum BorrowSystemOwner requester)
{
	uintptr pa;

	/* During early boot, check range tracking */
	if (!xinit_done) {
		for (pa = start_pa; pa < start_pa + size; pa += 0x1000) {
			if (!memory_range_check_access(pa, requester))
				return 0;
		}
		return 1;
	}

	/* Runtime: check page-by-page ownership */
	for (pa = start_pa; pa < start_pa + size; pa += 0x1000) {
		enum BorrowSystemOwner owner = borrow_get_system_owner(pa);
		if (owner != requester)
			return 0;
	}

	return 1;
}

/* ========== Memory Coordination Functions ========== */

/* Global memory coordination state */
struct MemoryCoordination mem_coord;

/* Initialize memory coordination system */
void
boot_memory_coordination_init(void)
{
	mem_coord.state = MEMORY_BOOTLOADER;
	mem_coord.current_owner = OWNER_BOOTLOADER;
	mem_coord.coordination_enabled = 1;

	/* Initialize range-based tracking */
	memory_range_init();

	print("boot_memory_coordination_init: initialized (state=BOOTLOADER)\n");
}

/* Transfer bootloader memory to kernel - simple state transition */
void
transfer_bootloader_to_kernel(void)
{
	/* Simple state transition - no per-page tracking needed */
	mem_coord.state = MEMORY_KERNEL_ACTIVE;
	mem_coord.current_owner = OWNER_KERNEL;

	print("transfer_bootloader_to_kernel: ownership transferred (BOOTLOADER → KERNEL)\n");
}

/* Establish memory ownership zones using range-based tracking (STATIC) */
void
establish_memory_ownership_zones(void)
{
	/* Define memory regions using efficient range tracking
	 * Each range covers potentially many pages with single entry
	 *
	 * NOTE: These are HARDCODED for early boot. Use
	 * establish_memory_ownership_zones_dynamic() after xinit()
	 * to discover actual kernel layout dynamically.
	 */

	/* Kernel code region: 0x200000 - 0x400000 (2MB) */
	memory_range_add(0x200000, 0x400000, OWNER_KERNEL);

	/* Kernel data region: 0x400000 - 0x600000 (2MB) */
	memory_range_add(0x400000, 0x600000, OWNER_KERNEL);

	/* CR3 continuation region: 0x600000 - 0x700000 (1MB) */
	memory_range_add(0x600000, 0x700000, OWNER_KERNEL);

	/* Page tables region: 0x210000 - 0x220000 (64KB) */
	memory_range_add(0x210000, 0x220000, OWNER_KERNEL);

	mem_coord.state = MEMORY_COORDINATED;

	print("establish_memory_ownership_zones: zones established (static, %d ranges)\n",
	      4);
}

/**
 * Discover runtime memory regions and register kernel-owned ranges with the memory tracker.
 *
 * Scans the global configuration's memory table and registers regions that belong to the kernel
 * with the memory-range tracking subsystem; prints discovered regions and then dumps the current
 * range table.
 *
 * Note: If invoked before xinit has completed, the function logs an error and returns without
 * registering any ranges.
 */
void
establish_memory_ownership_zones_dynamic(void)
{
	extern Conf conf;
	int i;
	uintptr start, region_end;

	if (!xinit_done) {
		print("establish_memory_ownership_zones_dynamic: ERROR - call after xinit()\n");
		return;
	}

	print("establish_memory_ownership_zones_dynamic: discovering memory from conf.mem[]\n");

	/* Discover kernel memory regions from conf.mem[] */
	for (i = 0; i < nelem(conf.mem); i++) {
		if (conf.mem[i].npage == 0)
			continue;

		start = conf.mem[i].base;
		region_end = start + (conf.mem[i].npage * BY2PG);

		/* Determine ownership based on region type */
		if (start >= conf.mem[i].kbase && start < conf.mem[i].klimit) {
			/* Kernel memory */
			memory_range_add_discovered(start, region_end, OWNER_KERNEL);
			print("  Kernel region [%#p-%#p] size=%#p\n",
			      start, region_end, region_end - start);
		} else {
			/* User/free memory - not tracked initially */
			print("  User/free region [%#p-%#p] size=%#p (not tracked)\n",
			      start, region_end, region_end - start);
		}
	}

	print("establish_memory_ownership_zones_dynamic: discovery complete\n");
	memory_range_dump();
}

/* Validate memory coordination ready - check zones are established */
int
validate_memory_coordination_ready(void)
{
	/* Check coordination enabled */
	if (!mem_coord.coordination_enabled) {
		print("validate_memory_coordination_ready: coordination disabled\n");
		return 0;
	}

	/* Check we're in coordinated state */
	if (mem_coord.state != MEMORY_COORDINATED && mem_coord.state != MEMORY_KERNEL_ACTIVE) {
		print("validate_memory_coordination_ready: wrong state %d\n", mem_coord.state);
		return 0;
	}

	/* Check kernel owns critical region (sample check) */
	if (memory_range_get_owner(0x200000) != OWNER_KERNEL) {
		print("validate_memory_coordination_ready: kernel doesn't own 0x200000\n");
		return 0;
	}

	print("validate_memory_coordination_ready: ready for CR3 switch\n");
	return 1;
}

/* Check if memory system is ready before CR3 switch */
int
memory_system_ready_before_cr3(void)
{
	/* Check if HHDM offset is known */
	if(saved_limine_hhdm_offset == 0)
		return 0;

	/* Check if memory coordination is initialized */
	if(mem_coord.state != MEMORY_KERNEL_ACTIVE)
		return 0;

	/* Check borrow pool accessibility */
	if(borrowpool.owners == nil || borrowpool.nbuckets == 0)
		return 0;

	return 1;
}

/* Validate that memory system is operational after CR3 switch */
int
post_cr3_memory_system_operational(void)
{
	/* After CR3 switch, verify we can still access our data structures */
	if (borrowpool.owners == nil || borrowpool.nbuckets == 0) {
		print("post_cr3_memory_system_operational: borrow pool inaccessible\n");
		return 0;
	}

	/* Verify coordination state is accessible */
	if (mem_coord.state != MEMORY_KERNEL_ACTIVE) {
		print("post_cr3_memory_system_operational: unexpected state %d\n", mem_coord.state);
		return 0;
	}

	print("post_cr3_memory_system_operational: memory system operational\n");
	return 1;
}
