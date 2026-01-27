/*
 * pool_freelist.c - Free list splay tree management
 *
 * This module implements a splay tree of free blocks, where:
 * - Each tree node represents a size class
 * - Nodes at the same size form a circular doubly-linked list
 * - The tree is kept balanced via splay operations
 *
 * Operations:
 * - pooladd: Add a free block to the tree
 * - pooldel: Remove a free block from the tree
 * - treelookupgt: Find smallest block >= requested size
 * - treesplay: Splay a node to the root
 * - checklist/checktree: Validation functions
 */

#include <libc.h>
#include <pool.h>
#include <u.h>
#include "pool_types.h"
#include "pool_internal.h"

/*
 * checklist: Validate a circular list of same-sized free blocks
 */
/*@
  @ requires t == \null || \valid(t);
  @ assigns \nothing;
  @*/
void checklist(Free *t) {
	Free *q;

	for (q = t->next; q != t; q = q->next) {
		assert(q->magic == FREE_MAGIC);
		assert(q->size == t->size);
		assert(q->left == Poison);
		assert(q->right == Poison);
		assert(q->next != nil && q->next != Poison && q->next->prev == q);
		assert(q->prev != nil && q->prev != Poison && q->prev->next == q);
	}
}

/*
 * checktree: Validate the entire splay tree recursively
 */
/*@
  @ requires t == \null || \valid(t);
  @ assigns \nothing;
  @*/
void checktree(Free *t, int a, int b) {
	assert(t->magic == FREE_MAGIC);
	assert(a < t->size && t->size < b);
	assert(t->left != Poison);
	assert(t->right != Poison);
	assert(t->next != nil && t->next != Poison && t->next->prev == t);
	assert(t->prev != nil && t->prev != Poison && t->prev->next == t);
	checklist(t);
	if (t->left)
		checktree(t->left, a, t->size);
	if (t->right)
		checktree(t->right, t->size, b);
}

/*
 * treelookupgt: Find smallest node in tree with size >= size
 * Returns nil if no such node exists
 */
Free *treelookupgt(Free *t, ulong size) {
	Free *lastgood; /* last node we saw that was big enough */

	lastgood = nil;
	for (;;) {
		if (t == nil)
			return lastgood;
		assert(t->magic == FREE_MAGIC);
		if (size == t->size)
			return t;
		if (size < t->size) {
			lastgood = t;
			t = t->left;
		} else
			t = t->right;
	}
}

/*
 * treesplay: Splay node of size size to the root and return new root
 * This is the classic top-down splay operation
 */
Free *treesplay(Free *t, ulong size) {
	Free N, *l, *r, *y;

	N.left = N.right = nil;
	l = r = &N;

	for (;;) {
		assert(t->magic == FREE_MAGIC);
		if (size < t->size) {
			y = t->left;
			if (y != nil) {
				assert(y->magic == FREE_MAGIC);
				if (size < y->size) {
					t->left = y->right;
					y->right = t;
					t = y;
				}
			}
			if (t->left == nil)
				break;
			r->left = t;
			r = t;
			t = t->left;
		} else if (size > t->size) {
			y = t->right;
			if (y != nil) {
				assert(y->magic == FREE_MAGIC);
				if (size > y->size) {
					t->right = y->left;
					y->left = t;
					t = y;
				}
			}
			if (t->right == nil)
				break;
			l->right = t;
			l = t;
			t = t->right;
		} else
			break;
	}

	l->right = t->left;
	r->left = t->right;
	t->left = N.right;
	t->right = N.left;

	return t;
}

/*
 * pooladd: Add a block to the free pool
 * The block is converted to a Free node and inserted into the splay tree
 */
Free *pooladd(Pool *p, Alloc *anode) {
	Free *node, *root;

	antagonism {
		memmark(_B2D(anode), 0xF7, anode->size - sizeof(Bhdr) - sizeof(Btail));
	}

	node = (Free *)anode;
	node->magic = FREE_MAGIC;
	node->left = node->right = nil;
	node->next = node->prev = node;

	if (p->freeroot != nil) {
		root = treesplay(p->freeroot, node->size);
		if (root->size > node->size) {
			node->left = root->left;
			node->right = root;
			root->left = nil;
		} else if (root->size < node->size) {
			node->right = root->right;
			node->left = root;
			root->right = nil;
		} else {
			node->left = root->left;
			node->right = root->right;
			root->left = root->right = Poison;

			node->prev = root->prev;
			node->next = root;
			node->prev->next = node;
			node->next->prev = node;
		}
	}
	p->freeroot = node;
	p->curfree += node->size;

	return node;
}

/*
 * pooldel: Remove a node from the free pool
 * Updates the splay tree structure and removes from circular list if needed
 */
Alloc *pooldel(Pool *p, Free *node) {
	Free *root;

	root = treesplay(p->freeroot, node->size);
	if (node == root && node->next == node) {
		if (node->left == nil)
			root = node->right;
		else {
			root = treesplay(node->left, node->size);
			assert(root->right == nil);
			root->right = node->right;
		}
	} else {
		if (node == root) {
			root = node->next;
			root->left = node->left;
			root->right = node->right;
		}
		assert(root->magic == FREE_MAGIC && root->size == node->size);
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}
	p->freeroot = root;
	p->curfree -= node->size;

	node->left = node->right = node->next = node->prev = Poison;

	antagonism {
		memmark(_B2D(node), 0xF9, node->size - sizeof(Bhdr) - sizeof(Btail));
	}

	node->magic = UNALLOC_MAGIC;

	return (Alloc *)node;
}
