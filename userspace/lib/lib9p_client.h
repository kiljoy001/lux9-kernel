/*
 * lib9p_client.h - Pure 9P Client Library for Lux9
 *
 * Direct exchange page access, bypassing syscalls entirely.
 * This is the preferred userspace interface for 9P operations.
 */

#ifndef _LIB9P_CLIENT_H_
#define _LIB9P_CLIENT_H_

#include <fcall.h>
#include <libc.h>
#include <u.h>

/* Forward declarations - full definitions in libc.h/fcall.h */
typedef struct Fcall Fcall;
typedef struct Dir Dir;

/* Exchange page layout (matches kernel 9p_router.h) */
#define P9_PAGE_SIZE 8192
#define P9_REQUEST_OFFSET 0x000
#define P9_REQUEST_SIZE 0xF00
#define P9_REPLY_OFFSET 0x1000
#define P9_REPLY_SIZE 0x1000
#define P9_CONTROL_OFFSET 0xF00
#define P9_CONTROL_SIZE 0x100

/* Fixed user virtual address for the Exchange Page */
#define EXCHANGE_PAGE_ADDR 0x7FFFFFFF0000ULL

/* Control Block (at offset 0xF00) */
typedef struct P9Control {
  volatile u32int doorbell;
  volatile u32int status;
  volatile u32int req_head;
  volatile u32int req_tail;
  volatile u32int rep_head;
  volatile u32int rep_tail;
  volatile u32int req_seq;
  volatile u32int rep_seq;
  uchar session_pebble[32];
  uchar reserved[192];
} P9Control;

/* Status codes */
#define P9_STATUS_IDLE 0
#define P9_STATUS_PENDING 1
#define P9_STATUS_COMPLETE 2
#define P9_STATUS_ERROR 3

/* Initialize 9P client - must be called before other functions */
void p9_init(void);

/* Core 9P Operations */
int p9_attach(char *path);                    /* Returns fid, -1 on error */
int p9_walk(int fid, char *name, int newfid); /* Walk to name, -1 on error */
int p9_open(int fid, int mode);               /* Open fid, returns iounit */
int p9_create(int fid, char *name, int perm,
              int mode); /* Create file, -1 on error */
long p9_read(int fid, void *buf, long count, vlong offset);
long p9_write(int fid, void *buf, long count, vlong offset);
int p9_stat(int fid, Dir *d);  /* Stat fid, -1 on error */
int p9_wstat(int fid, Dir *d); /* Write stat, -1 on error */
int p9_remove(int fid);        /* Remove file, -1 on error */
void p9_clunk(int fid);        /* Close fid */

/* High-level convenience functions */
int p9_openfile(char *path, int mode);                /* Open file by path */
int p9_createfile(char *path, int perm, int mode);    /* Create file at path */
long p9_readfile(char *path, void *buf, long count);  /* Read entire file */
long p9_writefile(char *path, void *buf, long count); /* Write to file */

/* Fid management */
int p9_allocfid(void);    /* Get new fid */
void p9_freefid(int fid); /* Release fid */

/* Error handling */
char *p9_errstr(void); /* Get last error string */

#endif /* _LIB9P_CLIENT_H_ */
