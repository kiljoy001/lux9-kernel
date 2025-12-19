#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"
#include "portlib.h"
#include "tos.h"
#include "u.h"
#include <error.h>

#ifndef BOOTVERBOSE
#define BOOTVERBOSE 0
#endif
#define BOOTPRINT(...)                                                         \
  do {                                                                         \
    if (bootdebug)                                                             \
      print(__VA_ARGS__);                                                      \
  } while (0)

extern int bootdebug;

extern uintptr *mmuwalk(uintptr *, uintptr, int, int);
extern void pmap(uintptr, uintptr, vlong);
extern void userpmap(uintptr va, uintptr pa, int perms);

struct initrd_file;
extern struct initrd_file *initrd_root;
extern void *initrd_base;
extern usize initrd_size;
extern void initrd_init(void *, usize);
extern void initrd_register(void);

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
#include "initcode.i"

/*
 * The first process kernel process starts here.
 */
static void uartprint_hex(uvlong v) {
  char hex[17];
  int i;
  for (i = 15; i >= 0; i--) {
    int nib = v & 0xF;
    hex[i] = (nib < 10) ? ('0' + nib) : ('A' + nib - 10);
    v >>= 4;
  }
  hex[16] = 0;
  uartputs(hex, 16);
}

static void proc0(void *arg) {
  Proc *proc;
  int i;
  extern void main(void); /* forward decl */
  extern void uartputs(char *, int);
  uartputs("BOOT[proc0]: ENTRY via uartputs\n", 28);

  if (bootdebug)
    print("BOOT[proc0]: ENTRY symbol main=%p\n", main);
  delay(100);

  USED(arg);
  KMap *k;
  Page *p;

  /* Start logging now that we're on a real stack; clock already armed */
  /* DISABLED: prbuf_start_consumer() - causes hang before scheduler starts */
  /* prbuf_start_consumer(); */

  BOOTPRINT("proc0: ENTRY\n");

  if (waserror())
    panic("proc0: init0 failed: %r");

  extern void main(void);
  if (bootdebug)
    print("DEBUG: symbol main=%p\n", main);

  /*
   * Check if we have an initrd module from Limine.
   * We need to register it with the device root.
   */
  if (initrd_base != nil && initrd_size > 0) {
    initrd_init(initrd_base, initrd_size);
    initrd_register();
  }

  up->pgrp = newpgrp();
  up->egrp = newegrp(); /* Use newegrp() to properly initialize all fields */
  up->fgrp = dupfgrp(nil);
  up->rgrp = newrgrp();
  BOOTPRINT("BOOT[proc0]: process groups ready\n");

  pebble_selftest();

  /*
   * These are o.k. because rootinit is null.
   * Then early kproc's will have a root and dot.
   */
  if (bootdebug)
    print("BOOT[proc0]: setting up root namespace\n");

  /* Provide fallback root namespace if #/ is not available */
  if (waserror()) {
    print("BOOT[proc0]: WARNING - root device '#/' not available, creating "
          "minimal namespace\n");
    /* Create minimal namespace without actual filesystem */
    up->slash = nil;
    up->dot = nil;
    poperror();
  } else {
    /* Try to set up proper root namespace */
    print("BOOT[proc0]: invoking namec(\"#/\")...\n");
    up->slash = namec("#/", Atodir, 0, 0);
    print("BOOT[proc0]: namec(\"#/\") returned %p\n", up->slash);

    if (up->slash == nil) {
      print("BOOT[proc0]: namec(\"#/\") failed, attempting fallback to "
            "devattach('/')\n");
      up->slash = devattach('/', 0);
    }

    if (up->slash != nil) {
      pathclose(up->slash->path);
      up->slash->path = newpath("/");
      up->dot = cclone(up->slash);
    } else {
      panic("proc0: could not obtain root device via namec or devattach");
    }
    poperror();
  }

  /* Ensure /boot exists and is bound if we are in fallback mode or initrd is
   * present */
  if (initrd_base != nil && initrd_size > 0) {
    /* We already called initrd_register() which adds to devroot,
     * but we verify we can reach it */
    if (bootdebug) {
      print("BOOT[proc0]: checking /boot via namec...\n");
      Chan *bootc = namec("/boot", Atodir, 0, 0);
      print("BOOT[proc0]: namec(\"/boot\") returned %p\n", bootc);

      if (bootc == nil) {
        print("BOOT[proc0]: WARNING: /boot not reachable despite "
              "initrd_register\n");
      } else {
        cclose(bootc);
      }
    }
  }
  if (bootdebug)
    print("BOOT[proc0]: root namespace setup complete\n");
  /* pebble_sip_issue_test(); */
  BOOTPRINT("BOOT[proc0]: setting up segments\n");

  /* Clear any existing user PML4 entries to force mmucreate to build mmuhead */
  print("BOOT[proc0]: clearing existing user PML4 entries\n");
  m->pml4[PTLX(UTZERO, 3)] = 0;
  m->pml4[PTLX(USTKTOP - 1, 3)] = 0;

  /*
   * Setup Text and Stack segments for initcode.
   */
  print("BOOT[proc0]: calling newseg for stack\n");
  up->seg[SSEG] =
      newseg(SG_STACK | SG_NOEXEC, USTKTOP - USTKSIZE, USTKSIZE / BY2PG);
  print("BOOT[proc0]: newseg returned for stack\n");

  /* Allocate initial stack page and map it */
  extern void uartprint_hex(uvlong);
  uartputs("BOOT[proc0]: calling newpage\n", 25);
  p = newpage(USTKTOP - BY2PG, nil);
  uartputs("BOOT[proc0]: newpage p=\n", 22);
  uartprint_hex((uvlong)p);
  uartputs("\n", 1);

  if ((p->pa & 0xFFF) != 0) {
    uartputs("BOOT[proc0]: p->pa UNALIGNED from newpage\n", 39);
    panic("proc0: p->pa unaligned");
  }
  print("BOOT[proc0]: newpage returned %p\n", p);
  uartputs("BOOT[proc0]: calling kmap\n", 20);
  k = kmap(p);
  print("BOOT[proc0]: kmap returned %p\n", k);
  memset((uchar *)VA(k), 0, BY2PG);
  if (p->pa == 0)
    print("BOOT[proc0]: stack page pa=0 (unexpected)\n");
  else
    print("BOOT[proc0]: stack page pa nonzero\n");
  {
    char **ustack;
    uintptr user_sp;

    ustack = (char **)((uchar *)VA(k) + BY2PG - sizeof(Tos) - 8 -
                       sizeof(ustack[0]) * 4);
    user_sp = USTKTOP - sizeof(Tos) - 8 - sizeof(ustack[0]) * 4;

    ustack[3] = ustack[2] = nil;
    strcpy((char *)&ustack[4], "boot");
    ustack[1] = (char *)(user_sp + sizeof(ustack[0]) * 4);
    ustack[0] = nil;
  }
  kunmap(k);

  uartputs("BOOT[proc0]: after kmap/memmove, p=\n", 31);
  uartprint_hex((uvlong)p);
  uartputs("\n", 1);

  uartputs("BOOT[proc0]: calling segpage\n", 24);
  segpage(up->seg[SSEG], p);
  /* segpage now calls userpmap() which creates MMU structures */
  if (dbg_getpte(USTKTOP - BY2PG) != 0)
    print("BOOT[proc0]: stack pte present\n");
  else
    print("BOOT[proc0]: stack pte missing\n");

  print("BOOT[proc0]: creating text segment\n");
  up->seg[TSEG] = newseg(SG_TEXT | SG_RONLY, UTZERO, 1);
  print("BOOT[proc0]: text segment created\n");
  up->seg[TSEG]->flushme = 1;
  print("BOOT[proc0]: allocating text page\n");
  p = newpage(UTZERO, nil);
  print("BOOT[proc0]: text page allocated, p=%p\n", p);
  print("BOOT[proc0]: mapping text page\n");
  k = kmap(p);
  print("BOOT[proc0]: text page mapped, k=%p\n", k);
  if (k == nil)
    panic("proc0: kmap failed");

  /*
   * Attempt to load and executes /boot/boot if it exists and is a CLR binary.
   * If it fails or doesn't exist, we panic for now as per requirements.
   */
  print("BOOT[proc0]: Attempting to load /boot/boot\\n");
  Chan *c = namec("/boot/boot", Aopen, OEXEC, 0);
  if (c != nil) {
    print("BOOT[proc0]: /boot/boot opened successfully\\n");

    /* Get file size */
    Dir *d = dirchanstat(c);
    if (d == nil) {
      print("BOOT[proc0]: failed to stat /boot/boot\\n");
      cclose(c);
    } else {
      long n = d->length;
      free(d);

      if (bootdebug)
        print("BOOT[proc0]: allocating %ld bytes for /boot/boot\\n", n);
      void *buf = malloc(n);
      if (buf == nil)
        panic("proc0: out of memory loading /boot/boot");

      long nread = devtab[c->type]->read(c, buf, n, 0);
      cclose(c);

      if (nread != n) {
        print("BOOT[proc0]: short read on /boot/boot\\n");
        free(buf);
      } else {
        /* Check for CLR signature (MZ) */
        char *b = (char *)buf;
        if (n >= 2 && b[0] == 'M' && b[1] == 'Z') {
          print("BOOT[proc0]: CLR signature found, executing assembly...\\n");

          /* Copy to user text segment just in case, though we execute from
           * implementation */
          /* memmove((uchar *)VA(k), buf, n); */ /* Not needed if we use
                                                    interpreter directly */

          /*
           * For the interpreter, we can execute directly from the kernel buffer
           * since the interpreter runs in kernel mode for now.
           */
          extern int clr_execute_assembly(void *data, ulong size);
          int ret = clr_execute_assembly(buf, n);
          print("BOOT[proc0]: clr_execute_assembly returned %d\\n", ret);

          /* If it returns, it's an exit */
          panic("proc0: /boot/boot exited");
        } else {
          print(
              "BOOT[proc0]: /boot/boot is not a CLR binary (no MZ header)\\n");
          free(buf);
        }
      }
    }
  } else {
    print("BOOT[proc0]: failed to open /boot/boot\\n");
  }

  /* Fallback to legacy initcode if /boot/boot failed */
  print("BOOT[proc0]: falling back to legacy initcode\\n");
  memmove((uchar *)VA(k), initcode, sizeof(initcode));
  memset((uchar *)VA(k) + sizeof(initcode), 0, BY2PG - sizeof(initcode));

  print("BOOT[proc0]: unmapping text page\\n");
  kunmap(k);
  if (p->pa == 0)
    print("BOOT[proc0]: text page pa=0 (unexpected)\\n");
  else
    print("BOOT[proc0]: text page pa nonzero\\n");
  print("BOOT[proc0]: about to call segpage for text\\n");
  segpage(up->seg[TSEG], p);
  print("BOOT[proc0]: segpage for text completed\\n");
  /* segpage now calls userpmap() which creates MMU structures */
  if (dbg_getpte(UTZERO) != 0)
    print("BOOT[proc0]: text pte present\\n");
  else
    print("BOOT[proc0]: text pte missing\\n");
  print("BOOT[proc0]: user segments populated\\n");
  {
    uintptr va = USTKTOP - BY2PG;
    uintptr idx3 = PTLX(va, 3);
    print("BOOT[proc0]: checking VA=0x%llx PML4idx=%lld\\n",
          (unsigned long long)va, (long long)idx3);

    if (m->pml4 != nil) {
      print("BOOT[proc0]: m->pml4[%lld]=0x%llx\\n", (unsigned long long)idx3,
            (unsigned long long)m->pml4[idx3]);

      uintptr *lvl2_walk = mmuwalk(m->pml4, va, 2, 0);
      if (lvl2_walk != nil) {
        print("BOOT[proc0]: mmuwalk L2 present *entry=0x%llx\\n", *lvl2_walk);
      } else {
        print("BOOT[proc0]: mmuwalk L2 missing\\n");
      }

      if (mmuwalk(m->pml4, va, 1, 0) != nil)
        print("BOOT[proc0]: mmuwalk L1 present\\n");
      else
        print("BOOT[proc0]: mmuwalk L1 missing\\n");

      uintptr *lvl2_direct = &m->pml4[idx3];
      print("BOOT[proc0]: direct &m->pml4[%lld]=0x%llx\\n",
            (unsigned long long)idx3, (unsigned long long)*lvl2_direct);
      if ((*lvl2_direct & PTEVALID) != 0)
        print("BOOT[proc0]: L2 entry VALID\\n");
      else
        print("BOOT[proc0]: L2 entry INVALID\\n");

      if ((*lvl2_direct & PTEVALID) != 0) {
        uintptr *pdpt = kaddr(PPN(*lvl2_direct));
        uintptr idx2 = PTLX(va, 2);
        print("BOOT[proc0]: PDPT=0x%llx idx=%lld\\n",
              (unsigned long long)(uintptr)pdpt, (unsigned long long)idx2);
        print("BOOT[proc0]: pdpt[%lld]=0x%llx\\n", (unsigned long long)idx2,
              (unsigned long long)pdpt[idx2]);
        if (pdpt[idx2] != 0)
          print("BOOT[proc0]: PDPT entry NONZERO\\n");
        else
          print("BOOT[proc0]: PDPT entry ZERO\\n");
      }
    } else {
      print("BOOT[proc0]: m->pml4 is NIL! Cannot check user mappings.\\n");
    }
  }

  /* Detailed page table chain check */
  {
    uintptr va = USTKTOP - BY2PG;
    uintptr idx3 = PTLX(va, 3);
    if (m->pml4 != nil) {
      uintptr pml4_entry = m->pml4[idx3];
      if (pml4_entry & PTEVALID) {
        uintptr *pdpt = kaddr(PPN(pml4_entry));
        uintptr idx2 = PTLX(va, 2);
        uintptr pdpt_entry = pdpt[idx2];
        if (pdpt_entry & PTEVALID) {
          uintptr *pd = kaddr(PPN(pdpt_entry));
          uintptr idx1 = PTLX(va, 1);
          uintptr pd_entry = pd[idx1];
          if (pd_entry & PTEVALID) {
            print("BOOT[proc0]: Full chain valid to PD\\n");
          } else {
            print("BOOT[proc0]: PD entry invalid (0x%llx)\\n",
                  (unsigned long long)pd_entry);
          }
        } else {
          print("BOOT[proc0]: PDPT entry invalid (0x%llx)\\n",
                (unsigned long long)pdpt_entry);
        }
      }
    }
  }
  print("BOOT[proc0]: Exited PML4 validation, about to check USTKTOP slot\\n");

  /* Restored m->pml4 check with safety */
  if (m->pml4 != nil) {
    uintptr ustktop_idx = PTLX(USTKTOP - 1, 3);
    print("BOOT[proc0]: USTKTOP=%#p, USTKTOP-1=%#p, PTLX(USTKTOP-1,3)=%lld\\n",
          USTKTOP, USTKTOP - 1, (long long)ustktop_idx);
    print("BOOT[proc0]: About to access m->pml4[%lld]\\n",
          (long long)ustktop_idx);

    uintptr pml4_value = m->pml4[ustktop_idx];
    print("BOOT[proc0]: Read m->pml4[%lld] = %#p\\n", (long long)ustktop_idx,
          pml4_value);

    if (pml4_value != 0)
      print("BOOT[proc0]: PML4 slot before mmuswitch nonzero\\n");
    else
      print("BOOT[proc0]: PML4 slot before mmuswitch zero\\n");
  } else {
    print("BOOT[proc0]: Skipping m->pml4 check (m->pml4 is NIL)\\n");
  }

  if (up->mmuhead == nil)
    print("BOOT[proc0]: mmuhead nil (no user mappings staged)\\n");
  else {
    MMU *p;
    int count = 0;
    for (p = up->mmuhead; p != nil && count < 5; p = p->next, count++) {
      if (p->level == 2)
        print("BOOT[proc0]: mmuhead[%d] PML4E index=%d\\n", count, p->index);
      else if (p->level == 1)
        print("BOOT[proc0]: mmuhead[%d] PDPE index=%d\\n", count, p->index);
      else if (p->level == 0)
        print("BOOT[proc0]: mmuhead[%d] PDE index=%d\\n", count, p->index);
      else
        print("BOOT[proc0]: mmuhead[%d] level=%d index=%d\\n", count, p->level,
              p->index);
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
    print("userinit: about to call mmuswitch, checking mmuhead...\\n");
    if (up->mmuhead == nil)
      print("userinit: mmuhead is NULL!\\n");
    else
      print("userinit: mmuhead has entries\\n");

    int s = splhi();
    print("userinit: calling mmuswitch\\n");
    mmuswitch(up);
    print("userinit: mmuswitch returned\\n");
    splx(s);
  }

  if (m->pml4 != nil) {
    uintptr idx = PTLX(USTKTOP - 1, 3);
    if ((m->pml4[idx] & PTEVALID) != 0)
      print("BOOT[proc0]: PML4 entry valid after mmuswitch\\n");
    else
      print("BOOT[proc0]: PML4 entry still invalid after mmuswitch\\n");
    uintptr *pdpt = kaddr(PPN(m->pml4[idx]));
    uintptr idx1 = PTLX(USTKTOP - BY2PG, 2);
    if (pdpt[idx1] != 0)
      print("BOOT[proc0]: PDPT entry after mmuswitch nonzero\\n");
    else
      print("BOOT[proc0]: PDPT entry after mmuswitch zero\\n");

    if (m->pml4[PTLX(USTKTOP - 1, 3)] != 0)
      print("BOOT[proc0]: PML4 slot after mmuswitch nonzero\\n");
    else
      print("BOOT[proc0]: PML4 slot after mmuswitch still zero\\n");
  }

  if (dbg_getpte(USTKTOP - BY2PG) != 0)
    print("BOOT[proc0]: stack pte present after mmuswitch\\n");
  else
    print("BOOT[proc0]: stack pte still missing after mmuswitch\\n");
  if (dbg_getpte(UTZERO) != 0)
    print("BOOT[proc0]: text pte present after mmuswitch\\n");
  else
    print("BOOT[proc0]: text pte still missing after mmuswitch\\n");

  if (m->pml4 != nil) {
    uintptr idx = PTLX(USTKTOP - 1, 3);
    if ((m->pml4[idx] & PTEVALID) != 0) {
      uintptr *pdpt = kaddr(PPN(m->pml4[idx]));
      uintptr idx1 = PTLX(USTKTOP - BY2PG, 2);
      if (pdpt[idx1] != 0) {
        // Success - page tables are set up correctly
      }
    }

    if (m->pml4[PTLX(USTKTOP - 1, 3)] != 0) {
      if (dbg_getpte(USTKTOP - BY2PG) != 0) {
        // Stack PTE is present
      }
      if (dbg_getpte(UTZERO) != 0) {
        // Text PTE is present
      }
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
  /* Phase 6: Setup 9P exchange page for proc0 */
  if (proc_setup_p9page(up) < 0)
    panic("proc0: p9page setup failed");

  print("BOOT[proc0]: about to call init0 - switching to userspace\\n");
  init0();

  /* init0 will never return */
  print("BOOT[proc0]: init0 returned - this should never happen!\\n");
  panic("init0");
}

void userinit(void) {
  extern void uartputs(char *, int);
  uartputs("userinit: ENTRY\n", 17);
  /* Don't set up = nil here - let kproc handle process initialization */
  uartputs("userinit: about to call kproc for *init*\n", 42);
  kstrdup(&eve, "");
  kproc("*init*", proc0, nil);
  uartputs("userinit: kproc returned\n", 25);
  print("BOOT[userinit]: spawned proc0 kernel process\n");
}
