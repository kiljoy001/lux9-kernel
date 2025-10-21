/*
 * Unified borrow checker for kernel primitives
 * Provides Rust-style ownership and borrowing for locks, memory, I/O, etc.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "borrowchecker.h"

/* Global borrow pool */
struct BorrowPool borrowpool;

static void
borrowdebugchar(char c)
{
	__asm__ volatile("outb %0, %1" : : "a"(c), "Nd"((ushort)0x3F8));
}

/* Initialize the borrow pool */
void
borrowinit(void)
{
	ulong i;

	borrowdebugchar('b');
	borrowdebugchar('0');

	/* Initialize hash table */
	borrowpool.nbuckets = 1024; /* Arbitrary size, can be tuned */
	borrowdebugchar('b');
	borrowdebugchar('1');
	borrowpool.owners = xalloc(borrowpool.nbuckets * sizeof(struct BorrowBucket));
	if (borrowpool.owners == nil) {
		borrowdebugchar('b');
		borrowdebugchar('x');
		panic("borrowinit: failed to allocate hash table");
	}

	borrowdebugchar('b');
	borrowdebugchar('2');
	for (i = 0; i < borrowpool.nbuckets; i++) {
		borrowpool.owners[i].head = nil;
	}

	borrowdebugchar('b');
	borrowdebugchar('3');
	borrowpool.nowners = 0;
	borrowpool.nshared = 0;
	borrowpool.nmut = 0;
	borrowdebugchar('b');
	borrowdebugchar('4');
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

/* Create new BorrowOwner for a key */
static struct BorrowOwner*
create_owner(uintptr key)
{
	ulong hash = borrow_hash(key);
	struct BorrowOwner *owner = xalloc(sizeof(struct BorrowOwner));
	if (owner == nil) {
		return nil;
	}

	owner->key = key;
	owner->owner = nil;
	owner->state = BORROW_FREE;
	owner->shared_count = 0;
	owner->mut_borrower = nil;
	owner->acquired_ns = 0;
	owner->borrow_deadline_ns = 0;
	owner->borrow_count = 0;
	owner->next = borrowpool.owners[hash].head;
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

/* Release ownership of a resource */
enum BorrowError
borrow_release(Proc *p, uintptr key)
{
	struct BorrowOwner *owner;

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

	if (owner->shared_count > 0 || owner->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EBORROWED;
	}

	owner->owner = nil;
	owner->state = BORROW_FREE;
	borrowpool.nowners--;
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

/* Borrow resource as shared */
enum BorrowError
borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key)
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

	if (own->mut_borrower != nil) {
		iunlock(&borrowpool.lock);
		return BORROW_EMUTBORROW;
	}

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

/* Return a shared borrow */
enum BorrowError
borrow_return_shared(Proc *borrower, uintptr key)
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

	if (own->shared_count == 0) {
		iunlock(&borrowpool.lock);
		return BORROW_ENOTBORROWED;
	}

	own->shared_count--;
	if (own->shared_count == 0) {
		own->state = BORROW_EXCLUSIVE;
		borrowpool.nshared--;
	}

	iunlock(&borrowpool.lock);
	return BORROW_OK;
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

/* Cleanup all resources for a process */
void
borrow_cleanup_process(Proc *p)
{
	ulong i;
	struct BorrowOwner *owner, *prev, *next;
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
				/* Force release */
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

			/* Remove from list if freed */
			if (owner->state == BORROW_FREE && owner->shared_count == 0 && owner->mut_borrower == nil) {
				if (prev == nil) {
					borrowpool.owners[i].head = next;
				} else {
					prev->next = next;
				}
				xfree(owner);
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
