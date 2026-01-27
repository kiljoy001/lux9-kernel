#ifndef _LOCK_H_
#define _LOCK_H_

#include "types_fwd.h"
#include "u.h"

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
