#include "router.h"

/*
 * /srv namespace: In-memory service registry
 * Plan 9 uses this for processes to publish named endpoints
 */
#define SRV_MAX_ENTRIES 64
#define SRV_NAME_SIZE 64

typedef struct SrvEntry {
  char name[SRV_NAME_SIZE];
  char owner[KNAMELEN];
  Chan *chan;    /* Posted channel */
  int owner_pid; /* PID of process that posted this */
  int active;    /* Entry is in use */
} SrvEntry;

static SrvEntry srv_registry[SRV_MAX_ENTRIES];
static Lock srv_lock;
static int srv_initialized = 0;

typedef struct NsRootEntry {
  const char *path;
  Chan *mchan;
  char spec[64];
} NsRootEntry;

static NsRootEntry ns_roots[] = {
    {"/srv", nil, {0}},
    {"/mnt", nil, {0}},
    {"/proc", nil, {0}},
    {"/env", nil, {0}},
};
static Lock ns_root_lock;

static int ns_root_exact_index(const char *path) {
  int i;

  if (path == nil)
    return -1;
  for (i = 0; i < (int)nelem(ns_roots); i++) {
    if (strcmp(path, ns_roots[i].path) == 0)
      return i;
  }
  return -1;
}

static int ns_root_match_index(const char *path, char **rel) {
  int i;

  if (path == nil)
    return -1;
  for (i = 0; i < (int)nelem(ns_roots); i++) {
    int plen = (int)strlen(ns_roots[i].path);
    if (strncmp(path, ns_roots[i].path, plen) != 0)
      continue;
    if (path[plen] != 0 && path[plen] != '/')
      continue;
    if (rel != nil) {
      *rel = (char *)path + plen;
      if (**rel == '/')
        (*rel)++;
    }
    return i;
  }
  return -1;
}

int p9_ns_managed_path(const char *path) {
  return ns_root_match_index(path, nil) >= 0;
}

int p9_ns_root_exact(const char *path) {
  return ns_root_exact_index(path) >= 0;
}

static int ns_srv_path(const char *path) {
  if (path == nil)
    return 0;
  if (strncmp(path, "/srv", 4) != 0)
    return 0;
  return path[4] == 0 || path[4] == '/';
}

static int ns_proc_text_matches(const char *name) {
  char *base;

  if (up == nil || up->text == nil || name == nil)
    return 0;
  base = strrchr(up->text, '/');
  if (base != nil)
    return strcmp(base + 1, name) == 0;
  return strcmp(up->text, name) == 0;
}

void p9_ns_enforce_owner(const char *target, const char *other) {
  int touches_srv = ns_srv_path(target) || ns_srv_path(other);

  if (ns_proc_text_matches("resurrection")) {
    if (!touches_srv)
      error("namespace handled by nsd");
    return;
  }

  if (ns_proc_text_matches("nsd")) {
    if (touches_srv)
      error("/srv handled by resurrection");
    return;
  }

  error("namespace handled in userspace");
}

int p9_ns_root_available(const char *path) {
  int idx;
  int ok = 0;

  lock(&ns_root_lock);
  idx = ns_root_match_index(path, nil);
  if (idx >= 0 && ns_roots[idx].mchan != nil)
    ok = 1;
  unlock(&ns_root_lock);
  return ok;
}

void p9_ns_publish_root(const char *path, Chan *mchan, const char *spec) {
  int idx;
  Chan *old;

  if (mchan == nil)
    return;

  idx = ns_root_exact_index(path);
  if (idx < 0)
    return;

  incref((Ref *)&mchan->ref);
  lock(&ns_root_lock);
  old = ns_roots[idx].mchan;
  ns_roots[idx].mchan = mchan;
  snprint(ns_roots[idx].spec, sizeof(ns_roots[idx].spec), "%s",
          spec != nil ? spec : "");
  unlock(&ns_root_lock);

  if (old != nil)
    cclose(old);
}

void p9_ns_unpublish_root(const char *path) {
  int idx;
  Chan *old;

  idx = ns_root_exact_index(path);
  if (idx < 0)
    return;

  lock(&ns_root_lock);
  old = ns_roots[idx].mchan;
  ns_roots[idx].mchan = nil;
  ns_roots[idx].spec[0] = 0;
  unlock(&ns_root_lock);

  if (old != nil)
    cclose(old);
}

Chan *p9_ns_attach_root(const char *path, char **rel) {
  int idx;
  Chan *mchan;
  Chan *root;
  char spec[sizeof(ns_roots[0].spec)];

  lock(&ns_root_lock);
  idx = ns_root_match_index(path, rel);
  if (idx < 0 || ns_roots[idx].mchan == nil) {
    unlock(&ns_root_lock);
    return nil;
  }

  mchan = ns_roots[idx].mchan;
  incref((Ref *)&mchan->ref);
  snprint(spec, sizeof(spec), "%s", ns_roots[idx].spec);
  unlock(&ns_root_lock);

  if (waserror()) {
    cclose(mchan);
    return nil;
  }
  root = mntattach(mchan, nil, spec, 0);
  cclose(mchan);
  poperror();

  if (root != nil) {
    root = cunique(root);
    if (root->path != nil)
      pathclose(root->path);
    root->path = newpath((BString){(char *)ns_roots[idx].path,
                                   (int)strlen(ns_roots[idx].path)});
  }
  return root;
}

static void srv_entry_init(SrvEntry *e) {
  if (e == nil)
    return;
  e->name[0] = 0;
  e->owner[0] = 0;
  e->chan = nil;
  e->owner_pid = 0;
  e->active = 0;
}

static void srv_entry_set_owner(SrvEntry *e, Proc *caller) {
  char *owner = eve;

  if (e == nil)
    return;
  if (caller != nil && caller->user != nil && caller->user[0] != 0)
    owner = caller->user;
  snprint(e->owner, sizeof(e->owner), "%s", owner != nil ? owner : "");
}

/*@
  @ terminates \true;
  @ assigns \nothing;
  @*/
static int srv_visible_to(Proc *caller, SrvEntry *e) {
  if (e == nil || !e->active)
    return 0;
  if (caller != nil && caller->wasm.initialized) {
    if (e->owner_pid == 0)
      return 1;
    return (ulong)e->owner_pid == caller->pid;
  }
  return 1;
}

/*@
  @ assigns srv_registry[0..SRV_MAX_ENTRIES-1], srv_initialized;
  @ terminates \true;
  @*/
void srv_init(void) {
  if (srv_initialized)
    return;
  for (int i = 0; i < SRV_MAX_ENTRIES; i++)
    srv_entry_init(&srv_registry[i]);
  srv_initialized = 1;
}

/* Find entry by name */
/*@
  @ requires name != \null && \valid_read(name);
  @ terminates \true;
  @ assigns \nothing;
  @*/
static SrvEntry *srv_find(char *name) {
  int i;
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (srv_registry[i].active && strcmp(srv_registry[i].name, name) == 0)
      return &srv_registry[i];
  }
  return nil;
}

/* Find free slot */
/*@
  @ terminates \true;
  @ assigns \nothing;
  @*/
static SrvEntry *srv_alloc(void) {
  int i;
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_registry[i].active)
      return &srv_registry[i];
  }
  return nil;
}

/*@
  @ requires name != \null && \valid_read(name);
  @ terminates \true;
  @*/
int srv_create_entry(Proc *caller, const char *name) {
  SrvEntry *e;

  if (!name || name[0] == 0 || strlen((char *)name) >= SRV_NAME_SIZE)
    return -1;

  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e == nil) {
    e = srv_alloc();
    if (e == nil) {
      unlock(&srv_lock);
      return -1;
    }
    srv_entry_init(e);
  }

  strcpy(e->name, (char *)name);
  srv_entry_set_owner(e, caller);
  e->owner_pid = caller ? (int)caller->pid : 0;
  e->active = 1;
  if (e->chan != nil) {
    cclose(e->chan);
    e->chan = nil;
  }
  unlock(&srv_lock);
  return 0;
}

/*@
  @ requires name != \null && \valid_read(name);
  @ terminates \true;
  @*/
int srv_post_fd(Proc *caller, const char *name, int fd) {
  SrvEntry *e;
  Chan *c;

  if (!name || name[0] == 0 || strlen((char *)name) >= SRV_NAME_SIZE)
    return -1;

  if (waserror())
    return -1;
  c = fdtochan(fd, -1, 0, 1);
  if (c == nil) {
    poperror();
    return -1;
  }

  if (waserror()) {
    cclose(c);
    nexterror();
  }

  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e == nil) {
    e = srv_alloc();
    if (e == nil) {
      unlock(&srv_lock);
      poperror();
      cclose(c);
      return -1;
    }
  }

  strcpy(e->name, (char *)name);
  srv_entry_set_owner(e, caller);
  if (e->chan != nil)
    cclose(e->chan);
  e->chan = c;
  e->owner_pid = caller ? (int)caller->pid : 0;
  e->active = 1;
  unlock(&srv_lock);

  poperror();
  poperror();
  return 0;
}

int srv_post_chan(Proc *caller, const char *name, Chan *c) {
  SrvEntry *e;

  if (!name || name[0] == 0 || c == nil || strlen((char *)name) >= SRV_NAME_SIZE)
    return -1;

  if (waserror())
    return -1;

  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e == nil) {
    e = srv_alloc();
    if (e == nil) {
      unlock(&srv_lock);
      poperror();
      cclose(c);
      return -1;
    }
  }

  strcpy(e->name, (char *)name);
  srv_entry_set_owner(e, caller);
  if (e->chan != nil)
    cclose(e->chan);
  e->chan = c;
  e->owner_pid = caller ? (int)caller->pid : 0;
  e->active = 1;
  unlock(&srv_lock);

  poperror();
  return 0;
}

/*@
  @ requires name != \null && \valid_read(name);
  @ terminates \true;
  @*/
Chan *srv_clone_chan(const char *name) {
  Chan *c = nil;
  SrvEntry *e;

  if (!name || name[0] == 0)
    return nil;

  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e != nil && e->chan != nil)
    c = cclone(e->chan);
  unlock(&srv_lock);
  return c;
}

/*@
  @ requires name != \null && \valid_read(name);
  @ terminates \true;
  @*/
int srv_remove_entry(Proc *caller, const char *name) {
  SrvEntry *e;

  if (!name || name[0] == 0)
    return -1;

  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e == nil) {
    unlock(&srv_lock);
    return -1;
  }
  if (caller != nil && (ulong)e->owner_pid != caller->pid && !iseve()) {
    unlock(&srv_lock);
    return -1;
  }
  if (e->chan != nil) {
    cclose(e->chan);
    e->chan = nil;
  }
  srv_entry_init(e);
  unlock(&srv_lock);
  return 0;
}

/*@
  @ requires \valid(name + (0..namelen-1));
  @ terminates \true;
  @*/
int srv_get_by_index(int index, char *name, int namelen) {
  int i;
  int seen = 0;

  if (index < 0 || namelen <= 0)
    return -1;

  srv_init();
  lock(&srv_lock);
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_registry[i].active)
      continue;
    if (seen == index) {
      snprint(name, namelen, "%s", srv_registry[i].name);
      unlock(&srv_lock);
      return 0;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}

int srv_get_by_index_for_proc(Proc *caller, int index, char *name,
                              int namelen) {
  int i;
  int seen = 0;

  if (index < 0 || namelen <= 0)
    return -1;

  srv_init();
  lock(&srv_lock);
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_visible_to(caller, &srv_registry[i]))
      continue;
    if (seen == index) {
      snprint(name, namelen, "%s", srv_registry[i].name);
      unlock(&srv_lock);
      return 0;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}

int srv_get_info_for_proc(Proc *caller, int index, char *name, int namelen,
                          char *owner, int ownerlen) {
  int i;
  int seen = 0;

  if (index < 0)
    return -1;

  srv_init();
  lock(&srv_lock);
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_visible_to(caller, &srv_registry[i]))
      continue;
    if (seen == index) {
      if (name != nil && namelen > 0)
        snprint(name, namelen, "%s", srv_registry[i].name);
      if (owner != nil && ownerlen > 0)
        snprint(owner, ownerlen, "%s",
                srv_registry[i].owner[0] != 0 ? srv_registry[i].owner : eve);
      unlock(&srv_lock);
      return 0;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}

int srv_index_of(const char *name) {
  int i;
  int seen = 0;

  if (!name || name[0] == 0)
    return -1;

  srv_init();
  lock(&srv_lock);
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_registry[i].active)
      continue;
    if (strcmp(srv_registry[i].name, (char *)name) == 0) {
      unlock(&srv_lock);
      return seen;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}

int srv_index_of_for_proc(Proc *caller, const char *name) {
  int i;
  int seen = 0;

  if (!name || name[0] == 0)
    return -1;

  srv_init();
  lock(&srv_lock);
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_visible_to(caller, &srv_registry[i]))
      continue;
    if (strcmp(srv_registry[i].name, (char *)name) == 0) {
      unlock(&srv_lock);
      return seen;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}

int srv_lookup_info_for_proc(Proc *caller, const char *name, char *owner,
                             int ownerlen) {
  int i;

  if (name == nil || name[0] == 0)
    return -1;

  srv_init();
  lock(&srv_lock);
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_visible_to(caller, &srv_registry[i]))
      continue;
    if (strcmp(srv_registry[i].name, (char *)name) == 0) {
      if (owner != nil && ownerlen > 0)
        snprint(owner, ownerlen, "%s",
                srv_registry[i].owner[0] != 0 ? srv_registry[i].owner : eve);
      unlock(&srv_lock);
      return 0;
    }
  }
  unlock(&srv_lock);
  return -1;
}

void srv_rename_user(const char *old, const char *new) {
  int i;

  if (old == nil || new == nil || old[0] == 0 || new[0] == 0)
    return;

  srv_init();
  lock(&srv_lock);
  for (i = 0; i < SRV_MAX_ENTRIES; i++) {
    if (!srv_registry[i].active)
      continue;
    if (strcmp(srv_registry[i].owner, (char *)old) != 0)
      continue;
    snprint(srv_registry[i].owner, sizeof(srv_registry[i].owner), "%s", new);
  }
  unlock(&srv_lock);
}
