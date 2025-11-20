uintptr get_hhdm_offset(void);
void*	bootstrap_alloc_aligned(ulong, ulong);
void*	xalloc(ulong);void*	xmerge(void*, void*);void*	xspanalloc(ulong, int, ulong);void	xfree(void*);
void	ilock(Lock*);void	iunlock(Lock*);void	panic(const char*, ...);void	ilock(Lock*);void	iunlock(Lock*);void	tsleep(Rendez*, int, void*);void	wakeup(Rendez*);void	qlock(QLock*);void	qunlock(QLock*);ulong	tk2ms(uvlong);uvlong	ms2tk(ulong);
int	waserror(void);void	poperror(void);void	resrcwait(const char*);void*	mallocz(ulong, int);ulong	msize(void*);int	islo(void);void*	malloc(ulong);void*	mallocalign(ulong, ulong, long, ulong);void	setmalloctag(void*, uintptr);void	setrealloctag(void*, uintptr);
