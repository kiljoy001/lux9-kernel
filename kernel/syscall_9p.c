/*
 * Pure 9P Syscall Elimination Layer
 *
 * Replaces traditional syscalls with direct 9P protocol dispatch.
 * All I/O operations route through the 9P router with MSGORD ordering.
 *
 * Phase 6: Complete syscall removal
 */

#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include "ureg.h"
#include <error.h>

/* Legacy syscall numbers for translation */
enum {
  RFORK = 19,
  OPEN = 14,
  READ = 15,
  WRITE = 20,
  CLOSE = 4,
  EXEC = 7,
  EXITS = 8,
  PIPE = 21,
  CREATE = 22,
  FD2PATH = 23,
  SEEK = 39,
  STAT = 42,
  FSTAT = 43,
};

/*
 * syscall_to_9p - Pure 9P dispatch replacing dosyscall()
 *
 * Called from trap handler when VectorSYSCALL fires.
 * Translates legacy syscall arguments into 9P Fcall messages.
 * Routes through p9_dispatch() with Pebble security and MSGORD ordering.
 *
 * Returns: result in ureg->ax, -1 on error
 */
void syscall_to_9p(Ureg *ureg) {
  Fcall t, r;
  ulong scallnr;
  uintptr *args;
  long result;

  scallnr = ureg->bp;                   /* RARG - syscall number */
  args = (uintptr *)(ureg->sp + BY2WD); /* Skip return address slot */

  memset(&t, 0, sizeof(t));
  memset(&r, 0, sizeof(r));

  /* Default error response */
  result = -1;

  switch (scallnr) {
  case OPEN:
    /* OPEN(path, mode) -> Tattach(aname=path) */
    t.type = Tattach;
    t.aname = (char *)args[0];
    t.fid = up->fid_counter++; /* Allocate fid */
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    result = t.fid; /* Return fid as fd */
    break;

  case READ:
    /* READ(fd, buf, count) -> Tread(fid, offset, count) */
    t.type = Tread;
    t.fid = (u32int)args[0];
    t.offset = up->fid_offsets[t.fid]; /* Track offset per-fid */
    t.count = (u32int)args[2];

    /* Allocate response buffer */
    r.data = smalloc(t.count);
    if (r.data == nil)
      error(Enomem);

    if (waserror()) {
      free(r.data);
      nexterror();
    }

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
      poperror();
      free(r.data);
      break;
    }

    /* Copy data to userspace */
    memmove((void *)args[1], r.data, r.count);
    up->fid_offsets[t.fid] += r.count;
    result = r.count;

    poperror();
    free(r.data);
    break;

  case WRITE:
    /* WRITE(fd, buf, count) -> Twrite(fid, offset, count, data) */
    t.type = Twrite;
    t.fid = (u32int)args[0];
    t.offset = up->fid_offsets[t.fid];
    t.count = (u32int)args[2];
    t.data = (char *)args[1];

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    up->fid_offsets[t.fid] += r.count;
    result = r.count;
    break;

  case CLOSE:
    /* CLOSE(fd) -> Tclunk(fid) */
    t.type = Tclunk;
    t.fid = (u32int)args[0];

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    result = 0;
    break;

  case EXEC:
    /* EXEC(path, argv) -> Write to /proc/self/ctl */
    t.type = Tattach;
    t.aname = "/proc/self/ctl";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    /* Write "exec <path>" command */
    {
      char cmd[256];
      snprint(cmd, sizeof(cmd), "exec %s", (char *)args[0]);

      memset(&t, 0, sizeof(t));
      t.type = Twrite;
      t.fid = r.qid.path; /* Use fid from attach */
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;
    }

    result = 0;
    break;

  case EXITS:
    /* EXITS(msg) -> Write to /proc/self/ctl */
    t.type = Tattach;
    t.aname = "/proc/self/ctl";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    /* Write "exit" command */
    {
      memset(&t, 0, sizeof(t));
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = 4;
      t.data = "exit";

      p9_dispatch(up, &t, &r);
    }

    pexit((char *)args[0], 1);
    /* NOTREACHED */
    break;

  case RFORK:
    /* RFORK(flags) -> Write to /proc/self/ctl */
    t.type = Tattach;
    t.aname = "/proc/self/ctl";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    /* Write "rfork <flags>" command */
    {
      char cmd[64];
      snprint(cmd, sizeof(cmd), "rfork %lud", args[0]);

      memset(&t, 0, sizeof(t));
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Result is new PID in response */
      result = 0; /* TODO: parse PID from response */
    }
    break;

  case PIPE:
    /* PIPE(fds) -> Tattach("/dev/pipe") + clone read/write fids */
    {
      int *fds = (int *)args[0];
      u32int rfid, wfid;

      t.type = Tattach;
      t.aname = "/dev/pipe";
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Allocate two fids for read/write ends */
      rfid = up->fid_counter++;
      wfid = up->fid_counter++;

      /* Walk to clone the fid for write end */
      memset(&t, 0, sizeof(t));
      t.type = Twalk;
      t.fid = up->fid_counter - 3; /* Original pipe fid */
      t.newfid = wfid;
      t.nwname = 0; /* Clone fid */

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Store fids in userspace array */
      fds[0] = rfid; /* Read end */
      fds[1] = wfid; /* Write end */
      up->fid_offsets[rfid] = 0;
      up->fid_offsets[wfid] = 0;
      result = 0;
    }
    break;

  case CREATE:
    /* CREATE(path, mode, perm) -> Tattach(dir) + Tcreate(name) */
    {
      char *path = (char *)args[0];
      int mode = (int)args[1];
      ulong perm = args[2];
      char *name, pathbuf[256];
      int i, len;

      /* Extract filename from path */
      len = strlen(path);
      strncpy(pathbuf, path, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = 0;

      name = pathbuf + len;
      while (name > pathbuf && name[-1] != '/')
        name--;

      if (name > pathbuf) {
        name[-1] = 0; /* Split directory from name */
        t.aname = pathbuf;
      } else {
        t.aname = ".";
        name = pathbuf;
      }

      /* Attach to directory */
      t.type = Tattach;
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Create file */
      memset(&t, 0, sizeof(t));
      t.type = Tcreate;
      t.fid = up->fid_counter - 1;
      t.name = name;
      t.perm = (u32int)perm;
      t.mode = (uchar)mode;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      up->fid_offsets[t.fid] = 0;
      result = t.fid;
    }
    break;

  case STAT:
    /* STAT(path, buf, nbuf) -> Tattach + Tstat */
    {
      char *path = (char *)args[0];
      uchar *buf = (uchar *)args[1];
      int nbuf = (int)args[2];

      t.type = Tattach;
      t.aname = path;
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Get stat */
      memset(&t, 0, sizeof(t));
      t.type = Tstat;
      t.fid = up->fid_counter - 1;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Copy stat buffer */
      if (r.stat && r.nstat > 0) {
        int n = r.nstat < nbuf ? r.nstat : nbuf;
        memmove(buf, r.stat, n);
        result = n;
      } else {
        result = 0;
      }

      /* Clunk the fid */
      memset(&t, 0, sizeof(t));
      t.type = Tclunk;
      t.fid = up->fid_counter - 1;
      p9_dispatch(up, &t, &r);
    }
    break;

  case FSTAT:
    /* FSTAT(fd, buf, nbuf) -> Tstat */
    {
      u32int fid = (u32int)args[0];
      uchar *buf = (uchar *)args[1];
      int nbuf = (int)args[2];

      t.type = Tstat;
      t.fid = fid;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Copy stat buffer */
      if (r.stat && r.nstat > 0) {
        int n = r.nstat < nbuf ? r.nstat : nbuf;
        memmove(buf, r.stat, n);
        result = n;
      } else {
        result = 0;
      }
    }
    break;

  case SEEK:
    /* SEEK(fd, offset, type) -> update fid_offsets */
    {
      u32int fid = (u32int)args[0];
      vlong *ret = (vlong *)args[1];
      vlong offset = *(vlong *)args[2];
      int type = (int)args[3];

      switch (type) {
      case 0: /* SEEK_SET */
        up->fid_offsets[fid] = offset;
        break;
      case 1: /* SEEK_CUR */
        up->fid_offsets[fid] += offset;
        break;
      case 2: /* SEEK_END - need stat to get size */
        memset(&t, 0, sizeof(t));
        t.type = Tstat;
        t.fid = fid;
        if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
          break;
        /* Parse length from stat - at offset 8+QIDSZ (length field) */
        if (r.stat && r.nstat >= 8 + 13 + 8) {
          uvlong len = GBIT64(r.stat + 8 + 13);
          up->fid_offsets[fid] = len + offset;
        }
        break;
      }
      if (ret)
        *ret = up->fid_offsets[fid];
      result = up->fid_offsets[fid];
    }
    break;

  case 5: /* DUP */
    /* DUP(oldfd, newfd) -> Twalk to clone fid */
    {
      u32int oldfid = (u32int)args[0];
      int newfd = (int)args[1];
      u32int newfid;

      if (newfd >= 0) {
        newfid = (u32int)newfd;
      } else {
        newfid = up->fid_counter++;
      }

      t.type = Twalk;
      t.fid = oldfid;
      t.newfid = newfid;
      t.nwname = 0; /* Clone fid */

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Copy offset */
      up->fid_offsets[newfid] = up->fid_offsets[oldfid];
      result = newfid;
    }
    break;

  case 3: /* CHDIR */
    /* CHDIR(path) -> Tattach and update up->dot */
    {
      char *path = (char *)args[0];

      t.type = Tattach;
      t.aname = path;
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Update current directory fid */
      up->dot_fid = t.fid;
      result = 0;
    }
    break;

  case 25: /* REMOVE */
    /* REMOVE(path) -> Tattach + Tremove */
    {
      char *path = (char *)args[0];

      t.type = Tattach;
      t.aname = path;
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Remove the file */
      memset(&t, 0, sizeof(t));
      t.type = Tremove;
      t.fid = up->fid_counter - 1;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      result = 0;
    }
    break;

  default:
    print("syscall_to_9p: unsupported syscall %lud\n", scallnr);
    result = -1;
    break;
  }

  ureg->ax = result;
}
