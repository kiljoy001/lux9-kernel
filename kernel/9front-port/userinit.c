#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

struct initrd_file;
extern struct initrd_file *initrd_root;
extern void *initrd_base;
extern usize initrd_size;
extern void initrd_init(void*, usize);

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

	spllo();

	if(waserror())
		panic("proc0: %s", up->errstr);

	if(initrd_base != nil && initrd_root == nil) {
		print("initrd: staging module\n");
		initrd_init(initrd_base, initrd_size);
		print("BOOT[proc0]: initrd staging complete\n");
	} else if(initrd_base == nil) {
		print("initrd: no initrd module present\n");
	}

	up->pgrp = newpgrp();
	up->egrp = smalloc(sizeof(Egrp));
	up->egrp->ref = 1;
	up->fgrp = dupfgrp(nil);
	up->rgrp = newrgrp();
	print("BOOT[proc0]: process groups ready\n");

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);
	print("BOOT[proc0]: root namespace acquired\n");

	/*
	 * Setup Text and Stack segments for initcode.
	 */
	up->seg[SSEG] = newseg(SG_STACK | SG_NOEXEC, USTKTOP-USTKSIZE, USTKSIZE / BY2PG);

	/* Allocate initial stack page and map it */
	p = newpage(USTKTOP - BY2PG, nil);
	k = kmap(p);
	memset((uchar*)VA(k), 0, BY2PG);
	kunmap(k);
	segpage(up->seg[SSEG], p);
	pmap(p->pa | PTEWRITE | PTEUSER, USTKTOP - BY2PG, BY2PG);

	up->seg[TSEG] = newseg(SG_TEXT | SG_RONLY, UTZERO, 1);
	up->seg[TSEG]->flushme = 1;
	p = newpage(UTZERO, nil);
	k = kmap(p);
	memmove((uchar*)VA(k), initcode, sizeof(initcode));
	memset((uchar*)VA(k)+sizeof(initcode), 0, BY2PG-sizeof(initcode));
	kunmap(k);
	segpage(up->seg[TSEG], p);
	pmap(p->pa | PTEUSER, UTZERO, BY2PG);
	print("BOOT[proc0]: user segments populated\n");

	/*
	 * Become a user process.
	 */
	up->kp = 0;
	up->noswap = 0;
	up->privatemem = 0;
	procpriority(up, PriNormal, 0);
	procsetup(up);

	flushmmu();

	poperror();

	/*
	 * init0():
	 *	call chandevinit()
	 *	setup environment variables
	 *	prepare the stack for initcode
	 *	switch to usermode to run initcode
	 */
	print("BOOT[proc0]: about to call init0 - switching to userspace\n");
	init0();

	/* init0 will never return */
	print("BOOT[proc0]: init0 returned - this should never happen!\n");
	panic("init0");
}

void
userinit(void)
{
	up = nil;
	kstrdup(&eve, "");
	kproc("*init*", proc0, nil);
	print("BOOT[userinit]: spawned proc0 kernel process\n");
}
