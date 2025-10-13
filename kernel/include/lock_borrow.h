
/*
 * Lock with integrated borrow checker for deadlock detection
 */

#pragma once

#include "dat.h"
#include "borrowchecker.h"

/* A wrapper around the basic Lock to add borrow checking */
typedef struct BorrowLock {
	Lock;
	uintptr	key;
} BorrowLock;

void borrow_lock_init(BorrowLock *bl, uintptr key);
void borrow_lock(BorrowLock *bl);
void borrow_unlock(BorrowLock *bl);
