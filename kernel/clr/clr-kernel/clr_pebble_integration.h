/*
 * CLR Deep Integration with Pebble Memory System
 *
 * ARCHITECTURAL BREAKTHROUGH:
 * The Pebble game rules ARE the garbage collector.
 * White tokens ARE the reference count - when balance hits 0, free it.
 * Red-Blue shadows ARE speculative execution and rollback.
 * Exchange pages ARE zero-copy message passing.
 *
 * This replaces stop-the-world GC with deterministic capability-based memory.
 * The heap is literally a Pebble game board.
 *
 * INTRUSIVE LINKED LISTS EVERYWHERE - no arrays, no dynamic allocation overhead.
 */

#ifndef CLR_PEBBLE_INTEGRATION_H
#define CLR_PEBBLE_INTEGRATION_H

#include "../../include/u.h"
#include "../../include/pebble.h"
#include "../../include/exchange.h"
#include "../clr-implementation/clr_runtime.h"

/* Forward declarations */
typedef u32int tasklet_id_t;
typedef u32int channel_id_t;

/* ========== White Token Reference List ========== */

/*
 * Instead of PebbleWhite **whites (array), use intrusive linked list.
 * Each white token in a list represents one reference to the object.
 */
typedef struct clr_white_ref {
	PebbleWhite *white;		/* The actual white token */
	struct clr_white_ref *next;
	struct clr_white_ref *prev;
} clr_white_ref_t;

/* ========== CLR Object = Black Pebble ========== */

/*
 * Every CLR object is a Black Pebble with intrusive list of White tokens.
 *
 * Small object optimization:
 * - inline_white: Store first white token inline (common case: 1 ref)
 * - white_list: Linked list for additional refs
 */
typedef struct clr_object {
	UserCapability black_cap;	/* The black pebble capability */
	void *data;			/* Actual memory pointer */

	/* Reference counting via white token list */
	clr_white_ref_t inline_white;	/* First ref stored inline */
	clr_white_ref_t *white_list;	/* Additional refs (intrusive list) */
	ulong white_count;		/* Total active references */

	/* Object metadata */
	clr_value_type_t type;		/* CLR type */
	ulong size;			/* Object size in bytes */

	/* Exchange support for zero-copy IPC */
	ExchangeHandle *exchange_handles;	/* Physical page handles */
	ulong exchange_npages;
	int is_prepared;		/* Prepared for exchange? */

	/* Concurrency */
	Lock lock;			/* Protects white_list modifications */

	/* Intrusive list for heap tracking */
	struct clr_object *next;
	struct clr_object *prev;
} clr_object_t;

/* ========== CLR Heap = Pebble State ========== */

/*
 * The CLR heap IS the pebble state.
 * Objects tracked via intrusive doubly-linked list.
 */
typedef struct clr_heap {
	PebbleState *pebble;		/* The pebble state IS the heap */

	/* Object tracking (intrusive list) */
	clr_object_t *objects_head;	/* Head of object list */
	clr_object_t *objects_tail;	/* Tail for fast append */
	ulong object_count;
	Lock objects_lock;		/* Protects object list */

	/* Exchange message queues (intrusive lists) */
	struct clr_exchange_msg *send_queue_head;
	struct clr_exchange_msg *send_queue_tail;
	struct clr_exchange_msg *recv_queue_head;
	struct clr_exchange_msg *recv_queue_tail;
	Lock send_lock;
	Lock recv_lock;

	/* Statistics */
	ulong total_allocs;
	ulong total_frees;
	ulong total_refs_created;
	ulong total_refs_released;
} clr_heap_t;

/* ========== Reference Counting via White Tokens ========== */

/*
 * Reference semantics:
 * - Create ref: Issue white token, add to white_list, increment white_count
 * - Drop ref: Remove from white_list, invalidate white, decrement white_count
 * - white_count == 0: Free black handle (object unreachable)
 *
 * NO MARK PHASE. NO SWEEP PHASE. Just refcount via whites.
 */

/* Allocate CLR object - creates black + first white token */
clr_object_t* clr_object_alloc(clr_heap_t *heap, ulong size, clr_value_type_t type);

/* Add reference - issues new white token, adds to list (thread-safe) */
PebbleWhite* clr_object_addref(clr_heap_t *heap, clr_object_t *obj);

/* Release reference - removes from list, burns white token, frees if balance=0 (thread-safe) */
int clr_object_release(clr_heap_t *heap, clr_object_t *obj, PebbleWhite *white);

/* Get reference count */
static inline ulong clr_object_refcount(clr_object_t *obj) {
	return obj ? obj->white_count : 0;
}

/* ========== Speculative Execution: Red-Blue Shadows ========== */

/*
 * Transactional memory via Red-Blue:
 * - Blue: Active working copy (mutations happen here)
 * - Red: Safe snapshot before risky operation
 *
 * Use cases:
 * - 9P transactions: Snapshot before handling request, rollback on error
 * - Driver calls: Snapshot before calling untrusted driver, rollback on fault
 * - Speculative optimization: Try optimization, rollback if assumptions violated
 */

/* Snapshot object before speculative execution */
int clr_object_snapshot(clr_heap_t *heap, clr_object_t *obj);

/* Commit speculative changes (discard red, keep blue) */
int clr_object_commit(clr_heap_t *heap, clr_object_t *obj);

/* Rollback to safe snapshot (discard blue, restore from red) */
int clr_object_rollback(clr_heap_t *heap, clr_object_t *obj);

/* Check if object has snapshot */
static inline int clr_object_has_snapshot(clr_object_t *obj) {
	USED(obj);
	/* TODO: Implement once Red-Blue API is available */
	return 0;
}

/* ========== Zero-Copy Message Passing ========== */

/*
 * CLR message passing via exchange pages:
 * - Sender prepares exchange (gets physical page handles)
 * - Sender transfers white token to receiver
 * - Receiver verifies white + accepts exchange pages
 * - Ownership physically transferred (zero-copy)
 *
 * GHOSTDAG ordering ensures message order across tasklets.
 */

typedef struct clr_exchange_msg {
	tasklet_id_t from;
	tasklet_id_t to;

	/* The payload object being transferred */
	clr_object_t *payload;

	/* White token authorizing receiver */
	PebbleWhite *white;

	/* Exchange page handles for zero-copy */
	ExchangeHandle *exchange_handles;
	ulong npages;

	/* Message ordering */
	u32int dag_id;		/* GHOSTDAG ordering */

	/* Intrusive list */
	struct clr_exchange_msg *next;
	struct clr_exchange_msg *prev;
} clr_exchange_msg_t;

/* Prepare object for zero-copy send */
clr_exchange_msg_t* clr_msg_prepare(clr_heap_t *heap,
                                     clr_object_t *obj,
                                     tasklet_id_t to,
                                     u32int dag_id);

/* Send message - transfers ownership via white + exchange */
int clr_msg_send(clr_heap_t *from_heap,
                 clr_exchange_msg_t *msg,
                 channel_id_t channel);

/* Receive message */
clr_exchange_msg_t* clr_msg_receive(clr_heap_t *to_heap,
                                     channel_id_t channel);

/* Accept message - verify white, accept exchange, gain ownership */
clr_object_t* clr_msg_accept(clr_heap_t *heap, clr_exchange_msg_t *msg);

/* Cancel message - rollback exchange, return white to sender */
int clr_msg_cancel(clr_heap_t *heap, clr_exchange_msg_t *msg);

/* ========== CLR Stack = Intrusive List of Stack Frames ========== */

/*
 * The evaluation stack is an intrusive linked list of stack slots.
 * Each slot holds a white token (if reference type).
 *
 * Push value = Allocate slot, issue white token, add to list
 * Pop value = Remove from list, burn white token, free slot
 *
 * Mathematical impossibility: A value CANNOT be freed while on stack.
 */

typedef struct clr_stack_slot {
	clr_value_t value;		/* Actual CLR value */
	PebbleWhite *white;		/* White token (if reference) */
	clr_object_t *obj;		/* Back-pointer to object */

	/* Intrusive list */
	struct clr_stack_slot *next;
	struct clr_stack_slot *prev;
} clr_stack_slot_t;

typedef struct clr_stack {
	UserCapability black_cap;	/* Stack structure capability */

	/* Stack slots (intrusive doubly-linked list) */
	clr_stack_slot_t *top;		/* Top of stack */
	clr_stack_slot_t *bottom;	/* Bottom for traversal */

	ulong depth;			/* Current stack depth */
	ulong max_depth;		/* Maximum allowed depth */

	Lock lock;			/* Protects stack operations */
} clr_stack_t;

/* Initialize stack (allocates via pebble) */
clr_stack_t* clr_stack_init(clr_heap_t *heap, ulong max_depth);

/* Push value - allocates slot, issues white token if reference type */
int clr_stack_push(clr_heap_t *heap, clr_stack_t *stack,
                   clr_value_t value, clr_object_t *obj);

/* Pop value - removes slot, releases white token if reference type */
int clr_stack_pop(clr_heap_t *heap, clr_stack_t *stack,
                  clr_value_t *value, clr_object_t **obj);

/* Peek at top value without popping */
int clr_stack_peek(clr_stack_t *stack, clr_value_t *value);

/* Duplicate top value - issues new white token */
int clr_stack_dup(clr_heap_t *heap, clr_stack_t *stack);

/* Cleanup stack - releases all whites, frees all slots */
void clr_stack_cleanup(clr_heap_t *heap, clr_stack_t *stack);

/* ========== CLR Locals = Intrusive List ========== */

/*
 * Local variables stored as intrusive list.
 * Each local has a white token if it's a reference type.
 */

typedef struct clr_local_slot {
	ulong index;			/* Local variable index */
	clr_value_t value;		/* Actual CLR value */
	PebbleWhite *white;		/* White token (if reference) */
	clr_object_t *obj;		/* Back-pointer to object */

	/* Intrusive list */
	struct clr_local_slot *next;
	struct clr_local_slot *prev;
} clr_local_slot_t;

typedef struct clr_locals {
	UserCapability black_cap;	/* Locals structure capability */

	/* Locals slots (intrusive doubly-linked list) */
	clr_local_slot_t *head;
	clr_local_slot_t *tail;

	ulong count;			/* Number of locals */
	ulong max_count;		/* Maximum allowed locals */

	Lock lock;			/* Protects local operations */
} clr_locals_t;

/* Initialize locals (allocates via pebble) */
clr_locals_t* clr_locals_init(clr_heap_t *heap, ulong max_count);

/* Load local - issues white token */
int clr_local_load(clr_heap_t *heap, clr_locals_t *locals,
                   ulong index, clr_value_t *value, clr_object_t **obj);

/* Store local - releases old white, issues new white */
int clr_local_store(clr_heap_t *heap, clr_locals_t *locals,
                    ulong index, clr_value_t value, clr_object_t *obj);

/* Cleanup locals - releases all whites, frees all slots */
void clr_locals_cleanup(clr_heap_t *heap, clr_locals_t *locals);

/* ========== CLR State with Pebble Backend ========== */

/*
 * This replaces the old malloc-based clr_state_t.
 * Everything uses intrusive lists and pebble allocation.
 */

typedef struct clr_pebble_state {
	clr_heap_t *heap;		/* Pebble heap */
	clr_stack_t *stack;		/* Pebble-backed stack */
	clr_locals_t *locals;		/* Pebble-backed locals */

	uintptr ip;			/* Instruction pointer */

	/* Speculative execution state */
	int is_speculative;		/* Currently executing speculatively? */
	ulong snapshot_depth;		/* Number of objects snapshotted */

	/* Statistics */
	ulong instructions_executed;
	ulong refs_created;
	ulong refs_released;
	ulong snapshots_taken;
	ulong commits;
	ulong rollbacks;
} clr_pebble_state_t;

/* Initialize CLR with pebble backend */
clr_pebble_state_t* clr_pebble_init(PebbleState *pebble,
                                     ulong stack_size,
                                     ulong locals_size,
                                     ulong heap_budget);

/* Execute instruction with pebble memory management */
clr_result_t clr_pebble_execute(clr_pebble_state_t *state,
                                const clr_instruction_t *instruction);

/* Cleanup state - releases all whites, frees all blacks */
void clr_pebble_cleanup(clr_pebble_state_t *state);

/* ========== Automatic Memory Reclamation ========== */

/*
 * No mark phase. No sweep phase. Just watch white_count.
 * When white_count hits 0, the object is unreachable - free it immediately.
 */

/* Free object when white_count == 0 (called internally) */
void clr_object_free_internal(clr_heap_t *heap, clr_object_t *obj);

/* Scan object list for unreachable objects and free them */
int clr_pebble_collect_unreachable(clr_heap_t *heap);

/* ========== Debug and Statistics ========== */

/* Print heap statistics */
void clr_pebble_print_stats(clr_heap_t *heap);

/* Verify heap integrity (debug) - walk all lists */
int clr_pebble_verify_heap(clr_heap_t *heap);

/* Print object details */
void clr_object_print_debug(clr_object_t *obj);

#endif /* CLR_PEBBLE_INTEGRATION_H */
