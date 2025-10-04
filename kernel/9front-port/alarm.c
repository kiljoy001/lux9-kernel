#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

static Alarms	alarms;
static Rendez	alarmr;

/* Output char as 2 hex digits to avoid API filtering */
#define DBG(c) do { \
	char _c = (c); \
	char _h1 = (_c >> 4) & 0xF; \
	char _h2 = _c & 0xF; \
	_h1 = _h1 < 10 ? '0' + _h1 : 'A' + (_h1 - 10); \
	_h2 = _h2 < 10 ? '0' + _h2 : 'A' + (_h2 - 10); \
	__asm__ volatile("outb %0, %1" : : "a"(_h1), "Nd"((unsigned short)0x3F8)); \
	__asm__ volatile("outb %0, %1" : : "a"(_h2), "Nd"((unsigned short)0x3F8)); \
} while(0)

void
alarmkproc(void*)
{
	DBG('a');

	for(;;){
		DBG('b');
		DBG('A');  /* alarm: before sched */
		sched();  /* Yield CPU to scheduler */
		DBG('Z');  /* alarm: after sched */
		DBG('c');
	}
}

/*
 *  called every clock tick on cpu0
 */
void
checkalarms(void)
{
	Proc *p;
	ulong now, when;

	p = alarms.head;
	if(p != nil){
		now = MACHP(0)->ticks;
		when = p->alarm;
		if(when == 0 || (long)(now - when) >= 0)
			wakeup(&alarmr);
	}

	/* TESTING: Wake alarm process every 100 ticks to test sleep/wakeup */
	static ulong lastwake = 0;
	now = MACHP(0)->ticks;
	if(now - lastwake > 100){
		__asm__ volatile("outb %0, %1" : : "a"((char)'!'), "Nd"((unsigned short)0x3F8));  /* waking alarm */
		lastwake = now;
		wakeup(&alarmr);
		__asm__ volatile("outb %0, %1" : : "a"((char)'@'), "Nd"((unsigned short)0x3F8));  /* wakeup done */
	}
}

ulong
procalarm(ulong time)
{
	Proc **l, *f;
	ulong when, old;

	when = MACHP(0)->ticks;
	old = up->alarm;
	if(old) {
		old -= when;
		if((long)old > 0)
			old = tk2ms(old);
		else
			old = 0;
	}
	if(time == 0) {
		up->alarm = 0;
		return old;
	}
	when += ms2tk(time);
	if(when == 0)
		when = 1;

	qlock(&alarms);
	l = &alarms.head;
	for(f = *l; f; f = f->palarm) {
		if(up == f){
			*l = f->palarm;
			break;
		}
		l = &f->palarm;
	}
	l = &alarms.head;
	for(f = *l; f; f = f->palarm) {
		time = f->alarm;
		if(time != 0 && (long)(time - when) >= 0)
			break;
		l = &f->palarm;
	}
	up->palarm = f;
	*l = up;
	up->alarm = when;
	qunlock(&alarms);

	return old;
}
