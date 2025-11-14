#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "lock_dag.h"

static Lock lockdaglock;
static int lockdag_initialized;
static int lockdag_next_id;
static LockDagNode *lockdag_nodes[LOCKDAG_MAX_NODES];
enum { EDGE_WORDS = (LOCKDAG_MAX_NODES + 63) / 64 };
static uvlong lockdag_edges[LOCKDAG_MAX_NODES][EDGE_WORDS];

static int
edge_allowed(int from, int to)
{
	if(from < 0 || to < 0)
		return 1;	/* Treat unknown nodes as always allowed */
	return (lockdag_edges[from][to / 64] >> (to % 64)) & 1;
}

static void
edge_set(int from, int to)
{
	if(from < 0 || to < 0)
		return;
	lockdag_edges[from][to / 64] |= 1ull << (to % 64);
}

static void
lockdag_push(Proc *p, LockDagNode *node, uintptr key)
{
	struct LockDagContext *ctx;

	if(p == nil || node == nil)
		return;

	ctx = &p->lockdag;
	if(ctx->depth >= LOCKDAG_STACK_DEPTH){
		ctx->overflow++;
		return;
	}
	ctx->stack[ctx->depth].node = node;
	ctx->stack[ctx->depth].key = key;
	ctx->depth++;
}

static void
lockdag_pop(Proc *p, LockDagNode *node, uintptr key)
{
	struct LockDagContext *ctx;

	if(p == nil || node == nil)
		return;

	ctx = &p->lockdag;
	for(int i = ctx->depth - 1; i >= 0; i--){
		if(ctx->stack[i].node == node && ctx->stack[i].key == key){
			ctx->depth = i;
			return;
		}
	}
}

void
lockdag_init(void)
{
	if(lockdag_initialized)
		return;
	lockdag_next_id = 0;
	memset(lockdag_nodes, 0, sizeof(lockdag_nodes));
	memset(lockdag_edges, 0, sizeof(lockdag_edges));
	lockdag_initialized = 1;
}

int
lockdag_register_node(LockDagNode *node)
{
	int id;

	if(node == nil || node->name == nil)
		return -1;

	lock(&lockdaglock);
	if(!lockdag_initialized)
		lockdag_init();
	if(node->id >= 0){
		unlock(&lockdaglock);
		return node->id;
	}
	if(lockdag_next_id >= LOCKDAG_MAX_NODES){
		unlock(&lockdaglock);
		print("lockdag: cannot register node %s, limit reached\n", node->name);
		return -1;
	}
	id = lockdag_next_id++;
	node->id = id;
	lockdag_nodes[id] = node;
	unlock(&lockdaglock);
	print("lockdag: registered node %s as %d\n", node->name, id);
	return id;
}

int
lockdag_allow_edge(LockDagNode *from, LockDagNode *to)
{
	if(from == nil || to == nil)
		return -1;
	if(from->id < 0 || to->id < 0)
		return -1;
	lock(&lockdaglock);
	edge_set(from->id, to->id);
	unlock(&lockdaglock);
	return 0;
}

void
lockdag_record_acquire(Proc *p, LockDagNode *node, uintptr key)
{
	struct LockDagContext *ctx;
	LockDagNode *prev;

	if(node == nil || p == nil)
		return;
	if(node->id < 0)
		lockdag_register_node(node);

	ctx = &p->lockdag;
	prev = (ctx->depth > 0) ? ctx->stack[ctx->depth - 1].node : nil;
	if(prev != nil && !edge_allowed(prev->id, node->id)){
		print("lockdag: suspicious edge %s -> %s (proc %lud key=%#p)\n",
			prev->name, node->name, (ulong)p->pid, (void*)key);
	}
	lockdag_push(p, node, key);
}

void
lockdag_record_release(Proc *p, LockDagNode *node, uintptr key)
{
	if(node == nil || p == nil)
		return;
	lockdag_pop(p, node, key);
}
