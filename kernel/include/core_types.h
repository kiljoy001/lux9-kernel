#pragma once

/*
 * core_types.h - Minimal Fundamental Types
 *
 * Contains types that are used by value in many headers (like Qid, Dir)
 * and forward declarations for complex structures.
 */

#include "u.h"

#define ERRMAX 128
#define KNAMELEN 28

/* Qid.type bits */
#define QTDIR 0x80     /* type bit for directories */
#define QTAPPEND 0x40  /* type bit for append only files */
#define QTEXCL 0x20    /* type bit for exclusive use files */
#define QTMOUNT 0x10   /* type bit for mounted channel */
#define QTAUTH 0x08    /* type bit for authentication file */
#define QTTMP 0x04     /* type bit for non-backupable file */
#define QTSYMLINK 0x02 /* type bit for symbolic link */
#define QTFILE 0x00    /* plain file */
#define QTMAX 0x80     /* maximum value of type bit */

/* Forward declarations */
typedef struct Lock Lock;
typedef struct QLock QLock;
typedef struct RWLock RWLock;
typedef struct Ref Ref;
// typedef struct Qid Qid; // This forward declaration is moved into the
// _QID_DEFINED_ block
typedef struct Dir Dir;
typedef struct Chan Chan;
typedef struct Proc Proc;
typedef struct Mach Mach;
typedef struct Ureg Ureg;
typedef struct Fcall Fcall;
typedef struct Dirtab Dirtab;
typedef struct FPssestate FPssestate;
typedef struct FPavxstate FPavxstate;
typedef struct FPalloc FPalloc;
typedef struct FPsave FPsave;
typedef struct PFPU PFPU;
typedef struct ISAConf ISAConf;
typedef struct Label Label;
typedef struct MMU MMU;
typedef struct PCArch PCArch;
typedef struct Pcidev Pcidev;
typedef struct Physseg Physseg;
typedef struct Watchdog Watchdog;
typedef struct Block Block;
typedef struct Image Image;
typedef struct Page Page;
typedef struct Segment Segment;
typedef struct Schedq Schedq;
typedef struct Egrp Egrp;
typedef struct Fgrp Fgrp;
typedef struct Pgrp Pgrp;
typedef struct Rgrp Rgrp;
typedef struct Cmdbuf Cmdbuf;
typedef struct Note Note;
typedef struct Bpool Bpool;
typedef struct Timer Timer;
typedef struct Rendezq Rendezq;
// typedef struct Rendez Rendez; // Duplicate
// typedef struct Ref Ref; // Duplicate
typedef struct Mhead Mhead;
typedef struct Mntrah Mntrah;
typedef struct Walkqid Walkqid;
typedef struct Dev Dev;
typedef struct DevConf DevConf;
typedef struct Uart Uart;
typedef struct Queue Queue;

/*
 * Universal Plan 9 Types (defined by value)
 */

typedef vlong Tval;

#ifndef _LABEL_DEFINED_
#define _LABEL_DEFINED_
struct Label {
  uintptr sp;  /* offset 0 */
  uintptr pc;  /* offset 8 */
  uintptr rbp; /* offset 16 - frame pointer for local variables */
  uintptr rbx; /* offset 24 - callee-saved */
  uintptr r12; /* offset 32 - callee-saved */
  uintptr r13; /* offset 40 - callee-saved */
  uintptr r14; /* offset 48 - callee-saved */
  uintptr r15; /* offset 56 - callee-saved */
};
#endif

#ifndef _WAITMSG_DEFINED_
#define _WAITMSG_DEFINED_
typedef struct OWaitmsg OWaitmsg;
struct OWaitmsg {
  char pid[12];      /* of loved one */
  char time[3 * 12]; /* of loved one and descendants */
  char msg[64];      /* compatibility BUG */
};

typedef struct Waitmsg Waitmsg;
struct Waitmsg {
  int pid;          /* of loved one */
  ulong time[3];    /* of loved one and descendants */
  char msg[ERRMAX]; /* actually variable-size in user mode */
};
#endif

/* Qid.type bits defined above */

#ifndef _QID_DEFINED_
#define _QID_DEFINED_
typedef struct Qid Qid;
struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
};
#endif

/* Directory Mode bits */
#define DMDIR 0x80000000    /* mode bit for directories */
#define DMAPPEND 0x40000000 /* mode bit for append only files */
#define DMEXCL 0x20000000   /* mode bit for exclusive use files */
#define DMMOUNT 0x10000000  /* mode bit for mounted channel */
#define DMREAD 0x4          /* mode bit for read permission */
#define DMWRITE 0x2         /* mode bit for write permission */
#define DMEXEC 0x1          /* mode bit for execute permission */

#ifndef _DIR_DEFINED_
#define _DIR_DEFINED_
struct Dir {
  ushort type;
  uint dev;
  Qid qid;
  ulong mode;
  ulong atime;
  ulong mtime;
  vlong length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
};
#endif

/*
 * Lock types must NOT be defined here as they are architecture-specific.
 * Use forward declarations only.
 */
