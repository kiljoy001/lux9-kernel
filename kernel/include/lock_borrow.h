
/*
 * Lock with integrated borrow checker for deadlock detection
 */

#pragma once

#include "dat.h"
#include "borrowchecker.h"
#include "lock_dag.h"

/* A wrapper around the basic Lock to add borrow checking */
typedef struct BorrowLock {
	Lock lock;
	uintptr	key;
	LockDagNode *dag_node;
} BorrowLock;

void borrow_lock_init(BorrowLock *bl, uintptr key, LockDagNode *node);
void borrow_lock(BorrowLock *bl);
void borrow_unlock(BorrowLock *bl);
