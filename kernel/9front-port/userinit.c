#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"
#include "portlib.h"
#include "tos.h"
#include "u.h"
#include <a.out.h>
#include <elf.h>
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
extern void pid2_selftest(void);

uintptr dbg_getpte(uintptr);

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

static void mount_wasm_device(char *spec, char *label) {
  if (waserror()) {
    print("BOOT[proc0]: WARNING - failed to mount %s on /wasm\n", label);
    poperror();
    return;
  }
  Chan *wasmpt = namec("/wasm", Amount, 0, 0);
  if (waserror()) {
    if (wasmpt)
      cclose(wasmpt);
    nexterror();
  }
  Chan *dev = namec(spec, Abind, 0, 0);
  if (waserror()) {
    if (dev)
      cclose(dev);
    if (wasmpt)
      cclose(wasmpt);
    nexterror();
  }
  cmount(dev, wasmpt, MAFTER, nil);
  poperror();
  cclose(dev);
  poperror();
  cclose(wasmpt);
  poperror();
}

/* Load ELF64 executable into process address space
 * Returns 1 on success, 0 on failure
 * Sets up TSEG with all PT_LOAD segments
 */
static int load_elf64(Chan *c, uintptr *out_entry) {
  Elf64_Ehdr ehdr;
  Elf64_Phdr *phdrs = nil;
  int i;

  /* Read ELF header */
  print("ELF: Reading header from chan type=%d qid=%llx\n", c->type,
        c->qid.path);
  long nread = devtab[c->type]->read(c, (uchar *)&ehdr, sizeof(ehdr), 0);
  print("ELF: Read %ld bytes (expected %lud)\n", nread, sizeof(ehdr));
  if (nread != sizeof(ehdr)) {
    print("ELF: Failed to read ELF header\n");
    return 0;
  }

  /* Verify ELF magic */
  if (ehdr.e_ident[0] != ELF_MAGIC_0 || ehdr.e_ident[1] != ELF_MAGIC_1 ||
      ehdr.e_ident[2] != ELF_MAGIC_2 || ehdr.e_ident[3] != ELF_MAGIC_3) {
    print("ELF: Bad magic: 0x%x%c%c%c\n", ehdr.e_ident[0], ehdr.e_ident[1],
          ehdr.e_ident[2], ehdr.e_ident[3]);
    return 0;
  }

  /* Verify ELF64 and x86-64 */
  if (ehdr.e_ident[4] != ELFCLASS64) {
    print("ELF: Not 64-bit (class=%d)\n", ehdr.e_ident[4]);
    return 0;
  }
  if (ehdr.e_machine != EM_X86_64) {
    print("ELF: Not x86-64 (machine=%d)\n", ehdr.e_machine);
    return 0;
  }
  if (ehdr.e_type != ET_EXEC) {
    print("ELF: Not executable (type=%d)\n", ehdr.e_type);
    return 0;
  }

  print("ELF: Valid ELF64 x86-64 executable, entry=0x%llx\n", ehdr.e_entry);
  print("ELF: %d program headers at offset 0x%llx\n", ehdr.e_phnum,
        ehdr.e_phoff);

  /* Allocate space for program headers */
  phdrs = malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));
  if (phdrs == nil) {
    print("ELF: Failed to allocate program headers\n");
    return 0;
  }

  /* Read program headers */
  if (devtab[c->type]->read(c, (uchar *)phdrs,
                            ehdr.e_phnum * sizeof(Elf64_Phdr), ehdr.e_phoff) !=
      ehdr.e_phnum * sizeof(Elf64_Phdr)) {
    print("ELF: Failed to read program headers\n");
    free(phdrs);
    return 0;
  }

  /* Find memory range needed */
  uintptr min_addr = ~0ULL;
  uintptr max_addr = 0;
  for (i = 0; i < ehdr.e_phnum; i++) {
    if (phdrs[i].p_type != PT_LOAD)
      continue;
    if (phdrs[i].p_vaddr < min_addr)
      min_addr = phdrs[i].p_vaddr;
    if (phdrs[i].p_vaddr + phdrs[i].p_memsz > max_addr)
      max_addr = phdrs[i].p_vaddr + phdrs[i].p_memsz;
  }

  if (min_addr == ~0ULL) {
    print("ELF: No loadable segments found\n");
    free(phdrs);
    return 0;
  }

/* Add guard pages: 4 pages (16KB) before and after for safety */
#define GUARD_PAGES 4
  uintptr guard_min = (min_addr & ~(BY2PG - 1)) - (GUARD_PAGES * BY2PG);
  uintptr guard_max =
      ((max_addr + BY2PG - 1) & ~(BY2PG - 1)) + (GUARD_PAGES * BY2PG);

  /* Calculate pages needed including guards */
  ulong total_len = guard_max - guard_min;
  ulong total_pages = total_len / BY2PG;
  print("ELF: Memory range 0x%llx-0x%llx (%ld pages, with %d guard pages each "
        "side)\n",
        guard_min, guard_max, total_pages, GUARD_PAGES);

  /* Create TSEG with guard pages */
  up->seg[TSEG] = newseg(SG_TEXT, guard_min, total_pages);
  up->seg[TSEG]->flushme = 1;

  /* Allocate guard pages only - PT_LOAD segments will allocate their own pages
   */
  /* NOTE: Guard pages are attributed to process Pebble budget as "system tax"
   */
  /* System tax = overhead pages required by the system (guards before/after
   * program) */
  print("ELF: Pre-allocating guard pages (system tax)\n");
  ulong guard_pages_allocated = 0;

  /* Page-align min and max addresses for guard allocation */
  uintptr aligned_min = min_addr & ~(BY2PG - 1);
  uintptr aligned_max = (max_addr + BY2PG - 1) & ~(BY2PG - 1);

  /* Allocate guard pages BEFORE program */
  for (uintptr addr = guard_min; addr < aligned_min; addr += BY2PG) {
    Page *p =
        newpage(addr, nil); /* Charges process Pebble budget as system tax */
    KMap *k = kmap(p);
    memset((uchar *)VA(k), 0, BY2PG); /* Zero-fill guard page */
    kunmap(k);
    segpage(up->seg[TSEG], p);
    guard_pages_allocated++;
  }

  /* Allocate guard pages AFTER program */
  for (uintptr addr = aligned_max; addr < guard_max; addr += BY2PG) {
    Page *p =
        newpage(addr, nil); /* Charges process Pebble budget as system tax */
    KMap *k = kmap(p);
    memset((uchar *)VA(k), 0, BY2PG); /* Zero-fill guard page */
    kunmap(k);
    segpage(up->seg[TSEG], p);
    guard_pages_allocated++;
  }
  print("ELF: Allocated %ld guard pages (system tax: %ldKB overhead)\n",
        guard_pages_allocated, (guard_pages_allocated * BY2PG) / 1024);

  /* Load each PT_LOAD segment */
  for (i = 0; i < ehdr.e_phnum; i++) {
    if (phdrs[i].p_type != PT_LOAD)
      continue;

    print("ELF: Loading segment %d: vaddr=0x%llx filesz=%lld memsz=%lld\n", i,
          phdrs[i].p_vaddr, phdrs[i].p_filesz, phdrs[i].p_memsz);

    uintptr vaddr = phdrs[i].p_vaddr;
    ulong file_off = phdrs[i].p_offset;
    ulong file_remaining = phdrs[i].p_filesz;
    ulong mem_remaining = phdrs[i].p_memsz;

    /* Allocate and load pages for this segment */
    while (mem_remaining > 0) {
      uintptr page_addr = vaddr & ~(BY2PG - 1);
      ulong page_off = vaddr & (BY2PG - 1);

      /* Allocate page if not already allocated */
      Page *p = newpage(page_addr, nil);
      KMap *k = kmap(p);

      /* Read file data for this page */
      ulong to_read = BY2PG - page_off;
      if (to_read > file_remaining)
        to_read = file_remaining;

      if (to_read > 0) {
        if (devtab[c->type]->read(c, (uchar *)VA(k) + page_off, to_read,
                                  file_off) != to_read) {
          print("ELF: Short read at offset 0x%lx\n", file_off);
        }
        file_off += to_read;
        file_remaining -= to_read;
      }

      /* Zero-fill BSS portion */
      ulong to_zero = BY2PG - page_off - to_read;
      if (to_zero > mem_remaining - to_read)
        to_zero = mem_remaining - to_read;
      if (to_zero > 0) {
        memset((uchar *)VA(k) + page_off + to_read, 0, to_zero);
      }

      kunmap(k);
      segpage(up->seg[TSEG], p);

      /* Advance to next page */
      ulong consumed = to_read + to_zero;
      vaddr += consumed;
      mem_remaining -= consumed;
    }
  }

  free(phdrs);
  *out_entry = ehdr.e_entry;
  print("ELF: Loaded successfully, entry point at 0x%llx\n", *out_entry);

  /* Exchange page is now handled by kernel_setup_init_exchange() in
   * devexchange.c */
  /* Do NOT create ESEG here - it conflicts with P9SEG and causes segment
   * shadowing */

  return 1;
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
  /* Budget is in tokens; init budget was reserved from global pool in
   * pebbleinit() */
  up->pebble.colorless_bank = PEBBLE_INIT_BUDGET;
  print("PEBBLE: granted %lud bytes (%dMB) init budget to proc0\n",
        up->pebble.colorless_bank, PEBBLE_INIT_BUDGET / (1024 * 1024));

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

  /* Set init namespace to allow 1024 total system processes */
  up->pgrp->spawn_limit = 1024;
  up->pgrp->spawn_count =
      0; /* Start at 0 (init itself will be counted on first fork) */

  /* Grant init spawn capability bound to its Pgrp */
  up->spawn_max_children = 128; /* Init can spawn 128 direct children */
  up->spawn_children = 0;
  uuid_pack_capability(&up->spawn_cap, up->pgrp->identity_hash, 0,
                       CAP_TYPE_SPAWN, 0xFF);
  print("BOOT[proc0]: granted CAP_TYPE_SPAWN (max_children=%d)\n",
        up->spawn_max_children);

  BOOTPRINT("BOOT[proc0]: process groups ready\n");

  pebble_selftest();
  pid2_selftest();

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

  /* Mount /srv registry device */
  if (waserror()) {
    print("BOOT[proc0]: WARNING - failed to mount #s on /srv\n");
    poperror();
  } else {
    Chan *srvdev = namec("#s", Abind, 0, 0);
    if (waserror()) {
      if (srvdev)
        cclose(srvdev);
      nexterror();
    }
    Chan *srvpt = namec("/srv", Amount, 0, 0);
    if (waserror()) {
      if (srvpt)
        cclose(srvpt);
      if (srvdev)
        cclose(srvdev);
      nexterror();
    }
    cmount(srvdev, srvpt, MREPL, nil);
    poperror();
    cclose(srvpt);
    poperror();
    cclose(srvdev);
    poperror();
  }

  /* TODO: Re-enable when lib9p provides proper /wasm server
   * These mounts fail because /wasm directory doesn't exist yet.
   * See: implementation_plan.md for lib9p fileserver library.
   */
#if 0
  /* Expose sandbox-visible devices under /wasm */
  mount_wasm_device("#X", "#X");
  mount_wasm_device("#c", "#c");
  mount_wasm_device("#s", "#s");
  mount_wasm_device("#B", "#B");
  mount_wasm_device("#Y", "#Y");
  mount_wasm_device("#Z", "#Z");
#endif

  /* CLR moved to userspace - no kernel initialization needed */
  /* pebble_sip_issue_test(); */
  BOOTPRINT("BOOT[proc0]: setting up segments\n");

  /* Clear any existing user PML4 entries to force mmucreate to build mmuhead */
  print("BOOT[proc0]: clearing existing user PML4 entries\n");
  m->pml4[PTLX(UTZERO, 3)] = 0;
  m->pml4[PTLX(USTKTOP - 1, 3)] = 0;

  /*
   * Setup Stack segment for init process.
   * Text segment (TSEG) is set up by ELF loader when loading /boot/init.
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

  /* Try to load /boot/init first, then /boot/boot */
  /* Use #/./boot/init to access directly via root device, avoiding spec
   * confusion */
  print("BOOT[proc0]: attempting to open #/./boot/init...\n");
  Chan *bc = nil;
  if (!waserror()) {
    bc = namec("#/./boot/init", Aopen, OREAD, 0);
    poperror();
  } else {
    print("BOOT[proc0]: namec #/./boot/init failed, trying #/./boot/boot...\n");
  }
  if (bc == nil && !waserror()) {
    bc = namec("#/./boot/boot", Aopen, OREAD, 0);
    poperror();
  }

  int loaded = 0;
  uintptr elf_entry = 0;

  if (bc != nil) {
    Exec exec;
    if (!waserror()) {
      print(
          "BOOT[proc0]: Found /boot/init or /boot/boot, checking header...\n");

      /* Try ELF first */
      if (load_elf64(bc, &elf_entry)) {
        loaded = 1;
        up->entry_point = elf_entry;
        print("BOOT[proc0]: ELF binary loaded successfully, entry=0x%lx\n",
              elf_entry);
      } else {
        /* Try WASM */
        struct M3Function *start_func = nil;
        /* Peek at magic for WASM check */
        uchar magic[4];
        if (devtab[bc->type]->read(bc, magic, 4, 0) == 4 && magic[0] == 0x00 &&
            magic[1] == 0x61 && magic[2] == 0x73 && magic[3] == 0x6d) {

          print("BOOT[proc0]: Detected WASM binary\n");
          if (wasm_exec_compile(bc, &start_func) == 0) {
            loaded = 1;
            /* Store start_func in entry_point for init0 to find */
            up->entry_point = (uintptr)start_func;
            /* Mark as WASM process for init0 */
            /* We need a flag, but up->wasm.initialized is redundant since
             * compile sets it */
            print("BOOT[proc0]: WASM init loaded successfully\n");

            /* Exchange page is now handled by kernel_setup_init_exchange() */
            /* Do NOT create ESEG here - conflicts with P9SEG */
          } else {
            print("BOOT[proc0]: WASM compile failed\n");
          }
        } else {
          /* Try Plan 9 a.out format */
          print("BOOT[proc0]: Not ELF or WASM, trying Plan 9 a.out...\n");

          /* Reset seek to 0 (read read checks from 0 but careful) */
          /* Actually we read magic separately, need to be careful.
           * But devtab read uses offset param, so just reuse 0 offset. */

          if (devtab[bc->type]->read(bc, (uchar *)&exec, sizeof(Exec), 0) ==
              sizeof(Exec)) {
            /* Accept S_MAGIC (amd64) or A_MAGIC (legacy) */
            if (exec.magic == S_MAGIC || exec.magic == A_MAGIC) {
              print("BOOT[proc0]: Loading a.out binary (text=%d data=%d)\n",
                    exec.text, exec.data);

              ulong total_len = exec.text + exec.data + exec.bss;
              ulong total_pages = (total_len + BY2PG - 1) / BY2PG;

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
                  if (devtab[bc->type]->read(bc, (uchar *)VA(k), to_read,
                                             file_off) != to_read)
                    print("BOOT: Short read on /boot/init\n");
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
              up->entry_point = UTZERO;
              print("BOOT[proc0]: a.out binary loaded successfully, "
                    "entry=0x%lx\n",
                    UTZERO);
            } else {
              print("BOOT[proc0]: Bad a.out magic 0x%x (expected 0x%x)\n",
                    exec.magic, S_MAGIC);
            }
          } else {
            print("BOOT[proc0]: Failed to read a.out header\n");
          }
        }
      }
      poperror();
    }
    cclose(bc);
  } else {
    print("BOOT[proc0]: /boot/init and /boot/boot not found\n");
  }

  if (!loaded) {
    /* /boot/init not found - this is a fatal error.
     * With the new boot architecture, init MUST be present in initrd.
     * The legacy initcode.S fallback has been removed (Phase 6 cleanup).
     */
    panic("BOOT[proc0]: /boot/init not found in initrd - cannot boot");
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
   *	prepare the stack for init process
   *	switch to usermode to run /boot/init
   *
   * Setup stub P9SEG for lazy exchange page allocation.
   * Page is allocated on first access via fault handler.
   */
  extern int proc_setup_p9seg_stub(Proc *);
  if (proc_setup_p9seg_stub(up) < 0)
    panic("proc0: failed to setup P9SEG stub");

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
