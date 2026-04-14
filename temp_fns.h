#ifndef _FNS_H_
#define _FNS_H_

#include "dat.h"
#include <stdarg.h>

void		_assert(char*);
void		accounttime(void);
Timer*		addclock0link(void (*)(void), int);
Physseg*	addphysseg(Physseg*);
void		addbootfile(char*, uchar*, ulong);
void		addwatchdog(Watchdog*);
Block*		adjustblock(Block*, int);
void		alarmkproc(void*);
Block*		allocb(int);
int		anyhigher(void);
int		anyready(void);
Image*		attachimage(Chan*, ulong size);
ulong		beswal(ulong);
uvlong		beswav(uvlong);
int		blocklen(Block*);
void		bootlinks(void);
void		cachedel(Image*, uintptr);
void		cachepage(Page*, Image*);
void		callwithureg(void(*)(Ureg*));
char*		chanpath(Chan*);
int		canlock(Lock*);
int		canpage(Proc*);
int		canqlock(QLock*);
int		cmpswap486(long*, long, long);
#define cmpswap(addr, old, new) cmpswap486((long*)(addr), (long)(old), (long)(new))
int		canrlock(RWLock*);
void		chandevinit(void);
void		chandevreset(void);
void		chandevshutdown(void);
void		chanfree(Chan*);
void		checkalarms(void);
void		checkpages(void);
void		checkb(Block*, char*);
void		cinit(void);
Chan*		cclone(Chan*);
void		cclose(Chan*);
void		ccloseq(Chan*);
void		closeegrp(Egrp*);
void		closefgrp(Fgrp*);
void		closepgrp(Pgrp*);
void		closergrp(Rgrp*);
long		clrfpintr(void);
_Noreturn void	cmderror(Cmdbuf*, char*);
int		cmount(Chan*, Chan*, int, char*);
void		confinit(void);
int		consactive(void);
extern void	(*consdebug)(void);

/* ... rest of the original content ... */

#endif /* _FNS_H_ */