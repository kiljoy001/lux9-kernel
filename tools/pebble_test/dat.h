#pragma once
#include "u.h"

/* Forward Declarations */
typedef struct Proc Proc;
/* typedef struct Lock Lock; // in lock.h */
typedef struct QLock QLock;
typedef struct Mach Mach;
typedef struct Ureg Ureg;
/* UserCapability is defined in blind_ledger.h */
typedef struct Conf Conf;

/* Structs */
struct Ureg {
  u64int pc;
  u64int sp;
  /* minimal */
};

/* struct Lock is defined in lock.h */
#include "lock.h"

struct QLock {
  Lock use;
  Proc *head;
  Proc *tail;
  int locked;
};

struct Mach {
  int machno;
};

#define MAXMACH 1
extern Mach *m;

/* Include Pebble Structs (Circular dependency handling) */
/* We need to define UserCapability before including pebble.h in kernel, but
 * here we just mock Proc */

/* BorrowChecker Types */
#include "blind_ledger.h"
#include "borrowchecker.h"
#include "pebble.h"

struct Proc {
  char text[128];
  ulong pid;
  char errstr[ERRMAX];
  PebbleState pebble;
  /* Minimal fields for borrowchecker/pebble */
};

extern Proc *up;
extern Conf conf;

typedef struct Confmem {
  uintptr base;
  ulong npage;
  uintptr kbase;
  uintptr klimit;
} Confmem;

struct Conf {
  ulong mem_size;
  Confmem mem[64];
};
