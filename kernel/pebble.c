#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"pebble.h"

Lock pebble_global_lock;
int pebble_enabled = 1;
int pebble_debug = PEBBLE_DEBUG;

static int pebble_initialized;

static void pebble_free_red(PebbleRed*);
static PebbleRed* pebble_detach_blue_locked(PebbleState*, PebbleBlue*);
static int pebble_blue_exists_locked(PebbleState*, PebbleBlue*);

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

	if(ps == nil)
		return nil;

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
		ps->whites[idx].size = size;
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
pebble_black_alloc(uintptr size, void **handle)
{
	PebbleState *ps;
	PebbleBlack *pb;
	PebbleBlue *blue;
	void *buf;

	if(handle == nil)
		error(PEBBLE_E_BADARG);
	if(size < PEBBLE_MIN_ALLOC || size > PEBBLE_MAX_ALLOC)
		error(PEBBLE_E_BADARG);

	ps = pebble_state();
	if(ps == nil)
		error(PEBBLE_E_PERM);

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

	pb = mallocz(sizeof(PebbleBlack), 1);
	if(pb == nil){
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

	blue = mallocz(sizeof(PebbleBlue), 1);
	if(blue == nil){
		free(pb);
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

	pb->addr = buf;
	pb->size = size;
	pb->flags = PEBBLE_CAP_BLACK | PEBBLE_CAP_ACTIVE;
	pb->blue = blue;

	blue->owner = pb;
	blue->blue_data = buf;
	blue->blue_size = size;
	blue->matching_red = nil;

	lock(&pebble_global_lock);
	pb->next = ps->black_list;
	ps->black_list = pb;

	blue->next = ps->blue_list;
	ps->blue_list = blue;
	ps->blue_count++;
	unlock(&pebble_global_lock);

	*handle = pb;
	if(pebble_debug)
		print("PEBBLE: black alloc pid=%lud handle=%#p size=%lud\n",
			up->pid, pb, size);
	return 0;
}

int
pebble_black_free(void *handle)
{
	PebbleState *ps;
	PebbleBlack *pb, **pp;
	PebbleBlue *blue;
	PebbleRed *red;
	ulong size;

	if(handle == nil)
		error(PEBBLE_E_BADARG);

	ps = pebble_state();
	if(ps == nil)
		error(PEBBLE_E_PERM);

	lock(&pebble_global_lock);
	pb = pebble_lookup_black_locked(ps, handle);
	if(pb == nil){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM);
	}
	if(pb->blue != nil && pb->blue->matching_red == nil){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_BUSY);
	}

	size = pb->size;
	for(pp = &ps->black_list; *pp != nil; pp = &(*pp)->next){
		if(*pp == pb){
			*pp = pb->next;
			break;
		}
	}
	blue = pb->blue;
	red = nil;
	if(blue != nil){
		red = pebble_detach_blue_locked(ps, blue);
		if(pb == blue->owner)
			blue->owner = nil;
	}
	ps->black_inuse -= size;
	ps->black_budget += size;
	ps->total_frees++;
	unlock(&pebble_global_lock);

	if(red != nil)
		pebble_free_red(red);
	if(blue != nil)
		free(blue);
	if(pb->addr != nil)
		xfree(pb->addr);
	free(pb);

	if(pebble_debug)
		print("PEBBLE: black free pid=%lud size=%lud\n", up->pid, size);
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

static PebbleRed*
pebble_detach_blue_locked(PebbleState *ps, PebbleBlue *blue)
{
	PebbleBlue **bp;
	PebbleRed **rp, *red;

	if(blue == nil)
		return nil;

	for(bp = &ps->blue_list; *bp != nil; bp = &(*bp)->next){
		if(*bp == blue){
			*bp = blue->next;
			ps->blue_count--;
			break;
		}
	}
	red = blue->matching_red;
	if(red != nil){
		for(rp = &ps->red_list; *rp != nil; rp = &(*rp)->next){
			if(*rp == red){
				*rp = red->next;
				ps->red_count--;
				break;
			}
		}
		blue->matching_red = nil;
	}
	return red;
}

static void
pebble_free_red(PebbleRed *red)
{
	if(red == nil)
		return;
	if(red->red_data != nil)
		xfree(red->red_data);
	free(red);
}

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

static int
pebble_blue_exists_locked(PebbleState *ps, PebbleBlue *blue)
{
	PebbleBlue *bp;

	if(ps == nil || blue == nil)
		return 0;
	for(bp = ps->blue_list; bp != nil; bp = bp->next)
		if(bp == blue)
			return 1;
	return 0;
}

int
pebble_has_matching_red(PebbleState *, PebbleBlue *blue)
{
	return blue != nil && blue->matching_red != nil;
}

PebbleRed*
pebble_duplicate_blue(PebbleState *, PebbleBlue *blue)
{
	PebbleRed *red;

	if(blue == nil)
		return nil;

	red = mallocz(sizeof(PebbleRed), 1);
	if(red == nil)
		return nil;
	red->red_data = xallocz(blue->blue_size, 1);
	if(red->red_data == nil){
		free(red);
		return nil;
	}
	memmove(red->red_data, blue->blue_data, blue->blue_size);
	red->red_size = blue->blue_size;
	return red;
}

void
pebble_mark_red(PebbleState *ps, PebbleBlue *blue, PebbleRed *red)
{
	if(ps == nil || blue == nil || red == nil)
		return;

	red->next = ps->red_list;
	ps->red_list = red;
	ps->red_count++;

	blue->matching_red = red;
	if(blue->owner != nil){
		PebbleBlack *pb = blue->owner;
		pb->red = red;
	}
}

int
pebble_red_copy(PebbleBlue *blue_obj, PebbleRed **red_copy)
{
	PebbleState *ps;
	PebbleRed *red, *existing;

	if(blue_obj == nil || red_copy == nil)
		error(PEBBLE_E_BADARG);

	ps = pebble_state();
	if(ps == nil)
		error(PEBBLE_E_PERM);

	lock(&pebble_global_lock);
	if(!pebble_blue_exists_locked(ps, blue_obj)){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM);
	}
	if(blue_obj->matching_red != nil){
		red = blue_obj->matching_red;
		unlock(&pebble_global_lock);
		*red_copy = red;
		return 0;
	}
	unlock(&pebble_global_lock);

	red = pebble_duplicate_blue(ps, blue_obj);
	if(red == nil)
		error(PEBBLE_E_NOMEM);

	lock(&pebble_global_lock);
	if(!pebble_blue_exists_locked(ps, blue_obj)){
		unlock(&pebble_global_lock);
		pebble_free_red(red);
		error(PEBBLE_E_PERM);
	}
	if(blue_obj->matching_red != nil){
		existing = blue_obj->matching_red;
		unlock(&pebble_global_lock);
		pebble_free_red(red);
		*red_copy = existing;
		return 0;
	}
	pebble_mark_red(ps, blue_obj, red);
	unlock(&pebble_global_lock);

	*red_copy = red;
	if(pebble_debug)
		print("PEBBLE: red copy pid=%lud blue=%#p red=%#p\n",
			up->pid, blue_obj, red);
	return 0;
}

static void
pebble_remove_red_locked(PebbleState *ps, PebbleRed *red)
{
	PebbleRed **rp;

	if(red == nil)
		return;
	for(rp = &ps->red_list; *rp != nil; rp = &(*rp)->next){
		if(*rp == red){
			*rp = red->next;
			ps->red_count--;
			break;
		}
	}
}

int
pebble_blue_discard(PebbleBlue *blue_obj)
{
	PebbleState *ps;
	PebbleRed *red;

	if(blue_obj == nil)
		error(PEBBLE_E_BADARG);

	ps = pebble_state();
	if(ps == nil)
		error(PEBBLE_E_PERM);

	lock(&pebble_global_lock);
	if(!pebble_blue_exists(ps, blue_obj)){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_PERM);
	}
	if(blue_obj->matching_red == nil){
		unlock(&pebble_global_lock);
		error(PEBBLE_E_BUSY);
	}

	red = pebble_detach_blue_locked(ps, blue_obj);
	if(red != nil)
		pebble_remove_red_locked(ps, red);
	if(blue_obj->owner != nil){
		PebbleBlack *pb = blue_obj->owner;
		pb->blue = nil;
		pb->red = nil;
	}
	unlock(&pebble_global_lock);

	pebble_free_red(red);
	free(blue_obj);
	if(pebble_debug)
		print("PEBBLE: blue discard pid=%lud\n", up->pid);
	return 0;
}

void
pebble_ensure_red_snapshots(PebbleState *ps)
{
	PebbleBlue *blue, **pending;
	int count, i;
	PebbleRed *red;

	if(ps == nil)
		return;

	lock(&pebble_global_lock);
	count = 0;
	for(blue = ps->blue_list; blue != nil; blue = blue->next)
		if(blue->matching_red == nil)
			count++;
	unlock(&pebble_global_lock);

	if(count == 0)
		return;

	pending = malloc(count * sizeof(PebbleBlue*));
	if(pending == nil){
		if(pebble_debug)
			print("PEBBLE: ensure_red_snapshots: no memory for pending list\n");
		return;
	}

	lock(&pebble_global_lock);
	i = 0;
	for(blue = ps->blue_list; blue != nil && i < count; blue = blue->next)
		if(blue->matching_red == nil)
			pending[i++] = blue;
	unlock(&pebble_global_lock);

	count = i;
	for(i = 0; i < count; i++){
		if(pending[i] == nil)
			continue;
		if(waserror()){
			if(pebble_debug)
				print("PEBBLE: ensure_red_snapshots failed: %s\n", up != nil ? up->errstr : "no proc");
			poperror();
			continue;
		}
		red = nil;
		pebble_red_copy(pending[i], &red);
		USED(red);
		poperror();
	}
	free(pending);
}

void
pebble_red_blue_exit(void)
{
	PebbleState *ps;

	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	if(!pebble_enabled) {
		__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
		return;
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
	ps = pebble_state();
	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
	if(ps == nil) {
		__asm__ volatile("outb %0, %1" : : "a"((char)'5'), "Nd"((unsigned short)0x3F8));
		return;
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));
	pebble_ensure_red_snapshots(ps);
	__asm__ volatile("outb %0, %1" : : "a"((char)'7'), "Nd"((unsigned short)0x3F8));
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
		if(pb->addr != nil)
			xfree(pb->addr);
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
	PebbleBlack *black;
	PebbleBlue *blue;
	PebbleRed *red;
	void *handle;

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

	white = pebble_issue_white(ps, nil, PEBBLE_MIN_ALLOC);
	if(white == nil)
		error(PEBBLE_E_AGAIN);

	handle = nil;
	pebble_white_verify(white, &handle);
	pebble_black_alloc(PEBBLE_MIN_ALLOC, &handle);

	black = handle;
	if(black == nil)
		error("pebble selftest: black handle nil");

	blue = black->blue;
	if(blue == nil)
		error("pebble selftest: blue missing");

	red = nil;
	pebble_red_copy(blue, &red);
	if(red == nil)
		error("pebble selftest: red missing");

	pebble_blue_discard(blue);
	pebble_black_free(black);

	poperror();
	print("PEBBLE: selftest PASS\n");
}

void
pebble_sip_issue_test(void)
{
	PebbleWhite *white;
	PebbleBlack *pb;
	PebbleRed *red;
	PebbleState *ps;
	void *hint;
	void *handle;

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

	white = pebble_issue_white(ps, nil, PEBBLE_MIN_ALLOC*2);
	if(white == nil)
		error(PEBBLE_E_AGAIN);

	hint = nil;
	pebble_white_verify(white, &hint);

	handle = nil;
	pebble_black_alloc(PEBBLE_MIN_ALLOC, &handle);
	pb = handle;
	if(pb == nil)
		error("pebble sip issue: black alloc nil");
	if(pb->blue == nil)
		error("pebble sip issue: blue missing");

	red = nil;
	pebble_red_copy(pb->blue, &red);
	if(red == nil)
		error("pebble sip issue: red missing");

	pebble_blue_discard(pb->blue);
	pebble_black_free(pb);

	poperror();
	print("PEBBLE: /dev/sip/issue test PASS\n");
}


