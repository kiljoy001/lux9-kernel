/*
 * Userspace Process Manager (PM) - Thin-Lux9 Ultra
 *
 * This service handles high-level process management (PIDs, wait queues,
 * fork/exec logic) in userspace, reducing the kernel to a minimal thread/vault
 * provider.
 */

#include <libc.h>
#include <u.h>
#define _LIB_H_
#include "../family.h"
#include <fcall.h>

typedef struct Process {
  int pid;
  int ppid;
  char name[32];
  u64int vault_id;
  int state;
} Process;

static Process procs[1024];
static int next_pid = 1;

static int pm_init(FamilyExchangePage *f) {
  print("PM: Initializing Process Manager Family\n");
  memset(procs, 0, sizeof(procs));
  return 0;
}

static int pm_allocate_channel(FamilyExchangePage *f, void *device_id,
                               u32int perms, u64int *chan_id) {
  /* In PM, generic channel allocation might represent a handle to a process */
  return 0;
}

/* 9P Handling for /proc interface */
/* This will eventually handle Tattach/Twalk etc for the diverted /proc */
int pm_handle_9p(Fcall *t, Fcall *r) {
  switch (t->type) {
  case Tattach:
    /* Attach to the PM root */
    break;

  /* Fork logic moved here! */
  case Tsyscall:
    if (t->scallnr == 2 /* SYS_RFORK */) {
      int pid = next_pid++;
      print("PM: Forking process, assigned PID %d\n", pid);
      r->retval = pid;
      return 0;
    }
    break;
  }
  return -1;
}

FamilyOps pm_ops = {
    .init = pm_init,
    .allocate_channel = pm_allocate_channel,
};

void pm_main(void) { family_register(FAMILY_PROC, &pm_ops, "PM"); }
