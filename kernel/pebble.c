#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"pebble.h"
#include	"blind_ledger.h"

Lock pebble_global_lock;
int pebble_enabled = 1;
int pebble_debug = PEBBLE_DEBUG;

static int pebble_initialized;

static void pebble_free_red(PebbleRed*);

static PebbleBlack*
pebble_lookup_black_by_cap_locked(PebbleState *ps, const UserCapability *cap)
{
    PebbleBlack *pb;
    for (pb = ps->black_list; pb != nil; pb = pb->next) {
        if (memcmp(pb->capability.hash, cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
            return pb;
        }
    }
    return nil;
}

static void
pebble_reset_state(PebbleState *ps)
{
	memset(ps, 0, sizeof(*ps));
	ps->black_budget = PEBBLE_DEFAULT_BUDGET;
	ps->white_head = 0;
	ps->white_pending = 0;
}

PebbleState*
pebble_state(void)
{
	if(up == nil)
		return nil;
	return &up->pebble;
}

void
pebbleinit(void)
{
	if(pebble_initialized)
		return;
	pebble_initialized = 1;
}

void
pebbleprocinit(Proc *p)
{
	if(p == nil)
		return;
	pebble_reset_state(&p->pebble);
}

static PebbleBlack*
pebble_lookup_black_locked(PebbleState *ps, void *handle)
{
	PebbleBlack *pb;

	for(pb = ps->black_list; pb != nil; pb = pb->next)
		if(pb == handle)
			return pb;
	return nil;
}

PebbleBlack*
pebble_lookup_black(PebbleState *ps, void *handle)
{
	PebbleBlack *pb;

	if(ps == nil || handle == nil)
		return nil;
	lock(&pebble_global_lock);
	pb = pebble_lookup_black_locked(ps, handle);
	unlock(&pebble_global_lock);
	return pb;
}

PebbleWhite*
pebble_issue_white(PebbleState *ps, void *data, ulong size)
{
	int i, idx;
	ulong pegged_size;

	if(ps == nil)
		return nil;

	/* Peg size to 8-byte quantum */
	pegged_size = (size + 7) & ~7;

	lock(&pebble_global_lock);
	for(i = 0; i < PEBBLE_MAX_TOKENS; i++){
		idx = (ps->white_head + i) % PEBBLE_MAX_TOKENS;
		if(ps->whites_active[idx])
			continue;
		ps->white_generation++;
		ps->whites_active[idx] = 1;
		ps->whites[idx].token = PEBBLE_TOKEN_MAGIC;
		ps->whites[idx].generation = ps->white_generation;
		ps->whites[idx].data_ptr = data;
		ps->whites[idx].size = pegged_size;
		ps->white_head = (idx + 1) % PEBBLE_MAX_TOKENS;
		unlock(&pebble_global_lock);
		return &ps->whites[idx];
	}
	unlock(&pebble_global_lock);
	return nil;
}

int
pebble_valid_white_token(PebbleState *ps, PebbleWhite *white)
{
	int i;

	if(ps == nil || white == nil)
		return 0;

	for(i = 0; i < PEBBLE_MAX_TOKENS; i++){
		if(ps->whites_active[i] && &ps->whites[i] == white){
			if(white->token != PEBBLE_TOKEN_MAGIC)
				return 0;
			return 1;
		}
	}
	return 0;
}

int
pebble_set_budget(ulong budget)
{
	PebbleState *ps;

	ps = pebble_state();
	if(ps == nil)
		return -1;

	lock(&pebble_global_lock);
	ps->black_budget = budget;
	unlock(&pebble_global_lock);
	return 0;
}

ulong
pebble_get_budget(void)
{
	PebbleState *ps;
	ulong budget;

	ps = pebble_state();
	if(ps == nil)
		return 0;
	lock(&pebble_global_lock);
	budget = ps->black_budget;
	unlock(&pebble_global_lock);
	return budget;
}

int
pebble_black_alloc(ulong size, UserCapability *out_cap)
{
	PebbleState *ps;
	PebbleBlack *pb = nil; // Initialize to nil for error handling
	PebbleBlue *blue = nil; // Initialize to nil for error handling
	void *buf = nil; // Initialize buf to nil for proper cleanup on early error
    BlindLedgerError ledger_err;
    enum BorrowError borrow_err;
    u8int vault_secret[BLIND_LEDGER_SECRET_SIZE];

	if(out_cap == nil)
		error(PEBBLE_E_BADARG);
	if(size < PEBBLE_MIN_ALLOC || size > PEBBLE_MAX_ALLOC)
		error(PEBBLE_E_BADARG);

	ps = pebble_state();
	if(ps == nil)
		error(PEBBLE_E_PERM);

    // Generate cryptographic secret via TPM-backed vault
    ledger_err = ledger_generate_secret(vault_secret);
    if(ledger_err != BLIND_LEDGER_OK)
        error(PEBBLE_E_NOMEM);

    lock(&pebble_global_lock);
	if(ps->white_verified == 0){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM);
	}
	if(ps->white_pending < size){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM);
	}
	if(ps->black_budget < size){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_AGAIN);
	}
	ps->white_pending -= size;
	ps->white_verified--;
	ps->black_budget -= size;
	ps->black_inuse += size;
	ps->total_allocs++;
	unlock(&pebble_global_lock);

    // --- Physical Memory Allocation ---
	buf = xallocz(size, 1);
	if(buf == nil){
		lock(&pebble_global_lock);
		ps->black_budget += size;
		ps->black_inuse -= size;
		ps->total_allocs--;
		ps->white_pending += size;
		ps->white_verified++;
		unlock(&pebble_global_lock);
		error(PEBBLE_E_NOMEM);
	}

    // --- Mint UserCapability via Blind Ledger ---
    ledger_err = ledger_mint(out_cap, (uintptr)buf, size, up, PEBBLE_CAP_BLACK, vault_secret); // PEBBLE_CAP_BLACK as initial permission
    if (ledger_err != BLIND_LEDGER_OK) {
        xfree(buf); // Rollback xallocz
        lock(&pebble_global_lock);
		ps->black_budget += size; // Rollback budget
		ps->black_inuse -= size;
		ps->total_allocs--;
		ps->white_pending += size;
		ps->white_verified++;
		unlock(&pebble_global_lock);
        error(PEBBLE_E_NOMEM); // Or BLIND_LEDGER_E_NOMEM mapped
    }

    // --- Acquire borrow checker ownership ---
    borrow_err = borrow_acquire(up, (uintptr)buf);
    if (borrow_err != BORROW_OK) {
        // Rollback ledger_mint
        ledger_burn(out_cap, up); // Pass up as owner, assuming it matches
        xfree(buf); // Rollback xallocz
        lock(&pebble_global_lock);
		ps->black_budget += size; // Rollback budget
		ps->black_inuse -= size;
		ps->total_allocs--;
		ps->white_pending += size;
		ps->white_verified++;
		unlock(&pebble_global_lock);
        error(PEBBLE_E_PERM); // Or a more specific borrow error mapped
    }

    // --- PebbleBlack Struct Allocation ---
	pb = mallocz(sizeof(PebbleBlack), 1);
	if(pb == nil){
        // Rollback borrow_acquire and ledger_mint
        borrow_release(up, (uintptr)buf);
        ledger_burn(out_cap, up);
		xfree(buf);
		lock(&pebble_global_lock);
		ps->black_budget += size;
		ps->black_inuse -= size;
		ps->total_allocs--;
		ps->white_pending += size;
		ps->white_verified++;
		unlock(&pebble_global_lock);
		error(PEBBLE_E_NOMEM);
	}

    // --- Populate PebbleBlack Struct ---
	memmove(&pb->capability, out_cap, sizeof(UserCapability)); // Store the UserCapability
	pb->physical_addr = buf; // Store the actual physical address
	pb->size = size; // Retain size for budgeting
	pb->flags = PEBBLE_CAP_BLACK | PEBBLE_CAP_ACTIVE;

	/* NOTE: Blue is NO LONGER auto-created - it's an independent token!
	 * If block I/O transactions are needed, use pebble_blue_alloc() separately.
	 * This enforces the circular economy: Black and Blue are separate colors.
	 */

	lock(&pebble_global_lock);
	pb->next = ps->black_list;
	ps->black_list = pb;
	unlock(&pebble_global_lock);

	if(pebble_debug)
		print("PEBBLE: black alloc pid=%lud cap=%H size=%lud\n",
			up->pid, out_cap->hash, size); // Print hash of UserCapability
	return 0;
}

int
pebble_black_free(const UserCapability *cap)
{
	PebbleState *ps;
	PebbleBlack *pb, **pp;
	ulong size;
	BlindLedgerError ledger_err;
	enum BorrowError borrow_err;
	BlindLedgerEntry entry; // To get physical_address from ledger

	if(cap == nil)
		error(PEBBLE_E_BADARG);

	ps = pebble_state();
	if(ps == nil)
		error(PEBBLE_E_PERM);

	lock(&pebble_global_lock);
	// Use the new lookup function
	pb = pebble_lookup_black_by_cap_locked(ps, cap);
	if(pb == nil){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM); // Changed to PERM, as cap not found implies unauthorized access or invalid cap
	}

	/* NOTE: Blue/Red coupling checks removed - they are now independent tokens!
	 * Blue and Red must be freed separately via pebble_blue_free() and pebble_red_free().
	 * Black tokens only manage BLACK state in the circular economy.
	 */

	// --- Get physical address from Blind Ledger (using ledger_verify) ---
	// This is needed for borrow_release and xfree
	// Unlock is done outside ledger_verify, so it's safe to call here.
	ledger_err = ledger_verify(cap, &entry);
	if (ledger_err != BLIND_LEDGER_OK) {
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM); // Cap found in Pebble but not valid in Ledger? Inconsistency.
	}

	size = pb->size; // Get size from PebbleBlack for budgeting

	// --- Remove from PebbleBlack list ---
	for(pp = &ps->black_list; *pp != nil; pp = &(*pp)->next){
		if(*pp == pb){
			*pp = pb->next;
			break;
		}
	}

	// --- Adjust Pebble budget (BLACK → COLORLESS) ---
	ps->black_inuse -= size;
	ps->black_budget += size;
	ps->total_frees++;
	unlock(&pebble_global_lock); // Unlock early before external calls

	// --- Release borrow checker ownership ---
	borrow_err = borrow_release(up, (uintptr)entry.physical_address);
	if (borrow_err != BORROW_OK) {
		// CRITICAL: Borrow checker state inconsistent with Pebble state
		// This indicates double-free, UAF, or severe corruption
		// Continuing would leave system in undefined state - MUST PANIC
		panic("pebble_black_free: FATAL - borrow_release failed for cap=%H at pa=%#p: error=%d\n"
		      "This indicates critical state corruption (double-free/UAF).\n"
		      "Borrow Checker and Pebble system out of sync.",
		      cap->hash, entry.physical_address, borrow_err);
	}

	// --- Burn UserCapability via Blind Ledger ---
	ledger_err = ledger_burn(cap, up);
	if (ledger_err != BLIND_LEDGER_OK) {
		// CRITICAL: Blind Ledger state inconsistent with Pebble state
		// This indicates double-burn, invalid capability, or severe corruption
		// Continuing would leave capability management in undefined state - MUST PANIC
		panic("pebble_black_free: FATAL - ledger_burn failed for cap=%H: error=%d\n"
		      "This indicates critical state corruption (double-burn/invalid cap).\n"
		      "Blind Ledger and Pebble system out of sync.",
		      cap->hash, ledger_err);
	}

	// --- Free physical memory and PebbleBlack struct ---
	// (Blue/Red are freed separately - they're independent colored tokens)
	xfree((void*)entry.physical_address);
	free(pb); // Free PebbleBlack struct

	if(pebble_debug)
		print("PEBBLE: black free pid=%lud cap=%H size=%lud\n", up->pid, cap->hash, size);
	return 0;
}

int
pebble_white_verify(PebbleWhite *white_cap, void **black_cap)
{
	PebbleState *ps;
	int i;
	void *ret;

	if(white_cap == nil || black_cap == nil)
		error(PEBBLE_E_BADARG);

	ps = pebble_state();
	if(ps == nil)
		error(PEBBLE_E_PERM);

	lock(&pebble_global_lock);
	if(!pebble_valid_white_token(ps, white_cap)){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM);
	}

	ret = white_cap->data_ptr;
	if(white_cap->size != 0)
		ps->white_pending += white_cap->size;
	ps->white_verified++;

	for(i = 0; i < PEBBLE_MAX_TOKENS; i++){
		if(&ps->whites[i] == white_cap){
			ps->whites_active[i] = 0;
			break;
		}
	}
	white_cap->token = 0;
	unlock(&pebble_global_lock);

	*black_cap = ret;
	if(pebble_debug)
		print("PEBBLE: white verify pid=%lud -> %#p\n", up->pid, ret);
	return 0;
}

/* REMOVED: pebble_detach_blue_locked() - coupled Blue/Red model deprecated */

static void
pebble_free_red(PebbleRed *red)
{
	PebbleState *ps;
	ulong size;

	if(red == nil)
		return;

	ps = pebble_state();
	if(ps == nil){
		/* Fallback: Just free memory without budget tracking */
		if(red->red_data != nil)
			xfree(red->red_data);
		free(red);
		return;
	}

	size = red->red_size;

	/* Free physical memory */
	if(red->red_data != nil)
		xfree(red->red_data);
	free(red);

	/* Return budget to colorless bank (state transition: RED → COLORLESS) */
	lock(&pebble_global_lock);
	ps->black_budget += size;
	ps->red_inuse -= size;
	unlock(&pebble_global_lock);
}

/* ========== New Independent Blue/Red API ========== */

/*
 * pebble_blue_alloc - Allocate independent Blue token for block I/O
 *
 * State transition: COLORLESS → BLUE
 * Consumes budget from colorless bank for separate allocation.
 */
PebbleBlue*
pebble_blue_alloc(ulong size)
{
	PebbleState *ps;
	PebbleBlue *blue;

	if(size == 0)
		return nil;

	ps = pebble_state();
	if(ps == nil)
		return nil;

	/* Check budget (state transition: COLORLESS → BLUE) */
	lock(&pebble_global_lock);
	if(ps->black_budget < size){
		unlock(&pebble_global_lock);
		return nil;  /* Insufficient budget */
	}
	ps->black_budget -= size;
	ps->blue_inuse += size;
	unlock(&pebble_global_lock);

	/* Allocate Blue structure */
	blue = mallocz(sizeof(PebbleBlue), 1);
	if(blue == nil){
		/* Rollback budget */
		lock(&pebble_global_lock);
		ps->black_budget += size;
		ps->blue_inuse -= size;
		unlock(&pebble_global_lock);
		return nil;
	}

	/* Allocate physical memory (backed by budget) */
	blue->blue_data = xallocz(size, 1);
	if(blue->blue_data == nil){
		/* Rollback budget */
		lock(&pebble_global_lock);
		ps->black_budget += size;
		ps->blue_inuse -= size;
		unlock(&pebble_global_lock);
		free(blue);
		return nil;
	}

	blue->blue_size = size;
	blue->flags = 0;

	/* Add to process Blue list */
	lock(&pebble_global_lock);
	blue->next = ps->blue_list;
	ps->blue_list = blue;
	ps->blue_count++;
	unlock(&pebble_global_lock);

	if(pebble_debug)
		print("PEBBLE: blue_alloc pid=%lud size=%lud\n", up->pid, size);

	return blue;
}

/*
 * pebble_blue_free - Free Blue token back to colorless bank
 *
 * State transition: BLUE → COLORLESS
 * Returns budget to colorless bank.
 */
int
pebble_blue_free(PebbleBlue *blue)
{
	PebbleState *ps;
	PebbleBlue **bp;
	ulong size;

	if(blue == nil)
		return 0;

	ps = pebble_state();
	if(ps == nil)
		return -1;

	size = blue->blue_size;

	/* Remove from process Blue list */
	lock(&pebble_global_lock);
	for(bp = &ps->blue_list; *bp != nil; bp = &(*bp)->next){
		if(*bp == blue){
			*bp = blue->next;
			ps->blue_count--;
			break;
		}
	}
	unlock(&pebble_global_lock);

	/* Free physical memory */
	if(blue->blue_data != nil)
		xfree(blue->blue_data);
	free(blue);

	/* Return budget to colorless bank (state transition: BLUE → COLORLESS) */
	lock(&pebble_global_lock);
	ps->black_budget += size;
	ps->blue_inuse -= size;
	unlock(&pebble_global_lock);

	if(pebble_debug)
		print("PEBBLE: blue_free pid=%lud size=%lud\n", up->pid, size);

	return 0;
}

/*
 * pebble_red_alloc - Allocate independent Red token for snapshot
 *
 * State transition: COLORLESS → RED
 * Consumes budget from colorless bank for separate allocation.
 */
PebbleRed*
pebble_red_alloc(ulong size)
{
	PebbleState *ps;
	PebbleRed *red;

	if(size == 0)
		return nil;

	ps = pebble_state();
	if(ps == nil)
		return nil;

	/* Check budget (state transition: COLORLESS → RED) */
	lock(&pebble_global_lock);
	if(ps->black_budget < size){
		unlock(&pebble_global_lock);
		return nil;  /* Insufficient budget */
	}
	ps->black_budget -= size;
	ps->red_inuse += size;
	unlock(&pebble_global_lock);

	/* Allocate Red structure */
	red = mallocz(sizeof(PebbleRed), 1);
	if(red == nil){
		/* Rollback budget */
		lock(&pebble_global_lock);
		ps->black_budget += size;
		ps->red_inuse -= size;
		unlock(&pebble_global_lock);
		return nil;
	}

	/* Allocate physical memory (backed by budget) */
	red->red_data = xallocz(size, 1);
	if(red->red_data == nil){
		/* Rollback budget */
		lock(&pebble_global_lock);
		ps->black_budget += size;
		ps->red_inuse -= size;
		unlock(&pebble_global_lock);
		free(red);
		return nil;
	}

	red->red_size = size;
	red->flags = 0;

	/* Add to process Red list */
	lock(&pebble_global_lock);
	red->next = ps->red_list;
	ps->red_list = red;
	ps->red_count++;
	unlock(&pebble_global_lock);

	if(pebble_debug)
		print("PEBBLE: red_alloc pid=%lud size=%lud\n", up->pid, size);

	return red;
}

/*
 * pebble_red_free - Free Red token back to colorless bank
 *
 * State transition: RED → COLORLESS
 * Returns budget to colorless bank.
 */
int
pebble_red_free(PebbleRed *red)
{
	PebbleState *ps;
	PebbleRed **rp;
	ulong size;

	if(red == nil)
		return 0;

	ps = pebble_state();
	if(ps == nil)
		return -1;

	size = red->red_size;

	/* Remove from process Red list */
	lock(&pebble_global_lock);
	for(rp = &ps->red_list; *rp != nil; rp = &(*rp)->next){
		if(*rp == red){
			*rp = red->next;
			ps->red_count--;
			break;
		}
	}
	unlock(&pebble_global_lock);

	/* Free physical memory */
	if(red->red_data != nil)
		xfree(red->red_data);
	free(red);

	/* Return budget to colorless bank (state transition: RED → COLORLESS) */
	lock(&pebble_global_lock);
	ps->black_budget += size;
	ps->red_inuse -= size;
	unlock(&pebble_global_lock);

	if(pebble_debug)
		print("PEBBLE: red_free pid=%lud size=%lud\n", up->pid, size);

	return 0;
}

/*
 * pebble_red_snapshot - Create Red snapshot from Blue data
 *
 * Allocates new Red token and copies Blue data to it.
 * Blue and Red are independent allocations.
 */
int
pebble_red_snapshot(PebbleBlue *blue, PebbleRed **out_red)
{
	PebbleRed *red;

	if(blue == nil || out_red == nil)
		return -1;

	/* Allocate Red token (COLORLESS → RED) */
	red = pebble_red_alloc(blue->blue_size);
	if(red == nil)
		return -1;

	/* Copy Blue data to Red */
	memmove(red->red_data, blue->blue_data, blue->blue_size);

	*out_red = red;
	return 0;
}

/* ========== Legacy API (DEPRECATED) ========== */

int
pebble_blue_exists(PebbleState *ps, PebbleBlue *blue)
{
	PebbleBlue *bp;

	if(ps == nil || blue == nil)
		return 0;

	for(bp = ps->blue_list; bp != nil; bp = bp->next)
		if(bp == blue)
			return 1;
	return 0;
}

/* REMOVED: pebble_blue_exists_locked() - coupled Blue/Red model deprecated */
/* REMOVED: pebble_has_matching_red() - coupled Blue/Red model deprecated */

/* REMOVED: pebble_duplicate_blue() - replaced by pebble_red_snapshot() */
/* REMOVED: pebble_mark_red() - coupled Blue/Red model deprecated */

int
pebble_red_copy(PebbleBlue *blue_obj, PebbleRed **red_copy)
{
	/* DEPRECATED: Use pebble_red_snapshot() instead.
	 * This wrapper maintains backward compatibility.
	 */
	if(blue_obj == nil || red_copy == nil)
		error(PEBBLE_E_BADARG);

	return pebble_red_snapshot(blue_obj, red_copy);
}

/* REMOVED: pebble_remove_red_locked() - coupled Blue/Red model deprecated */

int
pebble_blue_discard(PebbleBlue *blue_obj)
{
	/* DEPRECATED: Use pebble_blue_free() instead.
	 * This wrapper maintains backward compatibility.
	 */
	if(blue_obj == nil)
		error(PEBBLE_E_BADARG);

	return pebble_blue_free(blue_obj);
}

void
pebble_ensure_red_snapshots(PebbleState *ps)
{
	/* DEPRECATED: Blue/Red coupling removed.
	 * Blue and Red are now independent tokens managed separately.
	 * Applications must explicitly create Red snapshots via pebble_red_snapshot()
	 * when transaction safety is needed.
	 */
	USED(ps);
	return;
}

void
pebble_red_blue_exit(void)
{
	/* DEPRECATED: Blue/Red are now independent tokens, not coupled to Black.
	 * This function previously ensured Red snapshots for all Blue objects,
	 * but that coupling model has been removed.
	 *
	 * Blue/Red are only used in tests and must be managed explicitly via:
	 * - pebble_blue_alloc() / pebble_blue_free()
	 * - pebble_red_alloc() / pebble_red_free()
	 * - pebble_red_snapshot()
	 */
	return;
}

void
pebble_auto_verify(Proc *p, Ureg*)
{
	PebbleState *ps;

	if(!pebble_enabled || p == nil)
		return;
	ps = &p->pebble;
	lock(&pebble_global_lock);
	if(ps->drop_budget != 0){
		if(ps->drop_budget <= ps->black_inuse){
			ps->black_inuse -= ps->drop_budget;
			ps->black_budget += ps->drop_budget;
		}
		ps->drop_budget = 0;
	}
	unlock(&pebble_global_lock);
}

void
pebble_cleanup(Proc *p)
{
	PebbleState *ps;
	PebbleBlack *pb, *pbnext;
	PebbleBlue *blue, *bluenext;
	PebbleRed *red, *rednext;

	if(p == nil || !pebble_enabled)
		return;
	ps = &p->pebble;

	lock(&pebble_global_lock);
	pb = ps->black_list;
	ps->black_list = nil;
	blue = ps->blue_list;
	ps->blue_list = nil;
	red = ps->red_list;
	ps->red_list = nil;
	ps->black_inuse = 0;
	ps->black_budget = PEBBLE_DEFAULT_BUDGET;
	ps->white_verified = 0;
	ps->white_pending = 0;
	ps->blue_count = 0;
	ps->red_count = 0;
	unlock(&pebble_global_lock);

	for(; pb != nil; pb = pbnext){
		pbnext = pb->next;
		if(pb->physical_addr != nil)
			xfree(pb->physical_addr);
		free(pb);
	}
	for(; blue != nil; blue = bluenext){
		bluenext = blue->next;
		free(blue);
	}
	for(; red != nil; red = rednext){
		rednext = red->next;
		pebble_free_red(red);
	}

	memset(ps->whites_active, 0, sizeof(ps->whites_active));
}

void
pebble_selftest(void)
{
	PebbleState *ps;
	PebbleWhite *white;
	UserCapability black_cap;
	PebbleBlue *blue;
	PebbleRed *red;

	if(!pebble_enabled)
		return;
	ps = pebble_state();
	if(ps == nil)
		return;

	print("PEBBLE: selftest begin (pid=%lud)\n", up->pid);
	if(waserror()){
		print("PEBBLE: selftest FAIL: %s\n", up->errstr);
		poperror();
		return;
	}

	/* Test 1: White → Black allocation (circular economy) */
	white = pebble_issue_white(ps, nil, PEBBLE_MIN_ALLOC);
	if(white == nil)
		error("pebble selftest: white issue failed");

	pebble_white_verify(white, nil);
	if(pebble_black_alloc(PEBBLE_MIN_ALLOC, &black_cap) != 0)
		error("pebble selftest: black alloc failed");

	/* Test 2: Independent Blue allocation (COLORLESS → BLUE) */
	blue = pebble_blue_alloc(PEBBLE_MIN_ALLOC);
	if(blue == nil)
		error("pebble selftest: blue alloc failed");

	/* Test 3: Blue → Red snapshot (independent tokens) */
	if(pebble_red_snapshot(blue, &red) != 0)
		error("pebble selftest: red snapshot failed");
	if(red == nil)
		error("pebble selftest: red nil after snapshot");

	/* Test 4: Free all tokens back to colorless bank */
	if(pebble_red_free(red) != 0)
		error("pebble selftest: red free failed");
	if(pebble_blue_free(blue) != 0)
		error("pebble selftest: blue free failed");
	if(pebble_black_free(&black_cap) != 0)
		error("pebble selftest: black free failed");

	poperror();
	print("PEBBLE: selftest PASS (independent tokens, circular economy validated)\n");
}

void
pebble_sip_issue_test(void)
{
	PebbleWhite *white;
	UserCapability black_cap;
	PebbleBlue *blue;
	PebbleRed *red;
	PebbleState *ps;

	if(!pebble_enabled)
		return;
	ps = pebble_state();
	if(ps == nil)
		return;
	print("PEBBLE: /dev/sip/issue test begin\n");
	if(waserror()){
		print("PEBBLE: /dev/sip/issue test FAIL: %s\n", up->errstr);
		poperror();
		return;
	}

	/* Test 1: White token with larger size */
	white = pebble_issue_white(ps, nil, PEBBLE_MIN_ALLOC*2);
	if(white == nil)
		error("pebble sip issue: white issue failed");

	pebble_white_verify(white, nil);

	/* Test 2: Black allocation from white token */
	if(pebble_black_alloc(PEBBLE_MIN_ALLOC, &black_cap) != 0)
		error("pebble sip issue: black alloc failed");

	/* Test 3: Independent Blue allocation */
	blue = pebble_blue_alloc(PEBBLE_MIN_ALLOC);
	if(blue == nil)
		error("pebble sip issue: blue alloc failed");

	/* Test 4: Create Red snapshot */
	if(pebble_red_snapshot(blue, &red) != 0)
		error("pebble sip issue: red snapshot failed");
	if(red == nil)
		error("pebble sip issue: red nil");

	/* Test 5: Free all back to colorless (circular economy) */
	pebble_red_free(red);
	pebble_blue_free(blue);
	pebble_black_free(&black_cap);

	poperror();
	print("PEBBLE: /dev/sip/issue test PASS (circular economy validated)\n");
}


