#include "router.h"

/*
 * /srv namespace: In-memory service registry
 * Plan 9 uses this for processes to publish named endpoints
 */
#define SRV_MAX_ENTRIES 64
#define SRV_NAME_SIZE 64

typedef struct SrvEntry {
  char name[SRV_NAME_SIZE];
  Chan *chan;    /* Posted channel */
  int owner_pid; /* PID of process that posted this */
  int active;    /* Entry is in use */
} SrvEntry;

static SrvEntry srv_registry[SRV_MAX_ENTRIES];
static Lock srv_lock;
static int srv_initialized = 0;

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
  memset(srv_registry, 0, sizeof(srv_registry));
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
  }

  strcpy(e->name, (char *)name);
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
  memset(e, 0, sizeof(SrvEntry));
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
