/* clang-format off */
#include "u.h"

/* Local Plan 9 Syscall ABI fix */
#include <u.h>
typedef ulong *syscall_va_list;
#define SYSCALL_ARG(list, type) (*(type*)((list)++))
/* va_list macro removed to prevent stdarg.h conflict */
#define va_start(list, start) ((void)0)
#define va_end(list) ((void)0)




#define syscall_vainit(list, start) ((list) = (syscall_va_list)(start))










#define SYSCALL_ARG(list, type) (*(type*)((list)++))

#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "ureg.h"
#include <error.h>

#include "edf.h"
#include "elf.h"
#include "monocypher.h"
#include "pebble.h"
#include "tos.h"
/* clang-format on */

#include <a.out.h>

/* CLR compilation includes removed - CLR moved to userspace */
#include "exchange.h"

extern void crypto_blake2b_final(crypto_blake2b_ctx *ctx, u8int *out);
#include "proc_packet.h"

/* FSM Integration */
extern int proc_event(Proc *p, int event);

static void hash_binary(Chan *tc) {
  crypto_blake2b_ctx ctx;
  u8int buf[4096];
  long n;
  vlong off = 0;

  crypto_blake2b_init(&ctx, 64);
  while ((n = devtab[tc->type]->read(tc, buf, sizeof(buf), off)) > 0) {
    crypto_blake2b_update(&ctx, buf, n);
    off += n;
  }
  crypto_blake2b_final(&ctx, up->text_hash);
}

uintptr sysr1(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  if (!iseve())
    error(Eperm);
  return 0;
}

static void abortion(void) { pexit("fork aborted", 1); }

uintptr sysrfork(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  /*
   * Code using RFNOMNT expects to block all but
   * the following devices.
   */
  static char nomntdevs[] = "|decp";

  ulong pid, flag;
  int n, i;
  Proc *p;

  flag = SYSCALL_ARG(list, ulong);
  /* Check flags before we commit */
  if ((flag & (RFFDG | RFCFDG)) == (RFFDG | RFCFDG))
    error(Ebadarg);
  if ((flag & (RFNAMEG | RFCNAMEG)) == (RFNAMEG | RFCNAMEG))
    error(Ebadarg);
  if ((flag & (RFENVG | RFCENVG)) == (RFENVG | RFCENVG))
    error(Ebadarg);

  if ((flag & RFPROC) == 0) {
    Fgrp *ofg;
    Pgrp *opg;
    Rgrp *org;
    Egrp *oeg;

    if (flag & (RFMEM | RFNOWAIT))
      error(Ebadarg);

    ofg = up->fgrp;
    opg = up->pgrp;
    org = up->rgrp;
    oeg = up->egrp;

    if (waserror()) {
      if (up->fgrp != ofg) {
        closefgrp(up->fgrp);
        up->fgrp = ofg;
      }
      if (up->pgrp != opg) {
        closepgrp(up->pgrp);
        up->pgrp = opg;
      }
      if (up->rgrp != org) {
        closergrp(up->rgrp);
        up->rgrp = org;
      }
      if (up->egrp != oeg) {
        closeegrp(up->egrp);
        up->egrp = oeg;
      }
      nexterror();
    }

    /* File descriptors */
    if (flag & (RFFDG | RFCFDG)) {
      if (flag & RFFDG)
        up->fgrp = dupfgrp(ofg);
      else
        up->fgrp = dupfgrp(nil);
    }

    /* Process group */
    if (flag & (RFNAMEG | RFCNAMEG)) {
      up->pgrp = newpgrp();
      if (flag & RFNAMEG)
        pgrpcpy(up->pgrp, opg);
      /* inherit notallowed */
      memmove(up->pgrp->notallowed, opg->notallowed,
              sizeof up->pgrp->notallowed);
    }

    /* Rendezvous group */
    if (flag & RFREND)
      up->rgrp = newrgrp();

    /* Environment group */
    if (flag & (RFENVG | RFCENVG)) {
      up->egrp = newegrp();
      if (flag & RFENVG)
        envcpy(up->egrp, oeg);
    }

    if (ofg != up->fgrp)
      closefgrp(ofg);
    if (opg != up->pgrp)
      closepgrp(opg);
    if (org != up->rgrp)
      closergrp(org);
    if (oeg != up->egrp)
      closeegrp(oeg);

    poperror();

    if (flag & RFNOMNT)
      devmask(up->pgrp, 1, nomntdevs);

    if (flag & RFNOTEG) {
      qlock(&up->debug);
      setnoteid(up, 0); /* can't error() with 0 argument */
      qunlock(&up->debug);
    }
    return 0;
  }

  if ((p = newproc()) == nil)
    error("no procs");

  qlock(&up->debug);
  qlock(&p->debug);

  p->scallnr = up->scallnr;
  p->s = up->s;
  p->slash = up->slash;
  p->dot = up->dot;
  incref(p->dot);

  p->nnote = 0;
  p->notify = up->notify;
  p->notified = 0;
  p->notepending = 0;
  p->lastnote = nil;

  if ((flag & RFNOTEG) == 0)
    p->noteid = up->noteid;

  p->procmode = up->procmode;
  p->privatemem = up->privatemem;
  p->noswap = up->noswap;

  /* Universal CBS: Inherit parent's capabilities (monotonic - can only
   * decrease) Every process is a SIP by default with inherited capability
   * bitmap */
  p->capabilities = up->capabilities;
  p->hang = up->hang;
  if (up->procctl == Proc_tracesyscall)
    p->procctl = Proc_tracesyscall;
  p->kp = 0;

  /*
   * Craft a return frame which will cause the child to pop out of
   * the scheduler in user mode with the return register zero
   */
  forkchild(p, up->dbgreg);

  kstrdup(&p->text, up->text);
  kstrdup(&p->user, up->user);
  kstrdup(&p->args, "");
  p->nargs = 0;
  p->setargs = 0;

  p->insyscall = 0;
  memset(p->time, 0, sizeof(p->time));
  p->time[TReal] = MACHP(0)->ticks;
  p->kentry = up->kentry;
  p->pcycles = -p->kentry;

  pid = pidalloc(p);

  qunlock(&p->debug);
  qunlock(&up->debug);

  /* Abort the child process on error */
  if (waserror()) {
    p->kp = 1;
    kprocchild(p, abortion);
    ready(p);
    nexterror();
  }

  /* Make a new set of memory segments */
  n = flag & RFMEM;
  qlock(&p->seglock);
  if (waserror()) {
    qunlock(&p->seglock);
    nexterror();
  }
  for (i = 0; i < NSEG; i++)
    if (up->seg[i] != nil)
      p->seg[i] = dupseg(up->seg, i, n);
  qunlock(&p->seglock);
  poperror();

  /* File descriptors */
  if (flag & (RFFDG | RFCFDG)) {
    if (flag & RFFDG)
      p->fgrp = dupfgrp(up->fgrp);
    else
      p->fgrp = dupfgrp(nil);
  } else {
    p->fgrp = up->fgrp;
    incref(&up->fgrp->ref);
  }

  /* Process groups */
  if (flag & (RFNAMEG | RFCNAMEG)) {
    p->pgrp = newpgrp();
    if (flag & RFNAMEG)
      pgrpcpy(p->pgrp, up->pgrp);
    /* inherit notallowed */
    memmove(p->pgrp->notallowed, up->pgrp->notallowed,
            sizeof p->pgrp->notallowed);
  } else {
    p->pgrp = up->pgrp;
    incref(up->pgrp);
  }

  /* Rendezvous group */
  if (flag & RFREND)
    p->rgrp = newrgrp();
  else {
    p->rgrp = up->rgrp;
    incref(up->rgrp);
  }

  /* Environment group */
  if (flag & (RFENVG | RFCENVG)) {
    p->egrp = newegrp();
    if (flag & RFENVG)
      envcpy(p->egrp, up->egrp);
  } else {
    p->egrp = up->egrp;
    incref(up->egrp);
  }

  procfork(p);

  poperror(); /* abortion */

  if (flag & RFNOMNT)
    devmask(p->pgrp, 1, nomntdevs);

  if ((flag & RFNOWAIT) == 0) {
    p->parent = up;
    lock(&up->exl);
    up->nchild++;
    unlock(&up->exl);
  }

  /*
   *  since the bss/data segments are now shareable,
   *  any mmu info about this process is now stale
   *  (i.e. has bad properties) and has to be discarded.
   */
  /* Phase 6: Setup 9P exchange page for new user process */
  if (proc_setup_p9page(p) < 0)
    error(Enovmem);

  flushmmu();

  procpriority(p, up->basepri, up->fixedpri);
  if (up->wired)
    procwired(p, up->affinity);

  ready(p);
  sched();
  return pid;
}

static int shargs(char *s, int n, char **ap, int nap) {
  char *p;
  int i;

  if (n <= 2 || s[0] != '#' || s[1] != '!')
    return -1;
  s += 2;
  n -= 2; /* skip #! */
  if ((p = memchr(s, '\n', n)) == nil)
    return 0;
  *p = 0;
  i = tokenize(s, ap, nap - 1);
  ap[i] = nil;
  return i;
}

ulong beswal(ulong l) {
  uchar *p;

  p = (uchar *)&l;
  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

uvlong beswav(uvlong v) {
  uchar *p;

  p = (uchar *)&v;
  return ((uvlong)p[0] << 56) | ((uvlong)p[1] << 48) | ((uvlong)p[2] << 40) |
         ((uvlong)p[3] << 32) | ((uvlong)p[4] << 24) | ((uvlong)p[5] << 16) |
         ((uvlong)p[6] << 8) | (uvlong)p[7];
}

uintptr sysexec(void *list_void) {
  extern void uartputs(char *, int);
  char debug_buf[128];
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec ENTERED list_void=%p\n",
          list_void);
  uartputs(debug_buf, strlen(debug_buf));
  ulong *uargs = (ulong *)list_void;
  union {
    struct {
      Exec exec;
      uvlong hdr[1];
    } ehdr;
    char buf[256];
  } u;
  char line[256];
  char *progarg[32 + 1];
  volatile char *args, *elem, *file0;
  char **argv, **argp, **argp0;
  char *a, *e, *charp, *file;
  int i, n, indir, is_elf;
  ulong magic, ssize, nargs, nbytes;
  uintptr entry, text, data, bss, adata, abss, ebss, tstk, align, file_offset;
  uintptr text_base = UTZERO;
  int text_writable = 0;
  Segment *s, *ts;
  Image *img;
  Tos *tos;
  Chan *tc;
  Fgrp *f;
  int saved_nerrlab;
  const char *stage_desc;

  stage_desc = "start";
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec started, list=%p\n",
          list_void);
  uartputs(debug_buf, strlen(debug_buf));

  /* Save error stack level - we'll restore it before returning
   * The syscall wrapper will pop once after we return, so we need to be at
   * saved+1 */
  saved_nerrlab = up->nerrlab;
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec saved_nerrlab=%d\n",
          saved_nerrlab);
  uartputs(debug_buf, strlen(debug_buf));

  /* Initialize to nil before any error can occur */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec initializing variables\n");
  uartputs(debug_buf, strlen(debug_buf));
  args = elem = nil;
  file0 = nil;
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec variables initialized\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Set up error handler BEFORE any code that can call error() */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec about to call waserror()\n");
  uartputs(debug_buf, strlen(debug_buf));
  if (waserror()) {
    snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec ERROR PATH: %s\n",
            up->errstr);
    uartputs(debug_buf, strlen(debug_buf));
    print("sysexec: error at %s: %s\n", stage_desc, up->errstr);
    free(file0);
    free(elem);
    free(args);
    /* Disaster after commit */
    if (up->seg[SSEG] == nil)
      pexit(up->errstr, 1);
    nexterror();
  }
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec waserror() returned\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Now we have an error handler, safe to do validation that might error */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec getting file0 from uargs[0]\n");
  uartputs(debug_buf, strlen(debug_buf));
  stage_desc = "arg file0";
  file0 = (char *)uargs[0];
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec got file0=%p\n", file0);
  uartputs(debug_buf, strlen(debug_buf));
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec calling validaddr for file0\n");
  uartputs(debug_buf, strlen(debug_buf));
  stage_desc = "validaddr file0";
  validaddr((uintptr)file0, 1, 0);
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec validaddr returned\n");
  uartputs(debug_buf, strlen(debug_buf));
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec getting argp0\n");
  uartputs(debug_buf, strlen(debug_buf));
  stage_desc = "arg argp0";
  argp0 = (char **)uargs[1];
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec got argp0=%p\n", argp0);
  uartputs(debug_buf, strlen(debug_buf));
  stage_desc = "evenaddr argp0";
  evenaddr((uintptr)argp0);
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec evenaddr done\n");
  uartputs(debug_buf, strlen(debug_buf));
  stage_desc = "validaddr argp0";
  validaddr((uintptr)argp0, 2 * BY2WD, 0);
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec validaddr argp0 done\n");
  uartputs(debug_buf, strlen(debug_buf));
  if (*argp0 == nil)
    error(Ebadarg);
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec checked *argp0\n");
  uartputs(debug_buf, strlen(debug_buf));
  stage_desc = "validnamedup";
  file0 = validnamedup(file0, 1);
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec validated file '%s'\n",
          file0);
  uartputs(debug_buf, strlen(debug_buf));

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec setting up variables\n");
  uartputs(debug_buf, strlen(debug_buf));
  align = BY2PG - 1;
  indir = 0;
  is_elf = 0;
  file_offset = 0;
  file = file0;
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: sysexec entering loop with file='%s'\n", file);
  uartputs(debug_buf, strlen(debug_buf));
  for (;;) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: sysexec about to call namec('%s')\n", file);
    uartputs(debug_buf, strlen(debug_buf));
    stage_desc = "namec";
    tc = namec(file, Aopen, OEXEC, 0);
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: sysexec namec returned tc=%p\n", tc);
    uartputs(debug_buf, strlen(debug_buf));
    if (waserror()) {
      cclose(tc);
      nexterror();
    }
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: sysexec file opened, waserror set\n");
    uartputs(debug_buf, strlen(debug_buf));
    if (!indir) {
      snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec calling kstrdup\n");
      uartputs(debug_buf, strlen(debug_buf));
      kstrdup(&elem, up->genbuf);
      snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec kstrdup done\n");
      uartputs(debug_buf, strlen(debug_buf));
    }

    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: sysexec about to read from tc->type=%d\n", tc->type);
    uartputs(debug_buf, strlen(debug_buf));
    stage_desc = "read header";
    n = devtab[tc->type]->read(tc, u.buf, sizeof(u.buf), 0);
    snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec read returned n=%d\n",
            n);
    uartputs(debug_buf, strlen(debug_buf));
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: sysexec first 4 bytes: %02x %02x %02x %02x\n", u.buf[0],
            u.buf[1], u.buf[2], u.buf[3]);
    uartputs(debug_buf, strlen(debug_buf));

    /* Check for ELF signature */
    if (n >= 4 && u.buf[0] == 0x7f && u.buf[1] == 'E' && u.buf[2] == 'L' &&
        u.buf[3] == 'F') {
      snprint(debug_buf, sizeof(debug_buf),
              "DEBUG: sysexec detected ELF binary\n");
      uartputs(debug_buf, strlen(debug_buf));
      is_elf = 1;
    }

    /* Check for .NET/CLR PE/COFF signature ("MZ") */
    if (n >= 2 && u.buf[0] == 'M' && u.buf[1] == 'Z') {
      /* CLR execution moved to userspace - use userspace runtime */
      cclose(tc);
      poperror();
      error("CLR execution moved to userspace - recompile for WASM or use userspace CLR");
    }

    if (n >= sizeof(Exec)) {
      magic = beswal(u.ehdr.exec.magic);
      print("EXEC: magic=0x%08lx AOUT_MAGIC=0x%08lx S_MAGIC=0x%08lx\n", magic,
            AOUT_MAGIC, S_MAGIC);
      if (magic == AOUT_MAGIC) {
        print("EXEC: magic matches AOUT_MAGIC\n");
        if (magic & HDR_MAGIC) {
          print("EXEC: has HDR_MAGIC, checking header size n=%d "
                "sizeof(u.ehdr)=%d\n",
                n, (int)sizeof(u.ehdr));
          if (n < sizeof(u.ehdr))
            error("exec: header too small for expansion");
          entry = beswav(u.ehdr.hdr[0]);
          text = UTZERO + sizeof(u.ehdr);
          print("EXEC: expanded header: entry=%#llux text=%#llux\n", entry,
                text);
        } else {
          entry = beswal(u.ehdr.exec.entry);
          text = UTZERO + sizeof(Exec);
          print("EXEC: basic header: entry=%#llux text=%#llux\n", entry, text);
        }
        print("EXEC: checking entry < text: entry=%#llux text=%#llux\n", entry,
              text);
        if (entry < text)
          error("exec: entry point before text segment");
        text += beswal(u.ehdr.exec.text);

        print("EXEC: after adding text size: text=%#llux entry=%#llux "
              "USTKTOP-USTKSIZE=%#llux\n",
              text, entry, (uvlong)(USTKTOP - USTKSIZE));

        if (text <= entry || text >= (USTKTOP - USTKSIZE))
          error("exec: invalid text segment range");

        switch (magic) {
        case S_MAGIC: /* 2MB segment alignment for amd64 */
          align = 0x1fffff;
          break;
        case P_MAGIC: /* 16K segment alignment for spim */
        case V_MAGIC: /* 16K segment alignment for mips */
          align = 0x3fff;
          break;
        case R_MAGIC: /* 64K segment alignment for arm64 */
          align = 0xffff;
          break;
        }
        hash_binary(tc);
        break; /* for binary */
      }

      /* Check for ELF magic */
      if (n >= sizeof(Elf64_Ehdr) && u.buf[0] == ELF_MAGIC_0 &&
          u.buf[1] == ELF_MAGIC_1 && u.buf[2] == ELF_MAGIC_2 &&
          u.buf[3] == ELF_MAGIC_3) {
        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)u.buf;
        Elf64_Phdr phdr;
        int i;
        uintptr minva = ~0ULL, maxva_file = 0, maxva_mem = 0;
        uintptr elf_file_offset = 0; /* File offset of first LOAD segment */
        uintptr text_start = ~0ULL, text_end = 0;
        uintptr data_start = ~0ULL;
        uintptr data_file_end = 0;
        uintptr data_mem_end = 0;

        print("EXEC: detected ELF binary\n");

        /* Verify it's a 64-bit little-endian executable for x86_64 */
        if (ehdr->e_ident[4] != ELFCLASS64)
          error("ELF: not 64-bit");
        if (ehdr->e_ident[5] != ELFDATA2LSB)
          error("ELF: not little-endian");
        if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
          error("ELF: not executable");
        if (ehdr->e_machine != EM_X86_64)
          error("ELF: not x86_64");

        entry = ehdr->e_entry;
        print("EXEC: ELF entry point = %#llux\n", entry);

        /* Find the extent of loadable segments */
        for (i = 0; i < ehdr->e_phnum; i++) {
          devtab[tc->type]->read(tc, &phdr, sizeof(phdr),
                                 ehdr->e_phoff + i * sizeof(phdr));
          if (phdr.p_type == PT_LOAD) {
            if (phdr.p_vaddr < minva) {
              minva = phdr.p_vaddr;
              elf_file_offset = phdr.p_offset;
            }
            if (phdr.p_vaddr + phdr.p_filesz > maxva_file)
              maxva_file = phdr.p_vaddr + phdr.p_filesz;
            if (phdr.p_vaddr + phdr.p_memsz > maxva_mem)
              maxva_mem = phdr.p_vaddr + phdr.p_memsz;
            if (phdr.p_flags & PF_X) {
              if (phdr.p_vaddr < text_start)
                text_start = phdr.p_vaddr;
              if (phdr.p_vaddr + phdr.p_filesz > text_end)
                text_end = phdr.p_vaddr + phdr.p_filesz;
              if (phdr.p_flags & PF_W)
                text_writable = 1;
            } else if (phdr.p_flags & PF_W) {
              if (phdr.p_vaddr < data_start)
                data_start = phdr.p_vaddr;
              if (phdr.p_vaddr + phdr.p_filesz > data_file_end)
                data_file_end = phdr.p_vaddr + phdr.p_filesz;
              if (phdr.p_vaddr + phdr.p_memsz > data_mem_end)
                data_mem_end = phdr.p_vaddr + phdr.p_memsz;
            }
          }
        }

        print("EXEC: ELF file offset = %#llux\n", (uvlong)elf_file_offset);

        print("EXEC: ELF file range: %#llux - %#llux\n", minva, maxva_file);
        print("EXEC: ELF mem range: %#llux - %#llux\n", minva, maxva_mem);

        if (text_start == ~0ULL) {
          text_start = minva;
          text_end = maxva_file;
        }
        if (text_end < text_start)
          text_end = text_start;
        text = text_end > minva ? text_end - minva : 0;
        text_base = text_start;

        if (data_start != ~0ULL) {
          if (data_file_end < data_start)
            data_file_end = data_start;
          if (data_mem_end < data_file_end)
            data_mem_end = data_file_end;
          if (data_start < minva)
            data_start = minva;
          data = data_file_end > data_start ? data_file_end - data_start : 0;
          adata = data_start;
        } else {
          data = 0;
          adata = 0;
        }

        bss = maxva_mem > maxva_file ? maxva_mem - maxva_file : 0;

        print("EXEC: computed segments: text=%#llux data=%#llux bss=%#llux "
              "(text_writable=%d)\n",
              text, data, bss, text_writable);

        /* ELF binaries use page alignment */
        align = BY2PG - 1;
        is_elf = 1;
        file_offset = elf_file_offset;
        hash_binary(tc);
        break; /* for binary */
      }
    }

    if (indir++)
      error(Ebadexec);

    /*
     * Process #! /bin/sh args ...
     */
    memmove(line, u.buf, n);
    n = shargs(line, n, progarg, nelem(progarg));
    if (n < 1)
      error(Ebadexec);
    /*
     * First arg becomes complete file name
     */
    progarg[n++] = file;
    progarg[n] = nil;
    argp0++;
    file = progarg[0];
    progarg[0] = elem;
    poperror();
    cclose(tc);
  }

  if (is_elf) {
    /* For ELF, text/data/bss are already sizes, not addresses */
    /* adata is set to data_start in ELF block */
  } else {
    /* For a.out, text is end address, need to convert to size */
    adata = (text + align) & ~align;
    text -= UTZERO;
    data = beswal(u.ehdr.exec.data);
    bss = beswal(u.ehdr.exec.bss);
  }
  align = BY2PG - 1;

  abss = (adata + data + align) & ~align;
  ebss = (adata + data + bss + align) & ~align;
  if (adata >= (USTKTOP - USTKSIZE) || abss >= (USTKTOP - USTKSIZE) ||
      ebss >= (USTKTOP - USTKSIZE))
    error(Ebadexec);

  /*
   * Args: pass 1: count
   */
  nbytes =
      sizeof(Tos); /* hole for profiling clock at top of stack (and more) */
  nargs = 0;
  if (indir) {
    argp = progarg;
    while (*argp != nil) {
      a = *argp++;
      nbytes += strlen(a) + 1;
      nargs++;
    }
  }
  argp = argp0;
  while (*argp != nil) {
    a = *argp++;
    if (((uintptr)argp & (BY2PG - 1)) < BY2WD)
      validaddr((uintptr)argp, BY2WD, 0);
    validaddr((uintptr)a, 1, 0);
    e = vmemchr(a, 0, USTKSIZE);
    if (e == nil)
      error(Ebadarg);
    nbytes += (e - a) + 1;
    if (nbytes >= USTKSIZE)
      error(Enovmem);
    nargs++;
  }
  ssize = BY2WD * (nargs + 1) + ((nbytes + (BY2WD - 1)) & ~(BY2WD - 1));

  /*
   * 8-byte align SP for those (e.g. sparc) that need it.
   * execregs() will subtract another 4 bytes for argc.
   */
  if (BY2WD == 4 && (ssize + 4) & 7)
    ssize += 4;

  if (PGROUND(ssize) >= USTKSIZE)
    error(Enovmem);

  /*
   * Build the stack segment, putting it in kernel virtual for the moment
   */
  qlock(&up->seglock);
  if (waserror()) {
    qunlock(&up->seglock);
    nexterror();
  }
  s = up->seg[SSEG];
  /*
  print("EXEC: current stack segment base=%#llx top=%#llx size=%lud\n",
          s != nil ? (unsigned long long)s->base : 0ULL,
          s != nil ? (unsigned long long)s->top : 0ULL,
          s != nil ? s->size : 0UL);
  */
  do {
    tstk = s->base;
    if (tstk <= USTKSIZE)
      error(Enovmem);
  } while ((s = isoverlap(tstk - USTKSIZE, USTKSIZE)) != nil);
  /*
  print("EXEC: allocating temporary stack segment at [%#llx, %#llx)\n",
          (unsigned long long)(tstk-USTKSIZE),
          (unsigned long long)tstk);
  */
  up->seg[ESEG] =
      newseg(SG_STACK | SG_NOEXEC, tstk - USTKSIZE, USTKSIZE / BY2PG);
  qunlock(&up->seglock);

  if (waserror()) {
    qlock(&up->seglock);
    s = up->seg[ESEG];
    if (s != nil) {
      up->seg[ESEG] = nil;
      putseg(s);
    }
    nexterror();
  }

  /*
   * Args: pass 2: assemble; the pages will be faulted in
   */
  tos = (Tos *)(tstk - sizeof(Tos));
  tos->cyclefreq = m->cyclefreq;
  print("DEBUG: stack tos initialized\n");
  tos->kcycles = 0;
  tos->pcycles = 0;
  tos->clock = 0;

  argv = (char **)(tstk - ssize);
  charp = (char *)(tstk - nbytes);
  if (indir)
    argp = progarg;
  else
    argp = argp0;

  for (i = 0; i < nargs; i++) {
    if (indir && *argp == nil) {
      indir = 0;
      argp = argp0;
    }
    *argv++ = charp + (USTKTOP - tstk);
    a = *argp++;
    if (indir)
      e = strchr(a, 0);
    else {
      if (charp >= (char *)tos)
        error(Ebadarg);
      validaddr((uintptr)a, 1, 0);
      e = vmemchr(a, 0, (char *)tos - charp);
      if (e == nil)
        error(Ebadarg);
    }
    n = (e - a) + 1;
    memmove(charp, a, n);
    charp += n;
  }
  *argv = nil;

  /* copy args; easiest from new process's stack */
  a = (char *)(tstk - nbytes);
  n = charp - a;
  if (n > 128) /* don't waste too much space on huge arg lists */
    n = 128;
  args = smalloc(n);
  memmove(args, a, n);
  if (n > 0 && args[n - 1] != '\0') {
    /* make sure last arg is NUL-terminated */
    /* put NUL at UTF-8 character boundary */
    for (i = n - 1; i > 0; --i)
      if (fullrune(args + i, n - i))
        break;
    args[i] = 0;
    n = i + 1;
  }

  /* Attach text segment */
  /* attachimage returns a locked cache image */
  img = attachimage(tc, (PGROUND(text) + PGROUND(data)) >> PGSHIFT);
  if ((ts = img->s) != nil && ts->flen == text) {
    assert(ts->image == img);
    incref((Ref *)&ts->ref);
    putimage(img);
  } else {
    if (waserror()) {
      putimage(img);
      nexterror();
    }
    {
      int text_attr = SG_TEXT;
      if (!text_writable)
        text_attr |= SG_RONLY;
      ts = newseg(text_attr, text_base, PGROUND(text) >> PGSHIFT);
    }
    ts->flushme = 1;
    ts->image = img;
    ts->fstart = file_offset;
    ts->flen = text;
    /*
    print("EXEC: text segment fstart=%#llux flen=%#llux\n",
          (uvlong)ts->fstart, (uvlong)ts->flen);
    */
    img->s = ts;
    unlock(img);
    poperror();
  }

  /*
   * Committed.
   * Free old memory.
   * Special segments are maintained across exec
   */
  poperror();
  qlock(&up->seglock);

  for (i = SSEG; i <= BSEG; i++) {
    s = up->seg[i];
    if (s != nil) {
      /* prevent a second free if we have an error */
      up->seg[i] = nil;
      putseg(s);
    }
  }
  for (i = ESEG + 1; i < NSEG; i++) {
    s = up->seg[i];
    if (s != nil && (s->type & SG_CEXEC) != 0) {
      up->seg[i] = nil;
      putseg(s);
    }
  }

  /* Text. Shared. */
  assert(ts->ref > 0);
  up->seg[TSEG] = ts;
#ifdef DEBUG
  /*
  print("EXEC: mapped text segment base=%#llx size=%lud bytes (writable=%d)\n",
          (unsigned long long)up->seg[TSEG]->base,
          (unsigned long long)(up->seg[TSEG]->size*BY2PG),
          text_writable);
  */
#endif

  /* Data. Shared. */
  if (data > 0) {
    s = newseg(SG_DATA, adata, PGROUND(data) >> PGSHIFT);
    s->image = img;
    s->fstart = text;
    s->flen = data;
    incref((Ref *)&img->ref);
    up->seg[DSEG] = s;
#ifdef DEBUG
    /*
    print("EXEC: mapped data segment base=%#llx size=%lud bytes\n",
            (unsigned long long)s->base,
            (unsigned long long)(s->size*BY2PG));
    */
#endif
  } else {
    up->seg[DSEG] = nil;
#ifdef DEBUG
    /* print("EXEC: skipping data segment (size 0)\n"); */
#endif
  }

  /* BSS. Zero fill on demand */
  up->seg[BSEG] = newseg(SG_BSS, abss, (ebss - abss) >> PGSHIFT);

  /*
   * Move the stack
   */
  s = up->seg[ESEG];
  up->seg[ESEG] = nil;
  qlock(&s->qlock);
  s->base = USTKTOP - USTKSIZE;
  s->top = USTKTOP;
  relocateseg(s, USTKTOP - tstk);
  qunlock(&s->qlock);
  up->seg[SSEG] = s;
  qunlock(&up->seglock);
  poperror(); /* seglock */

  if (tc == img->c) {
    /* avoid double caching */
    tc->flag &= ~CCACHE;
    cclunk(tc);
  }
  cclose(tc);
  poperror(); /* tc */

  free(file0);
  poperror(); /* file0 */

  /*
   * Close on exec
   */
  if ((f = up->fgrp) != nil) {
    for (i = 0; i <= f->maxfd; i++)
      fdclose(i, CCEXEC);
  }

  qlock(&up->debug);
  free(up->text);
  up->text = elem;
  free(up->args);
  up->args = args;
  up->nargs = n;
  up->setargs = 0;

  freenotes(up);
  freenote(up->lastnote);
  up->lastnote = nil;
  up->notify = nil;
  up->notified = 0;
  up->noteureg = nil;
  up->privatemem = 0;
  up->noswap = 0;
  up->pcycles = -up->kentry;
  procsetup(up);
  qunlock(&up->debug);

  up->errbuf0[0] = '\0';
  up->errbuf1[0] = '\0';

  /*
   *  At this point, the mmu contains info about the old address
   *  space and needs to be flushed
   */
  flushmmu();

  if (up->hang)
    up->procctl = Proc_stopme;

  /* Force error stack to 1 - syscall wrapper will pop once to get to 0 */
  print("sysexec: before cleanup, nerrlab=%d\n", up->nerrlab);
  while (up->nerrlab > 1)
    poperror();
  while (up->nerrlab < 1)
    up->nerrlab++;
  print("sysexec: after cleanup, nerrlab=%d\n", up->nerrlab);

  return execregs(entry, ssize, nargs);
}

int return0(void *) { return 0; }

uintptr syssleep(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  long ms;

  ms = SYSCALL_ARG(list, long);
  if (ms <= 0) {
    if (up->edf != nil && (up->edf->flags & Admitted))
      edfyield();
    else
      yield();
  } else {
    tsleep(&up->sleep, return0, 0, ms);
  }
  return 0;
}

uintptr sysalarm(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  return procalarm(SYSCALL_ARG(list, ulong));
}

uintptr sysexits(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *status;
  char *inval = "invalid exit string";
  char buf[ERRMAX];

  status = SYSCALL_ARG(list, char *);
  if (status != nil) {
    if (waserror())
      status = inval;
    else {
      validaddr((uintptr)status, 1, 0);
      if (vmemchr(status, 0, ERRMAX) == nil) {
        memmove(buf, status, ERRMAX);
        buf[ERRMAX - 1] = 0;
        status = buf;
      }
      poperror();
    }
  }
  pexit(status, 1);
}

uintptr sys_wait(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  ulong pid;
  Waitmsg w;
  OWaitmsg *ow;

  ow = SYSCALL_ARG(list, OWaitmsg *);
  if (ow == nil)
    pid = pwait(nil);
  else {
    validaddr((uintptr)ow, sizeof(OWaitmsg), 1);
    evenaddr((uintptr)ow);
    pid = pwait(&w);
  }
  if (ow != nil) {
    readnum(0, ow->pid, NUMSIZE, w.pid, NUMSIZE);
    readnum(0, ow->time + TUser * NUMSIZE, NUMSIZE, w.time[TUser], NUMSIZE);
    readnum(0, ow->time + TSys * NUMSIZE, NUMSIZE, w.time[TSys], NUMSIZE);
    readnum(0, ow->time + TReal * NUMSIZE, NUMSIZE, w.time[TReal], NUMSIZE);
    strncpy(ow->msg, w.msg, sizeof(ow->msg) - 1);
    ow->msg[sizeof(ow->msg) - 1] = '\0';
  }
  return pid;
}

uintptr sysawait(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *p;
  Waitmsg w;
  uint n;

  p = SYSCALL_ARG(list, char *);
  n = SYSCALL_ARG(list, uint);
  validaddr((uintptr)p, n, 1);
  pwait(&w);
  return (uintptr)snprint(p, n, "%d %lud %lud %lud %q", w.pid, w.time[TUser],
                          w.time[TSys], w.time[TReal], w.msg);
}

void werrstr(char *fmt, ...) {
  va_list va;

  if (up == nil)
    return;

  va_start(va, fmt);
  vseprint(up->syserrstr, up->syserrstr + ERRMAX, fmt, va);
  va_end(va);
}

static int generrstr(char *buf, uint nbuf) {
  char *err;

  if (nbuf == 0)
    error(Ebadarg);
  if (nbuf > ERRMAX)
    nbuf = ERRMAX;
  validaddr((uintptr)buf, nbuf, 1);

  err = up->errstr;
  utfecpy(err, err + nbuf, buf);
  utfecpy(buf, buf + nbuf, up->syserrstr);

  up->errstr = up->syserrstr;
  up->syserrstr = err;

  return 0;
}

uintptr syserrstr(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *buf;
  uint len;

  buf = SYSCALL_ARG(list, char *);
  len = SYSCALL_ARG(list, uint);
  return (uintptr)generrstr(buf, len);
}

/* compatibility for old binaries */
uintptr sys_errstr(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  return (uintptr)generrstr(SYSCALL_ARG(list, char *), 64);
}

uintptr sysnotify(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int (*f)(void *, char *);
  f = SYSCALL_ARG(list, void *);
  if (f != nil)
    validaddr((uintptr)f, sizeof(void *), 0);
  up->notify = f;
  return 0;
}

int donotify(Ureg *ureg) {
  Ureg *nureg;
  char *msg;

  if (up->procctl)
    procctl();
  if (up->nnote == 0)
    return 0;

  spllo();
  qlock(&up->debug);
  msg = popnote(ureg);
  if (msg == nil) {
    qunlock(&up->debug);
    splhi();
    return 0;
  }
  splhi();
  fpunotify(up);
  spllo();
  qunlock(&up->debug);

  if (up->notify == nil || (nureg = notify(ureg, msg)) == nil) {
    if (up->lastnote->flag == NDebug)
      pprint("suicide: %s\n", msg);
    pexit(msg, up->lastnote->flag != NDebug);
  }

  /* word under Ureg is old ureg */
  *(Ureg **)((uintptr)nureg - BY2WD) = up->noteureg;
  up->noteureg = nureg;

  splhi();
  return 1;
}

uintptr sysnoted(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Ureg *nureg;
  int arg;

  arg = SYSCALL_ARG(list, int);

  qlock(&up->debug);
  if (up->notified) {
    splhi();
    fpunoted(up);
    spllo();
  } else if (arg != NRSTR) {
    qunlock(&up->debug);
    error(Ebadarg);
  }
  qunlock(&up->debug);

  nureg = up->noteureg;
  if (!okaddr((uintptr)nureg - BY2WD, BY2WD + sizeof(Ureg), 0)) {
    pprint("suicide: bad ureg in noted or call to noted when not notified\n");
    pexit("Suicide", 0);
  }

  switch (arg) {
  case NCONT:
  case NRSTR:
    /* word under Ureg is old ureg */
    up->noteureg = *(Ureg **)((uintptr)nureg - BY2WD);
    /* wet floor */
  case NSAVE:
    if (noted(up->dbgreg, nureg, arg)) {
      pprint("suicide: trap in noted\n");
      pexit("Suicide", 0);
    }
    break;
  default:
    up->lastnote->flag = NDebug;
    /* fall through */
  case NDFLT:
    noted(up->dbgreg, nureg, arg); /* for debugging */
    if (up->lastnote->flag == NDebug)
      pprint("suicide: %s\n", up->lastnote->msg);
    pexit(up->lastnote->msg, up->lastnote->flag != NDebug);
  }

  /* allow next note */
  up->notified = 0;

  return 0;
}

uintptr syssegbrk(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int i;
  uintptr addr;
  Segment *s;

  addr = SYSCALL_ARG(list, uintptr);
  for (i = 0; i < NSEG; i++) {
    s = up->seg[i];
    if (s == nil || addr < s->base || addr >= s->top)
      continue;
    switch (s->type & SG_TYPE) {
    case SG_TEXT:
    case SG_DATA:
    case SG_STACK:
    case SG_PHYSICAL:
    case SG_FIXED:
    case SG_STICKY:
      error(Ebadarg);
    default:
      return ibrk(SYSCALL_ARG(list, uintptr), i);
    }
  }
  error(Ebadarg);
}

uintptr syssegattach(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int attr;
  char *name;
  uintptr va;
  ulong len;

  attr = SYSCALL_ARG(list, int);
  name = SYSCALL_ARG(list, char *);
  va = SYSCALL_ARG(list, uintptr);
  len = SYSCALL_ARG(list, ulong);
  validaddr((uintptr)name, 1, 0);
  name = validnamedup(name, 1);
  if (waserror()) {
    free(name);
    nexterror();
  }
  va = segattach(attr, name, va, len);
  free(name);
  poperror();
  return va;
}

uintptr syssegdetach(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int i;
  uintptr addr;
  Segment *s;

  addr = SYSCALL_ARG(list, uintptr);

  qlock(&up->seglock);
  if (waserror()) {
    qunlock(&up->seglock);
    nexterror();
  }

  for (i = 0; i < NSEG; i++)
    if ((s = up->seg[i]) != nil) {
      qlock(&s->qlock);
      if ((addr >= s->base && addr < s->top) ||
          (s->top == s->base && addr == s->base))
        goto found;
      qunlock(&s->qlock);
    }

  error(Ebadarg);

found:
  /*
   * Check we are not detaching the initial stack segment.
   */
  if (s == up->seg[SSEG]) {
    qunlock(&s->qlock);
    error(Ebadarg);
  }
  qunlock(&s->qlock);
  up->seg[i] = nil;
  putseg(s);
  qunlock(&up->seglock);
  poperror();

  /* Ensure we flush any entries from the lost segment */
  flushmmu();
  return 0;
}

uintptr syssegfree(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Segment *s;
  uintptr from, to;

  from = SYSCALL_ARG(list, uintptr);
  to = SYSCALL_ARG(list, ulong);
  to += from;
  if (to < from)
    error(Ebadarg);
  s = seg(up, from, 1);
  if (s == nil)
    error(Ebadarg);
  to &= ~(BY2PG - 1);
  from = PGROUND(from);
  if (from >= to) {
    qunlock(&s->qlock);
    return 0;
  }
  if (to > s->top) {
    qunlock(&s->qlock);
    error(Ebadarg);
  }
  mfreeseg(s, from, (to - from) / BY2PG);
  qunlock(&s->qlock);
  flushmmu();
  return 0;
}

/* For binary compatibility */
uintptr sysbrk_(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  return ibrk(SYSCALL_ARG(list, uintptr), BSEG);
}

uintptr sysrendezvous(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  uintptr tag, val, new;
  Proc *p, **l;

  tag = SYSCALL_ARG(list, uintptr);
  new = SYSCALL_ARG(list, uintptr);
  l = &REND(up->rgrp, tag);

  lock(up->rgrp);
  for (p = *l; p != nil; p = p->rendhash) {
    if (p->rendtag == tag) {
      *l = p->rendhash;
      val = p->rendval;
      p->rendval = new;
      unlock(up->rgrp);

      ready(p);

      return val;
    }
    l = &p->rendhash;
  }

  /* Going to sleep here */
  up->rendtag = tag;
  up->rendval = new;
  up->rendhash = *l;
  *l = up;
  /* up->state = Rendezvous; -- REPLACED BY FSM */
  proc_event(up, EV_RENDEZ);
  unlock(up->rgrp);

  sched();

  return up->rendval;
}

/*
 * The implementation of semaphores is complicated by needing
 * to avoid rescheduling in syssemrelease, so that it is safe
 * to call from real-time processes.  This means syssemrelease
 * cannot acquire any qlocks, only spin locks.
 *
 * Semacquire and semrelease must both manipulate the semaphore
 * wait list.  Lock-free linked lists only exist in theory, not
 * in practice, so the wait list is protected by a spin lock.
 *
 * The semaphore value *addr is stored in user memory, so it
 * cannot be read or written while holding spin locks.
 *
 * Thus, we can access the list only when holding the lock, and
 * we can access the semaphore only when not holding the lock.
 * This makes things interesting.  Note that sleep's condition function
 * is called while holding two locks - r and up->rlock - so it cannot
 * access the semaphore value either.
 *
 * An acquirer announces its intention to try for the semaphore
 * by putting a Sema structure onto the wait list and then
 * setting Sema.waiting.  After one last check of semaphore,
 * the acquirer sleeps until Sema.waiting==0.  A releaser of n
 * must wake up n acquirers who have Sema.waiting set.  It does
 * this by clearing Sema.waiting and then calling wakeup.
 *
 * There are three interesting races here.

 * The first is that in this particular sleep/wakeup usage, a single
 * wakeup can rouse a process from two consecutive sleeps!
 * The ordering is:
 *
 * 	(a) set Sema.waiting = 1
 * 	(a) call sleep
 * 	(b) set Sema.waiting = 0
 * 	(a) check Sema.waiting inside sleep, return w/o sleeping
 * 	(a) try for semaphore, fail
 * 	(a) set Sema.waiting = 1
 * 	(a) call sleep
 * 	(b) call wakeup(a)
 * 	(a) wake up again
 *
 * This is okay - semacquire will just go around the loop
 * again.  It does mean that at the top of the for(;;) loop in
 * semacquire, phore.waiting might already be set to 1.
 *
 * The second is that a releaser might wake an acquirer who is
 * interrupted before he can acquire the lock.  Since
 * release(n) issues only n wakeup calls -- only n can be used
 * anyway -- if the interrupted process is not going to use his
 * wakeup call he must pass it on to another acquirer.
 *
 * The third race is similar to the second but more subtle.  An
 * acquirer sets waiting=1 and then does a final canacquire()
 * before going to sleep.  The opposite order would result in
 * missing wakeups that happen between canacquire and
 * waiting=1.  (In fact, the whole point of Sema.waiting is to
 * avoid missing wakeups between canacquire() and sleep().) But
 * there can be spurious wakeups between a successful
 * canacquire() and the following semdequeue().  This wakeup is
 * not useful to the acquirer, since he has already acquired
 * the semaphore.  Like in the previous case, though, the
 * acquirer must pass the wakeup call along.
 *
 * This is all rather subtle.  The code below has been verified
 * with the spin model /sys/src/9/port/semaphore.p.  The
 * original code anticipated the second race but not the first
 * or third, which were caught only with spin.  The first race
 * is mentioned in /sys/doc/sleep.ps, but I'd forgotten about it.
 * It was lucky that my abstract model of sleep/wakeup still managed
 * to preserve that behavior.
 *
 * I remain slightly concerned about memory coherence
 * outside of locks.  The spin model does not take
 * queued processor writes into account so we have to
 * think hard.  The only variables accessed outside locks
 * are the semaphore value itself and the boolean flag
 * Sema.waiting.  The value is only accessed with cmpswap,
 * whose job description includes doing the right thing as
 * far as memory coherence across processors.  That leaves
 * Sema.waiting.  To handle it, we call coherence() before each
 * read and after each write.		- rsc
 */

/* Add semaphore p with addr a to list in seg. */
static void semqueue(Segment *s, long *a, Sema *p) {
  memset(p, 0, sizeof *p);
  p->addr = a;
  lock(&s->sema.rendez.lock); /* protect semaphore list */
  p->next = &s->sema;
  p->prev = s->sema.prev;
  p->next->prev = p;
  p->prev->next = p;
  unlock(&s->sema.rendez.lock);
}

/* Remove semaphore p from list in seg. */
static void semdequeue(Segment *s, Sema *p) {
  lock(&s->sema.rendez.lock);
  p->next->prev = p->prev;
  p->prev->next = p->next;
  unlock(&s->sema.rendez.lock);
}

/* Wake up n waiters with addr a on list in seg. */
static void semwakeup(Segment *s, long *a, long n) {
  Sema *p;

  lock(&s->sema.rendez.lock);
  for (p = s->sema.next; p != &s->sema && n > 0; p = p->next) {
    if (p->addr == a && p->waiting) {
      p->waiting = 0;
      coherence();
      wakeup(&p->rendez);
      n--;
    }
  }
  unlock(&s->sema.rendez.lock);
}

/* Add delta to semaphore and wake up waiters as appropriate. */
long semrelease(Segment *s, long *addr, long delta) {
  long value;

  do
    value = *addr;
  while (!cmpswap(addr, value, value + delta));
  semwakeup(s, addr, delta);
  return value + delta;
}

/* Try to acquire semaphore using compare-and-swap */
static int canacquire(long *addr) {
  long value;

  while ((value = *addr) > 0)
    if (cmpswap(addr, value, value - 1))
      return 1;
  return 0;
}

/* Should we wake up? */
static int semawoke(void *p) {
  coherence();
  return !((Sema *)p)->waiting;
}

/* Acquire semaphore (subtract 1). */
int semacquire(Segment *s, long *addr, int block) {
  int acquired;
  Sema phore;

  if (canacquire(addr))
    return 1;
  if (!block)
    return 0;
  semqueue(s, addr, &phore);
  if (acquired = !waserror()) {
    for (;;) {
      phore.waiting = 1;
      coherence();
      if (canacquire(addr))
        break;
      sleep(&phore.rendez, semawoke, &phore);
    }
    poperror();
  }
  semdequeue(s, &phore);
  coherence(); /* not strictly necessary due to lock in semdequeue */
  if (!phore.waiting)
    semwakeup(s, addr, 1);
  if (!acquired)
    nexterror();
  return 1;
}

/* Acquire semaphore or time-out */
static int tsemacquire(Segment *s, long *addr, ulong ms) {
  int timedout, acquired;
  ulong t;
  Sema phore;

  if (canacquire(addr))
    return 1;
  if (ms == 0)
    return 0;
  timedout = 0;
  semqueue(s, addr, &phore);
  if (acquired = !waserror()) {
    for (;;) {
      phore.waiting = 1;
      coherence();
      if (canacquire(addr))
        break;
      t = MACHP(0)->ticks;
      tsleep(&phore.rendez, semawoke, &phore, ms);
      t = TK2MS(MACHP(0)->ticks - t);
      if (t >= ms) {
        timedout = 1;
        break;
      }
      ms -= t;
    }
    poperror();
  }
  semdequeue(s, &phore);
  coherence(); /* not strictly necessary due to lock in semdequeue */
  if (!phore.waiting)
    semwakeup(s, addr, 1);
  if (!acquired)
    nexterror();
  return !timedout;
}

uintptr syssemacquire(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int block;
  long *addr;
  Segment *s;

  addr = SYSCALL_ARG(list, long *);
  block = SYSCALL_ARG(list, int);
  evenaddr((uintptr)addr);
  s = seg(up, (uintptr)addr, 0);
  if (s == nil || (s->type & SG_RONLY) != 0 ||
      (uintptr)addr + sizeof(long) > s->top) {
    validaddr((uintptr)addr, sizeof(long), 1);
    error(Ebadarg);
  }
  if (*addr < 0)
    error(Ebadarg);
  return (uintptr)semacquire(s, addr, block);
}

uintptr systsemacquire(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  long *addr;
  ulong ms;
  Segment *s;

  addr = SYSCALL_ARG(list, long *);
  ms = SYSCALL_ARG(list, ulong);
  evenaddr((uintptr)addr);
  s = seg(up, (uintptr)addr, 0);
  if (s == nil || (s->type & SG_RONLY) != 0 ||
      (uintptr)addr + sizeof(long) > s->top) {
    validaddr((uintptr)addr, sizeof(long), 1);
    error(Ebadarg);
  }
  if (*addr < 0)
    error(Ebadarg);
  return (uintptr)tsemacquire(s, addr, ms);
}

uintptr syssemrelease(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  long *addr, delta;
  Segment *s;

  addr = SYSCALL_ARG(list, long *);
  delta = SYSCALL_ARG(list, long);
  evenaddr((uintptr)addr);
  s = seg(up, (uintptr)addr, 0);
  if (s == nil || (s->type & SG_RONLY) != 0 ||
      (uintptr)addr + sizeof(long) > s->top) {
    validaddr((uintptr)addr, sizeof(long), 1);
    error(Ebadarg);
  }
  /* delta == 0 is a no-op, not a release */
  if (delta < 0 || *addr < 0)
    error(Ebadarg);
  return (uintptr)semrelease(s, addr, delta);
}

/* For binary compatibility */
uintptr sys_nsec(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  vlong *v;

  /* return in register on 64bit machine */
  if (sizeof(uintptr) == sizeof(vlong)) {
    USED(list);
    return (uintptr)todget(nil, nil);
  }

  v = SYSCALL_ARG(list, vlong *);
  evenaddr((uintptr)v);
  validaddr((uintptr)v, sizeof(vlong), 1);
  *v = todget(nil, nil);
  return 0;
}

uintptr syspebblewhiteissue(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  ulong size;
  void **out;
  PebbleWhite *white;
  PebbleState *ps;

  size = SYSCALL_ARG(list, ulong);
  out = SYSCALL_ARG(list, void **);
  if (out == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  ps = pebble_state();
  if (ps == nil)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)out, sizeof(void *), 1);
  white = pebble_issue_white(ps, nil, size);
  if (white == nil)
    error(PEBBLE_E_AGAIN);
  *out = white;
  if (pebble_debug)
    print("PEBBLE: white issue pid=%lud size=%lud token=%#p\n", up->pid, size,
          white);
  return (uintptr)white;
}

uintptr syspebbleblackalloc(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  uintptr size;
  void **userp;
  void *handle;

  size = SYSCALL_ARG(list, uintptr);
  userp = SYSCALL_ARG(list, void **);
  if (userp == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)userp, sizeof(void *), 1);
  handle = nil;
  pebble_black_alloc(size, &handle);
  *userp = handle;
  return (uintptr)handle;
}

uintptr syspebbleblackfree(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  void *handle;

  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  handle = SYSCALL_ARG(list, void *);
  pebble_black_free(handle);
  return 0;
}

uintptr syspebblewhiteverify(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  PebbleWhite *white;
  void **out;
  void *black;

  white = SYSCALL_ARG(list, PebbleWhite *);
  out = SYSCALL_ARG(list, void **);
  if (out == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)out, sizeof(void *), 1);
  black = nil;
  pebble_white_verify(white, &black);
  *out = black;
  return (uintptr)black;
}

uintptr syspebbleredcopy(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  PebbleBlue *blue;
  PebbleRed **out;
  PebbleRed *red;

  blue = SYSCALL_ARG(list, PebbleBlue *);
  out = SYSCALL_ARG(list, PebbleRed **);
  if (out == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)out, sizeof(PebbleRed *), 1);
  red = nil;
  pebble_red_copy(blue, &red);
  *out = red;
  return (uintptr)red;
}

uintptr syspebblebluediscard(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  PebbleBlue *blue;

  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  blue = SYSCALL_ARG(list, PebbleBlue *);
  pebble_blue_discard(blue);
  return 0;
}

#include <systab.h>

int dosyscall(ulong scallnr, Sargs *args, uintptr *retp) {
  extern void uartputs(char *, int);
  char buf[128];
  snprint(buf, sizeof(buf), "DEBUG: dosyscall enter scallnr=%ld\n", scallnr);
  uartputs(buf, strlen(buf));

  if (scallnr == EXEC) {
    snprint(buf, sizeof(buf), "DEBUG: dosyscall EXEC handler address %p\n",
            systab[scallnr]);
    uartputs(buf, strlen(buf));
  }
  vlong startns, stopns;
  uintptr ret;
  int s;

  /*
   * DEBUG: Disabled verbose syscall tracing
   * print("dosyscall: entered, scallnr=%ld\n", scallnr);
   */

  // print("DEBUG: 1. m=%p\n", m);
  m->syscall++;
  // print("DEBUG: 2. up=%p\n", up);
  up->insyscall = 1;
  /* Re-enable interrupts for syscall processing (allows preemption/timers) */
  if (up && m && up->nlocks == 0)
    s = spllo();

  // if (1) print("DEBUG: Pre-Waserror: up=%p nerrlab=%d\n", up, up->nerrlab);
  if (!waserror()) {
    // print("DEBUG: Inside waserror\n");
    evenaddr((uintptr)args);
    validaddr((uintptr)args, sizeof(Sargs), 0);

    up->s = *args;
    // print("DEBUG: Copied args\n");
    syscall_va_list syscall_args;
    syscall_vainit(syscall_args, up->s.args);
    // print("DEBUG: vainit done\n");
    up->scallnr = scallnr;

    if (up->procctl == Proc_tracesyscall) {
      syscall_va_list trace_args;
      syscall_vainit(trace_args, up->s.args);
      syscallfmt(scallnr, userpc(), trace_args);
      splhi();
      up->procctl = Proc_stopme;
      procctl();
      spllo();
      todget(nil, &startns);
    }
    if (scallnr >= nsyscall || systab[scallnr] == nil) {
      postnote(up, 1, "sys: bad sys call", NDebug);
      error(Ebadarg);
    }
    up->psstate = sysctab[scallnr];
    /*
     * DEBUG: Disabled verbose syscall tracing
     * print("dosyscall: calling syscall handler\n");
     */
    // print("DEBUG: calling handler\n");
    snprint(buf, sizeof(buf), "DEBUG: About to call systab[%ld] at %p\n",
            scallnr, systab[scallnr]);
    uartputs(buf, strlen(buf));
    ret = systab[scallnr](syscall_args);
    snprint(buf, sizeof(buf), "DEBUG: systab[%ld] returned %#llux\n", scallnr,
            ret);
    uartputs(buf, strlen(buf));
    /*
     * DEBUG: Disabled verbose syscall tracing
     * print("dosyscall: syscall handler returned %#llux\n", ret);
     */
    poperror();
    if (scallnr == NOTED) {
      /* special case: noted() changes the ureg, return without setting *retp */
      splx(s);
      up->insyscall = 0;
      up->psstate = nil;
      return 1;
    }
  } else {
    /* failure: save the error buffer for errstr */
    char *e = up->syserrstr;
    up->syserrstr = up->errstr;
    up->errstr = e;
    ret = -1;
  }
  if (up->nerrlab) {
    int i;

    print("bad errstack [%lud]: %d extra\n", scallnr, up->nerrlab);
    for (i = 0; i < NERR; i++)
      print("sp=%#p pc=%#p\n", up->errlab[i].sp, up->errlab[i].pc);
    panic("error stack");
  }
  *retp = ret;
  if (up->procctl == Proc_tracesyscall) {
    todget(nil, &stopns);
    syscall_va_list ret_args;
    syscall_vainit(ret_args, up->s.args);
    sysretfmt(scallnr, ret_args, ret, startns, stopns);
    splhi();
    up->procctl = Proc_stopme;
    procctl();
  }
  splx(s);
  up->insyscall = 0;
  up->psstate = nil;
  return 0;
}

/*
 * sys_clr_compile - Compile Fruity IR to assembly via QBE
 *
 * Pipeline:
 *   1. Read Fruity IR module from /dev/clr file descriptor
 *   2. Translate Fruity IR → QBE IL (to intermediate page)
 *   3. Compile QBE IL → Assembly (to output page)
 *   4. Optionally copy QBE IL to debug page
 *
 * Arguments:
 *   fd_fruity - File descriptor to /dev/clr (contains fruity_module_t*)
 *   output_asm - Exchange handle for assembly output
 *   output_qbe - Exchange handle for QBE IL debug output (0 = skip)
 *   errorbuf - Userspace error buffer
 *   errorbuf_size - Size of error buffer
 *
 * Returns:
 *   0 on success
 *   -1 on error (error string written to errorbuf)
 */
uintptr sysclrcompile(void *list_void) {
  USED(list_void);
  /* QBE backend removed. This syscall is deprecated/disabled. */
  error("sysclrcompile: backend removed");
  return 0;
}
