/*
 * Kernel lock DAG metadata and tracing helpers.
 *
 * Each lock can optionally be associated with a LockDagNode. When code
 * acquires/releases that lock, the DAG helpers record the sequence so the
 * kernel can reason about ordering, emit diagnostics, and detect suspicious
 * edges.
 */

#pragma once

struct Proc;

enum {
	LOCKDAG_MAX_NODES	= 128,
	LOCKDAG_STACK_DEPTH	= 32,
};

typedef struct LockDagNode LockDagNode;

struct LockDagNode {
	const char *name;
	int id;		/* Assigned by lockdag_register_node(), -1 if unregistered */
};

#define LOCKDAG_NODE(label) { .name = (label), .id = -1 }

struct LockDagEntry {
	LockDagNode *node;
	uintptr key;
};

struct LockDagContext {
	struct LockDagEntry stack[LOCKDAG_STACK_DEPTH];
	int depth;
	int overflow;
};

void	lockdag_init(void);
int	lockdag_register_node(LockDagNode *node);
int	lockdag_allow_edge(LockDagNode *from, LockDagNode *to);
void	lockdag_record_acquire(Proc *p, LockDagNode *node, uintptr key);
void	lockdag_record_release(Proc *p, LockDagNode *node, uintptr key);
