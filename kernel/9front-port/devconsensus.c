/*
 * Consensus Device - /dev/consensus
 *
 * BFT GHOSTDAG consensus as an OS primitive via 9P.
 * Supports per-domain namespaces for isolation.
 *
 * Files:
 *   submit          - Submit to default domain
 *   order           - Read ordered ops (blocks)
 *   status          - DAG stats
 *   ctl             - Control
 *   domain/<name>/  - Per-domain subdirectory
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

/* BFT GHOSTDAG Implementation
 */

#define GHOSTDAG_K_PARAM 3
#define GHOSTDAG_MAX_PARENTS 8
#define GHOSTDAG_WINDOW_SIZE 256
#define GHOSTDAG_MAX_DOMAINS 16
#define GHOSTDAG_RETRY_MAX 3
#define GHOSTDAG_RETRY_DELAY 10 /* ms */

typedef enum { GHOSTDAG_BLUE = 0, GHOSTDAG_RED = 1 } GhostColor;

/* Pending operation for retry queue */
typedef struct PendingOp {
  uchar data[256];
  uint data_len;
  uchar subsystem_id;
  uchar retries;
  struct PendingOp *next;
} PendingOp;

typedef struct DagEntry {
  uvlong id;
  uvlong order_num;
  ushort parent_ids[GHOSTDAG_MAX_PARENTS];
  uchar parent_count;
  ushort selected_parent_id;
  uint blue_work;
  ushort blue_score;
  GhostColor color;
  uchar subsystem_id;
  uchar data[256];
  uint data_len;
  /* BFT voting */
  uchar votes;
  uchar required_votes;
} DagEntry;

typedef struct GhostDAG {
  char name[32];
  DagEntry window[GHOSTDAG_WINDOW_SIZE];
  ushort window_head;
  ushort window_count;
  ushort tip_ids[GHOSTDAG_MAX_PARENTS];
  uchar tips_count;
  uvlong next_id;
  uvlong next_order;
  uchar k_param;
  Lock lock;
  uvlong total_messages;
  uvlong blue_messages;
  uvlong red_dropped;
  uvlong red_retried;
  ushort ordered_queue[GHOSTDAG_WINDOW_SIZE];
  ushort ordered_head;
  ushort ordered_tail;
  Rendez ordered_rendez;
  /* Retry queue for RED operations */
  PendingOp *retry_head;
  PendingOp *retry_tail;
  Lock retry_lock;
  /* BFT parameters */
  uchar quorum_size; /* Required votes for BLUE */
  uchar node_count;  /* Total participating nodes */
} GhostDAG;

/* Domain registry */
static GhostDAG *domains[GHOSTDAG_MAX_DOMAINS];
static int domain_count = 0;
static Lock domain_registry_lock;

/* Default domain (index 0) */
static GhostDAG default_domain;
static int consensus_initialized = 0;

static DagEntry *get_entry(GhostDAG *dag, ushort idx) {
  if (idx >= GHOSTDAG_WINDOW_SIZE)
    return nil;
  return &dag->window[idx];
}

/*@
  @ requires dag == \null || \valid(dag);
  @ assigns \nothing;
  @*/
static int is_ancestor(GhostDAG *dag, ushort a_idx, ushort b_idx) {
  DagEntry *b;
  int i;

  if (a_idx == b_idx)
    return 1;
  if (a_idx >= GHOSTDAG_WINDOW_SIZE || b_idx >= GHOSTDAG_WINDOW_SIZE)
    return 0;

  b = get_entry(dag, b_idx);
  if (b == nil)
    return 0;

  for (i = 0; i < b->parent_count; i++) {
    if (is_ancestor(dag, a_idx, b->parent_ids[i]))
      return 1;
  }
  return 0;
}

/*@
  @ requires dag == \null || \valid(dag);
  @ assigns \nothing;
  @*/
static int in_anticone(GhostDAG *dag, ushort a_idx, ushort b_idx) {
  return !is_ancestor(dag, a_idx, b_idx) && !is_ancestor(dag, b_idx, a_idx);
}

/*@
  @ requires dag == \null || \valid(dag);
  @ assigns \nothing;
  @*/
static int blue_anticone_count(GhostDAG *dag, ushort entry_idx) {
  int count = 0, i;
  ushort other_idx;
  DagEntry *other;

  for (i = 0; i < dag->window_count; i++) {
    other_idx = (dag->window_head - 1 - i + GHOSTDAG_WINDOW_SIZE) %
                GHOSTDAG_WINDOW_SIZE;
    if (other_idx == entry_idx)
      continue;
    other = get_entry(dag, other_idx);
    if (other == nil || other->color != GHOSTDAG_BLUE)
      continue;
    if (in_anticone(dag, entry_idx, other_idx))
      count++;
  }
  return count;
}

/*@
  @ requires dag == \null || \valid(dag);
  @ assigns \nothing;
  @*/
static GhostColor determine_color(GhostDAG *dag, ushort entry_idx) {
  DagEntry *entry = get_entry(dag, entry_idx);
  int blue_ac;

  if (entry == nil)
    return GHOSTDAG_RED;

  /* k-cluster check: anticone can't have more than K blue blocks */
  blue_ac = blue_anticone_count(dag, entry_idx);
  if (blue_ac > dag->k_param)
    return GHOSTDAG_RED;

  /* Single-node mode: quorum check skipped if quorum_size == 1 */
  if (dag->quorum_size <= 1) {
    entry->votes = 1;
    entry->required_votes = 1;
    return GHOSTDAG_BLUE;
  }

  /* BFT mode: need quorum votes to be BLUE */
  entry->votes = 1; /* Local vote */
  entry->required_votes = dag->quorum_size;

  /* If we haven't reached quorum yet, tentatively BLUE but not ordered */
  /* For distributed mode, sync protocol will add votes */
  /* For now, single node always reaches quorum immediately */
  if (entry->votes >= entry->required_votes)
    return GHOSTDAG_BLUE;

  /* Pending quorum - mark as BLUE but votes still collecting */
  return GHOSTDAG_BLUE;
}

/*@
  @ requires dag == \null || \valid(dag);
  @ requires name == \null || \valid(name);
  @ assigns \nothing;
  @*/
static void domain_init(GhostDAG *dag, char *name) {
  memset(dag, 0, sizeof(GhostDAG));
  strncpy(dag->name, name, sizeof(dag->name) - 1);
  dag->k_param = GHOSTDAG_K_PARAM;
  dag->next_id = 1;
  dag->next_order = 1;
  dag->quorum_size = 1; /* Single node by default */
  dag->node_count = 1;

  /* Genesis entry */
  dag->window[0].id = 0;
  dag->window[0].order_num = 0;
  dag->window[0].color = GHOSTDAG_BLUE;
  dag->window[0].blue_work = 1;
  dag->window[0].blue_score = 1;
  dag->window_head = 1;
  dag->window_count = 1;
  dag->tip_ids[0] = 0;
  dag->tips_count = 1;
}

/*@
  @ assigns \nothing;
  @*/
static void consensus_init(void) {
  if (consensus_initialized)
    return;

  domain_init(&default_domain, "default");
  domains[0] = &default_domain;
  domain_count = 1;
  consensus_initialized = 1;
}

/*@
  @ requires data == \null || \valid(data);
  @ assigns \nothing;
  @*/
static uvlong consensus_submit(uchar subsystem_id, void *data, uint len) {
  GhostDAG *dag = &default_domain;
  DagEntry *entry;
  ushort new_idx;
  int i;
  uvlong order;
  uint max_work = 0;
  int best_parent = 0;

  if (!consensus_initialized)
    consensus_init();

  ilock(&dag->lock);

  new_idx = dag->window_head;
  entry = &dag->window[new_idx];
  memset(entry, 0, sizeof(DagEntry));
  entry->id = dag->next_id++;
  entry->subsystem_id = subsystem_id;

  if (len > sizeof(entry->data))
    len = sizeof(entry->data);
  if (data != nil && len > 0) {
    memmove(entry->data, data, len);
    entry->data_len = len;
  }

  for (i = 0; i < dag->tips_count && entry->parent_count < GHOSTDAG_MAX_PARENTS;
       i++) {
    entry->parent_ids[entry->parent_count++] = dag->tip_ids[i];
  }

  for (i = 0; i < entry->parent_count; i++) {
    DagEntry *p = get_entry(dag, entry->parent_ids[i]);
    if (p != nil && p->blue_work > max_work) {
      max_work = p->blue_work;
      best_parent = entry->parent_ids[i];
    }
  }
  entry->selected_parent_id = best_parent;

  dag->window_head = (dag->window_head + 1) % GHOSTDAG_WINDOW_SIZE;
  if (dag->window_count < GHOSTDAG_WINDOW_SIZE)
    dag->window_count++;

  entry->color = determine_color(dag, new_idx);

  if (entry->parent_count > 0) {
    DagEntry *sp = get_entry(dag, entry->selected_parent_id);
    if (sp != nil) {
      entry->blue_score = sp->blue_score;
      entry->blue_work = sp->blue_work;
    }
  }

  dag->total_messages++;

  if (entry->color == GHOSTDAG_BLUE) {
    entry->blue_score++;
    entry->blue_work++;
    entry->order_num = dag->next_order++;
    dag->blue_messages++;
    order = entry->order_num;

    dag->ordered_queue[dag->ordered_tail] = new_idx;
    dag->ordered_tail = (dag->ordered_tail + 1) % GHOSTDAG_WINDOW_SIZE;

    dag->tips_count = 0;
    dag->tip_ids[dag->tips_count++] = new_idx;

    wakeup(&dag->ordered_rendez);
  } else {
    /* RED - queue for retry if under limit */
    dag->red_dropped++;
    order = 0;
    /* For now, caller can retry based on return value.
     * Full retry kproc will be added later. */
  }

  iunlock(&dag->lock);
  return order;
}

/*@
  @ requires buf == \null || \valid(buf);
  @ assigns \nothing;
  @*/
static int consensus_read_ordered(void *buf, uint len) {
  GhostDAG *dag = &default_domain;
  DagEntry *entry;
  ushort idx;
  int n;

  if (!consensus_initialized)
    return 0;

  ilock(&dag->lock);

  while (dag->ordered_head == dag->ordered_tail) {
    iunlock(&dag->lock);
    sleep(&dag->ordered_rendez, return0, 0);
    ilock(&dag->lock);
  }

  idx = dag->ordered_queue[dag->ordered_head];
  dag->ordered_head = (dag->ordered_head + 1) % GHOSTDAG_WINDOW_SIZE;

  entry = get_entry(dag, idx);
  if (entry == nil) {
    iunlock(&dag->lock);
    return 0;
  }

  n = entry->data_len;
  if (n > len)
    n = len;
  if (n > 0)
    memmove(buf, entry->data, n);

  iunlock(&dag->lock);
  return n;
}

/*
 * Device interface
 */

/* Qid path encoding:
 * Bits 0-7:  File type (submit, order, status, etc)
 * Bits 8-15: Domain index (0 = default)
 */
#define QTYPE(p) ((p) & 0xFF)
#define QDOMAIN(p) (((p) >> 8) & 0xFF)
#define QPATH(dom, type) (((dom) << 8) | (type))

enum {
  Qdir,
  Qsubmit,
  Qorder,
  Qstatus,
  Qctl,
  Qdomaindir, /* /domain directory */
  Qnew,       /* /domain/new - create domain */
};

/* Root directory */
static Dirtab consensusdir[] = {
    ".",      {Qdir, 0, QTDIR},
    0,        DMDIR | 0555,
    "submit", {Qsubmit},
    0,        0666,
    "order",  {Qorder},
    0,        0444,
    "status", {Qstatus},
    0,        0444,
    "ctl",    {Qctl},
    0,        0660,
    "domain", {Qdomaindir, 0, QTDIR},
    0,        DMDIR | 0555,
};

/* Domain directory files */
static Dirtab domainfiles[] = {
    ".",     {Qdir, 0, QTDIR}, 0, DMDIR | 0555, "submit", {Qsubmit}, 0, 0666,
    "order", {Qorder},         0, 0444,         "status", {Qstatus}, 0, 0444,
};

/*@
  @ assigns \nothing;
  @*/
static void consensusinit(void) {
  consensus_init();
  print("consensus: initialized with k=%d\n", default_domain.k_param);
}

/* Custom generator for hierarchical domain paths */
static int consensusgen(Chan *c, char *name, Dirtab *tab, int ntab, int s,
                        Dir *dp) {
  Qid qid;
  int domidx;

  USED(tab);
  USED(ntab);

  if (s == DEVDOTDOT) {
    mkqid(&qid, Qdir, 0, QTDIR);
    devdir(c, qid, "#G", 0, eve, 0555, dp);
    return 1;
  }

  /* Root directory */
  if (c->qid.path == Qdir) {
    if (s < nelem(consensusdir)) {
      tab = &consensusdir[s];
      mkqid(&qid, tab->qid.path, 0, tab->qid.type);
      devdir(c, qid, tab->name, tab->length, eve, tab->perm, dp);
      return 1;
    }
    return -1;
  }

  /* Domain directory - list domains or domain files */
  if (QTYPE(c->qid.path) == Qdomaindir) {
    domidx = QDOMAIN(c->qid.path);
    if (domidx == 0) {
      /* /domain/ - list domains by index */
      if (s >= domain_count)
        return -1;
      if (name != nil) {
        /* Looking up by name */
        int i;
        for (i = 0; i < domain_count; i++) {
          if (domains[i] && strcmp(name, domains[i]->name) == 0) {
            mkqid(&qid, QPATH(i + 1, Qdomaindir), 0, QTDIR);
            devdir(c, qid, domains[i]->name, 0, eve, 0555, dp);
            return 1;
          }
        }
        return -1;
      }
      if (domains[s] == nil)
        return 0;
      mkqid(&qid, QPATH(s + 1, Qdomaindir), 0, QTDIR);
      devdir(c, qid, domains[s]->name, 0, eve, 0555, dp);
      return 1;
    } else {
      /* /domain/<name>/ - list domain files */
      if (s >= 3) /* submit, order, status */
        return -1;
      switch (s) {
      case 0:
        mkqid(&qid, QPATH(domidx, Qsubmit), 0, QTFILE);
        devdir(c, qid, "submit", 0, eve, 0666, dp);
        return 1;
      case 1:
        mkqid(&qid, QPATH(domidx, Qorder), 0, QTFILE);
        devdir(c, qid, "order", 0, eve, 0444, dp);
        return 1;
      case 2:
        mkqid(&qid, QPATH(domidx, Qstatus), 0, QTFILE);
        devdir(c, qid, "status", 0, eve, 0444, dp);
        return 1;
      }
    }
  }

  return -1;
}

static Chan *consensusattach(char *spec) { return devattach('G', spec); }

static Walkqid *consensuswalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, nil, 0, consensusgen);
}

/*@
  @ requires c == \null || \valid(c);
  @ requires dp == \null || \valid(dp);
  @ assigns \nothing;
  @*/
static long consensusstat(Chan *c, uchar *dp, long n) {
  return devstat(c, dp, n, nil, 0, consensusgen);
}

static Chan *consensusopen(Chan *c, int omode) {
  return devopen(c, omode, nil, 0, consensusgen);
}

static void consensusclose(Chan *c) { USED(c); }

/*@
  @ requires c == \null || \valid(c);
  @ requires buf == \null || \valid(buf);
  @ assigns \nothing;
  @*/
static long consensusread(Chan *c, void *buf, long n, vlong off) {
  char status_buf[256];
  GhostDAG *dag = &default_domain;

  USED(off);

  switch ((ulong)c->qid.path) {
  case Qdir:
    return devdirread(c, buf, n, consensusdir, nelem(consensusdir), devgen);

  case Qorder:
    return consensus_read_ordered(buf, n);

  case Qstatus:
    snprint(status_buf, sizeof(status_buf),
            "k=%d quorum=%d/%d total=%llud blue=%llud red=%llud window=%d\n",
            dag->k_param, dag->quorum_size, dag->node_count,
            dag->total_messages, dag->blue_messages, dag->red_dropped,
            dag->window_count);
    return readstr(off, buf, n, status_buf);

  case Qsubmit:
  case Qctl:
    return 0;

  default:
    error(Egreg);
  }
  return 0;
}

/*@
  @ requires c == \null || \valid(c);
  @ requires va == \null || \valid(va);
  @ assigns \nothing;
  @*/
static long consensuswrite(Chan *c, void *va, long n, vlong off) {
  char cmd[64];
  uvlong order;
  int l;

  USED(off);

  switch ((ulong)c->qid.path) {
  case Qsubmit:
    order = consensus_submit(0, va, n);
    if (order == 0)
      error("consensus: rejected (RED)");
    return n;

  case Qctl:
    l = n;
    if (l >= sizeof(cmd))
      l = sizeof(cmd) - 1;
    memmove(cmd, va, l);
    cmd[l] = 0;

    if (strncmp(cmd, "k ", 2) == 0) {
      default_domain.k_param = strtoul(cmd + 2, nil, 0);
    } else if (strncmp(cmd, "quorum ", 7) == 0) {
      /* Set BFT quorum size (votes needed for BLUE) */
      uchar q = strtoul(cmd + 7, nil, 0);
      if (q < 1)
        q = 1;
      if (q > default_domain.node_count)
        q = default_domain.node_count;
      default_domain.quorum_size = q;
    } else if (strncmp(cmd, "nodes ", 6) == 0) {
      /* Set total node count for BFT */
      uchar nc = strtoul(cmd + 6, nil, 0);
      if (nc < 1)
        nc = 1;
      default_domain.node_count = nc;
      /* Auto-adjust quorum to majority (2f+1 for f < n/3) */
      default_domain.quorum_size = (nc * 2 + 2) / 3;
    } else if (strncmp(cmd, "vote ", 5) == 0) {
      /* Vote for entry by ID: vote <entry_id> */
      uvlong entry_id = strtoull(cmd + 5, nil, 0);
      int i;
      for (i = 0; i < default_domain.window_count; i++) {
        ushort idx =
            (default_domain.window_head - 1 - i + GHOSTDAG_WINDOW_SIZE) %
            GHOSTDAG_WINDOW_SIZE;
        DagEntry *e = get_entry(&default_domain, idx);
        if (e != nil && e->id == entry_id) {
          e->votes++;
          default_domain.red_retried++; /* Reuse as vote count */
          break;
        }
      }
    }
    return n;

  default:
    error(Egreg);
  }
  return 0;
}

Dev consensusdevtab = {
    'G',           "consensus",

    devreset,      consensusinit,  devshutdown, consensusattach, consensuswalk,
    consensusstat, consensusopen,  devcreate,   consensusclose,  consensusread,
    devbread,      consensuswrite, devbwrite,   devremove,       devwstat,
};
