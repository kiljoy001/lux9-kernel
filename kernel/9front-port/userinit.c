#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"tos.h"
#include	"fns.h"
#include <error.h>

extern uintptr* mmuwalk(uintptr*, uintptr, int, int);
extern void pmap(uintptr, uintptr, vlong);

struct initrd_file;
extern struct initrd_file *initrd_root;
extern void *initrd_base;
extern usize initrd_size;
extern void initrd_init(void*, usize);

uintptr dbg_getpte(uintptr);

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
	if(p->pa == 0)
		print("BOOT[proc0]: stack page pa=0 (unexpected)\n");
	else
		print("BOOT[proc0]: stack page pa nonzero\n");
	{
		char **ustack;
		uintptr user_sp;

		ustack = (char**)((uchar*)VA(k) + BY2PG - sizeof(Tos) - 8 - sizeof(ustack[0])*4);
		user_sp = USTKTOP - sizeof(Tos) - 8 - sizeof(ustack[0])*4;

		ustack[3] = ustack[2] = nil;
		strcpy((char*)&ustack[4], "boot");
		ustack[1] = (char*)(user_sp + sizeof(ustack[0])*4);
		ustack[0] = nil;
	}
	kunmap(k);
	segpage(up->seg[SSEG], p);
	if(mmuwalk(m->pml4, USTKTOP - BY2PG, 1, 1) == nil)
		panic("proc0: mmuwalk stack");
	putmmu(USTKTOP - BY2PG, p->pa | PTEWRITE | PTEVALID | PTENOEXEC, p);
	if(dbg_getpte(USTKTOP - BY2PG) != 0)
		print("BOOT[proc0]: stack pte present\n");
	else
		print("BOOT[proc0]: stack pte missing\n");

	up->seg[TSEG] = newseg(SG_TEXT | SG_RONLY, UTZERO, 1);
	up->seg[TSEG]->flushme = 1;
	p = newpage(UTZERO, nil);
	k = kmap(p);
	memmove((uchar*)VA(k), initcode, sizeof(initcode));
	memset((uchar*)VA(k)+sizeof(initcode), 0, BY2PG-sizeof(initcode));
	kunmap(k);
	if(p->pa == 0)
		print("BOOT[proc0]: text page pa=0 (unexpected)\n");
	else
		print("BOOT[proc0]: text page pa nonzero\n");
	segpage(up->seg[TSEG], p);
	if(mmuwalk(m->pml4, UTZERO, 1, 1) == nil)
		panic("proc0: mmuwalk text");
	putmmu(UTZERO, p->pa | PTEVALID, p);
	if(dbg_getpte(UTZERO) != 0)
		print("BOOT[proc0]: text pte present\n");
	else
		print("BOOT[proc0]: text pte missing\n");
	print("BOOT[proc0]: user segments populated\n");
	if(mmuwalk(m->pml4, USTKTOP - BY2PG, 2, 0) != nil)
		print("BOOT[proc0]: mmuwalk level 2 present\n");
	else
		print("BOOT[proc0]: mmuwalk level 2 missing\n");
	if(mmuwalk(m->pml4, USTKTOP - BY2PG, 1, 0) != nil)
		print("BOOT[proc0]: mmuwalk level 1 present\n");
	else
		print("BOOT[proc0]: mmuwalk level 1 missing\n");
	{
		uintptr *lvl2 = mmuwalk(m->pml4, USTKTOP - BY2PG, 2, 0);
		if(lvl2 != nil){
			if((*lvl2 & PTEVALID) != 0)
				print("BOOT[proc0]: level 2 entry marked valid\n");
			else
				print("BOOT[proc0]: level 2 entry NOT valid\n");
			uintptr *pdpt = kaddr(PPN(*lvl2));
			uintptr idx1 = PTLX(USTKTOP - BY2PG, 2);
			if(pdpt[idx1] != 0)
				print("BOOT[proc0]: PDPT entry nonzero\n");
			else
				print("BOOT[proc0]: PDPT entry zero\n");
		}
	}
	if(m->pml4[PTLX(USTKTOP-1, 3)] != 0)
		print("BOOT[proc0]: PML4 slot before mmuswitch nonzero\n");
	else
		print("BOOT[proc0]: PML4 slot before mmuswitch zero\n");
	if(up->mmuhead == nil)
		print("BOOT[proc0]: mmuhead nil (no user mappings staged)\n");
	else if(up->mmuhead->level == 2)
		print("BOOT[proc0]: mmuhead level 2 (PML4E)\n");
	else if(up->mmuhead->level == 1)
		print("BOOT[proc0]: mmuhead level 1 (PDPE)\n");
	else if(up->mmuhead->level == 0)
		print("BOOT[proc0]: mmuhead level 0 (PDE)\n");
	else
		print("BOOT[proc0]: mmuhead level unexpected\n");

	/*
	 * Become a user process.
	 */
	up->kp = 0;
	up->noswap = 0;
	up->privatemem = 0;
	procpriority(up, PriNormal, 0);
	procsetup(up);

	/* Install user mappings now that proc0 drops kernel privileges */
	{
		int s = splhi();
		mmuswitch(up);
		splx(s);
	}
	{
		uintptr idx = PTLX(USTKTOP-1, 3);
		if((m->pml4[idx] & PTEVALID) != 0)
			print("BOOT[proc0]: PML4 entry valid after mmuswitch\n");
		else
			print("BOOT[proc0]: PML4 entry still invalid after mmuswitch\n");
		uintptr *pdpt = kaddr(PPN(m->pml4[idx]));
		uintptr idx1 = PTLX(USTKTOP - BY2PG, 2);
		if(pdpt[idx1] != 0)
			print("BOOT[proc0]: PDPT entry after mmuswitch nonzero\n");
		else
			print("BOOT[proc0]: PDPT entry after mmuswitch zero\n");
	}
	if(m->pml4[PTLX(USTKTOP-1, 3)] != 0)
		print("BOOT[proc0]: PML4 slot after mmuswitch nonzero\n");
	else
		print("BOOT[proc0]: PML4 slot after mmuswitch still zero\n");
	if(dbg_getpte(USTKTOP - BY2PG) != 0)
		print("BOOT[proc0]: stack pte present after mmuswitch\n");
	else
		print("BOOT[proc0]: stack pte still missing after mmuswitch\n");
	if(dbg_getpte(UTZERO) != 0)
		print("BOOT[proc0]: text pte present after mmuswitch\n");
	else
		print("BOOT[proc0]: text pte still missing after mmuswitch\n");

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
