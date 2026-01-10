#ifndef _KERNEL_ROUTER_H_
#define _KERNEL_ROUTER_H_

#include "../include/u.h"
#include "../include/portlib.h"
#include "../include/mem.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/ureg.h"
#include "../include/fcall.h"
#include "../include/9p_router.h"

/* Common utilities from 9p_router.c */
uchar *tsyscall_skip_argc(uchar *p, uchar *ep, u32int expected);
int p9_exchange_contains(Proc *p, void *ptr, ulong len);

/* Dispatch handlers */
int router_dispatch_fs(Proc *p, Fcall *t, Fcall *r);
int router_dispatch_proc(Proc *p, Fcall *t, Fcall *r);
int router_dispatch_ipc(Proc *p, Fcall *t, Fcall *r);
int router_dispatch_wasm(Proc *p, Fcall *t, Fcall *r);

/* External syscall declarations needed by handlers */
extern uintptr sys_exchange_alloc(void *);
extern uintptr sys_exchange_free(void *);
extern uintptr sys_exchange_publish(void *);
extern uintptr sys_exchange_subscribe(void *);
extern uintptr sys_exchange_unsubscribe(void *);
extern uintptr sys_exchange_receive(void *);
extern uintptr syspipe(void *);
extern uintptr sysrfork(void *);
extern void pexit(char *, int);
extern uintptr ibrk(uintptr, int);
extern ulong pwait(Waitmsg *);
extern uintptr sysexec(void *);
extern uvlong nsec(void);
extern void userpmap(uintptr, uintptr, int);

#endif /* _KERNEL_ROUTER_H_ */