#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <io.h>
#include "ureg.h"
#include "tos.h"

struct Timers
{
	Lock	lk;
	Timer	*head;
};

static Timers timers[MAXMACH];

ulong intrcount[MAXMACH];
ulong fcallcount[MAXMACH];

static vlong
tadd(Timers *tt, Timer *nt)
{
	Timer *t, **last;

	/* Called with tt locked */
	assert(nt->tt == nil);
	switch(nt->tmode){
	default:
		panic("timer");
		break;
	case Trelative:
		if(nt->tns <= 0)
			nt->tns = 1;
		nt->twhen = fastticks(nil) + ns2fastticks(nt->tns);
		break;
	case Tperiodic:
		assert(nt->tns >= 100000);	/* At least 100 µs period */
		if(nt->twhen == 0){
			/* look for another timer at same frequency for combining */
			for(t = tt->head; t; t = t->tnext){
				if(t->tmode == Tperiodic && t->tns == nt->tns)
					break;
			}
			if (t)
				nt->twhen = t->twhen;
			else
				nt->twhen = fastticks(nil);
		}
		nt->twhen += ns2fastticks(nt->tns);
		break;
	}

	for(last = &tt->head; t = *last; last = &t->tnext){
		if(t->twhen > nt->twhen)
			break;
	}
	nt->tnext = *last;
	*last = nt;
	nt->tt = tt;
	if(last == &tt->head)
		return nt->twhen;
	return 0;
}

static uvlong
tdel(Timer *dt)
{

	Timer *t, **last;
	Timers *tt;

	tt = dt->tt;
	if (tt == nil)
		return 0;
	for(last = &tt->head; t = *last; last = &t->tnext){
		if(t == dt){
			assert(dt->tt);
			dt->tt = nil;
			*last = t->tnext;
			break;
		}
	}
	if(last == &tt->head && tt->head)
		return tt->head->twhen;
	return 0;
}

/* add or modify a timer */
void
timeradd(Timer *nt)
{
	Timers *tt;
	vlong when;

	/* Must lock Timer struct before Timers struct */
	ilock(&nt->lock);
	if(tt = nt->tt){
		ilock(&tt->lk);
		tdel(nt);
		iunlock(&tt->lk);
	}
	tt = &timers[m->machno];
	ilock(&tt->lk);
	when = tadd(tt, nt);
	if(when)
		timerset(when);
	iunlock(&tt->lk);
	iunlock(&nt->lock);
}


void
timerdel(Timer *dt)
{
	Mach *mp;
	Timers *tt;
	uvlong when;

	/* avoid Tperiodic getting re-added */
	dt->tmode = Trelative;

	ilock(dt);
	if(tt = dt->tt){
		ilock(&tt->lk);
		when = tdel(dt);
		if(when && tt == &timers[m->machno]){
			timerset(tt->head->twhen);
		}
		iunlock(&tt->lk);
	}
	if((mp = dt->tactive) == nil || mp->machno == m->machno){
		iunlock(dt);
		return;
	}
	iunlock(dt);

	/* rare, but tf can still be active on another cpu */
	while(dt->tactive == mp && dt->tt == nil)
		if(up->state == Running && up->nlocks == 0 && islo())
			sched();
}

void
hzclock(Ureg *ur)
{
	m->ticks++;
	if(m->proc)
		m->proc->pc = ur->pc;

	if(m->flushmmu){
		if(up && up->newtlb)
			flushmmu();
		m->flushmmu = 0;
	}

	accounttime();
	dtracytick(ur);
	kmapinval();

	if(kproftimer != nil)
		kproftimer(ur->pc);

	if(active.machs[m->machno] == 0)
		return;

	if(active.exiting)
		exit(panicking);

	if(m->machno == 0)
		checkalarms();

	if(up && up->state == Running){
		if(userureg(ur)){
			/* user profiling clock */
			Tos *tos = (Tos*)(USTKTOP-sizeof(Tos));
			tos->clock += TK2MS(1);
			segclock(ur->pc);
		}

		hzsched();	/* in proc.c */
	}
}

void
timerintr(Ureg *u, Tval)
{
	Timer *t;
	Timers *tt;
	uvlong when, now;
	int callhzclock;

	intrcount[m->machno]++;
	callhzclock = 0;
	tt = &timers[m->machno];
	now = fastticks(nil);
	ilock(&tt->lk);
	while(t = tt->head){
		/*
		 * No need to ilock t here: any manipulation of t
		 * requires tdel(t) and this must be done with a
		 * lock to tt held.  We have tt, so the tdel will
		 * wait until we're done
		 */
		when = t->twhen;
		if(when > now){
			timerset(when);
			iunlock(&tt->lk);
			if(callhzclock)
				hzclock(u);
			return;
		}
		tt->head = t->tnext;
		assert(t->tt == tt);
		t->tt = nil;
		t->tactive = MACHP(m->machno);
		fcallcount[m->machno]++;
		iunlock(&tt->lk);
		if(t->tf)
			(*t->tf)(u, t);
		else
			callhzclock++;
		t->tactive = nil;
		ilock(&tt->lk);
		if(t->tmode == Tperiodic)
			tadd(tt, t);
	}
	iunlock(&tt->lk);
}

void
timersinit(void)
{
	Timer *t;

	print("timersinit: ENTRY\n");

	/*
	 * T->tf == nil means the HZ clock for this processor.
	 */
	print("timersinit: calling todinit\n");
	todinit();
	print("timersinit: todinit complete, calling xalloc\n");

	t = xalloc(sizeof(*t));
	if(t == nil)
		panic("timersinit: no memory for Timer");
	print("timersinit: xalloc complete, setting up timer\n");

	t->tmode = Tperiodic;
	t->tt = nil;
	t->tns = 1000000000/HZ;
	t->tf = nil;
	/* Debug: check lock key before timeradd */
	{
		Lock *lk = (Lock*)t;
		if(lk->key != 0) {
		}
	}
	print("timersinit: calling timeradd\n");
	timeradd(t);
	print("timersinit: timeradd complete, DONE\n");
}

Timer*
addclock0link(void (*f)(void), int ms)
{
	Timer *nt;
	uvlong when;

	print("addclock0link: ENTRY (ms=%d)\n", ms);

	/* Synchronize to hztimer if ms is 0 */
	nt = xalloc(sizeof(Timer));
	if(nt == nil)
		panic("addclock0link: no memory for Timer");
	print("addclock0link: xalloc complete\n");

	if(ms == 0)
		ms = 1000/HZ;
	nt->tns = (vlong)ms*1000000LL;
	nt->tmode = Tperiodic;
	nt->tt = nil;
	nt->tf = (void (*)(Ureg*, Timer*))f;

	print("addclock0link: acquiring timers[0].lk\n");
	ilock(&timers[0].lk);
	print("addclock0link: lock acquired, calling tadd\n");
	when = tadd(&timers[0], nt);
	print("addclock0link: tadd returned when=%lld\n", when);
	if(when){
		print("addclock0link: calling timerset\n");
		timerset(when);
		print("addclock0link: timerset returned\n");
	}
	iunlock(&timers[0].lk);
	print("addclock0link: DONE\n");
	return nt;
}

/*
 *  This tk2ms avoids overflows that the macro version is prone to.
 *  It is a LOT slower so shouldn't be used if you're just converting
 *  a delta.
 */
ulong
tk2ms(ulong ticks)
{
	uvlong t, hz;

	t = ticks;
	hz = HZ;
	t *= 1000L;
	t = t/hz;
	ticks = t;
	return ticks;
}

ulong
ms2tk(ulong ms)
{
	/* avoid overflows at the cost of precision */
	if(ms >= 1000000000/HZ)
		return (ms/1000)*HZ;
	return (ms*HZ+500)/1000;
}
