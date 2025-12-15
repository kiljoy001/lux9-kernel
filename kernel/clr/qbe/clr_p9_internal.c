/*
 * CLR P9 Internal Calls
 *
 * Kernel-side implementation of System.P9.P9Internal InternalCall methods.
 * These bridge managed .NET code to the kernel's 9P router.
 */

#include "kernel_compat.h"

/* Need 9P types and functions */
extern int p9_dispatch(void *proc, void *t, void *r);
extern void *up; /* Current process */

/* 9P message types from fcall.h */
#define Tattach 104
#define Rattach 105
#define Tread 116
#define Rread 117
#define Twrite 118
#define Rwrite 119
#define Tclunk 120
#define Rclunk 121
#define Tstat 124
#define Rstat 125
#define Rerror 107

/* Simplified Fcall structure for internal use */
typedef struct Fcall {
  unsigned char type;
  unsigned int fid;
  unsigned short tag;
  char *aname;        /* For Tattach */
  char *data;         /* For Tread/Twrite */
  unsigned int count; /* For Tread/Twrite */
  long long offset;   /* For Tread/Twrite */
  char *ename;        /* For Rerror */
                      /* Qid and other fields handled by real p9_dispatch */
} Fcall;

/* Global fid counter (atomic increment to avoid reuse races across calls) */
static unsigned int next_fid = 1;

/*
 * P9Internal.Attach(string path)
 *
 * Sends Tattach to the given path.
 * Returns fid for subsequent operations.
 */
unsigned int clr_p9_attach(const char *path) {
  Fcall t, r;
  unsigned int fid;

  if (path == NULL)
    return 0;

  /* Allocate new fid atomically */
  fid = __sync_fetch_and_add(&next_fid, 1);

  /* Build Tattach message */
  memset(&t, 0, sizeof(t));
  memset(&r, 0, sizeof(r));

  t.type = Tattach;
  t.fid = fid;
  t.tag = 0;
  t.aname = (char *)path;

  /* Dispatch through 9P router
   * This will:
   * 1. Extract Pebble from process's P9Control if present
   * 2. Validate with BlindLedger
   * 3. Route to appropriate device handler
   */
  if (p9_dispatch(up, &t, &r) < 0) {
    /* Error - return 0 to indicate failure */
    return 0;
  }

  /* Check for Rerror */
  if (r.type == Rerror) {
    return 0;
  }

  return fid;
}

/*
 * P9Internal.Read(uint fid, byte[] buffer, int offset, int count, long
 * position)
 *
 * Sends Tread to the given fid.
 * Returns number of bytes read.
 */
int clr_p9_read(unsigned int fid, void *buffer, int offset, int count,
                long long position) {
  Fcall t, r;
  char *data_ptr;

  if (buffer == NULL || count <= 0)
    return 0;

  /* Adjust buffer pointer by offset */
  data_ptr = (char *)buffer + offset;

  /* Build Tread message */
  memset(&t, 0, sizeof(t));
  memset(&r, 0, sizeof(r));

  t.type = Tread;
  t.fid = fid;
  t.tag = 0;
  t.count = (unsigned int)count;
  t.offset = position;

  /* Allocate response data buffer on stack (limited size) */
  char resp_buf[8192];
  if (count > sizeof(resp_buf))
    count = sizeof(resp_buf);

  r.data = resp_buf;

  /* Dispatch */
  if (p9_dispatch(up, &t, &r) < 0) {
    return 0;
  }

  /* Check for Rerror */
  if (r.type == Rerror) {
    return 0;
  }

  /* Copy data from response to buffer */
  if (r.type == Rread && r.count > 0) {
    memmove(data_ptr, r.data, r.count);
    return (int)r.count;
  }

  return 0;
}

/*
 * P9Internal.Write(uint fid, byte[] buffer, int offset, int count, long
 * position)
 *
 * Sends Twrite to the given fid.
 * Returns number of bytes written.
 */
int clr_p9_write(unsigned int fid, void *buffer, int offset, int count,
                 long long position) {
  Fcall t, r;
  char *data_ptr;

  if (buffer == NULL || count <= 0)
    return 0;

  /* Adjust buffer pointer by offset */
  data_ptr = (char *)buffer + offset;

  /* Build Twrite message */
  memset(&t, 0, sizeof(t));
  memset(&r, 0, sizeof(r));

  t.type = Twrite;
  t.fid = fid;
  t.tag = 0;
  t.data = data_ptr;
  t.count = (unsigned int)count;
  t.offset = position;

  /* Dispatch */
  if (p9_dispatch(up, &t, &r) < 0) {
    return 0;
  }

  /* Check for Rerror */
  if (r.type == Rerror) {
    return 0;
  }

  /* Return number of bytes written */
  if (r.type == Rwrite) {
    return (int)r.count;
  }

  return 0;
}

/*
 * P9Internal.Clunk(uint fid)
 *
 * Sends Tclunk to close the fid.
 */
void clr_p9_clunk(unsigned int fid) {
  Fcall t, r;

  /* Build Tclunk message */
  memset(&t, 0, sizeof(t));
  memset(&r, 0, sizeof(r));

  t.type = Tclunk;
  t.fid = fid;
  t.tag = 0;

  /* Dispatch - ignore errors on close */
  p9_dispatch(up, &t, &r);
}

/*
 * P9Internal.Stat(uint fid)
 *
 * Sends Tstat to get file length.
 * Returns length, or -1 on error.
 */
long long clr_p9_stat(unsigned int fid) {
  Fcall t, r;

  /* Build Tstat message */
  memset(&t, 0, sizeof(t));
  memset(&r, 0, sizeof(r));

  t.type = Tstat;
  t.fid = fid;
  t.tag = 0;

  /* Dispatch */
  if (p9_dispatch(up, &t, &r) < 0) {
    return -1;
  }

  /* Check for Rerror */
  if (r.type == Rerror) {
    return -1;
  }

  /* For now, return 0 - proper Tstat handling needs Dir parsing */
  /* TODO: Parse Dir structure from r.stat to get length */
  return 0;
}
