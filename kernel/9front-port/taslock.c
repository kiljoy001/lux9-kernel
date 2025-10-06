#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>
#include "edf.h"

#ifdef LOCKCYCLES
long maxlockcycles;
long maxilockcycles;
long cumlockcycles;
long cumilockcycles;
uintptr maxlockpc;
uintptr maxilockpc;
uintptr ilockpcs[0x100] = { [0xff] = 1 };
#endif

static void
lockloop(Lock *l, uintptr pc)
{
	Proc *p;

	if(panicking)
		return;

	p = l->p;
	print("lock %#p loop key %#lux pc %#p held by pc %#p proc %lud\n",
		l, l->key, pc, l->pc, p ? p->pid : 0);
	dumpaproc(up);
	if(p != nil)
		dumpaproc(p);
}

void
lock(Lock *l)
{
	int i;
	uintptr pc;

	/* Debug: print lock address */
	uintptr lock_addr = (uintptr)l;
	for(int shift = 60; shift >= 0; shift -= 4){
		int nibble = (lock_addr >> shift) & 0xF;
		char c = nibble < 10 ? '0' + nibble : 'A' + (nibble - 10);
		__asm__ volatile("outb %0, %1" : : "a"(c), "Nd"((unsigned short)0x3F8));
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)':'), "Nd"((unsigned short)0x3F8));

	__asm__ volatile("outb %0, %1" : : "a"((char)'['), "Nd"((unsigned short)0x3F8));
	pc = getcallerpc(&l);
	__asm__ volatile("outb %0, %1" : : "a"((char)']'), "Nd"((unsigned short)0x3F8));

	if(up)
		up->nlocks++;	/* prevent being scheded */
	__asm__ volatile("outb %0, %1" : : "a"((char)'{'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'T'), "Nd"((unsigned short)0x3F8));
	int tas_result = tas(&l->key);
	__asm__ volatile("outb %0, %1" : : "a"((char)'@'), "Nd"((unsigned short)0x3F8));
	if(tas_result == 0){
		__asm__ volatile("outb %0, %1" : : "a"((char)'}'), "Nd"((unsigned short)0x3F8));
		if(up)
			up->lastlock = l;
		l->pc = pc;
		l->p = up;
		l->m = MACHP(m->machno);
		l->isilock = 0;
#ifdef LOCKCYCLES
		l->lockcycles = -lcycles();
#endif
		return;
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'X'), "Nd"((unsigned short)0x3F8));
	if(up)
		up->nlocks--;
	__asm__ volatile("outb %0, %1" : : "a"((char)'Y'), "Nd"((unsigned short)0x3F8));

	for(;;){
		__asm__ volatile("outb %0, %1" : : "a"((char)'Z'), "Nd"((unsigned short)0x3F8));
		i = 0;
		while(l->key){
			if(conf.nmach < 2 && up && up->edf && (up->edf->flags & Admitted)){
				/*
				 * Priority inversion, yield on a uniprocessor; on a
				 * multiprocessor, the other processor will unlock
				 */
				print("inversion %#p pc %#p proc %lud held by pc %#p proc %lud\n",
					l, pc, up ? up->pid : 0, l->pc, l->p ? l->p->pid : 0);
				up->edf->d = µs();	/* yield to process with lock */
			}
			if(i++ > 100000000){
				i = 0;
				lockloop(l, pc);
			}
		}
		if(up)
			up->nlocks++;
		if(tas(&l->key) == 0){
			if(up)
				up->lastlock = l;
			l->pc = pc;
			l->p = up;
			l->m = MACHP(m->machno);
			l->isilock = 0;
#ifdef LOCKCYCLES
			l->lockcycles = -lcycles();
#endif
			return;
		}
		if(up)
			up->nlocks--;
	}
}

void
ilock(Lock *l)
{
	int x;
	uintptr pc;

	pc = getcallerpc(&l);

	x = splhi();
	if(tas(&l->key) != 0){
		/*
		 * Cannot also check l->pc, l->m, or l->isilock here
		 * because they might just not be set yet, or
		 * (for pc and m) the lock might have just been unlocked.
		 */
		for(;;){
			splx(x);
			while(l->key)
				;
			x = splhi();
			if(tas(&l->key) == 0)
				goto acquire;
		}
	}
acquire:
	/* Skip ilockdepth for early boot - m may not be fully initialized */
	/* m->ilockdepth++; */
	if(up)
		up->lastilock = l;
	l->sr = x;
	l->pc = pc;
	l->p = up;
	/* Skip setting l->m for now */
	/* l->m = MACHP(m->machno); */
	l->isilock = 1;
#ifdef LOCKCYCLES
	l->lockcycles = -lcycles();
#endif
}

int
canlock(Lock *l)
{
	if(up)
		up->nlocks++;
	if(tas(&l->key)){
		if(up)
			up->nlocks--;
		return 0;
	}

	if(up)
		up->lastlock = l;
	l->pc = getcallerpc(&l);
	l->p = up;
	l->m = MACHP(m->machno);
	l->isilock = 0;
#ifdef LOCKCYCLES
	l->lockcycles = -lcycles();
#endif
	return 1;
}

void
unlock(Lock *l)
{
#ifdef LOCKCYCLES
	l->lockcycles += lcycles();
	cumlockcycles += l->lockcycles;
	if(l->lockcycles > maxlockcycles){
		maxlockcycles = l->lockcycles;
		maxlockpc = l->pc;
	}
#endif
	if(l->key == 0)
		print("unlock(%#p): not locked: pc %#p\n",
			l, getcallerpc(&l));
	if(l->isilock)
		print("unlock(%#p) of ilock: pc %#p, held by %#p\n",
			l, getcallerpc(&l), l->pc);
	if(l->p != up){
		print("unlock(%#p): up changed: pc %#p, acquired at pc %#p, lock p %#p, unlock up %#p\n",
			l, getcallerpc(&l), l->pc, l->p, up);
		dumpaproc(l->p);
		dumpaproc(up);
	}
	l->m = nil;
	/* Inline mfence instead of calling coherence() */
	__asm__ volatile("mfence" ::: "memory");
	l->key = 0;

	/*
	 * Call sched if the need arose while locks were held
	 * But, don't do it from interrupt routines, hence the islo() test
	 */
	if(up && --up->nlocks == 0)
	if(up->state == Running && up->delaysched && islo())
		sched();
}

void
iunlock(Lock *l)
{
	ulong sr;


#ifdef LOCKCYCLES
	static uint n;
	l->lockcycles += lcycles();
	cumilockcycles += l->lockcycles;
	if(l->lockcycles > maxilockcycles){
		maxilockcycles = l->lockcycles;
		maxilockpc = l->pc;
	}
	if(l->lockcycles > 2400)
		ilockpcs[n++ % nelem(ilockpcs)]  = l->pc;
#endif
	if(l->key == 0){
		print("iunlock(%#p): not locked: pc %#p\n", l, getcallerpc(&l));
	}
	if(!l->isilock){
		print("iunlock(%#p) of lock: pc %#p, held by %#p\n", l, getcallerpc(&l), l->pc);
	}
	if(islo()){
		print("iunlock(%#p) while lo: pc %#p, held by %#p\n", l, getcallerpc(&l), l->pc);
	}

	sr = l->sr;
	l->m = nil;
	/* Inline mfence instead of calling coherence() */
	__asm__ volatile("mfence" ::: "memory");
	l->key = 0;
	/* Skip ilockdepth for now - test if that's the crash */
	/* m->ilockdepth--; */
	if(up)
		up->lastilock = nil;
	splx(sr);
}
