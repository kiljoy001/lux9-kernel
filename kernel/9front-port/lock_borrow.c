#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "lock_borrow.h"

/* Deadlock detection logic */
static void
borrow_check_deadlock(Proc *p, BorrowLock *l)
{
	Proc *owner;
	uintptr wait_key;
	int i;

	if (p == nil || l == nil) {
		return;
	}

	wait_key = l->key;

	for (i = 0; i < 100; i++) { /* 100 is a safe limit to prevent infinite loops */
		owner = borrow_get_owner(wait_key);
		if (owner == nil) {
			/* No owner, no deadlock */
			return;
		}

		if (owner == p) {
			/* Deadlock detected! */
			panic("deadlock detected: proc %lud is waiting for lock %#p held by itself", p->pid, (void*)wait_key);
		}

		wait_key = owner->waiting_for_key;
		if (wait_key == 0) {
			/* Owner is not waiting for anything, no deadlock */
			return;
		}
	}

	/* Exceeded loop limit, likely a very long chain or a bug */
	panic("deadlock check: exceeded loop limit");
}

/* Initialize a BorrowLock */
void
borrow_lock_init(BorrowLock *bl, uintptr key)
{
	memset(bl, 0, sizeof(*bl));
	bl->key = key;
}

/* Acquire a lock with deadlock detection */
void
borrow_lock(BorrowLock *bl)
{
	enum BorrowError err;

	if(up == nil){
		lock(&bl->lock);
		return;
	}

	up->waiting_for_key = bl->key;

	/* Check for deadlock before attempting to lock */
	borrow_check_deadlock(up, bl);

	/* Acquire the actual lock */
	lock(&bl->lock);

	/* Lock acquired, clear waiting key and register ownership */
	up->waiting_for_key = 0;
	err = borrow_acquire(up, bl->key);
	switch(err){
		case BORROW_OK:
			return;
		case BORROW_EALREADY:
			/* Recursive acquisition is not supported */
			unlock(&bl->lock);
			panic("borrow_lock: recursive acquire on key %#p by pid %lud", (void*)bl->key, up->pid);
		default:
			unlock(&bl->lock);
			panic("borrow_lock: borrow_acquire failed (err=%d key=%#p pid=%lud)",
				err, (void*)bl->key, up->pid);
		}
}

/* Release a lock */
void
borrow_unlock(BorrowLock *bl)
{
	if(up != nil) {
		enum BorrowError err = borrow_release(up, bl->key);
		if(err != BORROW_OK) {
			unlock(&bl->lock);
			panic("borrow_unlock: borrow_release failed (err=%d key=%#p pid=%lud)",
				err, (void*)bl->key, up->pid);
		}
	}
	unlock(&bl->lock);
}
