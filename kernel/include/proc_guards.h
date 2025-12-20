#pragma once
/* Proc structure guard macros - requires u.h, dat.h, portlib.h already included
 * by caller */

/* Runtime memory safety guards for Proc structure */

#ifdef ENABLE_MEMORY_GUARDS

/* Initialize guard canaries in a Proc structure */
static inline void proc_init_guards(Proc *p) {
  if (p == nil)
    return;
  p->guard_post = PROC_GUARD_MAGIC_POST;
}

/* Check guard canaries for corruption */
static inline void proc_check_guards(Proc *p, const char *location) {
  if (p == nil)
    return;

  if (p->guard_post != PROC_GUARD_MAGIC_POST) {
    panic("CORRUPTION: Proc overflow at %s: pid=%lud guard_post=%#llux "
          "(expected %#llux)",
          location, p->pid, p->guard_post, PROC_GUARD_MAGIC_POST);
  }
}

/* Validate pointer is in kernel address space */
static inline void proc_validate_ptr(Proc *p, const char *location) {
  uintptr addr = (uintptr)p;

  if (p == nil)
    panic("CORRUPTION: nil Proc at %s", location);

  /* Must be in kernel space */
  if (addr < KZERO) {
    panic("CORRUPTION: invalid Proc at %s: %#p (not in kernel space)", location,
          p);
  }

  /* Check alignment */
  if ((addr & 0x7) != 0) {
    panic("CORRUPTION: misaligned Proc at %s: %#p", location, p);
  }

  proc_check_guards(p, location);
}

/* Poison freed Proc to catch use-after-free */
static inline void proc_poison(Proc *p) {
  if (p == nil)
    return;

  ulong pid = p->pid;

  /* Overwrite with poison pattern 0xDD */
  memset((char *)p, 0xDD, sizeof(Proc) - sizeof(p->guard_post));

  p->guard_post = 0xDDDDDDDDDDDDDDDDULL;
  p->pid = pid;
}

#define VALIDATE_PROC(p) proc_validate_ptr((p), __func__)
#define CHECK_PROC_GUARDS(p) proc_check_guards((p), __func__)
#define INIT_PROC_GUARDS(p) proc_init_guards(p)
#define POISON_PROC(p) proc_poison(p)

#else

#define VALIDATE_PROC(p)                                                       \
  do {                                                                         \
  } while (0)
#define CHECK_PROC_GUARDS(p)                                                   \
  do {                                                                         \
  } while (0)
#define INIT_PROC_GUARDS(p)                                                    \
  do {                                                                         \
  } while (0)
#define POISON_PROC(p)                                                         \
  do {                                                                         \
  } while (0)

#endif
