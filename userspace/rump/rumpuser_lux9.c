/*
 * rumpuser_lux9.c
 * Rump hypervisor implementation for Lux9 userspace (bare metal)
 */

#define LIBRUMPUSER 1

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* Define macros expected by NetBSD headers if missing */
#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif
#ifndef __printflike
#define __printflike(a, b) __attribute__((__format__(__printf__, a, b)))
#endif

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Errno definitions */
#ifndef ENOMEM
#define ENOMEM 12
#endif
#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef ENOSYS
#define ENOSYS 38
#endif

/* Rumpuser interface */
#include <rump/rump_syscalls.h>
#include <rump/rumpuser.h>

/* External internal Lux9 syscall helpers (from start.S / init.c / resurrection
 * logic) */
#define SYS_WRITE 4 /* _WRITE syscall number from kernel/include/sys.h */
#define SYS_EXIT 8   /* EXITS syscall number from kernel/include/sys.h */

void _sys_write(int fd, const void *buf, size_t count) {
  /* Invoke Lux9 write syscall: syscall(SYS_WRITE, fd, buf, count) */
  __asm__ volatile("syscall" ::"a"(SYS_WRITE), "D"(fd), "S"(buf), "d"(count));
}

/* Memory Allocation: Simple Static Heap (64MB) */
#define HEAP_SIZE (64 * 1024 * 1024)
static char heap[HEAP_SIZE];
static size_t heap_pos = 0;

int rumpuser_malloc(size_t size, int alignment, void **ptr) {
  /* Align heap_pos */
  if (alignment > 0) {
    size_t mask = alignment - 1;
    if (heap_pos & mask) {
      heap_pos = (heap_pos + mask) & ~mask;
    }
  }

  if (heap_pos + size > HEAP_SIZE) {
    return ENOMEM;
  }

  *ptr = &heap[heap_pos];
  heap_pos += size;
  return 0;
}

void rumpuser_free(void *ptr, size_t size) {
  /* No-op for bump allocator */
  (void)ptr;
  (void)size;
}

/* Memory Map Stubs (using heap) */
int rumpuser_anonmmap(void *pref, size_t size, int align, int exec,
                      void **ptr) {
  return rumpuser_malloc(size, align, ptr);
}

void rumpuser_unmap(void *ptr, size_t size) { rumpuser_free(ptr, size); }

/* Curlwp Stubs (Single Threaded - Global LWP) */
static struct lwp *current_lwp = NULL;

struct lwp *rumpuser_curlwp(void) { return current_lwp; }

void rumpuser_curlwpop(int op, struct lwp *l) {
  switch (op) {
  case RUMPUSER_LWP_SET:
    current_lwp = l;
    break;
  case RUMPUSER_LWP_CLEAR:
    /* Only clear if it matches current */
    if (current_lwp == l)
      current_lwp = NULL;
    break;
  case RUMPUSER_LWP_CREATE:
  case RUMPUSER_LWP_DESTROY:
    /* No-op for now */
    break;
  }
}

int rumpuser_clock_gettime(int enum_rumpclock, int64_t *sec, long *nsec) {
  /* Stub: Return 0 time or incrementing time */
  *sec = 0;
  *nsec = 0;
  return 0;
}

int rumpuser_clock_sleep(int enum_rumpclock, int64_t sec, long nsec) {
  /* Stub: Spin loop or yield */
  return 0;
}

int rumpuser_getparam(const char *name, void *buf, size_t len) {
  /* Handle _RUMPUSER_NCPU and _RUMPUSER_HOSTNAME */
  /* ... stubs ... */
  return 0; // Success (mostly empty)
}

void rumpuser_putchar(int c) {
  /* Write single character to stdout using Lux9 syscall */
  char ch = (char)c;
  _sys_write(1, &ch, 1);
}

void rumpuser_exit(int retval) {
  /* Syscall exit */
  __asm__ volatile("syscall" ::"a"(SYS_EXIT), "D"(retval));
  while (1)
    ;
}

/* Stubs for thread/mutex/cv - Single Threaded for now */
void rumpuser_mutex_init(struct rumpuser_mtx **mtx, int flags) {
  *mtx = (void *)1;
}
void rumpuser_mutex_enter(struct rumpuser_mtx *mtx) {}
void rumpuser_mutex_exit(struct rumpuser_mtx *mtx) {}
void rumpuser_mutex_destroy(struct rumpuser_mtx *mtx) {}
void rumpuser_mutex_owner(struct rumpuser_mtx *mtx, struct lwp **l) {
  *l = NULL;
}
void rumpuser_mutex_enter_nowrap(struct rumpuser_mtx *mtx) {}
int rumpuser_mutex_tryenter(struct rumpuser_mtx *mtx) { return 0; }

void rumpuser_rw_init(struct rumpuser_rw **rw) { *rw = (void *)1; }
void rumpuser_rw_enter(int enum_rumprwlock, struct rumpuser_rw *rw) {}
void rumpuser_rw_exit(struct rumpuser_rw *rw) {}
void rumpuser_rw_destroy(struct rumpuser_rw *rw) {}
int rumpuser_rw_tryenter(int enum_rumprwlock, struct rumpuser_rw *rw) {
  return 0;
}
int rumpuser_rw_tryupgrade(struct rumpuser_rw *rw) { return 0; }
void rumpuser_rw_downgrade(struct rumpuser_rw *rw) {}
void rumpuser_rw_held(int enum_rumprwlock, struct rumpuser_rw *rw, int *held) {
  *held = 1;
}

void rumpuser_cv_init(struct rumpuser_cv **cv) { *cv = (void *)1; }
void rumpuser_cv_destroy(struct rumpuser_cv *cv) {}
void rumpuser_cv_wait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx) {}
void rumpuser_cv_wait_nowrap(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx) {
}
int rumpuser_cv_timedwait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx,
                          int64_t s, int64_t ns) {
  return 0;
}
void rumpuser_cv_broadcast(struct rumpuser_cv *cv) {}
void rumpuser_cv_signal(struct rumpuser_cv *cv) {}
void rumpuser_cv_has_waiters(struct rumpuser_cv *cv, int *n) { *n = 0; }

/* Init */
int rumpuser_init(int version, const struct rumpuser_hyperup *hyp) {
  if (version != RUMPUSER_VERSION)
    return EINVAL;
  /* Save hyp functions if needed for upcalls */
  return 0;
}

/* File I/O - Stubbed for now, eventually use 9P calls */
int rumpuser_open(const char *path, int mode, int *fd) { return ENOSYS; }
int rumpuser_close(int fd) { return 0; }
int rumpuser_getfileinfo(const char *path, uint64_t *size, int *type) {
  return ENOSYS;
}
void rumpuser_bio(int fd, int op, void *data, size_t dlen, int64_t off,
                  rump_biodone_fn biodone, void *biodone_arg) {
  /* Immediate error or success */
  biodone(biodone_arg, 0, ENOSYS);
}
int rumpuser_iovread(int fd, struct rumpuser_iovec *iov, size_t iovcnt,
                     int64_t off, size_t *ret) {
  return ENOSYS;
}
int rumpuser_iovwrite(int fd, const struct rumpuser_iovec *iov, size_t iovcnt,
                      int64_t off, size_t *ret) {
  return ENOSYS;
}
int rumpuser_syncfd(int fd, int flags, uint64_t start, uint64_t len) {
  return 0;
}

/* Dynloader stubs */
void rumpuser_dl_bootstrap(rump_modinit_fn a, rump_symload_fn b,
                           rump_compload_fn c) {}

/* Misc stubs */
int rumpuser_daemonize_begin(void) { return 0; }
int rumpuser_daemonize_done(int error) { return 0; }
void rumpuser_seterrno(int error) {}
int rumpuser_kill(int64_t pid, int sig) { return ENOSYS; }
int rumpuser_getrandom(void *buf, size_t buflen, int flags, size_t *ret) {
  return ENOSYS;
}
int rumpuser_thread_create(void *(*f)(void *), void *arg, const char *name,
                           int mustjoin, int priority, int cpuidx,
                           void **cookie) {
  return ENOSYS;
}
void rumpuser_thread_exit(void) {
  while (1)
    ;
}
int rumpuser_thread_join(void *cookie) { return ENOSYS; }
void rumpuser_dprintf(const char *fmt, ...) {}

#ifdef RUMP_SYSPROXY
/* Syscall proxy stubs if needed */
int rumpuser_sp_init(const char *a, const char *b, const char *c,
                     const char *d) {
  return ENOSYS;
}
int rumpuser_sp_copyin(void *a, const void *b, void *c, size_t d) {
  return ENOSYS;
}
int rumpuser_sp_copyinstr(void *a, const void *b, void *c, size_t *d) {
  return ENOSYS;
}
int rumpuser_sp_copyout(void *a, const void *b, void *c, size_t d) {
  return ENOSYS;
}
int rumpuser_sp_copyoutstr(void *a, const void *b, void *c, size_t *d) {
  return ENOSYS;
}
int rumpuser_sp_anonmmap(void *a, size_t b, void **c) { return ENOSYS; }
int rumpuser_sp_raise(void *a, int b) { return ENOSYS; }
void rumpuser_sp_fini(void *a) {}
#endif
