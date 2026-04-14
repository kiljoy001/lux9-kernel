/*
 * wasm_dev_srv.c - /srv/wasm service registry device
 *
 * Exposes active WASM sessions as a directory.
 * - Read directory: Lists registered named sessions.
 * - Attach to file: Mounts the WASM 9P file server.
 */

#include "../include/9p_router.h"
#include "../include/dat.h"
#include "../include/error.h"
#include "../include/fcall.h"
#include "../include/fns.h"
#include "../include/rbtree.h"
#include "../include/u.h"
#include "wasm_9p_integration.h"

enum {
  Qdir = 0,
};

typedef struct WasmSrvChanState {
  wasm_9p_session_t *session;
  uchar *resp_buf;
  uint resp_len;
} WasmSrvChanState;

/*@
  @ predicate valid_session_node(struct rb_node *node) =
  @   \valid(node) &&
  @   \valid((wasm_9p_session_t *)node);
  @*/

static WasmSrvChanState *wasm_srv_state(Chan *c) {
  return (WasmSrvChanState *)c->aux;
}

static long wasm_9p_chan_read(Chan *c, void *va, long n, vlong offset);

static void wasmsrvinit(void) {
  /* Integration init is handled elsewhere or lazy */
}

static Chan *wasmsrvattach(char *spec) { return devattach('W', spec); }

/*
 * Iterator for RB-tree to find nth named session
 * Returns: session pointer or nil
 */
static wasm_9p_session_t *wasm_get_nth_named_session(int n) {
  /* This is O(N) linear scan of the tree.
     For a high-performance registry, we would maintain a secondary list or
     index. For <1000 sessions, this is acceptable. */

  extern struct rb_root session_tree; /* Access global tree */
  extern Lock session_lock;


  lock(&session_lock);
  struct rb_node *node = rb_first(&session_tree);
  int count = 0;
  /*@
    @ loop invariant node == \null || valid_session_node(node);
    @ loop assigns count, node;
    @*/
  while (node) {
    wasm_9p_session_t *s = rb_entry(node, wasm_9p_session_t, rb);
    if (s->name != nil) {
      if (count == n) {
        unlock(&session_lock);
        return s;
      }
      count++;
    }
    node = rb_next(node);
  }
  unlock(&session_lock);
  return nil;
}

static wasm_9p_session_t *wasm_find_session_by_name(char *name) {
  extern struct rb_root session_tree;
  extern Lock session_lock;

  lock(&session_lock);
  struct rb_node *node = rb_first(&session_tree);
  /*@
    @ loop invariant node == \null || valid_session_node(node);
    @ loop assigns node;
    @*/
  while (node) {
    wasm_9p_session_t *s = rb_entry(node, wasm_9p_session_t, rb);
    if (s->name != nil && strcmp(s->name, name) == 0) {
      unlock(&session_lock);
      return s;
    }
    node = rb_next(node);
  }
  unlock(&session_lock);
  return nil;
}

static int wasmgen(Chan *c, char *name, Dirtab *, int, int s, Dir *dp) {
  Qid qid;

  if (s == DEVDOTDOT) {
    mkqid(&qid, Qdir, 0, QTDIR);
    devdir(c, qid, "#W", 0, eve, 0555, dp);
    return 1;
  }

  if (c->qid.path != Qdir) {
    /* If path is not root, we are inside a session file (or directory if we
     * supported hierarchy) */
    /* But we only expose files for sessions to attach to */
    return -1;
  }

  if (name != nil) {
    /* Lookup by name */
    wasm_9p_session_t *session = wasm_find_session_by_name(name);
    if (!session)
      return -1;

    /* Qid path = session_id (which is > 0) */
    mkqid(&qid, session->session_id, 0, QTFILE);
    devdir(c, qid, session->name, 0, eve, 0666, dp);
    return 1;
  }

  /* List by index */
  wasm_9p_session_t *session = wasm_get_nth_named_session(s);
  if (!session)
    return -1;

  mkqid(&qid, session->session_id, 0, QTFILE);
  devdir(c, qid, session->name, 0, eve, 0666, dp);
  return 1;
}

static Walkqid *wasmsrvwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, nil, 0, wasmgen);
}

static int wasmsrvstat(Chan *c, uchar *db, int n) {
  return devstat(c, db, n, nil, 0, wasmgen);
}

static Chan *wasmsrvopen(Chan *c, int omode) {
  /* If opening root, standard directory open */
  if (c->qid.path == Qdir) {
    if ((omode & 7) != OREAD)
      error(Eperm);
  } else {
    u32int session_id = (u32int)c->qid.path;
    wasm_9p_session_t *session = wasm_9p_get_session(session_id);
    if (!session)
      error(Eio);

    WasmSrvChanState *state = xallocz(sizeof(*state), 1);
    if (!state)
      error(Eio);

    state->resp_buf = xallocz(P9_MSG_SIZE, 0);
    if (!state->resp_buf) {
      xfree(state);
      error(Eio);
    }

    state->session = session;
    wasm_9p_session_ref(session_id);
    c->aux = state;
  }

  c->mode = openmode(omode);
  c->flag |= COPEN;
  c->offset = 0;
  return c;
}

static void wasmsrvclose(Chan *c) {
  WasmSrvChanState *state = wasm_srv_state(c);
  if (!state)
    return;

  if (state->session)
    wasm_9p_session_unref(state->session->session_id);
  if (state->resp_buf)
    xfree(state->resp_buf);
  xfree(state);
  c->aux = nil;
}

static long wasmsrvread(Chan *c, void *va, long n, vlong offset) {
  if (c->qid.path == Qdir)
    return devdirread(c, va, n, nil, 0, wasmgen);

  return wasm_9p_chan_read(c, va, n, offset);
}

static long wasmsrvwrite(Chan *c, void *va, long n, vlong offset) {
  if (c->qid.path == Qdir)
    error(Eperm);

  WasmSrvChanState *state = wasm_srv_state(c);
  if (!state || !state->session || !state->resp_buf)
    error(Eio);

  if (offset != 0)
    error(Eio);
  if (n <= 0)
    return 0;
  if (n > P9_MSG_SIZE)
    error(Eio);

  uchar buf[P9_MSG_SIZE];
  Fcall t;
  Fcall r;
  memmove(buf, va, n);
  t = (Fcall){0};
  r = (Fcall){0};

  if (convM2S(buf, n, &t) == 0)
    error(Eio);

  if (wasm_9p_route_to_wasm(&t, &r, state->session) < 0 && r.type != Rerror) {
    r.type = Rerror;
    r.ename = "wasm 9p route failed";
  }

  uint rep_size = convS2M(&r, state->resp_buf, P9_MSG_SIZE);
  if (rep_size == 0)
    error(Eio);

  state->resp_len = rep_size;
  return n;
}

/*
 * Attach to a WASM session (via mount)
 * When a user mounts /srv/wasm/my_server /n/local
 * The kernel calls attach on the device with the spec.
 * But here we are PROVIDING the /srv/wasm.
 * The standard 'mount' involves opening the file descriptor to the server and
 * pushing it. But since WASM is in-kernel, we can support direct attach via
 * something else?
 *
 * Plan 9 approach:
 * fd = open("/srv/wasm/mysrv", ORDWR);
 * mount(fd, "/n/local", MREPL, ...);
 *
 * When 'mount' writes to the file descriptor (if it's a pipe) or calls Tattach.
 * But devwasm exposes files. If we open one, we get a Chan.
 * If we treat this Chan as a 9P connection, we need to handle 9P messages on
 * read/write.
 *
 * IMPLEMENTATION CHOICE:
 * When we open "/srv/wasm/mysrv", we return a Chan that is "connected" to the
 * WASM server. Reads/Writes on this Chan should be 9P messages handled by the
 * WASM server.
 */
static long wasm_9p_chan_read(Chan *c, void *va, long n, vlong offset) {
  if (c->qid.path == Qdir)
    return devdirread(c, va, n, nil, 0, wasmgen);

  WasmSrvChanState *state = wasm_srv_state(c);
  if (!state || !state->resp_buf)
    error(Eio);

  if (n <= 0 || offset < 0)
    return 0;
  if ((ulong)offset >= state->resp_len)
    return 0;

  ulong avail = state->resp_len - (ulong)offset;
  if ((ulong)n > avail)
    n = avail;

  memmove(va, state->resp_buf + offset, n);
  if ((ulong)offset + n >= state->resp_len)
    state->resp_len = 0;

  return n;
}
/* Re-using srvread/write for now which are empty for files */

Dev wasmsrvdevtab = {
    'W',         "wasm",

        devreset,      wasmsrvinit,    devshutdown, wasmsrvattach, wasmsrvwalk,

        wasmsrvstat,   wasmsrvopen,    devcreate,   wasmsrvclose,  wasmsrvread,

        wasmsrvwrite,  devbread,       devbwrite,   devremove,     devwstat,
};
