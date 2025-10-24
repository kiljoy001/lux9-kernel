#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"tos.h"
#include	"fns.h"
#include <error.h>

extern uintptr* mmuwalk(uintptr*, uintptr, int, int);
extern void pmap(uintptr, uintptr, vlong);
extern void userpmap(uintptr va, uintptr pa, int perms);

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
	/* segpage now calls userpmap() which creates MMU structures */
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
	/* segpage now calls userpmap() which creates MMU structures */
	if(dbg_getpte(UTZERO) != 0)
		print("BOOT[proc0]: text pte present\n");
	else
		print("BOOT[proc0]: text pte missing\n");
	print("BOOT[proc0]: user segments populated\n");
	{
		uintptr va = USTKTOP - BY2PG;
		uintptr idx3 = PTLX(va, 3);
		print("BOOT[proc0]: checking VA=0x%llx PML4idx=%lld\n", (unsigned long long)va, (long long)idx3);
		print("BOOT[proc0]: m->pml4[%lld]=0x%llx\n", (unsigned long long)idx3, (unsigned long long)m->pml4[idx3]);
		
		uintptr *lvl2_walk = mmuwalk(m->pml4, va, 2, 0);
		if(lvl2_walk != nil) {
			print("BOOT[proc0]: mmuwalk L2 present *entry=0x%llx\n", *lvl2_walk);
		} else {
			print("BOOT[proc0]: mmuwalk L2 missing\n");
		}
			
		if(mmuwalk(m->pml4, va, 1, 0) != nil)
			print("BOOT[proc0]: mmuwalk L1 present\n");
		else
			print("BOOT[proc0]: mmuwalk L1 missing\n");
			
		uintptr *lvl2_direct = &m->pml4[idx3];
		print("BOOT[proc0]: direct &m->pml4[%lld]=0x%llx\n", (unsigned long long)idx3, (unsigned long long)*lvl2_direct);
		if((*lvl2_direct & PTEVALID) != 0)
			print("BOOT[proc0]: L2 entry VALID\n");
		else
			print("BOOT[proc0]: L2 entry INVALID\n");
			
		if((*lvl2_direct & PTEVALID) != 0) {
			uintptr *pdpt = kaddr(PPN(*lvl2_direct));
			uintptr idx2 = PTLX(va, 2);
			print("BOOT[proc0]: PDPT=0x%llx idx=%lld\n", (unsigned long long)(uintptr)pdpt, (unsigned long long)idx2);
			print("BOOT[proc0]: pdpt[%lld]=0x%llx\n", (unsigned long long)idx2, (unsigned long long)pdpt[idx2]);
			if(pdpt[idx2] != 0)
				print("BOOT[proc0]: PDPT entry NONZERO\n");
			else
				print("BOOT[proc0]: PDPT entry ZERO\n");
		}
	}
	
	/* Detailed page table chain check */
	{
		uintptr va = USTKTOP - BY2PG;
		uintptr idx3 = PTLX(va, 3);
		uintptr pml4_entry = m->pml4[idx3];
		if(pml4_entry & PTEVALID) {
			uintptr *pdpt = kaddr(PPN(pml4_entry));
			uintptr idx2 = PTLX(va, 2);
			uintptr pdpt_entry = pdpt[idx2];
			if(pdpt_entry & PTEVALID) {
				uintptr *pd = kaddr(PPN(pdpt_entry));
				uintptr idx1 = PTLX(va, 1);
				uintptr pd_entry = pd[idx1];
				if(pd_entry & PTEVALID) {
					print("BOOT[proc0]: Full chain valid to PD\n");
				} else {
					print("BOOT[proc0]: PD entry invalid (0x%llx)\n", (unsigned long long)pd_entry);
				}
			} else {
				print("BOOT[proc0]: PDPT entry invalid (0x%llx)\n", (unsigned long long)pdpt_entry);
			}
		}
	}
	if(m->pml4[PTLX(USTKTOP-1, 3)] != 0)
		print("BOOT[proc0]: PML4 slot before mmuswitch nonzero\n");
	else
		print("BOOT[proc0]: PML4 slot before mmuswitch zero\n");
		if(up->mmuhead == nil)
			print("BOOT[proc0]: mmuhead nil (no user mappings staged)\n");
		else {
			MMU *p;
			int count = 0;
			for(p = up->mmuhead; p != nil && count < 5; p = p->next, count++) {
				if(p->level == 2)
					print("BOOT[proc0]: mmuhead[%d] PML4E index=%d\n", count, p->index);
				else if(p->level == 1)
					print("BOOT[proc0]: mmuhead[%d] PDPE index=%d\n", count, p->index);
				else if(p->level == 0)
					print("BOOT[proc0]: mmuhead[%d] PDE index=%d\n", count, p->index);
				else
					print("BOOT[proc0]: mmuhead[%d] level=%d index=%d\n", count, p->level, p->index);
			}
		}

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
		print("userinit: about to call mmuswitch, checking mmuhead...\n");
		if(up->mmuhead == nil)
			print("userinit: mmuhead is NULL!\n");
		else
			print("userinit: mmuhead has entries\n");
			
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
	{
		uintptr idx = PTLX(USTKTOP-1, 3);
		if((m->pml4[idx] & PTEVALID) != 0) {
			uintptr *pdpt = kaddr(PPN(m->pml4[idx]));
			uintptr idx1 = PTLX(USTKTOP - BY2PG, 2);
			if(pdpt[idx1] != 0) {
				// Success - page tables are set up correctly
			}
		}
	}
	if(m->pml4[PTLX(USTKTOP-1, 3)] != 0) {
		if(dbg_getpte(USTKTOP - BY2PG) != 0) {
			// Stack PTE is present
		}
		if(dbg_getpte(UTZERO) != 0) {
			// Text PTE is present
		}
	}

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
