#include "../inc/server9p.h"
#include <lux.h>
#include <string.h>

/* path is expected to be something like "#X/0/ipcring" */
void srv_loop_ring(Srv *s, char *path) {
  uchar msg_buf[8192];
  if (!s->fidhash) {
    s->fidhashsize = 64;
    s->fidhash = pebble_alloc_zero(sizeof(Fid *) * s->fidhashsize);
    if (!s->fidhash)
      return;
  }

  /* 1. Map the IPC ring */
  IpcChannel *chan = segattach(0, path, nil, 4096);
  if (chan == (void *)-1)
    return;

  /* Construct path to ctl file */
  /* path is #X/N/ipcring, we want #X/N/ctl */
  char ctl_path[64];
  int len = strlen(path);
  if (len > 60)
    return;
  strcpy(ctl_path, path);
  /* Replace "ipcring" with "ctl" */
  /* We know ipcring is 7 chars. verify end */
  if (strcmp(ctl_path + len - 7, "ipcring") == 0) {
    strcpy(ctl_path + len - 7, "ctl");
  } else {
    /* Fallback or error */
    return;
  }

  int fd_ctl = sys_open(ctl_path, 1); /* OWRITE */

  /* 2. Main Loop */
  while (1) {
    /* Wait for submissions from kernel */
    int spins = 0;
    while (chan->submission.head == chan->submission.tail) {
      if (spins < 10000) {
        spins++;
        /* Busy wait for low latency */
        continue;
      }
      sys_sleep(1);
      spins = 0;
    }

    u32int head = chan->submission.head;
    u64int page_handle = chan->submission.pages[head & RING_MASK];

    /* The kernel uses the service's pool for forwarding.
     * We assume these are already mapped or accessible.
     * On Lux9, pool pages are shared physical pages.
     */
    BatchHeader *batch = (BatchHeader *)page_handle;

    int responded = 0; // Flag to track if any message was responded to

    if (batch->magic != BATCH_PAGE_MAGIC)
      goto skip;

    uchar *ptr = (uchar *)batch + BATCH_DATA_START;
    for (int i = 0; i < batch->num_messages; i++) {
      Req r;
      memset(&r, 0, sizeof(Req));
      r.srv = s;
      uuid_copy(&r.pid2, &batch->uuids[i]);

      u16int msg_len = *(u16int *)ptr;
      ptr += 2;

      if (convM2S(ptr, msg_len, &r.ifcall) > 0) {
        srv_dispatch(&r);

        if (r.responding) {
          /* Serialize response back into the SAME page (simple model) */
          /* We overwrite the T-message with R-message */
          u16int rep_len =
              convS2M(&r.ofcall, ptr, 8192 - (ptr - (uchar *)batch));
          /* Update the length prefix in the batch */
          *(u16int *)(ptr - 2) = rep_len;

          if (r.ofcall.type == Rstat && r.ofcall.stat)
            pebble_free(r.ofcall.stat);
          responded = 1; // Mark that a response was generated
        }
      }
      ptr += msg_len;
    }

  skip:
    chan->submission.head++;

    if (responded) {
      /* Return page to completion ring */
      u32int c_tail = chan->completion.tail;
      chan->completion.pages[c_tail & RING_MASK] = page_handle;
      chan->completion.tail++;

      /* Kick the kernel */
      if (fd_ctl >= 0) {
        sys_write(fd_ctl, "kickreply", 9);
      }
    }
  }
}
