/*
 * CLR Pebble Integration Implementation
 *
 * The heap is the Pebble game.
 * White tokens ARE references.
 * Red-Blue ARE transactions.
 * Exchange pages ARE zero-copy IPC.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "clr_pebble_integration.h"

/* ========== Helper: Intrusive List Operations ========== */

/* Add white ref to object's list */
static void
add_white_ref(clr_object_t *obj, clr_white_ref_t *ref)
{
	lock(&obj->lock);

	if(obj->white_list == nil){
		/* First additional ref */
		obj->white_list = ref;
		ref->next = nil;
		ref->prev = nil;
	} else {
		/* Add to head of list */
		ref->next = obj->white_list;
		ref->prev = nil;
		obj->white_list->prev = ref;
		obj->white_list = ref;
	}

	obj->white_count++;
	unlock(&obj->lock);
}

/* Remove white ref from object's list */
static void
remove_white_ref(clr_object_t *obj, clr_white_ref_t *ref)
{
	lock(&obj->lock);

	/* Check if it's the inline ref */
	if(ref == &obj->inline_white){
		/* Can't remove inline ref while count > 1 */
		if(obj->white_count > 1){
			unlock(&obj->lock);
			return;
		}
	} else {
		/* Remove from list */
		if(ref->prev)
			ref->prev->next = ref->next;
		else
			obj->white_list = ref->next;

		if(ref->next)
			ref->next->prev = ref->prev;
	}

	obj->white_count--;
	unlock(&obj->lock);
}

/* Add object to heap list */
static void
heap_add_object(clr_heap_t *heap, clr_object_t *obj)
{
	lock(&heap->objects_lock);

	obj->next = heap->objects_head;
	obj->prev = nil;

	if(heap->objects_head)
		heap->objects_head->prev = obj;
	else
		heap->objects_tail = obj;

	heap->objects_head = obj;
	heap->object_count++;

	unlock(&heap->objects_lock);
}

/* Remove object from heap list */
static void
heap_remove_object(clr_heap_t *heap, clr_object_t *obj)
{
	lock(&heap->objects_lock);

	if(obj->prev)
		obj->prev->next = obj->next;
	else
		heap->objects_head = obj->next;

	if(obj->next)
		obj->next->prev = obj->prev;
	else
		heap->objects_tail = obj->prev;

	heap->object_count--;

	unlock(&heap->objects_lock);
}

/* ========== Object Allocation ========== */

clr_object_t*
clr_object_alloc(clr_heap_t *heap, ulong size, clr_value_type_t type)
{
	clr_object_t *obj;
	PebbleWhite *white;
	BlindLedgerEntry entry;

	if(heap == nil || size == 0)
		error(PEBBLE_E_BADARG);

	/* Allocate object structure */
	obj = mallocz(sizeof(clr_object_t), 1);
	if(obj == nil)
		error(PEBBLE_E_NOMEM);

	/* Allocate black pebble (creates UserCapability) */
	if(pebble_black_alloc(size, &obj->black_cap) != 0){
		free(obj);
		error(PEBBLE_E_NOMEM);
	}

	/* Verify the capability to get physical address */
	if(ledger_verify(&obj->black_cap, &entry) != BLIND_LEDGER_OK){
		pebble_black_free(&obj->black_cap);
		free(obj);
		error(PEBBLE_E_PERM);
	}

	/* Issue white token for this allocation */
	white = pebble_issue_white(heap->pebble, (void*)entry.physical_address, size);
	if(white == nil){
		pebble_black_free(&obj->black_cap);
		free(obj);
		error(PEBBLE_E_AGAIN);
	}

	/* Initialize object */
	obj->data = (void*)entry.physical_address;
	obj->type = type;
	obj->size = size;
	obj->exchange_handles = nil;
	obj->exchange_npages = 0;
	obj->is_prepared = 0;

	/* First white token stored inline */
	obj->inline_white.white = white;
	obj->inline_white.next = nil;
	obj->inline_white.prev = nil;
	obj->white_list = nil;
	obj->white_count = 1;

	obj->next = nil;
	obj->prev = nil;

	/* Add to heap */
	heap_add_object(heap, obj);

	heap->total_allocs++;

	return obj;
}

/* ========== Reference Counting ========== */

PebbleWhite*
clr_object_addref(clr_heap_t *heap, clr_object_t *obj)
{
	PebbleWhite *white;
	clr_white_ref_t *ref;

	if(heap == nil || obj == nil)
		error(PEBBLE_E_BADARG);

	/* Issue new white token */
	white = pebble_issue_white(heap->pebble, obj->data, obj->size);
	if(white == nil)
		error(PEBBLE_E_AGAIN);

	/* Allocate ref node */
	ref = mallocz(sizeof(clr_white_ref_t), 1);
	if(ref == nil){
		/* Invalidate white we just issued */
		white->token = 0;
		error(PEBBLE_E_NOMEM);
	}

	ref->white = white;
	add_white_ref(obj, ref);

	heap->total_refs_created++;

	return white;
}

int
clr_object_release(clr_heap_t *heap, clr_object_t *obj, PebbleWhite *white)
{
	clr_white_ref_t *ref, *next;
	int found = 0;

	if(heap == nil || obj == nil || white == nil)
		error(PEBBLE_E_BADARG);

	lock(&obj->lock);

	/* Find the white in our list */
	if(obj->inline_white.white == white){
		ref = &obj->inline_white;
		found = 1;
	} else {
		for(ref = obj->white_list; ref != nil; ref = ref->next){
			if(ref->white == white){
				found = 1;
				break;
			}
		}
	}

	if(!found){
		unlock(&obj->lock);
		error(PEBBLE_E_PERM);
	}

	/* Remove from list */
	remove_white_ref(obj, ref);

	/* Invalidate white token */
	white->token = 0;

	/* Check if object is now unreachable */
	if(obj->white_count == 0){
		unlock(&obj->lock);
		/* Free the object */
		clr_object_free_internal(heap, obj);
	} else {
		unlock(&obj->lock);
	}

	/* Free ref node if not inline */
	if(ref != &obj->inline_white)
		free(ref);

	heap->total_refs_released++;

	return 0;
}

/* ========== Object Freeing ========== */

void
clr_object_free_internal(clr_heap_t *heap, clr_object_t *obj)
{
	clr_white_ref_t *ref, *next;

	if(obj == nil)
		return;

	/* Remove from heap list */
	heap_remove_object(heap, obj);

	/* Free exchange handles if prepared */
	if(obj->exchange_handles != nil)
		free(obj->exchange_handles);

	/* Free all remaining white refs (should be none, but cleanup anyway) */
	for(ref = obj->white_list; ref != nil; ref = next){
		next = ref->next;
		if(ref->white)
			ref->white->token = 0;
		free(ref);
	}

	/* Invalidate inline white */
	if(obj->inline_white.white)
		obj->inline_white.white->token = 0;

	/* Free black pebble */
	pebble_black_free(&obj->black_cap);

	/* Free object structure */
	free(obj);

	heap->total_frees++;
}

/* ========== Speculative Execution: Red-Blue ========== */

/*
 * Red-Blue Transactional Memory for Block Device Operations
 *
 * Token State Machine (Circular Economy):
 *   COLORLESS → WHITE → BLACK/BLUE/RED → COLORLESS
 *
 * Architecture:
 * - Blue and Red are SEPARATE colored tokens (not shadows)
 * - Each consumes budget from the colorless bank
 * - Used for block device I/O transaction safety (NOT for IPC - use exchange pages)
 *
 * Correct Flow:
 * 1. Allocate Blue from colorless bank (for block I/O working buffer)
 * 2. Allocate Red from colorless bank (for snapshot/rollback)
 * 3. Perform block I/O into Blue
 * 4. On success: Free Blue→colorless, Free Red→colorless (commit)
 * 5. On failure: Copy Red→Blue, Free both→colorless (rollback)
 *
 * TODO: This requires redesign to respect token economy.
 * Current Pebble Blue/Red API uses "matching_red" shadow model which
 * violates circular economy by not properly tracking separate allocations.
 */

int
clr_object_snapshot(clr_heap_t *heap, clr_object_t *obj)
{
	if(heap == nil || obj == nil)
		error(PEBBLE_E_BADARG);

	/* TODO: Implement proper token state machine transitions
	 * Requires: COLORLESS → WHITE → RED allocation
	 * Current shadow model is incompatible with circular economy
	 */
	error("Red-Blue snapshots require token economy redesign");
	return 0;
}

int
clr_object_commit(clr_heap_t *heap, clr_object_t *obj)
{
	if(heap == nil || obj == nil)
		error(PEBBLE_E_BADARG);

	/* TODO: Implement proper state transitions
	 * Should: Free Blue→COLORLESS, Free Red→COLORLESS
	 */
	error("Red-Blue commits require token economy redesign");
	return 0;
}

int
clr_object_rollback(clr_heap_t *heap, clr_object_t *obj)
{
	if(heap == nil || obj == nil)
		error(PEBBLE_E_BADARG);

	/* TODO: Implement proper state transitions
	 * Should: Copy Red→Blue, Free both→COLORLESS
	 */
	error("Red-Blue rollback requires token economy redesign");
	return 0;
}

/* ========== Stack Operations ========== */

clr_stack_t*
clr_stack_init(clr_heap_t *heap, ulong max_depth)
{
	clr_stack_t *stack;
	UserCapability cap;
	BlindLedgerEntry entry;
	ulong size;

	if(heap == nil)
		error(PEBBLE_E_BADARG);

	/* Allocate stack structure via pebble */
	size = sizeof(clr_stack_t);

	if(pebble_black_alloc(size, &cap) != 0)
		error(PEBBLE_E_NOMEM);

	/* Get physical address */
	if(ledger_verify(&cap, &entry) != BLIND_LEDGER_OK){
		pebble_black_free(&cap);
		error(PEBBLE_E_PERM);
	}

	stack = (clr_stack_t*)entry.physical_address;
	memset(stack, 0, sizeof(clr_stack_t));

	stack->black_cap = cap;
	stack->top = nil;
	stack->bottom = nil;
	stack->depth = 0;
	stack->max_depth = max_depth;

	return stack;
}

int
clr_stack_push(clr_heap_t *heap, clr_stack_t *stack,
               clr_value_t value, clr_object_t *obj)
{
	clr_stack_slot_t *slot;
	PebbleWhite *white;

	if(heap == nil || stack == nil)
		error(PEBBLE_E_BADARG);

	lock(&stack->lock);

	if(stack->depth >= stack->max_depth){
		unlock(&stack->lock);
		error("stack overflow");
	}

	/* Allocate slot */
	slot = mallocz(sizeof(clr_stack_slot_t), 1);
	if(slot == nil){
		unlock(&stack->lock);
		error(PEBBLE_E_NOMEM);
	}

	slot->value = value;
	slot->obj = obj;
	slot->white = nil;

	/* If this is a reference type, issue white token */
	if(obj != nil){
		white = clr_object_addref(heap, obj);
		slot->white = white;
	}

	/* Add to top of stack */
	slot->next = nil;
	slot->prev = stack->top;

	if(stack->top)
		stack->top->next = slot;
	else
		stack->bottom = slot;

	stack->top = slot;
	stack->depth++;

	unlock(&stack->lock);

	return 0;
}

int
clr_stack_pop(clr_heap_t *heap, clr_stack_t *stack,
              clr_value_t *value, clr_object_t **obj)
{
	clr_stack_slot_t *slot;

	if(heap == nil || stack == nil)
		error(PEBBLE_E_BADARG);

	lock(&stack->lock);

	if(stack->top == nil){
		unlock(&stack->lock);
		error("stack underflow");
	}

	slot = stack->top;

	/* Remove from stack */
	stack->top = slot->prev;
	if(stack->top)
		stack->top->next = nil;
	else
		stack->bottom = nil;

	stack->depth--;

	unlock(&stack->lock);

	/* Return value */
	if(value) *value = slot->value;
	if(obj) *obj = slot->obj;

	/* Release white token if reference */
	if(slot->white != nil && slot->obj != nil)
		clr_object_release(heap, slot->obj, slot->white);

	free(slot);

	return 0;
}

int
clr_stack_peek(clr_stack_t *stack, clr_value_t *value)
{
	if(stack == nil || value == nil)
		error(PEBBLE_E_BADARG);

	lock(&stack->lock);

	if(stack->top == nil){
		unlock(&stack->lock);
		error("stack underflow");
	}

	*value = stack->top->value;

	unlock(&stack->lock);

	return 0;
}

int
clr_stack_dup(clr_heap_t *heap, clr_stack_t *stack)
{
	clr_value_t value;
	clr_object_t *obj;

	if(heap == nil || stack == nil)
		error(PEBBLE_E_BADARG);

	lock(&stack->lock);

	if(stack->top == nil){
		unlock(&stack->lock);
		error("stack underflow");
	}

	value = stack->top->value;
	obj = stack->top->obj;

	unlock(&stack->lock);

	/* Push duplicate (will issue new white if reference) */
	return clr_stack_push(heap, stack, value, obj);
}

void
clr_stack_cleanup(clr_heap_t *heap, clr_stack_t *stack)
{
	clr_stack_slot_t *slot, *next;

	if(stack == nil)
		return;

	/* Free all slots */
	for(slot = stack->top; slot != nil; slot = next){
		next = slot->prev;

		/* Release white if reference */
		if(slot->white != nil && slot->obj != nil)
			clr_object_release(heap, slot->obj, slot->white);

		free(slot);
	}

	/* Free stack structure */
	pebble_black_free(&stack->black_cap);
}

/* ========== CLR Pebble State Initialization ========== */

clr_pebble_state_t*
clr_pebble_init(PebbleState *pebble,
                 ulong stack_size,
                 ulong locals_size,
                 ulong heap_budget)
{
	clr_pebble_state_t *state;
	clr_heap_t *heap;

	if(pebble == nil)
		error(PEBBLE_E_BADARG);

	/* Allocate state structure */
	state = mallocz(sizeof(clr_pebble_state_t), 1);
	if(state == nil)
		error(PEBBLE_E_NOMEM);

	/* Allocate heap structure */
	heap = mallocz(sizeof(clr_heap_t), 1);
	if(heap == nil){
		free(state);
		error(PEBBLE_E_NOMEM);
	}

	/* Initialize heap */
	heap->pebble = pebble;
	heap->objects_head = nil;
	heap->objects_tail = nil;
	heap->object_count = 0;
	heap->send_queue_head = nil;
	heap->send_queue_tail = nil;
	heap->recv_queue_head = nil;
	heap->recv_queue_tail = nil;
	heap->total_allocs = 0;
	heap->total_frees = 0;
	heap->total_refs_created = 0;
	heap->total_refs_released = 0;

	state->heap = heap;

	/* Initialize stack */
	state->stack = clr_stack_init(heap, stack_size);
	if(state->stack == nil){
		free(heap);
		free(state);
		error(PEBBLE_E_NOMEM);
	}

	/* Initialize locals */
	state->locals = clr_locals_init(heap, locals_size);
	if(state->locals == nil){
		clr_stack_cleanup(heap, state->stack);
		free(heap);
		free(state);
		error(PEBBLE_E_NOMEM);
	}

	state->ip = 0;
	state->is_speculative = 0;
	state->snapshot_depth = 0;
	state->instructions_executed = 0;
	state->refs_created = 0;
	state->refs_released = 0;
	state->snapshots_taken = 0;
	state->commits = 0;
	state->rollbacks = 0;

	return state;
}

void
clr_pebble_cleanup(clr_pebble_state_t *state)
{
	clr_object_t *obj, *next;

	if(state == nil)
		return;

	/* Cleanup stack */
	if(state->stack)
		clr_stack_cleanup(state->heap, state->stack);

	/* Cleanup locals */
	if(state->locals)
		clr_locals_cleanup(state->heap, state->locals);

	/* Free all objects */
	if(state->heap){
		for(obj = state->heap->objects_head; obj != nil; obj = next){
			next = obj->next;
			clr_object_free_internal(state->heap, obj);
		}
		free(state->heap);
	}

	free(state);
}

/* ========== Locals Implementation ========== */

clr_locals_t*
clr_locals_init(clr_heap_t *heap, ulong max_count)
{
	clr_locals_t *locals;
	UserCapability cap;
	BlindLedgerEntry entry;
	ulong size;

	if(heap == nil)
		error(PEBBLE_E_BADARG);

	/* Allocate locals structure via pebble */
	size = sizeof(clr_locals_t);

	if(pebble_black_alloc(size, &cap) != 0)
		error(PEBBLE_E_NOMEM);

	/* Get physical address */
	if(ledger_verify(&cap, &entry) != BLIND_LEDGER_OK){
		pebble_black_free(&cap);
		error(PEBBLE_E_PERM);
	}

	locals = (clr_locals_t*)entry.physical_address;
	memset(locals, 0, sizeof(clr_locals_t));

	locals->black_cap = cap;
	locals->head = nil;
	locals->tail = nil;
	locals->count = 0;
	locals->max_count = max_count;

	return locals;
}

int
clr_local_load(clr_heap_t *heap, clr_locals_t *locals,
               ulong index, clr_value_t *value, clr_object_t **obj)
{
	clr_local_slot_t *slot;

	if(heap == nil || locals == nil || value == nil)
		error(PEBBLE_E_BADARG);

	lock(&locals->lock);

	/* Find local by index */
	for(slot = locals->head; slot != nil; slot = slot->next){
		if(slot->index == index){
			*value = slot->value;
			if(obj) *obj = slot->obj;

			/* Issue new white token for reference */
			if(slot->obj != nil){
				PebbleWhite *white = clr_object_addref(heap, slot->obj);
				/* Caller now owns this white token */
			}

			unlock(&locals->lock);
			return 0;
		}
	}

	unlock(&locals->lock);
	error("local not found");
	return -1;
}

int
clr_local_store(clr_heap_t *heap, clr_locals_t *locals,
                ulong index, clr_value_t value, clr_object_t *obj)
{
	clr_local_slot_t *slot;

	if(heap == nil || locals == nil)
		error(PEBBLE_E_BADARG);

	lock(&locals->lock);

	/* Find existing local */
	for(slot = locals->head; slot != nil; slot = slot->next){
		if(slot->index == index){
			/* Release old white if reference */
			if(slot->white != nil && slot->obj != nil){
				unlock(&locals->lock);
				clr_object_release(heap, slot->obj, slot->white);
				lock(&locals->lock);
			}

			/* Store new value */
			slot->value = value;
			slot->obj = obj;

			/* Issue new white if reference */
			if(obj != nil){
				unlock(&locals->lock);
				slot->white = clr_object_addref(heap, obj);
				lock(&locals->lock);
			} else {
				slot->white = nil;
			}

			unlock(&locals->lock);
			return 0;
		}
	}

	/* Create new local */
	if(locals->count >= locals->max_count){
		unlock(&locals->lock);
		error("too many locals");
	}

	slot = mallocz(sizeof(clr_local_slot_t), 1);
	if(slot == nil){
		unlock(&locals->lock);
		error(PEBBLE_E_NOMEM);
	}

	slot->index = index;
	slot->value = value;
	slot->obj = obj;

	/* Issue white if reference */
	if(obj != nil){
		unlock(&locals->lock);
		slot->white = clr_object_addref(heap, obj);
		lock(&locals->lock);
	} else {
		slot->white = nil;
	}

	/* Add to list */
	slot->next = nil;
	slot->prev = locals->tail;

	if(locals->tail)
		locals->tail->next = slot;
	else
		locals->head = slot;

	locals->tail = slot;
	locals->count++;

	unlock(&locals->lock);
	return 0;
}

void
clr_locals_cleanup(clr_heap_t *heap, clr_locals_t *locals)
{
	clr_local_slot_t *slot, *next;

	if(locals == nil)
		return;

	/* Free all slots */
	for(slot = locals->head; slot != nil; slot = next){
		next = slot->next;

		/* Release white if reference */
		if(slot->white != nil && slot->obj != nil)
			clr_object_release(heap, slot->obj, slot->white);

		free(slot);
	}

	/* Free locals structure */
	pebble_black_free(&locals->black_cap);
}

/* ========== CLR Instruction Execution ========== */

clr_result_t
clr_pebble_execute(clr_pebble_state_t *state,
                   const clr_instruction_t *instruction)
{
	clr_value_t v1, v2, result;
	clr_object_t *obj1, *obj2;

	if(state == nil || instruction == nil)
		return CLR_ERROR_INVALID_LOCAL;

	state->instructions_executed++;

	/* Execute based on opcode */
	switch(instruction->opcode){
	case CIL_NOP:
		state->ip++;
		return CLR_SUCCESS;

	case CIL_DUP:
		/* Duplicate top of stack (issues new white if reference) */
		return clr_stack_dup(state->heap, state->stack);

	case CIL_POP:
		/* Pop value from stack (releases white if reference) */
		return clr_stack_pop(state->heap, state->stack, &v1, &obj1);

	case CIL_LDC_I4:
		/* Load constant int32 */
		result = clr_make_int32(instruction->arg.int32_arg);
		return clr_stack_push(state->heap, state->stack, result, nil);

	case CIL_ADD:
		/* Pop two values, add, push result */
		if(clr_stack_pop(state->heap, state->stack, &v2, &obj2) != 0)
			return CLR_ERROR_STACK_UNDERFLOW;
		if(clr_stack_pop(state->heap, state->stack, &v1, &obj1) != 0)
			return CLR_ERROR_STACK_UNDERFLOW;

		if(v1.type != CLR_INT32 || v2.type != CLR_INT32)
			return CLR_ERROR_TYPE_MISMATCH;

		result = clr_make_int32(v1.data.int32_val + v2.data.int32_val);
		return clr_stack_push(state->heap, state->stack, result, nil);

	case CIL_SUB:
		/* Pop two values, subtract, push result */
		if(clr_stack_pop(state->heap, state->stack, &v2, &obj2) != 0)
			return CLR_ERROR_STACK_UNDERFLOW;
		if(clr_stack_pop(state->heap, state->stack, &v1, &obj1) != 0)
			return CLR_ERROR_STACK_UNDERFLOW;

		if(v1.type != CLR_INT32 || v2.type != CLR_INT32)
			return CLR_ERROR_TYPE_MISMATCH;

		result = clr_make_int32(v1.data.int32_val - v2.data.int32_val);
		return clr_stack_push(state->heap, state->stack, result, nil);

	case CIL_MUL:
		/* Pop two values, multiply, push result */
		if(clr_stack_pop(state->heap, state->stack, &v2, &obj2) != 0)
			return CLR_ERROR_STACK_UNDERFLOW;
		if(clr_stack_pop(state->heap, state->stack, &v1, &obj1) != 0)
			return CLR_ERROR_STACK_UNDERFLOW;

		if(v1.type != CLR_INT32 || v2.type != CLR_INT32)
			return CLR_ERROR_TYPE_MISMATCH;

		result = clr_make_int32(v1.data.int32_val * v2.data.int32_val);
		return clr_stack_push(state->heap, state->stack, result, nil);

	case CIL_LDLOC:
		/* Load local variable onto stack */
		if(clr_local_load(state->heap, state->locals,
		                  instruction->arg.index_arg, &v1, &obj1) != 0)
			return CLR_ERROR_INVALID_LOCAL;
		return clr_stack_push(state->heap, state->stack, v1, obj1);

	case CIL_STLOC:
		/* Pop stack and store in local variable */
		if(clr_stack_pop(state->heap, state->stack, &v1, &obj1) != 0)
			return CLR_ERROR_STACK_UNDERFLOW;
		return clr_local_store(state->heap, state->locals,
		                       instruction->arg.index_arg, v1, obj1) == 0 ?
		       CLR_SUCCESS : CLR_ERROR_INVALID_LOCAL;

	case CIL_BR:
		/* Unconditional branch */
		state->ip = instruction->arg.branch_target;
		return CLR_SUCCESS;

	case CIL_RET:
		/* Return from function */
		return CLR_SUCCESS;

	default:
		return CLR_ERROR_TYPE_MISMATCH;
	}
}

/* ========== Debug and Statistics ========== */

void
clr_pebble_print_stats(clr_heap_t *heap)
{
	if(heap == nil)
		return;

	print("CLR Pebble Heap Statistics:\n");
	print("  Objects: %lud\n", heap->object_count);
	print("  Total allocations: %lud\n", heap->total_allocs);
	print("  Total frees: %lud\n", heap->total_frees);
	print("  Total refs created: %lud\n", heap->total_refs_created);
	print("  Total refs released: %lud\n", heap->total_refs_released);
	print("  Live objects: %lud\n", heap->total_allocs - heap->total_frees);
}

int
clr_pebble_verify_heap(clr_heap_t *heap)
{
	clr_object_t *obj;
	clr_white_ref_t *ref;
	int errors = 0;

	if(heap == nil)
		return -1;

	/* Walk all objects */
	for(obj = heap->objects_head; obj != nil; obj = obj->next){
		/* Verify white_count matches list */
		int count = (obj->inline_white.white != nil) ? 1 : 0;
		for(ref = obj->white_list; ref != nil; ref = ref->next)
			count++;

		if(count != obj->white_count){
			print("HEAP ERROR: Object %#p white_count=%lud but list has %d\n",
			      obj, obj->white_count, count);
			errors++;
		}

		/* Verify all whites are valid */
		if(obj->inline_white.white != nil){
			if(!pebble_valid_white_token(heap->pebble, obj->inline_white.white)){
				print("HEAP ERROR: Object %#p inline white is invalid\n", obj);
				errors++;
			}
		}

		for(ref = obj->white_list; ref != nil; ref = ref->next){
			if(!pebble_valid_white_token(heap->pebble, ref->white)){
				print("HEAP ERROR: Object %#p white in list is invalid\n", obj);
				errors++;
			}
		}
	}

	if(errors == 0)
		print("Heap verification: OK\n");
	else
		print("Heap verification: %d errors found\n", errors);

	return errors;
}

void
clr_object_print_debug(clr_object_t *obj)
{
	clr_white_ref_t *ref;
	int count;

	if(obj == nil){
		print("CLR Object: nil\n");
		return;
	}

	print("CLR Object %#p:\n", obj);
	print("  Data: %#p\n", obj->data);
	print("  Type: %d\n", obj->type);
	print("  Size: %lud bytes\n", obj->size);
	print("  Ref count: %lud\n", obj->white_count);

	count = 0;
	for(ref = obj->white_list; ref != nil; ref = ref->next)
		count++;

	print("  White refs in list: %d\n", count);
	print("  Exchange prepared: %s\n", obj->is_prepared ? "yes" : "no");
	print("  Has snapshot: %s\n", clr_object_has_snapshot(obj) ? "yes" : "no");
}

int
clr_pebble_collect_unreachable(clr_heap_t *heap)
{
	clr_object_t *obj, *next;
	int freed = 0;

	if(heap == nil)
		return 0;

	/* Walk all objects and free those with white_count == 0 */
	for(obj = heap->objects_head; obj != nil; obj = next){
		next = obj->next;

		if(obj->white_count == 0){
			clr_object_free_internal(heap, obj);
			freed++;
		}
	}

	return freed;
}
