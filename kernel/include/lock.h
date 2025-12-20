#ifndef _LOCK_H_
#define _LOCK_H_

#include "u.h"

#ifndef _PROC_DEFINED
#define _PROC_DEFINED
typedef struct Proc Proc;
#endif

#ifndef _MACH_DEFINED
#define _MACH_DEFINED
typedef struct Mach Mach;
#endif

typedef struct Lock Lock;

struct Lock {
  ulong key;
  ulong sr;
  uintptr pc;
  Proc *p;
  Mach *m;
  ushort isilock;
  long lockcycles;
} __attribute__((aligned(64)));

#endif
