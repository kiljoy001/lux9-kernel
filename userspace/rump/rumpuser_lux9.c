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

struct rumpuser_mtx {
  volatile int locked;
};

struct rumpuser_rw {
  volatile int locked;
};

struct rumpuser_cv {
  volatile int seq;
};

struct rumpuser_thread {
  int pid;
  int joinable;
};

extern void _exit(int status);

/* External internal Lux9 syscall helpers (from start.S / init.c / resurrection
 * logic) */
#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_MSG_OFFSET 0x000
#define P9_MSG_SIZE 0xF00
#define P9_CONTROL_OFFSET 0xF00

extern unsigned long long lux_exchange_base;
static inline unsigned long long exchange_base(void) {
  if (lux_exchange_base != 0)
    return lux_exchange_base;
  return EXCHANGE_PAGE_ADDR;
}

#define SYS_WRITE 20 /* WRITE syscall number from kernel/include/fcall.h */
#define SYS_EXIT 8   /* EXITS syscall number from kernel/include/sys.h */
#define SYS_NSEC 53  /* NSEC syscall number from kernel/include/fcall.h */
#define SYS_WAIT 166 /* WAIT syscall number from kernel/include/fcall.h */

#define Tsyscall 130
#define Rsyscall 131
#define Tsysfork 160
#define Rsysfork 161
#define Tsyssleep 170
#define Rsyssleep 171
#define Rerror 107

#define RFPROC (1 << 4)
#define RFMEM (1 << 5)
#define RFFDG (1 << 2)

struct P9Control {
  uint32_t doorbell;
  uint32_t status;
  uint32_t req_head;
  uint32_t req_tail;
  uint32_t rep_head;
  uint32_t rep_tail;
  uint32_t req_seq;
  uint32_t rep_seq;
};

#define P9_STATUS_IDLE 0
#define P9_STATUS_PENDING 1
#define P9_STATUS_COMPLETE 2
#define P9_STATUS_ERROR 3

static void ring_doorbell(volatile struct P9Control *ctl) {
  ctl->req_seq += 1;
  ctl->status = P9_STATUS_PENDING;
  __asm__ volatile("mfence" ::: "memory");
  ctl->doorbell = 1;
}

static void *lux_memcpy(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  while (n--)
    *d++ = *s++;
  return dst;
}

static int lux_str_contains(const char *s, const char *needle) {
  if (!s || !needle || !*needle)
    return 0;
  for (; *s; s++) {
    const char *a = s;
    const char *b = needle;
    while (*a && *b && *a == *b) {
      a++;
      b++;
    }
    if (*b == '\0')
      return 1;
  }
  return 0;
}

static inline void lux_syscall(void) {
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
}

static void put_u16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void put_u64(uint8_t *p, uint64_t v) {
  put_u32(p, (uint32_t)(v & 0xffffffffu));
  put_u32(p + 4, (uint32_t)(v >> 32));
}

static uint32_t get_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static uint64_t get_u64(const uint8_t *p) {
  return (uint64_t)get_u32(p) | ((uint64_t)get_u32(p + 4) << 32);
}

/* Global lock for Rump syscall serialization */
static volatile int rump_syscall_global_lock = 0;

static void rump_syscall_lock_acquire(void) {
  while (__sync_lock_test_and_set(&rump_syscall_global_lock, 1)) {
    __asm__ volatile("pause");
  }
}

static void rump_syscall_lock_release(void) {
  __sync_lock_release(&rump_syscall_global_lock);
}

static int lux_send_tsyscall(uint32_t scallnr, const void *data,
                             uint32_t data_len, uint64_t *retval,
                             const uint8_t **out_data, uint32_t *out_len) {
  uint8_t *page = (uint8_t *)exchange_base() + P9_MSG_OFFSET;
  uint32_t size = 4 + 1 + 2 + 4 + 4 + 4 + data_len;
  if (size > P9_MSG_SIZE)
    return -1;

  put_u32(page + 0, size);
  page[4] = Tsyscall;
  put_u16(page + 5, 1);
  put_u32(page + 7, scallnr);
  put_u32(page + 11, 0);
  put_u32(page + 15, data_len);
  if (data_len > 0 && data)
    lux_memcpy(page + 19, data, data_len);

  ring_doorbell((volatile struct P9Control *)(page + P9_CONTROL_OFFSET));
  lux_syscall();

  uint32_t rsize = get_u32(page + 0);
  if (rsize < 7 || rsize > P9_MSG_SIZE)
    return -1;
  uint8_t rtype = page[4];
  if (rtype == Rerror)
    return -1;
  if (rtype != Rsyscall)
    return -1;

  const uint8_t *p = page + 7;
  if (p + 8 + 4 > page + rsize)
    return -1;
  uint64_t rret = get_u64(p);
  p += 8;
  uint32_t rcount = get_u32(p);
  p += 4;
  if (p + rcount > page + rsize)
    return -1;

  if (retval)
    *retval = rret;
  if (out_data)
    *out_data = p;
  if (out_len)
    *out_len = rcount;
  return 0;
}

static int lux_send_tsyssleep(uint32_t ms) {
  uint8_t *page = (uint8_t *)exchange_base() + P9_MSG_OFFSET;
  uint32_t size = 4 + 1 + 2 + 4;
  put_u32(page + 0, size);
  page[4] = Tsyssleep;
  put_u16(page + 5, 1);
  put_u32(page + 7, ms);

  ring_doorbell((volatile struct P9Control *)(page + P9_CONTROL_OFFSET));
  lux_syscall();

  uint32_t rsize = get_u32(page + 0);
  if (rsize < 7 || rsize > P9_MSG_SIZE)
    return -1;
  uint8_t rtype = page[4];
  if (rtype == Rerror)
    return -1;
  if (rtype != Rsyssleep)
    return -1;
  return 0;
}

static int lux_send_tsysfork(uint32_t flags, uint32_t *pid) {
  uint8_t *page = (uint8_t *)exchange_base() + P9_MSG_OFFSET;
  uint32_t size = 4 + 1 + 2 + 4;
  put_u32(page + 0, size);
  page[4] = Tsysfork;
  put_u16(page + 5, 1);
  put_u32(page + 7, flags);

  ring_doorbell((volatile struct P9Control *)(page + P9_CONTROL_OFFSET));
  lux_syscall();

  uint32_t rsize = get_u32(page + 0);
  if (rsize < 7 || rsize > P9_MSG_SIZE)
    return 0;
  uint8_t rtype = page[4];
  if (rtype == Rerror)
    return -1;
  if (rtype != Rsysfork)
    return 0;
  if (pid)
    *pid = get_u32(page + 7);
  return 0;
}

static int lux_sys_write(int fd, const void *buf, size_t count) {
  uint8_t header[8];
  put_u32(header + 0, (uint32_t)fd);
  put_u32(header + 4, (uint32_t)count);

  uint8_t payload[256 + 8];
  size_t max_copy = sizeof(payload) - sizeof(header);
  if (count > max_copy)
    count = max_copy;
  lux_memcpy(payload, header, sizeof(header));
  if (count > 0)
    lux_memcpy(payload + sizeof(header), buf, count);

  uint64_t retval = 0;
  rump_syscall_lock_acquire();
  int rc = lux_send_tsyscall(SYS_WRITE, payload,
                             (uint32_t)(sizeof(header) + count), &retval, 0, 0);
  rump_syscall_lock_release();
  if (rc < 0)
    return -1;
  return (int)retval;
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
  (void)enum_rumpclock;
  uint64_t now = 0;
  rump_syscall_lock_acquire();
  if (lux_send_tsyscall(SYS_NSEC, 0, 0, &now, 0, 0) < 0)
    now = 0;
  rump_syscall_lock_release();
  *sec = (int64_t)(now / 1000000000ULL);
  *nsec = (long)(now % 1000000000ULL);
  return 0;
}

int rumpuser_clock_sleep(int enum_rumpclock, int64_t sec, long nsec) {
  (void)enum_rumpclock;
  uint64_t total_ns = (uint64_t)sec * 1000000000ULL + (uint64_t)nsec;
  uint32_t ms = (uint32_t)(total_ns / 1000000ULL);
  if (ms == 0 && total_ns > 0)
    ms = 1;
  rump_syscall_lock_acquire();
  lux_send_tsyssleep(ms);
  rump_syscall_lock_release();
  return 0;
}

int rumpuser_getparam(const char *name, void *buf, size_t len) {
  if (!name || !buf || len == 0)
    return EINVAL;
  if (lux_str_contains(name, "NCPU")) {
    if (len < sizeof(int))
      return EINVAL;
    *(int *)buf = 1;
    return 0;
  }
  if (lux_str_contains(name, "HOSTNAME")) {
    const char *host = "lux9";
    size_t n = 0;
    while (host[n] && n + 1 < len) {
      ((char *)buf)[n] = host[n];
      n++;
    }
    ((char *)buf)[n] = '\0';
    return 0;
  }
  return EINVAL;
}

void rumpuser_putchar(int c) {
  /* Write single character to stdout using Lux9 syscall */
  char ch = (char)c;
  lux_sys_write(1, &ch, 1);
}

void rumpuser_exit(int retval) { _exit(retval); }

/* Lightweight thread/mutex/cv support for rump bootstrap. */
void rumpuser_mutex_init(struct rumpuser_mtx **mtx, int flags) {
  (void)flags;
  struct rumpuser_mtx *m = 0;
  if (rumpuser_malloc(sizeof(*m), 8, (void **)&m) != 0) {
    *mtx = 0;
    return;
  }
  m->locked = 0;
  *mtx = m;
}
void rumpuser_mutex_enter(struct rumpuser_mtx *mtx) {
  if (!mtx)
    return;
  while (__sync_lock_test_and_set(&mtx->locked, 1)) {
    __asm__ volatile("pause");
  }
}
void rumpuser_mutex_exit(struct rumpuser_mtx *mtx) {
  if (!mtx)
    return;
  __sync_lock_release(&mtx->locked);
}
void rumpuser_mutex_destroy(struct rumpuser_mtx *mtx) { (void)mtx; }
void rumpuser_mutex_owner(struct rumpuser_mtx *mtx, struct lwp **l) {
  (void)mtx;
  *l = NULL;
}
void rumpuser_mutex_enter_nowrap(struct rumpuser_mtx *mtx) {
  rumpuser_mutex_enter(mtx);
}
int rumpuser_mutex_tryenter(struct rumpuser_mtx *mtx) {
  if (!mtx)
    return EINVAL;
  return __sync_lock_test_and_set(&mtx->locked, 1) ? 1 : 0;
}

void rumpuser_rw_init(struct rumpuser_rw **rw) {
  struct rumpuser_rw *r = 0;
  if (rumpuser_malloc(sizeof(*r), 8, (void **)&r) != 0) {
    *rw = 0;
    return;
  }
  r->locked = 0;
  *rw = r;
}
void rumpuser_rw_enter(int enum_rumprwlock, struct rumpuser_rw *rw) {
  (void)enum_rumprwlock;
  if (!rw)
    return;
  while (__sync_lock_test_and_set(&rw->locked, 1)) {
    __asm__ volatile("pause");
  }
}
void rumpuser_rw_exit(struct rumpuser_rw *rw) {
  if (!rw)
    return;
  __sync_lock_release(&rw->locked);
}
void rumpuser_rw_destroy(struct rumpuser_rw *rw) { (void)rw; }
int rumpuser_rw_tryenter(int enum_rumprwlock, struct rumpuser_rw *rw) {
  (void)enum_rumprwlock;
  if (!rw)
    return EINVAL;
  return __sync_lock_test_and_set(&rw->locked, 1) ? 1 : 0;
}
int rumpuser_rw_tryupgrade(struct rumpuser_rw *rw) { return 0; }
void rumpuser_rw_downgrade(struct rumpuser_rw *rw) {}
void rumpuser_rw_held(int enum_rumprwlock, struct rumpuser_rw *rw, int *held) {
  (void)enum_rumprwlock;
  if (!rw) {
    *held = 0;
    return;
  }
  *held = rw->locked ? 1 : 0;
}

void rumpuser_cv_init(struct rumpuser_cv **cv) {
  struct rumpuser_cv *c = 0;
  if (rumpuser_malloc(sizeof(*c), 8, (void **)&c) != 0) {
    *cv = 0;
    return;
  }
  c->seq = 0;
  *cv = c;
}
void rumpuser_cv_destroy(struct rumpuser_cv *cv) { (void)cv; }
void rumpuser_cv_wait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx) {
  if (!cv || !mtx)
    return;
  int snapshot = cv->seq;
  rumpuser_mutex_exit(mtx);
  while (cv->seq == snapshot) {
    rumpuser_clock_sleep(0, 0, 1000000);
  }
  rumpuser_mutex_enter(mtx);
}
void rumpuser_cv_wait_nowrap(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx) {
  rumpuser_cv_wait(cv, mtx);
}
int rumpuser_cv_timedwait(struct rumpuser_cv *cv, struct rumpuser_mtx *mtx,
                          int64_t s, int64_t ns) {
  if (!cv || !mtx)
    return EINVAL;
  int snapshot = cv->seq;
  rumpuser_mutex_exit(mtx);
  int64_t remaining = s * 1000000000LL + ns;
  while (cv->seq == snapshot && remaining > 0) {
    int64_t step = remaining > 10000000LL ? 10000000LL : remaining;
    rumpuser_clock_sleep(0, 0, (long)step);
    remaining -= step;
  }
  rumpuser_mutex_enter(mtx);
  return (cv->seq == snapshot) ? 1 : 0;
}
void rumpuser_cv_broadcast(struct rumpuser_cv *cv) {
  if (cv)
    __sync_fetch_and_add(&cv->seq, 1);
}
void rumpuser_cv_signal(struct rumpuser_cv *cv) {
  if (cv)
    __sync_fetch_and_add(&cv->seq, 1);
}
void rumpuser_cv_has_waiters(struct rumpuser_cv *cv, int *n) {
  (void)cv;
  *n = 0;
}

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
  (void)flags;
  if (!buf) {
    if (ret)
      *ret = 0;
    return EINVAL;
  }
  uint64_t seed = 0;
  rump_syscall_lock_acquire();
  lux_send_tsyscall(SYS_NSEC, 0, 0, &seed, 0, 0);
  rump_syscall_lock_release();
  seed ^= (uint64_t)(uintptr_t)buf;
  uint8_t *out = (uint8_t *)buf;
  for (size_t i = 0; i < buflen; i++) {
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    out[i] = (uint8_t)seed;
  }
  if (ret)
    *ret = buflen;
  return 0;
}
int rumpuser_thread_create(void *(*f)(void *), void *arg, const char *name,
                           int mustjoin, int priority, int cpuidx,
                           void **cookie) {
  (void)name;
  (void)priority;
  (void)cpuidx;
  struct rumpuser_thread *t = 0;
  if (rumpuser_malloc(sizeof(*t), 8, (void **)&t) != 0)
    return ENOMEM;
  t->pid = -1;
  t->joinable = mustjoin ? 1 : 0;

  uint32_t pid = 0;
  int rc = lux_send_tsysfork(RFPROC | RFMEM | RFFDG, &pid);
  if (rc < 0)
    return rc;
  if (pid == 0) {
    (void)f(arg);
    _exit(0);
    return 0;
  }

  t->pid = (int)pid;
  if (cookie)
    *cookie = t;
  return 0;
}
void rumpuser_thread_exit(void) { _exit(0); }
int rumpuser_thread_join(void *cookie) {
  struct rumpuser_thread *t = (struct rumpuser_thread *)cookie;
  if (!t || t->pid <= 0)
    return EINVAL;
  for (;;) {
    uint64_t ret = 0;
    rump_syscall_lock_acquire();
    int rc = lux_send_tsyscall(SYS_WAIT, 0, 0, &ret, 0, 0);
    rump_syscall_lock_release();
    if (rc < 0)
      return rc;
    if ((int)ret == t->pid)
      return 0;
  }
}
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
