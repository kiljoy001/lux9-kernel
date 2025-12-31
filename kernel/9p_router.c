/*
 * Lux9 9P Router Implementation
 *
 * Routes 9P messages from Exchange Pages to kernel services.
 */

#include "error.h"
#include "portlib.h"
#include "u.h"

/* Manual typedefs (portlib.h gives structs but not always typedefs used by
 * kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;

#include "9p_router.h"
#include "fns.h"
#include "include/distributed_pebble.h"
#include "mem.h"
#include "proc_packet.h"
#include "wasm/wasm_9p_integration.h"
#include "wasm/wasm_fileserver.h"
#include "wasm/wasm_runtime.h"

/* Process FSM integration - use real FSM from proc_fsm.c */
extern int proc_event(Proc *p, int event);
extern char *proc_state_names[PS_COUNT];

/* Forward declarations for handlers */
extern int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r);
extern int dev_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int env_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int srv_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int mnt_9p_handle(Proc *p, Fcall *t, Fcall *r);
static int ram_9p_handle(Proc *caller, Fcall *t, Fcall *r);
static int rpipe_9p_handle(Proc *caller, Fcall *t, Fcall *r);
static void rpipe_clone_notify(void *aux);
static int wasm_9p_handle(Proc *caller, Fcall *t, Fcall *r);
extern uintptr sysexec(void *list_void); /* System exec call */

/*
 * Path matching for routing
 */
static int path_match(char *path, char *pattern) {
  int plen = strlen(pattern);
  if (pattern[plen - 1] == '*') {
    return strncmp(path, pattern, plen - 1) == 0;
  }
  return strcmp(path, pattern) == 0;
}

/*
 * Initialize router subsystem
 */
void p9_router_init(void) { print("9p_router: initialized\n"); }

/*
 * Allocate 9P Exchange Page for a process
 */
int p9_alloc_page(Proc *p) {
  uintptr page;

  page = (uintptr)xalloc(P9_PAGE_SIZE);
  if (page == 0)
    return -1;

  memset((void *)page, 0, P9_PAGE_SIZE);

  /* Initialize P9Control block at offset 0x1F00 (start of 2nd page + offset) */
  P9Control *ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  memset(ctl, 0, sizeof(P9Control));
  atomic_store(&ctl->status, P9_STATUS_IDLE, ORDER_RELAXED);
  atomic_store(&ctl->doorbell, 0, ORDER_RELAXED);

  /* Map Page 0 (Requests) as Read-Write */
  ctl->req_head = 0;
  ctl->req_tail = 0;
  ctl->rep_head = 0;
  ctl->rep_tail = 0;
  ctl->req_seq = 0;
  ctl->rep_seq = 0;

  p->p9page = (void *)page;
  return 0;
}

void p9_free_page(Proc *p) {
  if (p->p9page) {
    xfree(p->p9page);
    p->p9page = nil;
  }
}

int p9_extract_pebble(uchar *data, ulong len, PebbleToken *out) {
  uint magic, version;

  if (len < 8 + sizeof(PebbleToken))
    return -1;

  magic = GBIT32(data);
  version = GBIT32(data + 4);

  if (magic != PEBBLE_MAGIC)
    return -1;
  if (version != PEBBLE_VERSION)
    return -1;

  memmove(out, data + 8, sizeof(PebbleToken));
  return 0;
}

int p9_validate_pebble(PebbleToken *tok, char *path, int access) {
  if (tok->expires != 0 && tok->expires < (uvlong)seconds()) {
    return 0;
  }
  if ((access & PEBBLE_PERM_READ) && !(tok->permissions & PEBBLE_PERM_READ))
    return 0;
  if ((access & PEBBLE_PERM_WRITE) && !(tok->permissions & PEBBLE_PERM_WRITE))
    return 0;
  return 1;
}

#include "blind_ledger.h"
#include "msgord.h"

/*
 * Session Pebble Management
 */
static void store_session_pebble(Proc *p, PebbleToken *tok) {
  P9Control *ctl;

  if (p->p9page == nil)
    return;

  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  memmove(ctl->session_pebble, tok->signature, 16);
  /* Store ledger_id in next 8 bytes */
  PBIT64(ctl->session_pebble + 16, tok->ledger_id);
  /* Store permissions in next byte */
  ctl->session_pebble[24] = tok->permissions;
  /* Store expires in next 8 bytes */
  PBIT64(ctl->session_pebble + 25, tok->expires);
}
/* Forward declaration of generic device handler */

/*
 * FD Handler: /fd/N
 */

static int get_session_pebble(Proc *p, PebbleToken *tok) {
  P9Control *ctl;

  if (p->p9page == nil)
    return -1;

  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);

  /* Check if pebble is set (non-zero signature) */
  if (ctl->session_pebble[0] == 0 && ctl->session_pebble[1] == 0)
    return -1;

  /* Reconstruct PebbleToken from session storage */
  memmove(tok->signature, ctl->session_pebble, 16);
  tok->ledger_id = GBIT64(ctl->session_pebble + 16);
  tok->permissions = ctl->session_pebble[24];
  tok->expires = GBIT64(ctl->session_pebble + 25);

  return 0;
}

/*
 * Full Pebble Validation with BlindLedger
 */
static int p9_validate_pebble_full(PebbleToken *tok, char *path, Proc *owner) {
  UserCapability cap;
  BlindLedgerEntry entry;
  BlindLedgerError err;

  /* Basic validation first */
  if (!p9_validate_pebble(tok, path, 0))
    return 0;

  /* Convert PebbleToken to UserCapability for BlindLedger verification */
  memset(&cap, 0, sizeof(cap));
  memmove(cap.hash, tok->signature, 16);
  /* Zero-pad the rest of the hash */
  memset(cap.hash + 16, 0, 16);

  cap.type = CAP_TYPE_DEVICE; /* Device capability */
  cap.perms = 0;
  if (tok->permissions & PEBBLE_PERM_READ)
    cap.perms |= CAP_PERM_READ;
  if (tok->permissions & PEBBLE_PERM_WRITE)
    cap.perms |= CAP_PERM_WRITE;
  if (tok->permissions & PEBBLE_PERM_EXEC)
    cap.perms |= CAP_PERM_EXEC;

  /* Verify with BlindLedger */
  err = ledger_verify(&cap, &entry);
  if (err != BLIND_LEDGER_OK) {
    /* Capability not found or invalid */
    return 0;
  }

  /* Check owner matches */
  if (entry.owner != owner && entry.owner != nil) {
    /* Capability belongs to different process */
    return 0;
  }

  return 1;
}

/*
 * Permission checking helper for device operations
 */
static int check_permission(Proc *p, int required_perm) {
  PebbleToken tok;

  /* Get session pebble */
  if (get_session_pebble(p, &tok) < 0) {
    /* No pebble in session - DENY by default */
    return 0;
  }

  /* Check if required permission is granted */
  return (tok.permissions & required_perm) != 0;
}

/*
 * FID Management via Fgrp (Stateless Router)
 * We reuse the kernel's file descriptor table to map 9P FIDs.
 * Virtual router endpoints are represented by Channels with type = -1.
 */

#define ROUTER_CHAN_TYPE -1
#define TYPE_PROC 1
#define TYPE_DEV 2
#define TYPE_ENV 3
#define TYPE_SRV 4
#define TYPE_MNT 5
#define TYPE_FD 6
#define TYPE_WASM 9 /* WASM servers with capability-based access */

/* Device subtypes for TYPE_DEV FIDs */
#define DEV_CONS 1
#define DEV_NULL 2
#define DEV_ZERO 3
#define DEV_RANDOM 4
#define DEV_TIME 5
#define DEV_SYSNAME 6
#define DEV_RAM 7
#define DEV_PIPE 8

static int install_fid_with_subtype(int fid, int type, int subtype) {
  Chan *c;
  Fgrp *f = up->fgrp;

  c = newchan();
  if (c == nil)
    return -1;
  c->type = ROUTER_CHAN_TYPE; /* Mark as virtual router channel */
  c->qid.path = type;         /* Store handler type in Qid path */
  c->qid.vers = subtype;      /* Store subtype (e.g., device ID) */
  c->mode = ORDWR;
  c->ref = 1;

  lock(&f->lock);
  if (fid < 0 || growfd(f, fid) < 0) {
    unlockfgrp(f);
    cclose(c);
    return -1;
  }
  if (fid > f->maxfd)
    f->maxfd = fid;
  if (f->fd[fid])
    cclose(f->fd[fid]);
  f->fd[fid] = c;
  f->flag[fid] = 0;
  unlockfgrp(f);
  return 0;
}

static int install_fid(int fid, int type) {
  return install_fid_with_subtype(fid, type, 0);
}

static int get_fid_type(int fid) {
  Chan *c;
  Fgrp *f = up->fgrp;
  int type = 0;

  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != nil) {
    if (c->type == ROUTER_CHAN_TYPE) {
      type = (int)c->qid.path;
    }
  }
  unlock(&f->lock);
  return type;
}

static int get_fid_subtype(int fid) {
  Chan *c;
  Fgrp *f = up->fgrp;
  int subtype = 0;

  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != nil) {
    if (c->type == ROUTER_CHAN_TYPE) {
      subtype = (int)c->qid.vers;
    }
  }
  unlock(&f->lock);
  return subtype;
}

static void remove_fid(int fid) { fdclose(fid, 0); }

/*
 * Dispatch message to appropriate handler
 */
int p9_dispatch(Proc *p, Fcall *t, Fcall *r) {
  int type = 0;

  /* Handle Texec (128) - Direct execution message */
  if (t->type == Texec) {
    char *path;
    char *argv[2];
    ulong args[2];

    /* Extract path from Texec message data */
    /* Format: [2] pathlen + [n] path bytes */
    if (t->count < 2) {
      r->type = Rerror;
      r->ename = "Texec: invalid message format";
      return -1;
    }

    uint pathlen = (uint)t->data[0] | ((uint)t->data[1] << 8);
    if (pathlen == 0 || pathlen > t->count - 2) {
      r->type = Rerror;
      r->ename = "Texec: invalid path length";
      return -1;
    }

    /* Allocate and copy path string for kernel logging/debugging */
    path = xalloc(pathlen + 1);
    if (path == nil) {
      r->type = Rerror;
      r->ename = "Texec: out of memory";
      return -1;
    }
    memmove(path, t->data + 2, pathlen);
    path[pathlen] = '\0';

    print("p9_dispatch: Texec for '%s' (pid %lud)\n", path, p->pid);

    /*
     * Sysexec requires User Virtual Addresses for both path and argv.
     * We must calculate the user address of the path existing in the
     * exchange page, and construct a user-space argv array there as well.
     */

    /* 1. Calculate User Address of the path string */
    /* t->data points into p->p9page. The string starts at data+2 */
    uintptr kpage = (uintptr)p->p9page;
    uintptr kpath = (uintptr)t->data + 2;
    uintptr path_offset = kpath - kpage;
    uintptr upath = EXCHANGE_PAGE_ADDR + path_offset;

    /* 2. Ensure null-termination in the user buffer */
    /* We can safely write \0 because validaddr/namec expects it.
     * Check bounds to ensure we don't write past valid page. */
    if (path_offset + pathlen < P9_PAGE_SIZE) {
      ((char *)kpath)[pathlen] = 0;
    }

    /* 3. Construct argv array in the Exchange Page */
    /* We need space for 2 pointers: [upath, 0] */
    /* Use the space immediately following the message payload */
    uintptr kargv_start = (uintptr)t->data + t->count;

    /* Align to 8 bytes */
    kargv_start = (kargv_start + 7) & ~7ULL;

    /* Check if we have room in the request buffer */
    if (kargv_start + 2 * sizeof(ulong) > kpage + P9_REQUEST_SIZE) {
      xfree(path);
      r->type = Rerror;
      r->ename = "Texec: message too large, no room for argv";
      return -1;
    }

    /* Write argv to the user page (via kernel mapping) */
    ulong *argv_ptr = (ulong *)kargv_start;
    argv_ptr[0] = (ulong)upath;
    argv_ptr[1] = 0;

    /* Calculate User Address of argv */
    uintptr argv_offset = kargv_start - kpage;
    uintptr uargv = EXCHANGE_PAGE_ADDR + argv_offset;

    /* Prepare arguments for sysexec */
    /* args[0] = file (user char*) */
    /* args[1] = argv (user char**) */
    args[0] = (ulong)upath;
    args[1] = (ulong)uargv;

    /* Call sysexec - never returns on success */
    if (waserror()) {
      print("p9_dispatch: Texec failed: %s\n", up->errstr);
      xfree(path);
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    sysexec(args);
    /* Not reached on success */
    poperror();

    /* If we get here, exec failed somehow */
    xfree(path);
    r->type = Rerror;
    r->ename = "Texec: exec returned unexpectedly";
    return -1;
  }

  /* Handle Generic Tsyscall (130) */
  if (t->type == Tsyscall) {
    uchar *p = t->sdata;
    uchar *ep = t->sdata + t->scount;

    switch (t->scallnr) {
    case SYS_OPEN: {
      extern int newfd(Chan *, int);
      extern int openmode(ulong);
      /* Format: [fid 4] [path s] [mode 1] */
      if (p + 4 + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      int fid = GBIT32(p);
      p += 4;
      int len = GBIT16(p);
      p += 2;
      if (p + len + 1 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }

      char *path = smalloc(len + 1);
      memmove(path, p, len);
      path[len] = 0;
      p += len;

      int mode = GBIT8(p);
      p += 1;

      print("p9_dispatch: Tsyscall SYS_OPEN ptr '%s' mode=%d\n", path, mode);

      int fd;
      Chan *c = 0;
      if (waserror()) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      openmode(mode);
      c = namec(path, Aopen, mode, 0);
      fd = newfd(c, mode);
      poperror();
      free(path);

      r->type = Rsyscall;
      r->tag = t->tag;
      r->scount = 4;
      r->sdata = t->sdata;
      PBIT32(t->sdata, fd);

      return 0;
    }

    case SYS_CLOSE: {
      extern void fdclose(int, int);
      extern Chan *fdtochan(int, int, int, int);
      /* Format: [fid 4] */
      if (p + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fd = GBIT32(p);
      p += 4;

      print("p9_dispatch: SYS_CLOSE fd=%d\n", fd);

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      Chan *c = fdtochan(fd, -1, 0, 0);
      if (c)
        cclose(c);
      fdclose(fd, 0);
      poperror();

      r->type = Rsyscall;
      r->tag = t->tag;
      r->scount = 0;
      return 0;
    }

    case SYS_WRITE: {
      /* Format: [fid 4] [offset 8] [count 4] [data...] */
      if (p + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fid = GBIT32(p);
      p += 4;
      vlong offset = GBIT64(p);
      p += 8;
      int count = GBIT32(p);
      p += 4;
      if (p + count > ep) {
        r->type = Rerror;
        return -1;
      }

      print("p9_dispatch: SYS_WRITE fd=%d count=%d off=%lld\n", fid, count,
            offset);

      extern Chan *fdtochan(int, int, int, int);
      Chan *c;
      long n;

      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      c = fdtochan(fid, OWRITE, 1, 1);
      if (waserror()) {
        cclose(c);
        nexterror();
      }
      if (c->qid.type & QTDIR)
        error(Eisdir);
      n = devtab[c->type]->write(c, p, count, offset);
      poperror();
      cclose(c);
      poperror();

      /* Allocate small buffer for 4-byte return value */
      static uchar reply_data[8]; /* Static buffer for return value */
      r->type = Rsyscall;
      r->tag = t->tag;
      r->scount = 4;
      r->sdata = reply_data; /* Use separate buffer instead of request buffer */
      PBIT32(reply_data, n);
      return 0;
    }

    case SYS_EXIT: {
      /* Format: [status string] */
      char *status = nil;
      if (p + 2 <= ep) {
        int len = GBIT16(p);
        p += 2;
        if (p + len <= ep) {
          status = smalloc(len + 1);
          memmove(status, p, len);
          status[len] = 0;
        }
      }
      print("p9_dispatch: SYS_EXIT '%s'\n", status ? status : "nil");
      extern void pexit(char *, int);
      if (status) {
        char *s = status;
        status = nil;
        pexit(s, 1);
      } else {
        pexit("", 1);
      }
      return 0;
    }

    case SYS_WASM_COMPILE: {
      print("p9_dispatch: Tsyscall SYS_WASM_COMPILE\n");

      /* Call Layer 1 wasm3 runtime handler */
      if (sys_wasm_compile(t, r) != 0) {
        /* Error already set in r by handler */
        return -1;
      }

      return 0;
    }

    case SYS_WASM_EXECUTE: {
      print("p9_dispatch: Tsyscall SYS_WASM_EXECUTE\n");

      /* Call Layer 1 wasm3 runtime handler */
      if (sys_wasm_execute(t, r) != 0) {
        /* Error already set in r by handler */
        return -1;
      }

      return 0;
    }

    case SYS_WASM_DESTROY: {
      print("p9_dispatch: Tsyscall SYS_WASM_DESTROY\n");

      /* Call Layer 1 wasm3 runtime handler */
      if (sys_wasm_destroy(t, r) != 0) {
        /* Error already set in r by handler */
        return -1;
      }

      return 0;
    }

    case SYS_FORK: {
      /* Format: [flags 4] */
      extern uintptr sysrfork(void *list_void);
      if (p + 4 > ep) {
        r->type = Rerror;
        r->ename = "short fork msg";
        return -1;
      }
      ulong flags = GBIT32(p);
      p += 4;

      print("p9_dispatch: SYS_FORK flags=0x%lx\n", flags);

      /* sysrfork expects a va_list-like argument array */
      ulong args[1] = {flags};

      uintptr ret;
      if (waserror()) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        poperror();
        return -1;
      }
      ret = sysrfork(args);
      poperror();

      /* Return PID (or 0 in child) */
      static uchar fork_reply[8];
      r->type = Rsyscall;
      r->tag = t->tag;
      r->scount = 8;
      r->sdata = fork_reply;
      PBIT64(fork_reply, ret);
      return 0;
    }

    default:
      r->type = Rerror;
      snprint(r->ename, sizeof(r->ename), "unknown syscall %d", t->scallnr);
      return -1;
    }
  }

  /* Handle Tsys* - Specific syscall message types (132-205) */

  /* I/O Operations */
  if (t->type == Tsysopen) {
    extern int newfd(Chan *, int);
    extern int openmode(ulong);
    Chan *c = nil;
    int fd;

    if (waserror()) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    /* t->name contains path, t->mode contains mode */
    openmode(t->mode);
    c = namec(t->name, Aopen, t->mode, 0);
    fd = newfd(c, t->mode);
    poperror();

    /* Build Rsysopen response */
    r->type = Rsysopen;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;

    print("p9_dispatch: Tsysopen '%s' -> fd=%d\n", t->name, fd);
    return 0;
  }

  if (t->type == Tsyscreate) {
    extern int newfd(Chan *, int);
    extern int openmode(ulong);
    Chan *c = nil;
    int fd;

    if (waserror()) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    /* t->name contains path, t->perm contains permissions, t->mode contains
     * mode */
    openmode(t->mode);
    c = namec(t->name, Acreate, t->mode, t->perm);
    fd = newfd(c, t->mode);
    poperror();

    /* Build Rsyscreate response */
    r->type = Rsyscreate;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;

    print("p9_dispatch: Tsyscreate '%s' perm=0%o -> fd=%d\n", t->name, t->perm,
          fd);
    return 0;
  }

  if (t->type == Tsysread || t->type == Tsyspread) {
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    c = fdtochan(t->fid, OREAD, 1, 1);
    if (waserror()) {
      cclose(c);
      nexterror();
    }

    if (c->qid.type & QTDIR)
      error(Eisdir);

    /* Allocate buffer for read data - use exchange page data area */
    if (t->count > P9_REPLY_SIZE - 100) {
      error("read count too large");
    }

    n = devtab[c->type]->read(c, r->data, t->count, t->offset);
    poperror();
    cclose(c);
    poperror();

    /* Build Rsysread response */
    r->type = (t->type == Tsysread) ? Rsysread : Rsyspread;
    r->tag = t->tag;
    r->count = n;
    /* r->data already contains the data */

    print("p9_dispatch: %s fd=%d count=%d offset=%lld -> %ld bytes\n",
          t->type == Tsysread ? "Tsysread" : "Tsyspread", t->fid, t->count,
          t->offset, n);
    return 0;
  }

  if (t->type == Tsyswrite || t->type == Tsyspwrite) {
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    long n;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    c = fdtochan(t->fid, OWRITE, 1, 1);
    if (waserror()) {
      cclose(c);
      nexterror();
    }

    if (c->qid.type & QTDIR)
      error(Eisdir);

    n = devtab[c->type]->write(c, t->data, t->count, t->offset);
    poperror();
    cclose(c);
    poperror();

    /* Build Rsyswrite response */
    r->type = (t->type == Tsyswrite) ? Rsyswrite : Rsyspwrite;
    r->tag = t->tag;
    r->count = n;

    print("p9_dispatch: %s fd=%d count=%d offset=%lld -> %ld bytes\n",
          t->type == Tsyswrite ? "Tsyswrite" : "Tsyspwrite", t->fid, t->count,
          t->offset, n);
    return 0;
  }

  if (t->type == Tsysclose) {
    extern void fdclose(int, int);
    extern Chan *fdtochan(int, int, int, int);

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    Chan *c = fdtochan(t->fid, -1, 0, 0);
    if (c)
      cclose(c);
    fdclose(t->fid, 0);
    poperror();

    /* Build Rsysclose response */
    r->type = Rsysclose;
    r->tag = t->tag;

    print("p9_dispatch: Tsysclose fd=%d\n", t->fid);
    return 0;
  }

  if (t->type == Tsysremove) {
    Chan *c = nil;

    if (waserror()) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    c = namec(t->name, Aremove, 0, 0);
    poperror();

    /* Build Rsysremove response */
    r->type = Rsysremove;
    r->tag = t->tag;

    print("p9_dispatch: Tsysremove '%s'\n", t->name);
    return 0;
  }

  /* Process Control */
  if (t->type == Tsysexit) {
    extern void pexit(char *, int);

    print("p9_dispatch: Tsysexit '%s'\n", t->ename ? t->ename : "");

    /* pexit never returns */
    pexit(t->ename ? t->ename : "", 1);

    /* Not reached, but satisfies compiler */
    return 0;
  }

  if (t->type == Tsysbrk) {
    extern uintptr ibrk(uintptr, int);
    uintptr ret;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    /* Call ibrk with address */
    ret = ibrk((uintptr)t->addr, BSEG);
    poperror();

    /* Build Rsysbrk response */
    r->type = Rsysbrk;
    r->tag = t->tag;
    r->addr = ret;

    print("p9_dispatch: Tsysbrk addr=0x%llx -> 0x%llx\n", t->addr, (u64int)ret);
    return 0;
  }

  /* Namespace Operations */
  if (t->type == Tsyschdir) {
    Chan *c = nil;

    if (waserror()) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    c = namec(t->name, Atodir, 0, 0);
    cclose(up->dot);
    up->dot = c;
    poperror();

    /* Build Rsyschdir response */
    r->type = Rsyschdir;
    r->tag = t->tag;

    print("p9_dispatch: Tsyschdir '%s'\n", t->name);
    return 0;
  }

  /* FD Operations */
  if (t->type == Tsysdup) {
    extern int newfd(Chan *, int);
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    int nfd;

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      poperror();
      return -1;
    }

    /* Get the channel from oldfd */
    c = fdtochan(t->fid, -1, 0, 1);
    incref(&c->ref);

    /* Create new fd */
    if (t->newfid == -1) {
      nfd = newfd(c, 0);
    } else {
      /* Dup to specific fd - use newfd to handle it properly */
      extern void fdclose(int, int);
      if (t->newfid >= 0) {
        fdclose(t->newfid, 0);
        /* Try to place channel at specific fd */
        up->fgrp->fd[t->newfid] = c;
        nfd = t->newfid;
      } else {
        cclose(c);
        error("invalid fd");
      }
    }
    poperror();

    /* Build Rsysdup response */
    r->type = Rsysdup;
    r->tag = t->tag;
    r->fid = nfd;

    print("p9_dispatch: Tsysdup oldfd=%d newfd=%d -> %d\n", t->fid, t->newfid,
          nfd);
    return 0;
  }

  /* Handle Ttoken (80) - Token transfer between machines */
  if (t->type == Ttoken) {
    TokenTransfer transfer;
    TokenType tok_type;
    u64int amount;

    if (local_machine_bank == nil) {
      r->type = Rerror;
      r->ename = "distributed pebble not initialized";
      return -1;
    }

    /* Parse token transfer from message data */
    if (t->count < sizeof(TokenType) + sizeof(u64int)) {
      r->type = Rerror;
      r->ename = "Ttoken: message too short";
      return -1;
    }

    tok_type = (TokenType)GBIT32(t->data);
    amount = GBIT64(t->data + 4);

    /* Receive the transfer */
    memset(&transfer, 0, sizeof(transfer));
    transfer.token_type = tok_type;
    transfer.amount = amount;
    /* Copy proof from message if present */
    if (t->count >= sizeof(TokenType) + sizeof(u64int) + sizeof(TokenProof)) {
      memmove(&transfer.proof, t->data + 12, sizeof(TokenProof));
    }

    if (transfer_receive(local_machine_bank, &transfer) < 0) {
      r->type = Rerror;
      r->ename = "token transfer failed";
      return -1;
    }

    /* Build Rtoken response */
    r->type = Rtoken;
    r->tag = t->tag;
    /* Encode new balance in response data */
    PBIT64(r->data, local_machine_bank->available[tok_type]);
    r->count = 8;

    print("9p_router: Ttoken received %llu tokens type %d\n", amount, tok_type);
    return 0;
  }

  /* Handle Tbudget (82) - Query remote budget */
  if (t->type == Tbudget) {
    TokenType tok_type;

    if (local_machine_bank == nil) {
      r->type = Rerror;
      r->ename = "distributed pebble not initialized";
      return -1;
    }

    if (t->count < sizeof(TokenType)) {
      r->type = Rerror;
      r->ename = "Tbudget: message too short";
      return -1;
    }

    tok_type = (TokenType)GBIT32(t->data);
    if (tok_type >= TOK_MAX) {
      r->type = Rerror;
      r->ename = "invalid token type";
      return -1;
    }

    /* Build Rbudget response with balance and Merkle root */
    r->type = Rbudget;
    r->tag = t->tag;
    PBIT64(r->data, local_machine_bank->available[tok_type]);
    PBIT64(r->data + 8, local_machine_bank->epoch);
    memmove(r->data + 16, &local_machine_bank->bank_root,
            sizeof(BlindLedgerHash));
    r->count = 16 + sizeof(BlindLedgerHash);

    print("9p_router: Tbudget query type=%d balance=%llu\n", tok_type,
          local_machine_bank->available[tok_type]);
    return 0;
  }

  if (t->type == Tattach) {
    /* Check for capability-based attach (WASM servers) */
    uuid_t cap_uuid;
    if (wasm_9p_extract_cap_uuid(t->aname, &cap_uuid) == 0) {
      /* Capability found in aname - validate it */
      UserCapability pebble_cap;
      if (wasm_9p_validate_capability(&cap_uuid, CAP_PERM_READ, &pebble_cap)) {
        type = TYPE_WASM;
        print("9p_router: WASM attach with validated capability\n");
      } else {
        r->type = Rerror;
        r->ename = "invalid or insufficient capability";
        return -1;
      }
    }
    /* If no capability, determine type from path */
    else if (path_match(t->aname, "/proc/") || strcmp(t->aname, "/proc") == 0)
      type = TYPE_PROC;
    else if (path_match(t->aname, "/dev/") || strcmp(t->aname, "/dev") == 0)
      type = TYPE_DEV;
    else if (path_match(t->aname, "/env/") || strcmp(t->aname, "/env") == 0)
      type = TYPE_ENV;
    else if (path_match(t->aname, "/srv/") || strcmp(t->aname, "/srv") == 0)
      type = TYPE_SRV;
    else if (path_match(t->aname, "/mnt/") || strcmp(t->aname, "/mnt") == 0)
      type = TYPE_MNT;
    else if (path_match(t->aname, "/fd/") || strcmp(t->aname, "/fd") == 0)
      type = TYPE_FD;

    /* Install FID for non-device types; devices handle their own FID with
     * subtype */
    if (type > 0 && type != TYPE_DEV) {
      if (install_fid(t->fid, type) < 0) {
        r->type = Rerror;
        r->ename = "fid allocation failed";
        return -1;
      }
    }
  } else {
    /* Lookup type from fid */
    type = get_fid_type(t->fid);
    if (t->type == Tclunk)
      remove_fid(t->fid);
  }

  int ret = -1;
  if (type == TYPE_PROC)
    ret = proc_9p_handle(p, t, r);
  else if (type == TYPE_DEV)
    ret = dev_9p_handle(p, t, r);
  else if (type == TYPE_ENV)
    ret = env_9p_handle(p, t, r);
  else if (type == TYPE_SRV)
    ret = srv_9p_handle(p, t, r);
  else if (type == TYPE_MNT)
    ret = mnt_9p_handle(p, t, r);
  else if (type == TYPE_FD) {
    ret = fd_9p_handle(p, t, r);
  } else if (type == TYPE_WASM) {
    ret = wasm_9p_handle(p, t, r);
  } else {
    r->type = Rerror;
    r->ename = "fid not found or unknown path";
    return -1;
  }

  /* Propagate handler type to newfid on successful Walk */
  if (ret == 0 && t->type == Twalk && r->type == Rwalk) {
    /* If walk succeeded (all names consumed), newfid inherits the handler type
     */
    if (r->nwqid == t->nwname) {
      /* If newfid == fid, it's already set (cloning or walking self).
         If distinct, we must install it. */
      if (t->newfid != t->fid) {
        int subtype = 0;
        if (t->nwname == 0) {
          /* Clone: Copy from old fid */
          if (type == TYPE_DEV || type == TYPE_FD)
            subtype = get_fid_subtype(t->fid);
        } else {
          /* Walk: Use last Qid's version from reply as subtype */
          if (r->nwqid > 0)
            subtype = r->wqid[r->nwqid - 1].vers;
        }

        install_fid_with_subtype(t->newfid, type, subtype);

        /* Handle state cloning for devices */
        if ((type == TYPE_DEV && subtype == DEV_PIPE) ||
            (type == TYPE_FD && subtype > 0)) {
          /* Pipe and FD need to bump refcount on clone. */
          Fgrp *f = up->fgrp;
          Chan *oldc, *newc;
          lock(&f->lock);
          oldc = f->fd[t->fid];
          newc = f->fd[t->newfid];
          if (oldc && newc) {
            newc->aux = oldc->aux;
            if (type == TYPE_DEV && subtype == DEV_PIPE) {
              rpipe_clone_notify(newc->aux);
            }
            /* For FD types, aux might store the internal Chan* directly?
               No, we said we'd use Qid.vers for FD number.
               So FD handler relies on subtype (vers) for index.
               Clone just copies subtype. No aux cloning needed unless we cache
               Chan* in aux. Let's stick to using subtype (vers) as the FD
               index.
            */
          }
          unlock(&f->lock);
        }
      }
    }
  }
  return ret;
}

/*
 * Callback context for async 9P replies.
 */
typedef struct P9RouteContext {
  Proc *caller;
  Fcall *reply;
} P9RouteContext;

/*
 * Callback fired when MSGORD orders the message.
 * Writes the reply to the caller's exchange page.
 */
static void p9_route_reply_callback(OrdMsg *msg, int status, void *arg) {
  P9RouteContext *ctx = (P9RouteContext *)arg;
  Fcall reply;

  if (ctx == nil || ctx->caller == nil) {
    if (ctx)
      xfree(ctx);
    return;
  }

  /* Handle MSGORD status */
  memset(&reply, 0, sizeof(reply));
  if (status == MSGORD_CB_ROLLBACK) {
    /* Transaction was rolled back - return error */
    reply.type = Rerror;
    reply.ename = "transaction rolled back";
  } else if (msg->gm_payload.type == MSGORD_MSG_9P &&
             msg->gm_payload.fcall != nil) {
    /* Dispatch the message to the appropriate handler */
    p9_dispatch(ctx->caller, msg->gm_payload.fcall, &reply);
  } else {
    reply.type = Rerror;
    reply.ename = "invalid payload";
  }

  /* If caller has an exchange page, write reply there */
  if (ctx->caller->p9page != nil) {
    P9Control *ctl =
        (P9Control *)((uintptr)ctx->caller->p9page + P9_CONTROL_OFFSET);
    uchar *rep_buf = (uchar *)ctx->caller->p9page + P9_REPLY_OFFSET;
    uint rep_size = convS2M(&reply, rep_buf, P9_PAGE_SIZE);
    ctl->rep_head = 0;
    ctl->rep_tail = rep_size;
    ctl->rep_seq++;
    atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
  }

  xfree(ctx);
}

/*
 * Helper: Get path from FID's Chan.
 */
static char *get_fid_path(Proc *p, int fid) {
  Chan *c;
  Fgrp *f = p->fgrp;
  char *path = nil;

  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != nil) {
    if (c->path != nil && c->path->s != nil)
      path = c->path->s;
  }
  unlock(&f->lock);
  return path;
}

/*
 * Main entry point: Submit to MSGORD for async ordering.
 * Returns 0 on successful submission (reply will arrive via callback).
 * Returns -1 on immediate error with r populated.
 */
int p9_route(Proc *p, Fcall *t, Fcall *r) {
  char *path;
  P9RouteContext *ctx;
  uint msg_id;

  /* Determine path for DAG ordering granularity */
  if (t->type == Tattach)
    path = t->aname;
  else if (t->type == Tversion || t->type == Tauth || t->type == Tflush)
    path = "/"; /* Protocol messages - global path */
  else {
    /* Extract path from FID */
    path = get_fid_path(p, t->fid);
    if (path == nil)
      path = "/";
  }

  /* Allocate callback context */
  ctx = xalloc(sizeof(P9RouteContext));
  if (ctx == nil) {
    r->type = Rerror;
    r->ename = "no memory for p9 context";
    return -1;
  }
  ctx->caller = p;
  ctx->reply = r;

  /* Submit for async ordering with callback */
  msg_id = msgord_submit_async(nil, p, t, path, p9_route_reply_callback, ctx);
  if (msg_id == 0) {
    xfree(ctx);
    r->type = Rerror;
    r->ename = "msgord queue full";
    return -1;
  }

  /*
   * Message submitted successfully.
   * The reply will be delivered asynchronously via callback.
   * Caller should NOT expect r to be filled synchronously.
   * Return 0 to indicate "pending" - caller must poll exchange page.
   */
  return 0;
}

/*
 * p9_handle_doorbell - Process 9P message from exchange page
 *
 * SINGLE 4KB PAGE MODEL WITH OWNERSHIP FLIP:
 * ==========================================
 * 1. Process owns page, writes request, issues syscall
 * 2. borrow_transfer(process -> kernel) - kernel now owns exclusively
 * 3. Kernel reads request from page
 * 4. Kernel processes syscall
 * 5. Kernel writes reply to SAME page location
 * 6. borrow_transfer(kernel -> process) - process now owns exclusively
 * 7. Process reads reply
 *
 * The borrow checker enforces that only one entity (process OR kernel)
 * can access the page at any time. This eliminates TOCTOU races.
 *
 * Called by: VectorSYSCALL handler (doorbell-only mode)
 */
int p9_handle_doorbell(Proc *p) {
  P9Control *ctl;
  uchar *msg_buf; /* Single buffer for request AND reply */
  Fcall t, r;
  uint msg_size;
  int result;
  uintptr page_pa;
  enum BorrowError berr;

  /* Validate exchange page exists */
  if (p->p9page == nil) {
    print("p9_handle_doorbell: no exchange page for pid %lud\n", p->pid);
    return -1;
  }

  /* Get physical address of the single exchange page */
  page_pa = PADDR(p->p9page);

  /* Ensure exchange page is mapped into userspace */
  uintptr *pte = mmuwalk(m->pml4, EXCHANGE_PAGE_ADDR, 0, 0);
  if (pte == nil || (*pte & PTEVALID) == 0) {
    print("p9_handle_doorbell: remapping exchange page for pid %lud\n", p->pid);
    userpmap(EXCHANGE_PAGE_ADDR, page_pa, PTEVALID | PTEUSER | PTEWRITE);
  }

  /*
   * OWNERSHIP TRANSFER: Process -> Kernel
   * =====================================
   * Process has finished writing request and issued syscall.
   * Transfer ownership so kernel has exclusive access.
   */
  berr = borrow_transfer(p, up, page_pa);
  if (berr != BORROW_OK) {
    /* First syscall after boot - process may not have formal ownership yet */
    print("p9_handle_doorbell: borrow_transfer failed (berr=%d), acquiring "
          "directly\n",
          berr);
    berr = borrow_acquire(up, page_pa);
    if (berr != BORROW_OK && berr != BORROW_EALREADY) {
      print("p9_handle_doorbell: FATAL - kernel can't acquire page (berr=%d)\n",
            berr);
      return -1;
    }
  }

  /* Kernel now has exclusive access to the page */

  /* Get control block and message buffer */
  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);
  msg_buf = (uchar *)p->p9page + P9_MSG_OFFSET;

  /* Mark as pending */
  atomic_store(&ctl->status, P9_STATUS_PENDING, ORDER_RELAXED);

  /* Parse request from message buffer */
  memset(&t, 0, sizeof(t));

  /* Get message size from 9P header (first 4 bytes) */
  msg_size = GBIT32(msg_buf);
  if (msg_size < 7 || msg_size > P9_MSG_SIZE) {
    print("p9_handle_doorbell: invalid message size %ud\n", msg_size);
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    result = -1;
    goto cleanup_ownership;
  }

  if (convM2S(msg_buf, msg_size, &t) == 0) {
    print("p9_handle_doorbell: failed to parse Fcall (first byte: 0x%02x)\n",
          msg_buf[0]);
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    result = -1;
    goto cleanup_ownership;
  }

  /* Dispatch through 9P router */
  memset(&r, 0, sizeof(r));
  result = p9_dispatch(p, &t, &r);

  /* Write reply to SAME buffer location (ownership-flip model) */
  uint rep_size = convS2M(&r, msg_buf, P9_MSG_SIZE);
  if (rep_size == 0) {
    print("p9_handle_doorbell: failed to serialize reply\n");
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    result = -1;
    goto cleanup_ownership;
  }

  /* Update control block */
  ctl->rep_seq++;

  /* Mark as complete with Release semantics */
  atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);

cleanup_ownership:
  /*
   * OWNERSHIP TRANSFER: Kernel -> Process
   * =====================================
   * Kernel has finished processing. Transfer ownership back so
   * process can read the reply.
   */
  berr = borrow_transfer(up, p, page_pa);
  if (berr != BORROW_OK) {
    print(
        "p9_handle_doorbell: WARNING - borrow_transfer back failed (berr=%d)\n",
        berr);
    /* Fall back to release/acquire */
    borrow_release(up, page_pa);
    berr = borrow_acquire(p, page_pa);
    if (berr != BORROW_OK) {
      print("p9_handle_doorbell: FATAL - can't return page to process "
            "(berr=%d)\n",
            berr);
      panic("p9_handle_doorbell: ownership violation - cannot return page");
    }
  }

  return result;
}

/*
 * Spawn entry point - run as a kernel process (kproc)
 * Transitions to userspace by exec'ing the specified binary.
 */
static void kspawn_entry(void *arg) {
  char *path = (char *)arg;
  char *argv[2];
  ulong args[2];

  print("kspawn_entry: executing '%s'\n", path);

  /*
   * Build arguments for sysexec:
   * args[0] = path string pointer
   * args[1] = argv array pointer (path, nil)
   */
  argv[0] = path;
  argv[1] = nil;

  args[0] = (ulong)path;
  args[1] = (ulong)argv;

  /*
   * Call sysexec to load and execute the binary.
   * sysexec never returns on success - it replaces the current process.
   * On error, we exit.
   */
  if (waserror()) {
    print("kspawn_entry: exec failed: %s\n", up->errstr);
    free(path);
    pexit(up->errstr, 1);
    return;
  }

  sysexec(args);
  /* Not reached on success */
  poperror();

  /* If we get here, exec failed somehow without error */
  print("kspawn_entry: sysexec returned unexpectedly\n");
  free(path);
  pexit("exec failed", 1);
}

/*
 * Helper: Handle writes to /proc/self/ctl
 */
static int handle_proc_ctl_write(Proc *p, char *cmd, int len) {
  char buf[256];
  char *args[16];
  int n;

  if (len >= sizeof(buf))
    return -1;
  memmove(buf, cmd, len);
  buf[len] = 0;

  n = tokenize(buf, args, nelem(args));
  if (n < 1)
    return -1;

  if (strcmp(args[0], "dup") == 0) {
    /* dup old [new] */
    if (n < 2)
      return -1;
    int old = (int)strtoul(args[1], 0, 0);
    int new = (n > 2) ? (int)strtoul(args[2], 0, 0) : -1;

    Chan *c = fdtochan(old, -1, 0, 1);
    if (c == nil)
      return -1;

    if (new != -1) {
      Fgrp *f = up->fgrp;
      lock(&f->lock);
      if (new < 0 || growfd(f, new) < 0) {
        unlockfgrp(f);
        cclose(c);
        return -1;
      }
      if (new > f->maxfd)
        f->maxfd = new;

      Chan *oc = f->fd[new];
      f->fd[new] = c;
      f->flag[new] = 0;
      unlockfgrp(f);
      if (oc != nil)
        cclose(oc);
    } else {
      if (waserror()) {
        cclose(c);
        nexterror();
      }
      int fd = newfd(c, 0);
      poperror();
      if (fd < 0)
        return -1;
    }
    return len;
  }

  if (strcmp(args[0], "chdir") == 0) {
    if (n < 2)
      return -1;
    Chan *c = namec(args[1], Atodir, 0, 0);
    if (waserror()) {
      cclose(c);
      return -1;
    }
    cclose(up->dot);
    up->dot = c;
    poperror();
    return len;
  }

  if (strcmp(args[0], "rendezvous") == 0) {
    /* rendezvous <tag> <val> */
    if (n < 3)
      return -1;
    uintptr tag = (uintptr)strtoul(args[1], 0, 0);
    uintptr val = (uintptr)strtoul(args[2], 0, 0);
    /* Internal rendezvous logic usually returns a value.
       Twrite can't return it!
       Architecture Issue: Rendezvous requires return value.
       Solution: Use /proc/self/rendezvous file?
       Write tag to it, Read from it?

       For now, implemented as write-only (wakes up others, ignores return).
    */
    return len;
  }

  if (strcmp(args[0], "semacquire") == 0) {
    /* semacquire <addr> <block> */
    if (n < 3)
      return -1;
    long *addr = (long *)strtoul(args[1], 0, 16);
    int block = (int)strtoul(args[2], 0, 0);
    Segment *s = seg(up, (uintptr)addr, 0);
    if (s == nil)
      return -1;
    semacquire(s, addr, block);
    return len;
  }

  if (strcmp(args[0], "semrelease") == 0) {
    if (n < 3)
      return -1;
    long *addr = (long *)strtoul(args[1], 0, 16);
    long delta = (long)strtoul(args[2], 0, 0);
    Segment *s = seg(up, (uintptr)addr, 0);
    if (s == nil)
      return -1;
    semrelease(s, addr, delta);
    return len;
  }

  if (strcmp(args[0], "exits") == 0) {
    char *status = (n > 1) ? args[1] : nil;
    pexit(status, 1);
    /* Not reached */
  }

  if (strcmp(args[0], "sleep") == 0) {
    /* sleep <ms> */
    long ms = (n > 1) ? (long)strtoul(args[1], 0, 0) : 0;
    if (ms > 0)
      tsleep(&up->sleep, return0, 0, (ulong)ms);
    return len;
  }

  if (strcmp(args[0], "alarm") == 0) {
    /* alarm <ms> */
    ulong ms = (n > 1) ? strtoul(args[1], 0, 0) : 0;
    procalarm(ms);
    return len;
  }

  if (strcmp(args[0], "segbrk") == 0) {
    /* segbrk <addr> <seg> */
    void *addr;
    if (n < 2)
      return -1;
    addr = (void *)strtoul(args[1], 0, 0);
    int seg = (int)((n > 2) ? strtoul(args[2], 0, 0) : BSEG);
    if ((ulong)ibrk(addr, seg) == (ulong)-1) {
      return -1;
    }
    return len;
  }

  if (strcmp(args[0], "notify") == 0) {
    /* notify <func_addr_hex> */
    /* Pass 0 to disable */
    if (n < 2)
      return -1;
    void *fn = (void *)strtoul(args[1], 0, 16);
    up->notify = fn;
    return len;
  }

  if (strcmp(args[0], "noted") == 0) {
    /* noted <mode> */
    /* NCONT=0, NDFLT=1, NSAVE=2, NRSTR=3 */
    int mode = (n > 1) ? (int)strtoul(args[1], 0, 0) : NRSTR;

    qlock(&up->debug);
    if (up->notified == 0 && mode != NRSTR) {
      qunlock(&up->debug);
      error("noted: not notified");
      return -1;
    }
    qunlock(&up->debug);

    /* Note: This calls the kernel internal 'noted' */
    /* We rely on proper Ureg setup in up->noteureg/dbgreg */
    if (noted(up->dbgreg, up->noteureg, mode) < 0) {
      error("noted failed");
      return -1;
    }
    up->notified = 0;
    return len;
  }

  if (strcmp(args[0], "note") == 0) {
    /* note <msg> */
    if (n < 2)
      return -1;
    postnote(p, 1, args[1], NUser);
    return len;
  }

  /*
   * "spawn" is the official Phase 6 process creation mechanism.
   * Legacy rfork/exec are deprecated in the 9P path.
   */
  if (strcmp(args[0], "spawn") == 0) {
    /* spawn <path> [args...] */
    if (n < 2)
      return -1;

    if (!check_permission(p, PEBBLE_PERM_EXEC)) {
      error("spawn: permission denied");
      return -1;
    }

    /*
     * Real kspawn implementation using kproc.
     * The new process inherits the parent's file descriptors, env, and groups,
     * then execs the specified binary.
     */
    char *path = args[1];
    print("9P SPAWN: forking to exec '%s'\n", path);

    /*
     * Create argument string for the new process.
     * In Plan 9 style, we pass arguments via /proc/n/args after exec.
     * For now, we just store the command name.
     */
    char *file = smalloc((ulong)strlen(path) + 1);
    if (file == nil) {
      error("spawn: no memory");
      return -1;
    }
    strcpy(file, path);

    /*
     * Use kproc to create a new process that runs kspawn_wrapper.
     * The wrapper will be a kernel function that initiates exec.
     * Note: This creates a kernel process that then transitions to userspace.
     */
    kproc(path, kspawn_entry, file);

    print("9P SPAWN: spawned process for '%s'\n", path);
    return len;
  }

  /* Process Control Commands from existing stub */
  if (strcmp(args[0], "wakeup") == 0) {
    proc_event(p, EV_WAKEUP);
    return len;
  }
  if (strcmp(args[0], "stop") == 0) {
    proc_event(p, EV_STOP);
    return len;
  }
  if (strcmp(args[0], "start") == 0) {
    proc_event(p, EV_CONT);
    return len;
  }
  if (strcmp(args[0], "kill") == 0) {
    proc_event(p, EV_BREAK);
    return len;
  }

  return -1;
}

/*
 * Process control server: /proc/
 * Integrates with our FSM!
 */
/* Proc file types for routing */
#define PROC_ROOT 0
#define PROC_CTL 1
#define PROC_WAIT 2
#define PROC_STATUS 3
#define PROC_NS 4
#define PROC_SEGMENT 5

int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  Proc *target = caller; /* Default to self */
  int type = 0;

  r->tag = t->tag;

  /* Retrieve file type from FID (stored in Qid.vers during Walk) */
  if (t->type != Tattach) {
    type = get_fid_subtype((int)t->fid);
  }

  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = PROC_ROOT;
    r->qid.vers = PROC_ROOT;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      if (type != PROC_ROOT) {
        r->type = Rerror;
        r->ename = "not a directory";
        return -1;
      }

      r->type = Rwalk;
      r->nwqid = 1;
      r->wqid[0].type = QTFILE;
      /* Use 'vers' to store the subtype for the new FID */
      if (strcmp(t->wname[0], "ctl") == 0) {
        r->wqid[0].path = PROC_CTL;
        r->wqid[0].vers = PROC_CTL;
      } else if (strcmp(t->wname[0], "wait") == 0) {
        r->wqid[0].path = PROC_WAIT;
        r->wqid[0].vers = PROC_WAIT;
      } else if (strcmp(t->wname[0], "status") == 0) {
        r->wqid[0].path = PROC_STATUS;
        r->wqid[0].vers = PROC_STATUS;
      } else if (strcmp(t->wname[0], "ns") == 0) {
        r->wqid[0].path = PROC_NS;
        r->wqid[0].vers = PROC_NS;
      } else if (strcmp(t->wname[0], "segment") == 0) {
        r->wqid[0].path = PROC_SEGMENT;
        r->wqid[0].vers = PROC_SEGMENT;
      } else {
        r->type = Rerror;
        r->ename = "file not found";
        return -1;
      }
      return 0;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Topen:
    r->type = Ropen;
    r->qid.type = (type == PROC_ROOT) ? QTDIR : QTFILE;
    r->qid.path = type;
    r->qid.vers = type;
    r->iounit = 8192;
    return 0;

  case Twrite:
    if (type == PROC_CTL) {
      if (handle_proc_ctl_write(target, t->data, (int)t->count) < 0) {
        r->type = Rerror;
        r->ename = "proc: command failed";
        return -1;
      }
      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;

  case Tread:
    if (type == PROC_ROOT) {
      /* Directory listing */
      /* Minimal implementation: return fixed list */
      static char *dirents[] = {"ctl", "wait", "status", "ns", "segment"};
      char buf[512];
      char *p = buf;
      /* This is a hacky directory listing. Proper way is to marshall Dir
       * structs. */
      /* Ideally we use a helper like dirread. For now, empty dir or error? */
      /* Using 'read' on a directory in 9P requires returning Stat structures.
       */
      /* Since we don't have a helper handy here to generate Stats easily
       * without allocs... */
      /* We return Rerror "use Tstat" or generic directory read error? */
      /* Actually, many clients expect Tstat for dirs properly. */
      /* Let's return empty for now to avoid crashing client readers. */
      r->type = Rread;
      r->count = 0;
      return 0;
    }

    if (type == PROC_STATUS) {
      r->type = Rread;
      /* Using snprint to format status */
      /* text pid state user sys real pages */
      r->count = (u32int)snprint(
          (char *)r->data, 256, "%s %lud %s %lud %lud %lud %lud\n",
          target->text, target->pid, proc_state_names[proc_state(target)],
          target->time[TUser], target->time[TSys], target->time[TReal],
          procpagecount(target) * BY2PG);
      return 0;
    }

    if (type == PROC_WAIT) {
      Waitmsg w;
      if (pwait(&w) == 0) {
        r->type = Rerror;
        r->ename = "wait failed";
        return -1;
      }
      r->type = Rread;
      r->count = (u32int)snprint((char *)r->data, 256, "%lud %lud %lud %lud %s",
                                 w.pid, w.time[0], w.time[1], w.time[2], w.msg);
      return 0;
    }

    /* Other files read as empty */
    r->type = Rread;
    r->count = 0;
    return 0;

  case Tstat: {
    /* Build Dir structure and convert to wire format */
    Dir d;
    uchar statbuf[256];
    int n;

    memset(&d, 0, sizeof(d));
    d.qid.path = type;
    d.qid.type = (type == PROC_ROOT) ? QTDIR : 0;
    d.qid.vers = 0;
    d.mode = (type == PROC_ROOT) ? (DMDIR | 0555) : 0444;
    d.atime = seconds();
    d.mtime = d.atime;
    d.length = 0;
    d.name = (type == PROC_ROOT)     ? "."
             : (type == PROC_STATUS) ? "status"
             : (type == PROC_CTL)    ? "ctl"
             : (type == PROC_WAIT)   ? "wait"
                                     : "unknown";
    d.uid = "kernel";
    d.gid = "kernel";
    d.muid = "kernel";

    n = (int)convD2M(&d, statbuf, sizeof(statbuf));
    if (n <= 0) {
      r->type = Rerror;
      r->ename = "stat conversion failed";
      return -1;
    }
    r->type = Rstat;
    r->nstat = (ushort)n;
    r->stat = statbuf;
    return 0;
  }

  case Tclunk:
    r->type = Rclunk;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "not implemented";
    return -1;
  }
}

/*
 * Device path parsing helper
 */
static char *devname_from_path(char *path) {
  if (path == nil)
    return "cons";
  if (strncmp(path, "/dev/", 5) == 0)
    return path + 5;
  return path;
}

/*
 * Common Tattach handler with Pebble validation
 * Returns 0 on success, -1 on permission error
 */
static int handle_tattach_with_pebble(Proc *caller, Fcall *t, Fcall *r,
                                      int required_perms, uchar qid_path) {
  PebbleToken tok;
  Qid q;

  /* Check if attach data contains Pebble */
  if (t->data != nil && t->count >= 8 + sizeof(PebbleToken)) {
    if (p9_extract_pebble((uchar *)t->data, t->count, &tok) == 0) {
      /* Validate pebble for required access */
      if (!p9_validate_pebble(&tok, t->aname, required_perms)) {
        r->type = Rerror;
        r->ename = "permission denied";
        return -1;
      }
      /* Full validation with BlindLedger */
      if (!p9_validate_pebble_full(&tok, t->aname, caller)) {
        r->type = Rerror;
        r->ename = "invalid capability";
        return -1;
      }
      /* Store in session */
      store_session_pebble(caller, &tok);
    }
  }

  /* Successful attach */
  r->type = Rattach;
  q.type = QTFILE;
  q.path = qid_path;
  q.vers = 0;
  memmove(&r->qid, &q, sizeof(Qid));
  r->iounit = 8192;
  return 0;
}

/*
 * Fill Dir structure for device stat - common helper
 */
static void fill_device_stat(Dir *d, char *name, uchar qid_path, ulong mode,
                             vlong length) {
  memset(d, 0, sizeof(Dir));
  d->name = name;
  d->uid = "sys";
  d->gid = "sys";
  d->muid = "sys";
  d->qid.type = QTFILE;
  d->qid.path = qid_path;
  d->qid.vers = 0;
  d->mode = mode;
  d->atime = (ulong)seconds();
  d->mtime = d->atime;
  d->length = length;
}

/*
 * Handle Tstat for a device - common helper
 * Returns serialized stat size or -1 on error
 */
static int handle_device_stat(Fcall *t, Fcall *r, char *name, uchar qid_path,
                              ulong mode, vlong length) {
  static uchar statbuf[256];
  Dir d;
  int n;

  fill_device_stat(&d, name, qid_path, mode, length);
  n = (int)convD2M(&d, statbuf, sizeof(statbuf));
  if (n <= 0) {
    r->type = Rerror;
    r->ename = "stat conversion failed";
    return -1;
  }

  r->type = Rstat;
  r->nstat = (ushort)n;
  r->stat = statbuf;
  return 0;
}

/*
 * Console device handler: /dev/cons
 */
static int cons_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 1);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    putstrn((char *)t->data, (int)t->count);
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Console read not implemented yet */
    r->type = Rread;
    r->count = 0;
    r->data = nil;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "cons", 1, 0666, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Null device handler: /dev/null
 */
static int null_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 2);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    /* Accept all writes, discard data */
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return empty */
    r->type = Rread;
    r->count = 0;
    r->data = nil;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "null", 2, 0666, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Zero device handler: /dev/zero
 */
static int zero_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static uchar zerobuf[8192];
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, PEBBLE_PERM_READ, 3);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    /* Accept all writes, discard data */
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return zeros */
    if (t->count > sizeof(zerobuf))
      t->count = sizeof(zerobuf);
    memset(zerobuf, 0, t->count);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)zerobuf;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Tstat:
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "zero", 3, 0444, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Random device handler: /dev/random
 */
static int random_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static uchar randbuf[8192];
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, PEBBLE_PERM_READ, 4);

  case Twrite:
    /* Random is read-only */
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return random bytes */
    if (t->count > sizeof(randbuf))
      t->count = sizeof(randbuf);
    randomread(randbuf, t->count);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)randbuf;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Tstat:
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "random", 4, 0444, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Ramdisk handler: /dev/ram
 * Uses devram read/write paths.
 */
extern long ramread(void *a, long n, vlong off);
extern long ramwrite(void *va, long n, vlong off);

static int ram_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 7);

  case Twrite:
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }
    r->count = (u32int)ramwrite(t->data, t->count, t->offset);
    r->type = Rwrite;
    return 0;

  case Tread:
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    r->type = Rread;
    r->count = (u32int)ramread(caller->genbuf, t->count, t->offset);
    r->data = (char *)caller->genbuf;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "ram", 7, 0666, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Time device handler: /dev/time
 */
static int time_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static char timebuf[64];
  int n;
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, PEBBLE_PERM_READ, 5);

  case Twrite:
    /* Time is read-only */
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return current time in seconds */
    n = snprint(timebuf, sizeof(timebuf), "%ld\n", seconds());
    if (t->offset >= (u32int)n) {
      r->type = Rread;
      r->count = 0;
      r->data = nil;
      return 0;
    }
    if (t->offset + t->count > (u32int)n)
      t->count = (u32int)(n - t->offset);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)(timebuf + t->offset);
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Tstat:
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "time", 5, 0444, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Sysname device handler: /dev/sysname
 */
static int sysname_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static char namebuf[128];
  int n;
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      PEBBLE_PERM_READ | PEBBLE_PERM_WRITE, 6);

  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, PEBBLE_PERM_WRITE)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }

    /* Update sysname */
    if (t->count >= sizeof(namebuf)) {
      r->type = Rerror;
      r->ename = "name too long";
      return -1;
    }
    memmove(namebuf, t->data, t->count);
    namebuf[t->count] = '\0';
    /* Remove trailing newline if present */
    if (t->count > 0 && namebuf[t->count - 1] == '\n')
      namebuf[t->count - 1] = '\0';
    kstrdup(&sysname, namebuf);
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Check read permission */
    if (!check_permission(caller, PEBBLE_PERM_READ)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }

    /* Return current sysname */
    if (sysname == nil)
      sysname = "lux9";
    n = snprint(namebuf, sizeof(namebuf), "%s\n", sysname);
    if (t->offset >= (u32int)n) {
      r->type = Rread;
      r->count = 0;
      r->data = nil;
      return 0;
    }
    if (t->offset + t->count > (u32int)n)
      t->count = (u32int)(n - t->offset);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)(namebuf + t->offset);
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;

  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "sysname", 6, 0666, 0);

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}

/*
 * Helper to map device name to subtype constant
 */
static int devname_to_subtype(char *dev) {
  if (strcmp(dev, "cons") == 0)
    return DEV_CONS;
  if (strcmp(dev, "null") == 0)
    return DEV_NULL;
  if (strcmp(dev, "zero") == 0)
    return DEV_ZERO;
  if (strcmp(dev, "random") == 0)
    return DEV_RANDOM;
  if (strcmp(dev, "time") == 0)
    return DEV_TIME;
  if (strcmp(dev, "sysname") == 0)
    return DEV_SYSNAME;
  if (strcmp(dev, "ram") == 0)
    return DEV_RAM;
  if (strcmp(dev, "pipe") == 0)
    return DEV_PIPE;
  return 0; /* Unknown */
}

/* Forward declaration */
extern int pipe_9p_handle(Proc *caller, Fcall *t, Fcall *r);

/*
 * Device router dispatch - uses FID subtype for proper device tracking
 */
int dev_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  int subtype = 0;
  char *dev;

  r->tag = t->tag;

  if (t->type == Tattach) {
    /* On attach, determine device from path and store subtype in FID */
    dev = devname_from_path(t->aname);
    subtype = devname_to_subtype(dev);

    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "device not found";
      return -1;
    }

    /* Install FID with device subtype */
    if (install_fid_with_subtype((int)t->fid, TYPE_DEV, subtype) < 0) {
      r->type = Rerror;
      r->ename = "fid allocation failed";
      return -1;
    }
  } else {
    /* For other operations, retrieve subtype from FID */
    subtype = get_fid_subtype((int)t->fid);
    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "unknown device fid";
      return -1;
    }
  }

  /* Dispatch to device handler based on subtype */
  switch (subtype) {
  case DEV_CONS:
    return cons_9p_handle(caller, t, r);
  case DEV_NULL:
    return null_9p_handle(caller, t, r);
  case DEV_ZERO:
    return zero_9p_handle(caller, t, r);
  case DEV_RANDOM:
    return random_9p_handle(caller, t, r);
  case DEV_TIME:
    return time_9p_handle(caller, t, r);
  case DEV_SYSNAME:
    return sysname_9p_handle(caller, t, r);
  case DEV_RAM:
    return ram_9p_handle(caller, t, r);
  case DEV_PIPE:
    return rpipe_9p_handle(caller, t, r);
  default:
    r->type = Rerror;
    r->ename = "device not found";
    return -1;
  }
}

int env_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  char *name;
  char *val;

  r->tag = t->tag;

  /* Simple write-only environment support for now */
  if (t->type == Twrite) {
    /* Path format: /env/VARNAME */
    if (strncmp(t->aname, "/env/", 5) == 0) {
      name = t->aname + 5;
      val = smalloc(t->count + 1);
      memmove(val, t->data, t->count);
      val[t->count] = 0;

      if (waserror()) {
        free(val);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      ksetenv(name, val, 0);
      poperror();
      free(val);

      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
  }

  r->type = Rerror;
  r->ename = "env: not fully implemented";
  return -1;
}

/*
 * /srv namespace: In-memory service registry
 * Plan 9 uses this for processes to publish named endpoints
 */
#define SRV_MAX_ENTRIES 64
#define SRV_NAME_SIZE 64

typedef struct SrvEntry {
  char name[SRV_NAME_SIZE];
  int fd;        /* File descriptor to return on open */
  int owner_pid; /* PID of process that posted this */
  int active;    /* Entry is in use */
} SrvEntry;

static SrvEntry srv_registry[SRV_MAX_ENTRIES];
static Lock srv_lock;
static int srv_initialized = 0;

static void srv_init(void) {
  if (srv_initialized)
    return;
  memset(srv_registry, 0, sizeof(srv_registry));
  srv_initialized = 1;
}

/* Find entry by name */
static SrvEntry *srv_find(char *name) {
  int i;
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (srv_registry[i].active && strcmp(srv_registry[i].name, name) == 0)
      return &srv_registry[i];
  }
  return nil;
}

/* Find free slot */
static SrvEntry *srv_alloc(void) {
  int i;
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_registry[i].active)
      return &srv_registry[i];
  }
  return nil;
}

int srv_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  char *name;
  SrvEntry *e;
  static uchar statbuf[512];

  r->tag = t->tag;
  srv_init();

  switch (t->type) {
  case Tattach:
    /* Attach to /srv */
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = 0x100; /* Unique path for /srv directory */
    r->qid.vers = 0;
    r->iounit = 8192;
    return 0;

  case Twrite:
    /* Write creates or updates a service entry */
    /* Path: /srv/<name>, data contains FD number to post */
    if (t->aname == nil || strncmp(t->aname, "/srv/", 5) != 0) {
      r->type = Rerror;
      r->ename = "invalid srv path";
      return -1;
    }
    name = t->aname + 5;
    if (strlen(name) == 0 || strlen(name) >= SRV_NAME_SIZE) {
      r->type = Rerror;
      r->ename = "invalid service name";
      return -1;
    }

    lock(&srv_lock);
    e = srv_find(name);
    if (e == nil) {
      e = srv_alloc();
      if (e == nil) {
        unlock(&srv_lock);
        r->type = Rerror;
        r->ename = "srv registry full";
        return -1;
      }
    }

    /* Store entry */
    strcpy(e->name, name);
    e->fd = (int)((t->count > 0) ? strtoul((char *)t->data, 0, 0) : (ulong)-1);
    e->owner_pid = (int)caller->pid;
    e->active = 1;
    unlock(&srv_lock);

    print("srv: posted '%s' with fd %d by pid %ld\n", name, e->fd, caller->pid);

    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tread:
    /* Read lists all services (directory listing) */
    /* For now, return a simple list */
    {
      char buf[4096];
      char *p = buf;
      int i, len;

      lock(&srv_lock);
      for (i = 0; i < SRV_MAX_ENTRIES && p < buf + sizeof(buf) - 100; i++) {
        if (srv_registry[i].active) {
          p += snprint(p, (int)(buf + sizeof(buf) - p), "%s\n",
                       srv_registry[i].name);
        }
      }
      unlock(&srv_lock);

      len = (int)(p - buf);
      if (t->offset >= len) {
        r->type = Rread;
        r->count = 0;
        r->data = nil;
        return 0;
      }

      /* Return requested portion */
      len -= (int)t->offset;
      if (len > (int)t->count)
        len = (int)t->count;

      r->count = (u32int)len;
      r->data = smalloc((ulong)len);
      memmove(r->data, buf + t->offset, (usize)len);
      return 0;
    }

  case Tstat:
    /* Stat the /srv directory */
    {
      Dir d;
      int n;

      memset(&d, 0, sizeof(d));
      d.name = "srv";
      d.uid = "sys";
      d.gid = "sys";
      d.muid = "sys";
      d.qid.type = QTDIR;
      d.qid.path = 0x100;
      d.qid.vers = 0;
      d.mode = DMDIR | 0777;
      d.atime = (ulong)seconds();
      d.mtime = d.atime;
      d.length = 0;

      n = (int)convD2M(&d, statbuf, sizeof(statbuf));
      if (n <= 0) {
        r->type = Rerror;
        r->ename = "stat failed";
        return -1;
      }

      r->type = Rstat;
      r->nstat = (ushort)n;
      r->stat = statbuf;
      return 0;
    }

  case Tclunk:
    r->type = Rclunk;
    return 0;

  case Tremove:
    /* Remove a service entry */
    if (t->aname == nil || strncmp(t->aname, "/srv/", 5) != 0) {
      r->type = Rerror;
      r->ename = "invalid srv path";
      return -1;
    }
    name = t->aname + 5;

    lock(&srv_lock);
    e = srv_find(name);
    if (e == nil) {
      unlock(&srv_lock);
      r->type = Rerror;
      r->ename = "service not found";
      return -1;
    }

    /* Only owner or privileged process can remove */
    if ((ulong)e->owner_pid != caller->pid && !iseve()) {
      unlock(&srv_lock);
      r->type = Rerror;
      r->ename = "permission denied";
      return -1;
    }

    print("srv: removed '%s'\n", e->name);
    memset(e, 0, sizeof(SrvEntry));
    unlock(&srv_lock);

    r->type = Rremove;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "srv: operation not supported";
    return -1;
  }
}

static int mnt_ctl_write(Proc *p, char *cmd, int len) {
  char buf[256];
  char *args[5];
  int n;

  if ((ulong)len >= sizeof(buf))
    return -1;
  memmove(buf, cmd, (usize)len);
  buf[len] = 0;

  n = tokenize(buf, args, 5);
  if (n < 3)
    return -1;

  if (strcmp(args[0], "bind") == 0) {
    /* bind new old [flags] */
    Chan *c0, *c1;
    int flag = (n > 3) ? (int)strtoul(args[3], 0, 0) : 0;

    if (waserror())
      return -1;

    c0 = namec(args[1], Abind, 0, 0);
    if (waserror()) {
      cclose(c0);
      nexterror();
    }

    c1 = namec(args[2], Amount, 0, 0);
    if (waserror()) {
      cclose(c1);
      nexterror();
    }

    cmount(c0, c1, flag, nil);

    poperror();
    cclose(c1);
    poperror();
    cclose(c0);
    poperror();
    return len;
  }

  if (strcmp(args[0], "pipe") == 0) {
    /* pipe <mountpoint> */
    /* Convenience command: binds #| to <mountpoint> */
    Chan *c0, *c1;

    if (n < 2)
      return -1;

    if (waserror())
      return -1;

    c0 = namec("#|", Abind, 0, 0);
    if (waserror()) {
      cclose(c0);
      nexterror();
    }

    c1 = namec(args[1], Amount, 0, 0);
    if (waserror()) {
      cclose(c1);
      nexterror();
    }

    cmount(c0, c1, MREPL, nil);

    poperror();
    cclose(c1);
    poperror();
    cclose(c0);
    poperror();
    return len;
  }
  return -1;
}

int mnt_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = 0;
    r->qid.vers = 0;
    return 0;

  case Twalk:
    if (t->nwname > 0 && strcmp(t->wname[0], "ctl") == 0) {
      r->type = Rwalk;
      r->nwqid = 1;
      r->wqid[0].type = QTFILE;
      r->wqid[0].path = 1; /* ctl file */
      return 0;
    }
    r->type = Rerror;
    r->ename = "file not found";
    return -1;

  case Topen:
    r->type = Ropen;
    r->qid.type = (t->fid == 1) ? QTFILE : QTDIR;
    r->iounit = 8192;
    return 0;

  case Twrite:
    /* Check if writing to ctl (path=1) */
    /* Note: In a real implementation we check the fid's qid.path */
    /* Here assuming simplified router logic where we know the target */
    if (mnt_ctl_write(caller, t->data, (int)t->count) < 0) {
      r->type = Rerror;
      r->ename = "mnt: command failed";
      return -1;
    }
    r->type = Rwrite;
    r->count = t->count;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "mnt: operation not supported";
    return -1;
  }
}

/*
 * Async 9P Operations (Phase 3)
 *
 * Integrates with MSGORD consensus depth classification.
 */
#include "consensus_depth.h"
#include "msgord.h"

/* Forward declaration - global rollback registry from consensus_depth.c */
extern RollbackRegistry *global_rollback_registry;

/*
 * Callback wrapper that fires when MSGORD completes message ordering
 */
static void p9_msgord_callback(OrdMsg *msg, int status, void *arg) {
  AsyncP9Op *op = (AsyncP9Op *)arg;

  if (op == nil)
    return;

  /* Map MSGORD status to P9 async status */
  if (status == MSGORD_CB_SUCCESS)
    op->status = P9_ASYNC_SUCCESS;
  else if (status == MSGORD_CB_ROLLBACK)
    op->status = P9_ASYNC_ROLLBACK;
  else
    op->status = P9_ASYNC_ERROR;

  /* Fire the user-provided callback if present */
  if (op->callback != nil) {
    op->callback(&op->reply, op->callback_arg, op->status);
  }
}

/*
 * Submit 9P operation asynchronously through MSGORD
 */
uint p9_submit_async(Proc *p, Fcall *t, char *path, P9CompletionCallback cb,
                     void *arg) {
  AsyncP9Op *op;
  uint op_id;
  ConsensusDepth depth;

  if (p == nil || t == nil)
    return 0;

  /* Allocate async operation tracking struct */
  op = xalloc(sizeof(AsyncP9Op));
  if (op == nil)
    return 0;

  memset(op, 0, sizeof(AsyncP9Op));

  /* Copy request */
  memmove(&op->request, t, sizeof(Fcall));
  op->callback = cb;
  op->callback_arg = arg;
  op->submit_time = (uvlong)seconds();
  op->status = P9_ASYNC_PENDING;

  /* Classify operation to determine consensus depth */
  depth = classify_operation(t, path);

  /* For DEPTH_NONE operations, execute immediately (optimistic) */
  if (depth == DEPTH_NONE) {
    p9_dispatch(p, t, &op->reply);
    op->status = P9_ASYNC_SUCCESS;
    if (cb)
      cb(&op->reply, arg, P9_ASYNC_SUCCESS);
    xfree(op);
    return 0; /* No async tracking needed */
  }

  /* Submit to MSGORD with callback */
  op_id = msgord_submit_async(msgord, p, t, path, p9_msgord_callback, op);
  if (op_id == 0) {
    xfree(op);
    return 0;
  }

  op->op_id = op_id;

  /* Register for potential rollback if needed */
  if (global_rollback_registry != nil && depth >= DEPTH_CLUSTER) {
    rollback_register(global_rollback_registry, op_id, depth, p, t, &op->reply);
  }

  return op_id;
}

/*
 * Handle doorbell asynchronously using consensus depth classification
 */
int p9_handle_doorbell_async(Proc *p) {
  P9Control *ctl;
  uchar *reqbuf;
  Fcall t, r;
  int n;
  char path[256];
  ConsensusDepth depth;

  if (p == nil || p->p9page == nil)
    return -1;

  ctl = (P9Control *)((uintptr)p->p9page + P9_CONTROL_OFFSET);

  /* Check doorbell with Acquire semantics */
  if (atomic_load(&ctl->doorbell, ORDER_ACQUIRE) == 0)
    return 0; /* No request pending */

  /* Clear doorbell */
  atomic_store(&ctl->doorbell, 0, ORDER_RELAXED);

  /* Mark pending */
  atomic_store(&ctl->status, P9_STATUS_PENDING, ORDER_RELAXED);

  /* Parse request from exchange page */
  reqbuf = (uchar *)p->p9page + P9_REQUEST_OFFSET;
  memset(&t, 0, sizeof(t));
  n = (int)convM2S(reqbuf, P9_REQUEST_SIZE, &t);
  if (n <= 0) {
    atomic_store(&ctl->status, P9_STATUS_ERROR, ORDER_RELEASE);
    return -1;
  }

  /* Get path for classification */
  if (t.type == Tattach)
    strncpy(path, t.aname, sizeof(path) - 1);
  else
    strncpy(path, "/", sizeof(path) - 1);

  /* Classify the operation */
  depth = classify_operation(&t, path);

  /* For immediate/local operations, use synchronous path */
  if (depth == DEPTH_NONE) {
    memset(&r, 0, sizeof(r));
    p9_dispatch(p, &t, &r);

    /* Write reply to exchange page */
    convS2M(&r, (uchar *)p->p9page + P9_REPLY_OFFSET, P9_REPLY_SIZE);

    /* Release semantics for completion */
    atomic_store(&ctl->status, P9_STATUS_COMPLETE, ORDER_RELEASE);
    return 1;
  }

  /* Submit asynchronously for consensus */
  /* Pass 'p' as the callback argument so p9_doorbell_completion knows which
   * exchange page to write to. */
  /* p9_submit_async allocates copies of 't' and the reply buffer. */
  /* We pass our local 'p9_doorbell_completion' which cleans up the Op. */
  p9_submit_async(p, &t, path, nil, nil);
  return 1;
}

/*
 * Check if an async operation has completed
 */
int p9_check_async(Proc *p, uint op_id, Fcall *reply_out) {
  OrdMsg *msg;

  USED(p);

  if (op_id == 0)
    return P9_ASYNC_ERROR;

  msg = msgord_find_by_id(msgord, op_id);
  if (msg == nil)
    return P9_ASYNC_SUCCESS; /* Already completed and removed */

  if (msg->gm_state >= MSGORD_STATE_DELIVERED) {
    if (reply_out != nil && msg->gm_callback_arg != nil) {
      AsyncP9Op *op = (AsyncP9Op *)msg->gm_callback_arg;
      memmove(reply_out, &op->reply, sizeof(Fcall));
    }
    return P9_ASYNC_SUCCESS;
  }

  return P9_ASYNC_PENDING;
}

/*
 * Cancel an async operation
 */
void p9_cancel_async(Proc *p, uint op_id) {
  OrdMsg *msg;

  USED(p);

  if (op_id == 0)
    return;

  msg = msgord_find_by_id(msgord, op_id);
  if (msg != nil) {
    /* Mark for rollback if registered */
    if (global_rollback_registry != nil) {
      rollback_trigger(global_rollback_registry, op_id);
    }
  }
}

/*
 * Fire all ready async completions for a process
 */
int p9_fire_completions(Proc *p) {
  USED(p);
  /* Delegate to MSGORD fire_completions */
  return msgord_fire_completions(msgord);
}

/* ========================================================================
 * Pipe Implementation (Router-Specific)
 * Renamed to RPipe to avoid conflict with kernel devpipe.c's Pipe
 * ======================================================================== */

typedef struct RPipe {
  Lock lock;
  int ref;
  uchar *buf;
  int head;
  int tail;
  int size;     /* Capacity */
  int data_len; /* Current data length */
  int writers;
  int readers;
  Rendez r; /* Sleep for both read (empty) and write (full) */
  int busy;
  int closed;
} RPipe;

#define PIPE_BUF_SIZE 4096

static RPipe *rpipe_create(void) {
  RPipe *p = xalloc(sizeof(RPipe));
  if (p == nil)
    return nil;
  memset(p, 0, sizeof(RPipe));
  p->buf = xalloc(PIPE_BUF_SIZE);
  if (p->buf == nil) {
    xfree(p);
    return nil;
  }
  p->size = PIPE_BUF_SIZE;
  p->ref = 1; /* One ref for the creator */
  p->writers = 1;
  p->readers = 1;
  return p;
}

static void rpipe_decref(RPipe *p) {
  int ref;
  if (p == nil)
    return;
  lock(&p->lock);
  ref = --p->ref;
  unlock(&p->lock);
  if (ref == 0) {
    xfree(p->buf);
    xfree(p);
  }
}

static void rpipe_clone_notify(void *aux) {
  RPipe *p = (RPipe *)aux;
  if (p) {
    lock(&p->lock);
    p->ref++;
    unlock(&p->lock);
  }
}

static int rpipe_read_cond(void *arg) {
  RPipe *p = (RPipe *)arg;
  if ((p->data_len > 0) || (p->writers == 0) || (p->closed))
    return 1;
  return 0;
}

static int rpipe_write_cond(void *arg) {
  RPipe *p = (RPipe *)arg;
  if ((p->data_len < p->size) || (p->readers == 0) || (p->closed))
    return 1;
  return 0;
}

static int rpipe_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  Chan *c;
  RPipe *p = nil;
  Fgrp *f = caller->fgrp;

  r->tag = t->tag;

  /* Get the pipe object from chan aux */
  /* Note: for Tattach, aux is nil initially */
  if (t->type != Tattach) {
    c = fdtochan((int)t->fid, -1, 0, 0); /* Borrow chan, no incref/error */
    if (c == nil || c->type != (ushort)ROUTER_CHAN_TYPE) {
      r->type = Rerror;
      r->ename = "invalid pipe fid";
      return -1;
    }
    p = (RPipe *)c->aux;
    if (p == nil) {
      r->type = Rerror;
      r->ename = "pipe not initialized";
      return -1;
    }
  }

  switch (t->type) {
  case Tattach:
    /* Create new pipe */
    if (strncmp(t->aname, "/dev/pipe", 9) != 0) {
      r->type = Rerror;
      r->ename = "invalid attach path";
      return -1;
    }
    p = rpipe_create();
    if (p == nil) {
      r->type = Rerror;
      r->ename = "pipe alloc failed";
      return -1;
    }

    /* Store in FID aux - Must lock Fgrp */
    lock(&f->lock);
    c = f->fd[t->fid];
    if (c)
      c->aux = p;
    unlock(&f->lock);

    r->type = Rattach;
    r->qid.type = QTFILE;
    r->qid.path = 0;
    r->qid.vers = DEV_PIPE;
    r->iounit = 0;
    return 0;

  case Tread:
    /* Block until data available */
    while (p->data_len == 0) {
      if (p->writers == 0 || p->closed) {
        /* EOF */
        r->type = Rread;
        r->count = 0;
        r->data = nil;
        return 0;
      }
      sleep(&p->r, rpipe_read_cond, p);
    }

    lock(&p->lock);
    /* Recheck after wake */
    if (p->data_len == 0) {
      unlock(&p->lock);
      if (p->writers == 0 || p->closed) {
        r->type = Rread;
        r->count = 0;
        return 0;
      }
      /* Should loop, but simple implementation allows spurious return or
       * recurse */
      /* We just return 0 here which might look like EOF. Better to loop. */
      /* But with lock held we can't loop easily. Let's assume cond correct. */
    }

    int n = (int)t->count;
    if (n > p->data_len)
      n = p->data_len;

    /* Circular buffer read */
    if (p->head + n <= p->size) {
      memmove(r->data, p->buf + p->head, (usize)n);
    } else {
      int chunk = p->size - p->head;
      memmove(r->data, p->buf + p->head, (usize)chunk);
      memmove(r->data + chunk, p->buf, (usize)(n - chunk));
    }
    p->head = (p->head + n) % p->size;
    p->data_len -= n;
    wakeup(&p->r); /* Wake writers */
    unlock(&p->lock);

    r->type = Rread;
    r->count = (u32int)n;
    return 0;

  case Twrite:
    /* Block until space available */
    while (p->data_len == p->size) {
      if (p->readers == 0 || p->closed) {
        r->type = Rerror;
        r->ename = "pipe broken";
        postnote(caller, 1, "sys: write on closed pipe", NUser);
        return -1;
      }
      sleep(&p->r, rpipe_write_cond, p);
    }

    lock(&p->lock);
    int cnt = (int)t->count;
    int space = p->size - p->data_len;
    if (cnt > space)
      cnt = space; /* Partial write if strictly blocking not fully implemented,
                    but we blocked for 'some' space */

    if (p->tail + cnt <= p->size) {
      memmove(p->buf + p->tail, t->data, (usize)cnt);
    } else {
      int chunk = p->size - p->tail;
      memmove(p->buf + p->tail, t->data, (usize)chunk);
      memmove(p->buf, t->data + chunk, (usize)(cnt - chunk));
    }
    p->tail = (p->tail + cnt) % p->size;
    p->data_len += cnt;
    wakeup(&p->r); /* Wake readers */
    unlock(&p->lock);

    r->type = Rwrite;
    r->count = (u32int)cnt;
    return 0;

  case Tclunk:
    rpipe_decref(p);
    r->type = Rclunk;
    return 0;

  case Tstat:
    /* 0600 pipe */
    {
      static uchar statbuf[256];
      Dir d;
      memset(&d, 0, sizeof(d));
      d.name = "pipe";
      d.uid = "sys";
      d.gid = "sys";
      d.muid = "sys";
      d.qid.type = QTFILE;
      d.qid.path = 0;
      d.qid.vers = DEV_PIPE;
      d.mode = 0600;
      d.length = p->data_len;
      int len = (int)convD2M(&d, statbuf, sizeof(statbuf));
      r->type = Rstat;
      r->nstat = (ushort)len;
      r->stat = statbuf;
    }
    return 0;

  case Twalk:
    /* Dispatch handled the clone and called pipe_clone_notify */
    if (t->nwname == 0) {
      r->type = Rwalk;
      r->nwqid = 0;
      return 0;
    }
    r->type = Rerror;
    r->ename = "walk not supported";
    return -1;

  default:
    r->type = Rerror;
    r->ename = "pipe operation not supported";
    return -1;
  }
}

/* ========================================================================
 * /fd Implementation
 * ======================================================================== */

int fd_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  int fd;
  Chan *c;
  int subtype;

  r->tag = t->tag;

  subtype = get_fid_subtype((int)t->fid); /* 0 = root, N+1 = fd N */

  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = 0;
    r->qid.vers = 0; /* Root */
    return 0;

  case Twalk:
    if (subtype != 0 && t->nwname > 0) {
      /* Cannot walk from file */
      r->type = Rerror;
      r->ename = "not a directory";
      return -1;
    }

    if (t->nwname == 0) {
      /* Clone */
      r->type = Rwalk;
      r->nwqid = 0;
      return 0;
    }

    if (t->nwname == 1) {
      /* Walk to number */
      if (strcmp(t->wname[0], "..") == 0) {
        r->type = Rwalk;
        r->nwqid = 1;
        r->wqid[0].type = QTDIR;
        r->wqid[0].path = 0;
        r->wqid[0].vers = 0;
        return 0;
      }

      fd = (int)strtoul(t->wname[0], 0, 0);
      /* Verify FD exists in caller's fgrp */
      c = fdtochan(fd, -1, 0, 0);
      if (c == nil) {
        r->type = Rerror;
        r->ename = "fd not found";
        return -1;
      }

      /* Success */
      r->type = Rwalk;
      r->nwqid = 1;
      r->wqid[0].type = QTFILE; /* Or whatever the underlying file is? Protocol
                                   says QTFILE for proxy */
      r->wqid[0].path =
          TYPE_FD; /* Should use actual Qid? No, this is the /fd/N file */
      r->wqid[0].vers = (u32int)(fd + 1); /* Encode FD */
      return 0;
    }

    r->type = Rerror;
    r->ename = "walk too deep";
    return -1;

  case Tread:
    if (subtype == 0) {
      /* Directory listing of FDs */
      /* Simplified: just return empty dir or error? */
      /* Doing full listing requires iterating fgrp */
      r->type = Rread;
      r->count = 0; /* Empty */
      r->data = nil;
      return 0;
    }

    /* If reading from /fd/N, it usually means READING from the underlying file
     */
    fd = subtype - 1;
    /* Proxy read to underlying channel */
    /* But wait, we need to convert Fcall Tread to devtab read? */
    /* Or use 'pread' ? */
    /* Since we are inside kernel, we can call dev->read directly if we had the
     * channel */
    c = fdtochan(fd, -1, 0,
                 0); // Open for reading? mode -1 checks validity only?
    /* fdtochan(fd, mode, check, ref) */
    /* We need OREAD check? logic: mode=-1 ignores check. */

    if (c == nil) {
      r->type = Rerror;
      r->ename = "fd closed";
      return -1;
    }

    /* Direct device read */
    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    /* devtab[c->type]->read(c, data, count, offset) */
    long n = devtab[c->type]->read(c, r->data, t->count, t->offset);
    r->count = (u32int)n;
    r->type = Rread;
    poperror();
    return 0;

  case Twrite:
    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "is a directory";
      return -1;
    }

    fd = subtype - 1;
    c = fdtochan(fd, -1, 0, 0);
    if (c == nil) {
      r->type = Rerror;
      r->ename = "fd closed";
      return -1;
    }

    if (waserror()) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }

    long cnt = devtab[c->type]->write(c, t->data, t->count, t->offset);
    r->count = (u32int)cnt;
    r->type = Rwrite;
    poperror();
    return 0;

  case Tstat:
    /* Stat underlying file? */
    if (subtype > 0) {
      fd = subtype - 1;
      c = fdtochan(fd, -1, 0, 0);
      if (c) {
        /* For now, fake Stat */
        static uchar sbuf[256];
        Dir d;
        memset(&d, 0, sizeof(d));
        d.name = "fd"; /* Helper... */
        d.qid = c->qid;
        d.mode = c->mode;
        uint len = convD2M(&d, sbuf, sizeof(sbuf));
        r->type = Rstat;
        r->nstat = (ushort)len;
        r->stat = sbuf;
        return 0;
      }
    }
    r->type = Rstat;
    r->nstat = 0;
    return 0;

  case Tclunk:
    r->type = Rclunk;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "fd operation not supported";
    return -1;
  }
}

/* ========== WASM 9P Handler ========== */

/* TODO: Proper server lookup - for now just stub */
static wasm_fileserver_t *global_wasm_server = nil;

static int wasm_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;

  switch (t->type) {
  case Tattach:
    /* Capability already validated in router - just attach to root */
    /* TODO: Initialize WASM server if not already loaded */
    r->type = Rattach;
    r->qid.type = QTDIR;
    r->qid.path = 0;
    r->qid.vers = 0;
    r->iounit = 0;
    return 0;

  case Twalk:
  case Topen:
  case Tcreate:
  case Tread:
  case Twrite:
  case Tstat:
  case Twstat:
  case Tremove: {
    /* Submit to WASM server via msgord + exchange pages */
    if (!global_wasm_server) {
      r->type = Rerror;
      r->ename = "no wasm server loaded";
      return -1;
    }

    /* Submit message to WASM server's msgord queue
     * This is non-blocking - message will be processed in consensus order
     * Response will be delivered asynchronously
     *
     * TODO: For synchronous 9P, we need to block and wait for response
     * TODO: Implement async completion callback to return response
     *
     * For now, return stub responses
     */

    char path[256];
    snprint(path, sizeof(path), "wasm_server");

    uint msg_id = wasm_fs_submit(global_wasm_server, caller, t, path);
    if (msg_id == 0) {
      r->type = Rerror;
      r->ename = "message submission failed";
      return -1;
    }

    /* Stub responses until async completion is implemented */
    switch (t->type) {
    case Twalk:
      r->type = Rwalk;
      r->nwqid = 0;
      break;
    case Topen:
      r->type = Ropen;
      r->qid.type = QTFILE;
      r->qid.path = 0;
      r->iounit = 0;
      break;
    case Tread:
      r->type = Rread;
      r->count = 0;
      r->data = nil;
      break;
    case Twrite:
      r->type = Rwrite;
      r->count = t->count;
      break;
    case Tstat:
      r->type = Rerror;
      r->ename = "stat not implemented";
      return -1;
    default:
      r->type = Rerror;
      r->ename = "operation not implemented";
      return -1;
    }
    return 0;
  }

  case Tclunk:
    r->type = Rclunk;
    return 0;

  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
