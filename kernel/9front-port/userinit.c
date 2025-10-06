#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

/*
 * The initcode array contains the binary text of the first
 * user process. Its job is to invoke the exec system call
 * for /boot/boot.
 * Initcode does not link with standard plan9 libc _main()
 * trampoline due to size constrains. Instead it is linked
 * with a small machine specific trampoline init9.s that
 * only sets the base address register and passes arguments
 * to startboot() (see port/initcode.c).
 */
#include	"initcode.i"

/*
 * The first process kernel process starts here.
 */
static void
proc0(void*)
{
	KMap *k;
	Page *p;

	/* Entry marker */
	__asm__ volatile("outb %0, %1" : : "a"((char)'C'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));

	spllo();

	/* After spllo */
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));

	if(waserror())
		panic("proc0: %s", up->errstr);
	__asm__ volatile("outb %0, %1" : : "a"((char)'/'), "Nd"((unsigned short)0x3F8));

	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	up->pgrp = newpgrp();
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	up->egrp = smalloc(sizeof(Egrp));
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
	up->egrp->ref = 1;
	up->fgrp = dupfgrp(nil);
	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
	up->rgrp = newrgrp();
	__asm__ volatile("outb %0, %1" : : "a"((char)'5'), "Nd"((unsigned short)0x3F8));

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);
	__asm__ volatile("outb %0, %1" : : "a"((char)'7'), "Nd"((unsigned short)0x3F8));

	/*
	 * Setup Text and Stack segments for initcode.
	 */
	up->seg[SSEG] = newseg(SG_STACK | SG_NOEXEC, USTKTOP-USTKSIZE, USTKSIZE / BY2PG);
	__asm__ volatile("outb %0, %1" : : "a"((char)'8'), "Nd"((unsigned short)0x3F8));
	up->seg[TSEG] = newseg(SG_TEXT | SG_RONLY, UTZERO, 1);
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	up->seg[TSEG]->flushme = 1;
	p = newpage(UTZERO, nil);
	__asm__ volatile("outb %0, %1" : : "a"((char)'a'), "Nd"((unsigned short)0x3F8));
	k = kmap(p);
	__asm__ volatile("outb %0, %1" : : "a"((char)'b'), "Nd"((unsigned short)0x3F8));
	memmove((uchar*)VA(k), initcode, sizeof(initcode));
	__asm__ volatile("outb %0, %1" : : "a"((char)'c'), "Nd"((unsigned short)0x3F8));
	memset((uchar*)VA(k)+sizeof(initcode), 0, BY2PG-sizeof(initcode));
	kunmap(k);
	__asm__ volatile("outb %0, %1" : : "a"((char)'d'), "Nd"((unsigned short)0x3F8));
	segpage(up->seg[TSEG], p);
	__asm__ volatile("outb %0, %1" : : "a"((char)'e'), "Nd"((unsigned short)0x3F8));

	/*
	 * Become a user process.
	 */
	up->kp = 0;
	up->noswap = 0;
	up->privatemem = 0;
	procpriority(up, PriNormal, 0);
	__asm__ volatile("outb %0, %1" : : "a"((char)'f'), "Nd"((unsigned short)0x3F8));
	procsetup(up);
	__asm__ volatile("outb %0, %1" : : "a"((char)'g'), "Nd"((unsigned short)0x3F8));

	flushmmu();
	__asm__ volatile("outb %0, %1" : : "a"((char)'h'), "Nd"((unsigned short)0x3F8));

	poperror();
	__asm__ volatile("outb %0, %1" : : "a"((char)'i'), "Nd"((unsigned short)0x3F8));

	/*
	 * init0():
	 *	call chandevinit()
	 *	setup environment variables
	 *	prepare the stack for initcode
	 *	switch to usermode to run initcode
	 */
	print("proc0: about to call init0 - switching to userspace\n");
	init0();
	__asm__ volatile("outb %0, %1" : : "a"((char)'j'), "Nd"((unsigned short)0x3F8));

	/* init0 will never return */
	print("proc0: init0 returned - this should never happen!\n");
	panic("init0");
}

void
userinit(void)
{
	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	up = nil;
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	kstrdup(&eve, "");
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
	kproc("*init*", proc0, nil);
	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
}
