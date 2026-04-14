#include "../inc/server9p.h"
#include "../src/lux_internal.h"

/* Default hash size for FIDs */
#define FID_HASH_SIZE 64

/* Helper: pebble_malloc wrapper */
static void *srv_malloc(ulong size) {
  void *p;
  if (pebble_alloc(size, &p) < 0)
    return nil;
  return p;
}

static void srv_free(void *p) { pebble_free(p); }

/* Helper: Create a Qid */
void mkqid(Qid *q, u64int path, u32int vers, u8int type) {
  q->path = path;
  q->vers = vers;
  q->type = type;
}

/* Internal: Get FID from hash */
static Fid *get_fid(Srv *s, u32int fid) {
  if (!s->fidhash)
    return nil;
  Fid *f = s->fidhash[fid % s->fidhashsize];
  while (f) {
    if (f->fid == fid)
      return f;
    f = f->next;
  }
  return nil;
}

/* Internal: Add FID to hash */
static Fid *alloc_fid(Srv *s, u32int fid) {
  Fid *f = srv_malloc(sizeof(Fid));
  if (!f)
    return nil;
  memset(f, 0, sizeof(Fid));
  f->fid = fid;
  f->omode = -1;

  int bucket = fid % s->fidhashsize;
  f->next = s->fidhash[bucket];
  s->fidhash[bucket] = f;
  return f;
}

/* Internal: Remove FID from hash */
static void free_fid(Srv *s, Fid *f) {
  int bucket = f->fid % s->fidhashsize;
  Fid **prev = &s->fidhash[bucket];
  while (*prev) {
    if (*prev == f) {
      *prev = f->next;
      srv_free(f);
      return;
    }
    prev = &(*prev)->next;
  }
}

/* Respond to a request */
void srv_respond(Req *r, char *error) {
  if (error) {
    r->ofcall.type = Rerror;
    r->ofcall.ename = error;
  } else {
    r->ofcall.type = r->ifcall.type + 1;
  }
  r->ofcall.tag = r->ifcall.tag;
  r->responding = 1;
}

/* Internal: Dispatch a single request */
void srv_dispatch(Req *r) {
  Srv *s = r->srv;
  uchar read_storage[8192];

  r->fid = get_fid(s, r->ifcall.fid);
  r->newfid = nil;
  int must_free_fid = 0;

  switch (r->ifcall.type) {
  case Tversion:
    if (r->ifcall.msize > 8192)
      r->ofcall.msize = 8192;
    else
      r->ofcall.msize = r->ifcall.msize;
    r->ofcall.version = "9P2000";
    srv_respond(r, nil);
    break;
  case Tauth:
    srv_respond(r, "authentication not required");
    break;
  case Tattach:
    if (r->fid) {
      srv_respond(r, "fid already in use");
      break;
    }
    r->fid = alloc_fid(s, r->ifcall.fid);
    if (!r->fid) {
      srv_respond(r, "no fids");
      break;
    }
    must_free_fid = 1;
    if (s->attach)
      s->attach(r);
    else
      srv_respond(r, "not implemented");
    if (r->responding && r->ofcall.type != Rerror)
      r->fid->qid = r->ofcall.qid;
    break;
  case Twalk: {
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (r->ifcall.newfid != r->ifcall.fid) {
      if (get_fid(s, r->ifcall.newfid)) {
        srv_respond(r, "fid already in use");
        break;
      }
      r->newfid = alloc_fid(s, r->ifcall.newfid);
      if (!r->newfid) {
        srv_respond(r, "no fids");
        break;
      }
      must_free_fid = 1;
    } else {
      r->newfid = r->fid;
    }
    if (s->walk)
      s->walk(r);
    else
      srv_respond(r, "not implemented");
    if (r->responding && r->ofcall.type != Rerror) {
      Fid *target = r->newfid ? r->newfid : r->fid;
      if (target) {
        if (r->ifcall.nwname == 0)
          target->qid = r->fid->qid;
        else if (r->ofcall.nwqid > 0)
          target->qid = r->ofcall.wqid[r->ofcall.nwqid - 1];
      }
    }
    break;
  }
  case Topen:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (s->open)
      s->open(r);
    else
      srv_respond(r, "not implemented");
    if (r->responding && r->ofcall.type != Rerror)
      r->fid->omode = r->ifcall.mode;
    break;
  case Tcreate:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (s->create)
      s->create(r);
    else
      srv_respond(r, "not implemented");
    if (r->responding && r->ofcall.type != Rerror) {
      r->fid->omode = r->ifcall.mode;
      r->fid->qid = r->ofcall.qid;
    }
    break;
  case Tread:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (r->ifcall.count > sizeof(read_storage)) {
      srv_respond(r, "read too large");
      break;
    }
    if (r->ifcall.count > 0) {
      r->ofcall.data = (char *)read_storage;
    }
    if (s->read)
      s->read(r);
    else
      srv_respond(r, "not implemented");
    break;
  case Twrite:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (s->write)
      s->write(r);
    else
      srv_respond(r, "not implemented");
    break;
  case Tclunk:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (s->clunk)
      s->clunk(r);
    else
      srv_respond(r, nil);
    if (r->responding && r->ofcall.type != Rerror)
      free_fid(s, r->fid);
    break;
  case Tremove:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (s->remove)
      s->remove(r);
    else
      srv_respond(r, "not implemented");
    if (r->responding && r->ofcall.type != Rerror)
      free_fid(s, r->fid);
    break;
  case Tstat:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (s->stat)
      s->stat(r);
    else
      srv_respond(r, "not implemented");
    break;
  case Twstat:
    if (!r->fid) {
      srv_respond(r, "unknown fid");
      break;
    }
    if (s->wstat)
      s->wstat(r);
    else
      srv_respond(r, "not implemented");
    break;
  case Tflush:
    if (s->flush)
      s->flush(r);
    else
      srv_respond(r, nil);
    break;
  default:
    srv_respond(r, "unknown message");
  }

  if (must_free_fid && r->responding && r->ofcall.type == Rerror) {
    if (r->newfid && r->newfid != r->fid)
      free_fid(s, r->newfid);
    else if (r->fid)
      free_fid(s, r->fid);
  }
}

/* Main Server Loop */
void srv_loop(Srv *s, int fd_in, int fd_out) {
  /* Allocate FID hash if needed */
  if (!s->fidhash) {
    s->fidhashsize = FID_HASH_SIZE;
    s->fidhash = srv_malloc(sizeof(Fid *) * s->fidhashsize);
    memset(s->fidhash, 0, sizeof(Fid *) * s->fidhashsize);
  }

  uchar buf[8192];
  uint n;

  while (1) {
    /* 1. Read message size (4 bytes) */
    n = sys_read(fd_in, buf, 4);
    if (n != 4)
      break;

    u32int size = GBIT32(buf);
    if (size > sizeof(buf))
      break; /* Too big */

    /* 2. Read rest of message */
    n = sys_read(fd_in, buf + 4, size - 4);
    if (n != size - 4)
      break;

    /* 3. Unmarshall */
    Req r;
    memset(&r, 0, sizeof(Req));
    r.srv = s;
    if (convM2S(buf, size, &r.ifcall) != size) {
      continue; /* Framing error */
    }

    /* 4. Dispatch */
    srv_dispatch(&r);

    /* 5. Write Response */
    if (r.responding) {
      n = convS2M(&r.ofcall, buf, sizeof(buf));
      sys_write(fd_out, buf, n);
      if (r.ofcall.type == Rstat && r.ofcall.stat)
        srv_free(r.ofcall.stat);
    }
  }
}

void srv_init(Srv *s) {
  s->fidhash = nil;
  s->fidhashsize = 0;
}
