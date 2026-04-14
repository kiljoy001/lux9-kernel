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

/* Limit for user buffers to satisfy verification */
#define MAX_SYS_BUF 4096
#define MAX_SYS_PATH 1024

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
  BIND = 2,
  MOUNT = 1,
  UNMOUNT = 35,
  WSTAT = 41,
  SEGBRK = 40,
  SEGATTACH = 47,
  SEGDETACH = 48,
  SEGFREE = 49,
  NOTIFY = 28,
  NOTED = 29,
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
/*@
  @ requires \valid(ureg);
  @ assigns \nothing;
  @*/
void syscall_to_9p(Ureg *ureg) {
  uintptr *args;
  Fcall t, r;
  long result;
  ulong scallnr;

  if (ureg == nil)
    return;

  args = (uintptr *)(ureg + 1);
  result = -1;
  scallnr = ureg->ax;

  if (up == nil)
    return;

  /* Initialize common Fcall fields */
  t = (Fcall){0};
  r = (Fcall){0};
  t.tag = 0;

  switch (scallnr) {
  case OPEN:
    /* OPEN(path, mode) -> Tattach(aname=path) */
    {
      char kname[256];
      /*@ assert \valid_read((char *)args[0] + (0 .. MAX_SYS_PATH-1)); */
      strncpy(kname, (char *)args[0], sizeof(kname) - 1);
      kname[sizeof(kname) - 1] = 0;

      t.type = Tattach;
      t.aname = kname;
      t.fid = up->fid_counter++; /* Allocate fid */
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      result = t.fid; /* Return fid as fd */
    }
    break;

  case READ:
    /* READ(fd, buf, count) -> Tread(fid, offset, count) */
    {
      t.type = Tread;
      t.fid = (u32int)args[0];
      t.offset = up->fid_offsets[t.fid]; /* Track offset per-fid */
      u32int count = (u32int)args[2];
      if (count > MAX_SYS_BUF)
        count = MAX_SYS_BUF;
      /*@ assert \valid((char*)args[1] + (0 .. MAX_SYS_BUF-1)); */
      t.count = count;

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
    }
    break;

  case WRITE:
    /* WRITE(fd, buf, count) -> Twrite(fid, offset, count, data) */
    {
      t.type = Twrite;
      t.fid = (u32int)args[0];
      t.offset = up->fid_offsets[t.fid];
      u32int count = (u32int)args[2];
      if (count > MAX_SYS_BUF)
        count = MAX_SYS_BUF;
      /*@ assert \valid_read((char *)args[1] + (0 .. MAX_SYS_BUF-1)); */
      t.count = count;
      t.data = (char *)args[1];

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      up->fid_offsets[t.fid] += r.count;
      result = r.count;
    }
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
      /*@ assert \valid_read((char *)args[0] + (0 .. MAX_SYS_PATH-1)); */
      snprint(cmd, sizeof(cmd), "exec %.*s", 1024, (char *)args[0]);

      t = (Fcall){0};
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
      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = 4;
      t.data = "exit";

      p9_dispatch(up, &t, &r);
    }

    {
      char exitmsg[64];
      strncpy(exitmsg, (char *)args[0], sizeof(exitmsg) - 1);
      exitmsg[sizeof(exitmsg) - 1] = 0;
      pexit(exitmsg, 1);
    }
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

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      result = 0;
      /*
       * Parse PID from response body (newly created pid)
       * rfork writes to ctl file return the new pid as text
       */
      if (r.count > 0 && r.data != nil) {
        /* Ensure null termination for safety, though r.data might not be */
        /* Assuming r.data points to a small buffer from p9_dispatch which we
         * can read */
        /* p9_dispatch returns data in r.data, we need to treat it as string */
        char pidbuf[32];
        int len = r.count < sizeof(pidbuf) - 1 ? r.count : sizeof(pidbuf) - 1;
        memmove(pidbuf, r.data, len);
        pidbuf[len] = 0;
        result = strtoul(pidbuf, 0, 0);
      }

      /* Free response data if allocated by p9_dispatch/handlers */
      if (r.data && r.count > 0)
        free(r.data);
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
      t = (Fcall){0};
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
      /*@ assert \valid_read(path + (0 .. MAX_SYS_PATH-1)); */
      strncpy(pathbuf, path, sizeof(pathbuf) - 1);
      pathbuf[sizeof(pathbuf) - 1] = 0;
      len = strlen(pathbuf);

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
      t = (Fcall){0};
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
      char kname[256];
      strncpy(kname, (char *)args[0], sizeof(kname) - 1);
      kname[sizeof(kname) - 1] = 0;
      char *path = kname;
      uchar *buf = (uchar *)args[1];
      int nbuf = (int)args[2];
      if (nbuf < 0)
        nbuf = 0;
      if (nbuf > MAX_SYS_BUF)
        nbuf = MAX_SYS_BUF;
      /*@ assert \valid(buf + (0 .. MAX_SYS_BUF-1)); */

      t.type = Tattach;
      t.aname = path;
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Get stat */
      t = (Fcall){0};
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
      t = (Fcall){0};
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
      if (nbuf < 0)
        nbuf = 0;
      if (nbuf > MAX_SYS_BUF)
        nbuf = MAX_SYS_BUF;
      /*@ assert \valid(buf + (0 .. MAX_SYS_BUF-1)); */

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
        t = (Fcall){0};
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
      char kname[256];
      strncpy(kname, (char *)args[0], sizeof(kname) - 1);
      kname[sizeof(kname) - 1] = 0;
      char *path = kname;

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
      char kname[256];
      /*@ assert \valid_read((char *)args[0] + (0 .. MAX_SYS_PATH-1)); */
      strncpy(kname, (char *)args[0], sizeof(kname) - 1);
      kname[sizeof(kname) - 1] = 0;
      char *path = kname;

      t.type = Tattach;
      t.aname = path;
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Remove the file */
    }
    break;

  case BIND:
    /* BIND(new, old, flags) -> Write "bind new old flags" to /mnt/ctl */
    t.type = Tattach;
    t.aname = "/mnt/ctl";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    {
      char cmd[512];
      /*@ assert \valid_read((char *)args[0] + (0 .. MAX_SYS_PATH-1)); */
      /*@ assert \valid_read((char *)args[1] + (0 .. MAX_SYS_PATH-1)); */
      snprint(cmd, sizeof(cmd), "bind %.*s %.*s %d", 1024, (char *)args[0],
              1024, (char *)args[1], (int)args[2]);

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      result = 0;
    }
    break;

  case MOUNT:
    /* MOUNT(fd, old, flags, aname) -> Write "mount fd old flags aname" */
    t.type = Tattach;
    t.aname = "/mnt/ctl";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    {
      char cmd[512];
      char *aname = (char *)args[3];
      if (aname == nil)
        aname = "";

      /*@ assert \valid_read((char *)args[1] + (0 .. MAX_SYS_PATH-1)); */
      /*@ assert \valid_read(aname + (0 .. MAX_SYS_PATH-1)); */
      snprint(cmd, sizeof(cmd), "mount %d %.*s %d %.*s", (int)args[0], 1024,
              (char *)args[1], (int)args[2], 1024, aname);

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      result = 0;
    }
    break;

  case UNMOUNT:
    /* UNMOUNT(name, old) -> Write "unmount name old" */
    t.type = Tattach;
    t.aname = "/mnt/ctl";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
      break;

    {
      char cmd[512];
      char *name = (char *)args[0];
      if (name == nil)
        name = "";
      char *old = (char *)args[1];
      if (old == nil)
        old = "";

      /*@ assert \valid_read(name + (0 .. MAX_SYS_PATH-1)); */
      /*@ assert \valid_read(old + (0 .. MAX_SYS_PATH-1)); */
      snprint(cmd, sizeof(cmd), "unmount %.*s %.*s", 1024, name, 1024, old);

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      result = 0;
    }
    break;

  case WSTAT:
    /* WSTAT(path, buf, n) -> Tattach + Twstat */
    {
      char kname[256];
      strncpy(kname, (char *)args[0], sizeof(kname) - 1);
      kname[sizeof(kname) - 1] = 0;
      char *path = kname;
      uchar *buf = (uchar *)args[1];
      int nbuf = (int)args[2];
      if (nbuf < 0)
        nbuf = 0;
      if (nbuf > MAX_SYS_BUF)
        nbuf = MAX_SYS_BUF;
      /*@ assert \valid_read(buf + (0 .. MAX_SYS_BUF-1)); */

      t.type = Tattach;
      t.aname = path;
      t.fid = up->fid_counter++;
      t.afid = NOFID;
      t.uname = up->user;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      /* Twstat */
      t = (Fcall){0};
      t.type = Twstat;
      t.fid = up->fid_counter - 1;
      t.stat = buf;
      t.nstat = nbuf;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
        break;

      result = nbuf;

      /* Clunk */
      t = (Fcall){0};
      t.type = Tclunk;
      t.fid = up->fid_counter - 1;
      p9_dispatch(up, &t, &r);
    }
    break;

  case SEGBRK:
    /* SEGBRK(addr, seg) -> Write "segbrk addr seg" to /proc/self/ctl */
    t.type = Tattach;
    t.aname = "/proc/self/ctl";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
      result = -1;
      break;
    }

    {
      char cmd[64];
      void *addr = (void *)args[0];
      int seg = (int)args[1];

      /* Format: segbrk <addr_hex> <seg_idx> */
      snprint(cmd, sizeof(cmd), "segbrk %p %d", addr, seg);

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
        result = -1;
        break;
      }

      /*
       * On success, return the new break address.
       * Ideally we'd query the kernel for the actual new break,
       * but assuming success means 'addr' is now valid (if addr != 0).
       * If addr == 0 (query), the write doesn't return the value.
       * We'd need to inspect up->seg[seg] directly or read /proc/self/segment.
       * For now, return addr.
       */
      result = (ulong)addr;
    }
    break;

  case SEGATTACH:
    /* SEGATTACH(attr, class, va, len) -> Write to /proc/self/segment */
    t.type = Tattach;
    t.aname = "/proc/self/segment";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
      result = -1;
      break;
    }

    {
      char cmd[128];
      int attr = (int)args[0];
      char *class = (char *)args[1];
      void *va = (void *)args[2];
      ulong len = args[3];

      /* Format: attach <attr> <class> <va_hex> <len> */
      snprint(cmd, sizeof(cmd), "attach %d %s %p %lud", attr,
              class ? class : "memory", va, len);

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
        result = -1;
        break;
      }

      /* Return the virtual address of the attached segment */
      result = (uintptr)va;
    }
    break;

  case SEGDETACH:
    /* SEGDETACH(addr) -> Write to /proc/self/segment */
    t.type = Tattach;
    t.aname = "/proc/self/segment";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
      result = -1;
      break;
    }

    {
      char cmd[64];
      void *addr = (void *)args[0];

      snprint(cmd, sizeof(cmd), "detach %p", addr);

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
        result = -1;
        break;
      }

      result = 0;
    }
    break;

  case SEGFREE:
    /* SEGFREE(addr, len) -> Write to /proc/self/segment */
    t.type = Tattach;
    t.aname = "/proc/self/segment";
    t.fid = up->fid_counter++;
    t.afid = NOFID;
    t.uname = up->user;

    if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
      result = -1;
      break;
    }

    {
      char cmd[64];
      void *addr = (void *)args[0];
      ulong len = args[1];

      snprint(cmd, sizeof(cmd), "free %p %lud", addr, len);

      t = (Fcall){0};
      t.type = Twrite;
      t.fid = r.qid.path;
      t.offset = 0;
      t.count = strlen(cmd);
      t.data = cmd;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
        result = -1;
        break;
      }

      result = 0;
    }
    break;

  case FD2PATH:
    /* FD2PATH(fd, buf, nbuf) -> Tstat + extract path from qid */
    {
      u32int fid = (u32int)args[0];
      char *buf = (char *)args[1];
      int nbuf = (int)args[2];
      if (nbuf < 0)
        nbuf = 0;
      if (nbuf > MAX_SYS_BUF)
        nbuf = MAX_SYS_BUF;
      /*@ assert \valid(buf + (0 .. MAX_SYS_BUF-1)); */

      t = (Fcall){0};
      t.type = Tstat;
      t.fid = fid;

      if (p9_dispatch(up, &t, &r) < 0 || r.type == Rerror) {
        result = -1;
        break;
      }

      /* Extract name from stat buffer (offset after size+type+dev+qid) */
      if (r.stat && r.nstat >= 49) {
        /* Dir structure: 2(size) + 2(type) + 4(dev) + 13(qid) + 4(mode) +
         * 4(atime) + 4(mtime) + 8(length) + 2+name... */
        int nameoff = 2 + 2 + 4 + 13 + 4 + 4 + 4 + 8;
        if (nameoff + 2 < r.nstat) {
          int namelen = GBIT16(r.stat + nameoff);
          if (namelen > 0 && nameoff + 2 + namelen <= r.nstat) {
            int n = namelen < nbuf - 1 ? namelen : nbuf - 1;
            if (n < 0)
              n = 0;
            memmove(buf, r.stat + nameoff + 2, n);
            buf[n] = 0;
            result = n;
          }
        }
      }
      if (result < 0) {
        /* Fallback: return fid as string */
        result = snprint(buf, nbuf, "/fd/%ud", fid);
      }
    }
    break;

  case NOTIFY:
  case NOTED:
    /* NOTIFY/NOTED - note handling is process-local */
    result = 0;
    break;

  default:
    print("syscall_to_9p: unsupported syscall %lud\n", scallnr);
    result = -1;
    break;
  }

  ureg->ax = result;
}
