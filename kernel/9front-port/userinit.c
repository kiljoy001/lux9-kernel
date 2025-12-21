#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"
#include "portlib.h"
#include "tos.h"
#include "u.h"
#include <a.out.h>
#include <error.h>

#ifndef BOOTVERBOSE
#define BOOTVERBOSE 0
#endif
#define BOOTPRINT(...)                                                         \
  do {                                                                         \
    if (BOOTVERBOSE)                                                           \
      print(__VA_ARGS__);                                                      \
  } while (0)

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

  print("BOOT[proc0]: ENTRY symbol main=%p\n", main);
  delay(100);

  USED(arg);
  KMap *k;
  Page *p;

  /* Grant initial Pebble budget for proc0 bootstrap */
  /* This must happen before any newpage() calls */
  up->pebble.colorless_bank = PEBBLE_INIT_BUDGET;
  print("PEBBLE: granted %dMB init budget to proc0\n",
        PEBBLE_INIT_BUDGET / (1024 * 1024));

  /* Start logging now that we're on a real stack; clock already armed */
  /* DISABLED: prbuf_start_consumer() - causes hang before scheduler starts */
  /* prbuf_start_consumer(); */

  BOOTPRINT("proc0: ENTRY\n");

  if (waserror())
    panic("proc0: init0 failed: %r");

  extern void main(void);
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

  /* Run WASM Pipeline Validation */
  extern void clr_init(void);
  clr_init();

  /*
   * These are o.k. because rootinit is null.
   * Then early kproc's will have a root and dot.
   */
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
    up->slash = namec("#/", Atodir, 0, 0);
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
    Chan *bootc = namec("/boot", Atodir, 0, 0);
    if (bootc == nil) {
      print("BOOT[proc0]: WARNING: /boot not reachable despite "
            "initrd_register\n");
    } else {
      cclose(bootc);
    }
  }
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

  /* Try to load /boot/init (CLR) first, then /boot/boot */
  Chan *bc = namec("/boot/init", Aopen, OREAD, 0);
  if (bc == nil)
    bc = namec("/boot/boot", Aopen, OREAD, 0);

  int loaded = 0;

  if (bc != nil) {
    Exec exec;
    if (!waserror()) {
      print("BOOT[proc0]: Found /boot/boot, checking header...\n");
      if (cread(bc, (uchar *)&exec, sizeof(Exec), 0) == sizeof(Exec)) {
        /* Accept S_MAGIC (amd64) or A_MAGIC (legacy) */
        if (exec.magic == S_MAGIC || exec.magic == A_MAGIC) {
          print("BOOT[proc0]: Loading CLR from /boot/boot (text=%d data=%d)\n",
                exec.text, exec.data);

          ulong total_len = exec.text + exec.data + exec.bss;
          ulong total_pages = (total_len + BY2PG - 1) / BY2PG;

          /* Create TSEG large enough for everything.
           * Removing SG_RONLY to allow data writes if needed by simple binaries
           */
          print("BOOT[proc0]: Creating TSEG size=%ld pages\n", total_pages);
          up->seg[TSEG] = newseg(SG_TEXT, UTZERO, total_pages);
          up->seg[TSEG]->flushme = 1;

          ulong file_off = sizeof(Exec); /* Skip 32-byte header */
          ulong virt_addr = UTZERO;
          ulong remaining = exec.text + exec.data;

          for (int i = 0; i < total_pages; i++) {
            Page *p = newpage(virt_addr, nil);
            KMap *k = kmap(p);

            long to_read = BY2PG;
            if (remaining < BY2PG)
              to_read = remaining;

            if (to_read > 0) {
              if (cread(bc, (uchar *)VA(k), to_read, file_off) != to_read)
                print("BOOT: Short read on /boot/boot\n");
              file_off += to_read;
              remaining -= to_read;
            }

            /* Zero out BSS or partial page */
            if (to_read < BY2PG)
              memset((uchar *)VA(k) + to_read, 0, BY2PG - to_read);

            kunmap(k);
            segpage(up->seg[TSEG], p);
            virt_addr += BY2PG;
          }
          loaded = 1;
          print("BOOT[proc0]: CLR loaded successfully\n");
        } else {
          print("BOOT[proc0]: /boot/boot bad magic 0x%x (expected 0x%x)\n",
                exec.magic, S_MAGIC);
        }
      } else {
        print("BOOT[proc0]: Failed to read /boot/boot header\n");
      }
      poperror();
    }
    cclose(bc);
  } else {
    print("BOOT[proc0]: /boot/boot not found\n");
  }

  if (!loaded) {
    print("BOOT[proc0]: Fallback - using legacy initcode\n");
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

    /* TODO: Load and compile CLR init from /boot/boot
     * For now, use legacy initcode[] until CLR userspace execution is working
     */
    print("BOOT[proc0]: initcode size=%d, first bytes: %02x %02x %02x %02x\\n",
          (int)sizeof(initcode), initcode[0], initcode[1], initcode[2],
          initcode[3]);
    memmove((uchar *)VA(k), initcode, sizeof(initcode));
    memset((uchar *)VA(k) + sizeof(initcode), 0, BY2PG - sizeof(initcode));

    print("BOOT[proc0]: unmapping text page\n");
    kunmap(k);
    if (p->pa == 0)
      print("BOOT[proc0]: text page pa=0 (unexpected)\n");
    else
      print("BOOT[proc0]: text page pa nonzero\n");
    print("BOOT[proc0]: about to call segpage for text\n");
    segpage(up->seg[TSEG], p);
    print("BOOT[proc0]: segpage for text completed\n");
  }

  /* segpage now calls userpmap() which creates MMU structures */
  if (dbg_getpte(UTZERO) != 0)
    print("BOOT[proc0]: text pte present\n");
  else
    print("BOOT[proc0]: text pte missing\n");
  print("BOOT[proc0]: user segments populated\n");
  {
    uintptr va = USTKTOP - BY2PG;
    uintptr idx3 = PTLX(va, 3);
    print("BOOT[proc0]: checking VA=0x%llx PML4idx=%lld\n",
          (unsigned long long)va, (long long)idx3);
    print("BOOT[proc0]: m->pml4[%lld]=0x%llx\n", (unsigned long long)idx3,
          (unsigned long long)m->pml4[idx3]);

    uintptr *lvl2_walk = mmuwalk(m->pml4, va, 2, 0);
    if (lvl2_walk != nil) {
      print("BOOT[proc0]: mmuwalk L2 present *entry=0x%llx\n", *lvl2_walk);
    } else {
      print("BOOT[proc0]: mmuwalk L2 missing\n");
    }

    if (mmuwalk(m->pml4, va, 1, 0) != nil)
      print("BOOT[proc0]: mmuwalk L1 present\n");
    else
      print("BOOT[proc0]: mmuwalk L1 missing\n");

    uintptr *lvl2_direct = &m->pml4[idx3];
    print("BOOT[proc0]: direct &m->pml4[%lld]=0x%llx\n",
          (unsigned long long)idx3, (unsigned long long)*lvl2_direct);
    if ((*lvl2_direct & PTEVALID) != 0)
      print("BOOT[proc0]: L2 entry VALID\n");
    else
      print("BOOT[proc0]: L2 entry INVALID\n");

    if ((*lvl2_direct & PTEVALID) != 0) {
      uintptr *pdpt = kaddr(PPN(*lvl2_direct));
      uintptr idx2 = PTLX(va, 2);
      print("BOOT[proc0]: PDPT=0x%llx idx=%lld\n",
            (unsigned long long)(uintptr)pdpt, (unsigned long long)idx2);
      print("BOOT[proc0]: pdpt[%lld]=0x%llx\n", (unsigned long long)idx2,
            (unsigned long long)pdpt[idx2]);
      if (pdpt[idx2] != 0)
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
    if (pml4_entry & PTEVALID) {
      uintptr *pdpt = kaddr(PPN(pml4_entry));
      uintptr idx2 = PTLX(va, 2);
      uintptr pdpt_entry = pdpt[idx2];
      if (pdpt_entry & PTEVALID) {
        uintptr *pd = kaddr(PPN(pdpt_entry));
        uintptr idx1 = PTLX(va, 1);
        uintptr pd_entry = pd[idx1];
        if (pd_entry & PTEVALID) {
          print("BOOT[proc0]: Full chain valid to PD\n");
        } else {
          print("BOOT[proc0]: PD entry invalid (0x%llx)\n",
                (unsigned long long)pd_entry);
        }
      } else {
        print("BOOT[proc0]: PDPT entry invalid (0x%llx)\n",
              (unsigned long long)pdpt_entry);
      }
    }
  }
  print("BOOT[proc0]: Exited PML4 validation, about to check USTKTOP slot\n");

  /* Verify m and m->pml4 are valid before access */
  if (m == nil) {
    panic("proc0: m is NULL!");
  }
  if (m->pml4 == nil) {
    panic("proc0: m->pml4 is NULL!");
  }

  uintptr ustktop_idx = PTLX(USTKTOP - 1, 3);
  print("BOOT[proc0]: USTKTOP=%#p, USTKTOP-1=%#p, PTLX(USTKTOP-1,3)=%lld\n",
        USTKTOP, USTKTOP - 1, (long long)ustktop_idx);

  /* Safe access with pointer validation above */
  print("BOOT[proc0]: About to access m->pml4[%lld]\n", (long long)ustktop_idx);
  uintptr pml4_value = m->pml4[ustktop_idx];
  print("BOOT[proc0]: Read m->pml4[%lld] = %#p\n", (long long)ustktop_idx,
        pml4_value);

  if (pml4_value != 0)
    print("BOOT[proc0]: PML4 slot before mmuswitch nonzero\n");
  else
    print("BOOT[proc0]: PML4 slot before mmuswitch zero\n");
  if (up->mmuhead == nil)
    print("BOOT[proc0]: mmuhead nil (no user mappings staged)\n");
  else {
    MMU *p;
    int count = 0;
    for (p = up->mmuhead; p != nil && count < 5; p = p->next, count++) {
      if (p->level == 2)
        print("BOOT[proc0]: mmuhead[%d] PML4E index=%d\n", count, p->index);
      else if (p->level == 1)
        print("BOOT[proc0]: mmuhead[%d] PDPE index=%d\n", count, p->index);
      else if (p->level == 0)
        print("BOOT[proc0]: mmuhead[%d] PDE index=%d\n", count, p->index);
      else
        print("BOOT[proc0]: mmuhead[%d] level=%d index=%d\n", count, p->level,
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
    print("userinit: about to call mmuswitch, checking mmuhead...\n");
    if (up->mmuhead == nil)
      print("userinit: mmuhead is NULL!\n");
    else
      print("userinit: mmuhead has entries\n");

    int s = splhi();
    print("userinit: calling mmuswitch\n");
    mmuswitch(up);
    print("userinit: mmuswitch returned\n");
    splx(s);
  }
  {
    uintptr idx = PTLX(USTKTOP - 1, 3);
    if ((m->pml4[idx] & PTEVALID) != 0)
      print("BOOT[proc0]: PML4 entry valid after mmuswitch\n");
    else
      print("BOOT[proc0]: PML4 entry still invalid after mmuswitch\n");
    uintptr *pdpt = kaddr(PPN(m->pml4[idx]));
    uintptr idx1 = PTLX(USTKTOP - BY2PG, 2);
    if (pdpt[idx1] != 0)
      print("BOOT[proc0]: PDPT entry after mmuswitch nonzero\n");
    else
      print("BOOT[proc0]: PDPT entry after mmuswitch zero\n");
  }
  if (m->pml4[PTLX(USTKTOP - 1, 3)] != 0)
    print("BOOT[proc0]: PML4 slot after mmuswitch nonzero\n");
  else
    print("BOOT[proc0]: PML4 slot after mmuswitch still zero\n");
  if (dbg_getpte(USTKTOP - BY2PG) != 0)
    print("BOOT[proc0]: stack pte present after mmuswitch\n");
  else
    print("BOOT[proc0]: stack pte still missing after mmuswitch\n");
  if (dbg_getpte(UTZERO) != 0)
    print("BOOT[proc0]: text pte present after mmuswitch\n");
  else
    print("BOOT[proc0]: text pte still missing after mmuswitch\n");
  {
    uintptr idx = PTLX(USTKTOP - 1, 3);
    if ((m->pml4[idx] & PTEVALID) != 0) {
      uintptr *pdpt = kaddr(PPN(m->pml4[idx]));
      uintptr idx1 = PTLX(USTKTOP - BY2PG, 2);
      if (pdpt[idx1] != 0) {
        // Success - page tables are set up correctly
      }
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

  print("BOOT[proc0]: about to call init0 - switching to userspace\n");
  init0();

  /* init0 will never return */
  print("BOOT[proc0]: init0 returned - this should never happen!\n");
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
