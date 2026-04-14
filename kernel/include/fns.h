#pragma once
#include "dat.h"
#include <stdarg.h>

void _assert(char *);
void accounttime(void);
Timer *addclock0link(void (*)(void), int);
Physseg *addphysseg(Physseg *);
void addbootfile(char *, uchar *, ulong);
void addwatchdog(Watchdog *);
void alarmkproc(void *);
int anyhigher(void);
int anyready(void);
Image *attachimage(Chan *, ulong size);
ulong beswal(ulong);
uvlong beswav(uvlong);
void bootlinks(void);
void cachedel(Image *, uintptr);
void cachepage(Page *, Image *);
void callwithureg(void (*)(Ureg *));
char *chanpath(Chan *);
/*@ requires \valid(l);
  @ terminates \true;
  @ assigns \nothing;
  @*/
int canlock(Lock *l);
int canpage(Proc *);
/*@ requires \valid(q);
  @ terminates \true;
  @ assigns \nothing;
  @*/
int canqlock(QLock *q);
int cmpswap486(long *, long, long);
#define cmpswap(addr, old, new)                                                \
  cmpswap486((long *)(addr), (long)(old), (long)(new))
int canrlock(RWLock *);
void chandevinit(void);
void chandevreset(void);
void chandevshutdown(void);
void chanfree(Chan *);
void checkalarms(void);
void checkpages(void);
void cinit(void);
Chan *cclone(Chan *);
void cclose(Chan *);
Chan *cunique(Chan *);
void closeegrp(Egrp *);
void closefgrp(Fgrp *);
void closepgrp(Pgrp *);
void closergrp(Rgrp *);
long clrfpintr(void);
_Noreturn void cmderror(Cmdbuf *, char *);
int cmount(Chan *, Chan *, int, char *);
void confinit(void);
int consactive(void);
extern void (*consdebug)(void);
void cpushutdown(void);
int copen(Chan *);
void cclunk(Chan *);
void copypage(Page *, Page *);
void countpagerefs(ulong *, int);
int cread(Chan *, uchar *, int, vlong);
void ctrunc(Chan *);
void cunmount(Chan *, Chan *);
void cupdate(Chan *, uchar *, int, vlong);
void cwrite(Chan *, uchar *, int, vlong);
uintptr dbgpc(Proc *);
Page *deadpage(Page *);
/*@ requires \valid(r);
  @ terminates \true;
  @ assigns r->ref;
  @ ensures r->ref < \old(r->ref);
  @*/
long decref(Ref *r);
int decrypt(void *, void *, int);
void delay(int);
Proc *dequeueproc(Schedq *, Proc *);
int delphysseg(char *);
/*@ assigns \nothing; */
Chan *devattach(int, char *spec);
Chan *devclone(Chan *);
int devconfig(int, char *, DevConf *);
Chan *devcreate(Chan *, char *, int, ulong);
/*@ requires c != \null;
  @ requires name == \null || \valid(name);
  @ requires user == \null || \valid(user);
  @ requires dp != \null;
  @ terminates \true;
  @ assigns *dp;
  */
void devdir(Chan *c, Qid qid, char *name, vlong length, char *user, long perm,
            Dir *dp);
/*@ requires c != \null;
  @ terminates \true;
  @ assigns \nothing;
  */
long devdirread(Chan *c, char *va, long n, Dirtab *tab, int ntab, Devgen *gen);
Devgen devgen;
void devinit(void);
int devno(int, int);
Chan *devopen(Chan *, int, Dirtab *, int, Devgen *);
void devpermcheck(char *, ulong, int);
void devpower(int);
void devremove(Chan *);
void devreset(void);
void devshutdown(void);
/*@ requires c != \null;
  @ terminates \true;
  @ assigns \nothing;
  */
int devstat(Chan *c, uchar *dp, int n, Dirtab *tab, int ntab, Devgen *gen);
/*@ requires c != \null;
  @ terminates \true;
  @ assigns \result \from \nothing;
  */
Walkqid *devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab,
                 int ntab, Devgen *gen);
int devwstat(Chan *, uchar *, int);
Dir *dirchanstat(Chan *);
int donotify(Ureg *);
void syscall_to_9p(Ureg *); /* Phase 6: Pure 9P dispatch replaces dosyscall */
void drawactive(int);
void drawcmap(void);
void dtracytick(Ureg *);
void dumpaproc(Proc *);
void dumpregs(Ureg *);
void dumpstack(void);
Fgrp *dupfgrp(Fgrp *);
void dupswap(Page *);
void edfinit(Proc *);
char *edfadmit(Proc *);
int edfready(Proc *);
void edfrecord(Proc *);
void edfrun(Proc *, int);
void edfstop(Proc *);
void edfyield(void);
int emptystr(char *);
int encrypt(void *, void *, int);
void envcpy(Egrp *, Egrp *);
int eqchan(Chan *, Chan *, int);
int eqchantdqid(Chan *, int, int, Qid, int);
int eqqid(Qid, Qid);
#ifdef __FRAMAC__
/* Frama-C compatible version without attributes */
/*@
  @ requires \valid_read(e);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \false;
  @*/
void lux9_error(char *e);
#define error(e) lux9_error(e)
#else
/*@ requires e != \null;
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \false;
  */
_Noreturn void error(char *e);
#endif

void eqlock(QLock *);
void qlock(QLock *);
void qunlock(QLock *);
uintptr execregs(uintptr, ulong, ulong);
void exhausted(char *);
void exit(int);
uvlong fastticks(uvlong *);
uvlong fastticks2ns(uvlong);
uvlong fastticks2us(uvlong);
int fault(uintptr, uintptr, int);
int fixfault(Segment *, uintptr, int);
void faultnote(char *, char *, uintptr);
void fdclose(int, int);
Chan *fdtochan(int, int, int, int);
int findmount(Chan **, Mhead **, int, int, Qid);
void flushmmu(void);
void forceclosefgrp(void);
void forkchild(Proc *, Ureg *);
void forkret(void);
void fpunotify(Proc *);
void fpunoted(Proc *);
/*@ terminates \true;
  @ assigns \nothing;
  @ behavior null:
  @   assumes p == \null;
  @   assigns \nothing;
  @ behavior valid:
  @   assumes p != \null;
  @   requires \freeable(p);
  @   assigns \nothing;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void free(void *p);
int freebroken(void);
void freenote(Note *);
void freenotes(Proc *);
void freepages(Page *, Page *, ulong);
void getcolor(ulong, ulong *, ulong *, ulong *);
uintptr getmalloctag(void *);
uintptr getrealloctag(void *);
_Noreturn void gotolabel(Label *);
/*@ requires name == \null || valid_string(name);
  @ assigns \result \from name[0 .. ACSL_MAXSTR-1];
  @ ensures \result == \null || valid_string(\result);
  @*/
char *getconf(char *name);
char *getconfenv(void);
long hostdomainwrite(char *, int);
long hostownerwrite(char *, int);
extern void (*hwrandbuf)(void *, ulong);
void hzsched(void);
uintptr ibrk(uintptr, int);
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  @*/
void ilock(Lock *l);
_Noreturn void interrupted(void);
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  @*/
void iunlock(Lock *l);
ulong imagecached(void);
ulong imagereclaim(ulong);
/*@ requires \valid(r);
  @ terminates \true;
  @ assigns r->ref;
  @ ensures r->ref > \old(r->ref);
  @*/
long incref(Ref *r);
void init0(void);
void initseg(void);
/*@
  @ requires name == \null || valid_string(name);
  @ assigns \nothing;
  @*/
int ioalloc(ulong addr, ulong size, ulong align, char *name);
void iofree(ulong);
void iomapinit(ulong);
/*@
  @ requires name == \null || valid_string(name);
  @ assigns \nothing;
  @*/
int ioreserve(ulong addr, ulong size, ulong align, char *name);
/*@
  @ requires name == \null || valid_string(name);
  @ assigns \nothing;
  @*/
int ioreservewin(ulong addr, ulong size, ulong align, ulong win, char *name);
int iounused(ulong, ulong);
/*@
  @ requires valid_string(fmt);
  @ assigns \nothing;
  @*/
int iprint(char *fmt, ...);
/*@
  @ requires valid_string(fmt);
  @ assigns \nothing;
  @*/
int iprint_intr(char *fmt, ...);
void isdir(Chan *);
int iseve(void);
int islo(void);
Segment *isoverlap(uintptr, uintptr);
Physseg *findphysseg(char *);
int kenter(Ureg *);
void kexit(Ureg *);
void kickpager(void);
void killbig(void);
void killproc(Proc *, int);
/*@
  @ requires valid_string(name);
  @ assigns \result;
  @*/
int kproc(char *name, void (*fn)(void *), void *arg);
void kprocchild(Proc *, void (*)(void));
void linkproc(void);
extern void (*kproftimer)(uintptr);
void ksetenv(char *, char *, int);
int kopen(char *, int);
void kstrcpy(char *, char *, int);
void kstrdup(char **, char *);
/*@ requires \valid(l);
  @ terminates \true;
  @ assigns *l;
  @*/
void lock(Lock *l);
void logopen(Log *);
void logclose(Log *);
char *logctl(Log *, int, char **, Logflag *);
void logn(Log *, int, void *, int);
long logread(Log *, void *, ulong, long);
void log(Log *, int, char *, ...);
Cmdtab *lookupcmd(Cmdbuf *, Cmdtab *, int);
Page *lookpage(Image *, uintptr);
#define MS2NS(n) (((vlong)(n)) * 1000000LL)
void machinit(void);
/*@ terminates \true;
  @ assigns \result \from size;
  @ behavior success:
  @   assumes size > 0;
  @   assigns \result \from size;
  @   ensures \result != \null && \valid((char *)\result + (0 .. size - 1));
  @ behavior failure:
  @   assumes size > 0;
  @   assigns \result \from size;
  @   ensures \result == \null;
  @ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from size;
  @   ensures \result == \null;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void *malloc(ulong size);
/*@ terminates \true;
  @ assigns \result \from size;
  @ behavior success:
  @   assumes size > 0;
  @   assigns \result \from size;
  @   ensures \result != \null && \valid((char *)\result + (0 .. size - 1));
  @   ensures \forall integer i; 0 <= i < size ==> ((char *)\result)[i] == 0;
  @*/
void *mallocz(ulong size, int clr);
/*@ terminates \true;
  @ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid((char *)\result);
  @ behavior nonzero:
  @   assumes size > 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid(((char *)\result) + (0 .. (integer)size
  - 1));
  @ complete behaviors;
  @ disjoint behaviors;
  */
void *mallocalign(ulong size, ulong align, long offset, ulong span);
void mallocsummary(void);
void memmapdump(void);
uvlong memmapnext(uvlong, ulong);
uvlong memmapsize(uvlong, uvlong);
void memmapadd(uvlong, uvlong, ulong);
uvlong memmapalloc(uvlong, uvlong, uvlong, ulong);
void memmapfree(uvlong, uvlong, ulong);
void mfreeseg(Segment *, uintptr, ulong);
void microdelay(int);
uvlong mk64fract(uvlong, uvlong);
void mkqid(Qid *, vlong, ulong, int);
void mmurelease(Proc *);
void mmuswitch(Proc *);
Chan *mntattach(Chan *, Chan *, char *, int);
Chan *mntauth(Chan *, char *);
int mntversion(Chan *, char *, int, int);
void mouseresize(void);
void mountfree(Mount *);
ulong ms2tk(ulong);
ulong msize(void *);
ulong ms2tk(ulong);
uvlong ms2fastticks(ulong);
void mul64fract(uvlong *, uvlong, uvlong);
void muxclose(Mnt *);
Chan *namec(char *, int, int, ulong);
_Noreturn void namelenerror(char *, int, char *);
int needpages(void *);
Chan *newchan(void);
Egrp *newegrp(void);
int growfd(Fgrp *, int);
void unlockfgrp(Fgrp *);
int newfd(Chan *, int);
Chan *fdtochan_fgrp(Fgrp *, int, int, int, int);
Mhead *newmhead(Chan *);
Mhead *newmhead(Chan *);
/*@ requires spec != \null;
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == \null || \valid(\result);
  @*/
Mount *newmount(Chan *, int, char *spec);
Image *newimage(ulong);
Page *newpage(uintptr, Segment *);
/*@
  @ requires s.len >= 0;
  @ requires s.data == \null || \valid(s.data + (0..s.len-1));
  @ assigns \nothing;
  @ ensures \valid(\result);
  @*/
Path *newpath(BString s);
Pgrp *newpgrp(void);
Rgrp *newrgrp(void);
Proc *newproc(void);
_Noreturn void nexterror(void);
Ureg *notify(Ureg *, char *);
int noted(Ureg *, Ureg *, int);
FPsave *notefpsave(Proc *);
ulong nkpages(Confmem *);
uvlong ns2fastticks(uvlong);
int okaddr(uintptr, ulong, int);
int openmode(ulong);
void pageinit(void);
ulong pagereclaim(Image *);
_Noreturn void panic(const char *fmt, ...);
Cmdbuf *parsecmd(char *a, int n);
void pathclose(Path *);
ulong perfticks(void);
_Noreturn void pexit(char *, int);
void pgrpcpy(Pgrp *, Pgrp *);
void namespace_cid_update(Pgrp *);
void namespace_cid_update_locked(Pgrp *);
ulong pidalloc(Proc *);
#define waserror() setlabel(&up->errlab[up->nerrlab++])
#define poperror() up->nerrlab--
void portcountpagerefs(ulong *, int);
char *popnote(Ureg *);
/*@ requires s == \null || valid_string(s); */
int postnote(Proc *, int, char *s, int);
void postnotepg(ulong, char *, int);
/*@ requires valid_string(fmt);
  @ assigns \nothing;
  @*/
int pprint(char *fmt, ...);
void preempted(int);
void prflush(void);
void printinit(void);
void setkprintqsize(char *);
void prbuf_init(void);
int prbuf_print(char *, int);
void prbuf_start_consumer(void);
int prbuf_has_data(void);
int prbuf_ready(void);
void prbuf_kprint_open(void);
void prbuf_kprint_close(void);
long prbuf_kprint_read(void *, long);
ulong procalarm(ulong);
void procctl(void);
int procfdprint(Chan *, int, char *, int);
void procflushseg(Segment *);
void procflushpseg(Physseg *);
void procflushothers(void);
int procindex(ulong);
void procinit0(void);
void procinterrupt(Proc *);
ulong procpagecount(Proc *);
void procpriority(Proc *, int, int);
void procsetuser(char *);
Proc *proctab(int);
extern void (*proctrace)(Proc *, int, vlong);
void procwired(Proc *, int);
Pte *ptealloc(void);
int pushnote(Proc *, Note *);
void putimage(Image *);
void putmhead(Mhead *);
void putmmu(uintptr, uintptr, Page *);
void putpage(Page *);
void putseg(Segment *);
void putstrn(char *, int);
void putswap(Page *);
ulong pwait(Waitmsg *);

/* Queue I/O functions (qio.c) */
Queue *qopen(int limit, int msg, void (*kick)(void *), void *arg);
Queue *qbypass(void (*bypass)(void *, Block *), void *arg);
void qfree(Queue *q);
void qclose(Queue *q);
void qreopen(Queue *q);
void qhangup(Queue *q, char *msg);
void qflush(Queue *q);
int qlen(Queue *q);
int qcanread(Queue *q);
int qisclosed(Queue *q);
Block *qbread(Queue *q, int len);
long qread(Queue *q, void *vp, int len);
long qbwrite(Queue *q, Block *b);
int qwrite(Queue *q, void *vp, int len);
int qiwrite(Queue *q, void *vp, int len);
int qproduce(Queue *q, void *vp, int len);
int qconsume(Queue *q, void *vp, int len);
int qpass(Queue *q, Block *b);
int qpassnolim(Queue *q, Block *b);
int qdiscard(Queue *q, int len);
Block *qget(Queue *q);
Block *qremove(Queue *q);
void qputback(Queue *q, Block *b);
Block *qcopy(Queue *q, int len, ulong offset);
int qaddlist(Queue *q, Block *b);

void randominit(void);
ulong randomread(void *, ulong);
long ram9pread(void *, long, vlong);
long ram9pwrite(void *, long, vlong);
ulong ram9psize(void);
void rdb(void);
long readblist(Block *, uchar *, long, ulong);
int readnum(ulong, char *, ulong, ulong, int);
int readstr(ulong, char *, ulong, char *);
void ready(Proc *);
void *realloc(void *v, ulong size);
void rebootcmd(int, char **);
void reboot(void *, void *, ulong);
void relocateseg(Segment *, uintptr);
void renameuser(char *, char *);
void resched(char *);
void resrcwait(char *);
int return0(void *);
void rlock(RWLock *);
long rtctime(void);
void runlock(RWLock *);
Proc *runproc(void);
void sched(void);
_Noreturn void schedinit(void);
extern void (*screenputs)(char *, int);
void *secalloc(ulong);
void secfree(void *);
long seconds(void);
uintptr segattach(int, char *, uintptr, uintptr);
void segclock(uintptr);
long segio(Segio *, Segment *, void *, long, vlong, int);
void segpage(Segment *, Page *);
int setcolor(ulong, ulong, ulong, ulong);
void setkernur(Ureg *, Proc *);
int setlabel(Label *);
void setmalloctag(void *, uintptr);
ulong setnoteid(Proc *, ulong);
void setrealloctag(void *, uintptr);
void setregisters(Ureg *, char *, char *, int);
void setupwatchpts(Proc *, Watchpt *, int);
char *skipslash(char *);
void sleep(Rendez *, int (*)(void *), void *);
/*@
  @ requires size > 0;
  @ assigns \nothing;
  @ ensures \valid((char*)\result + (0 .. size-1));
  @ ensures \fresh(\result, size);
  @*/
void *smalloc(ulong size);
void *pebble_meta_alloc(ulong);
void pebble_meta_free(void *);
int splhi(void);
int spllo(void);
void splx(int);
void splxpc(int);
char *srvname(Chan *);
void srvrenameuser(char *, char *);
void shrrenameuser(char *, char *);
int swapcount(uintptr);
int swapfull(void);
void syscallfmt(ulong syscallno, uintptr pc, ulong *list);
void sysretfmt(ulong syscallno, ulong *list, uintptr ret, uvlong start,
               uvlong stop);
void timeradd(Timer *);
void timerdel(Timer *);
void timersinit(void);
void timerintr(Ureg *, Tval);
void timerset(Tval);
ulong tk2ms(ulong);
#define TK2MS(x) ((x) * (1000 / HZ))
uvlong tod2fastticks(vlong);
vlong todget(vlong *, vlong *);
void todsetfreq(vlong);
void todinit(void);
void todset(vlong, vlong, int);
/*@
  @ requires \valid(r);
  @ terminates \true;
  @ assigns \nothing;
  @*/
void tsleep(Rendez *r, int (*fn)(void *), void *arg, ulong ms);
/*@
  @ requires \valid(r);
  @ terminates \true;
  @ assigns \nothing;
  @*/
void sleep(Rendez *r, int (*fn)(void *), void *arg);
void twakeup(Ureg *, Timer *);
int uartctl(Uart *, char *);
int uartgetc(void);
void uartkick(void *);
void uartmouse(char *, int (*)(Queue *, int), int);
void uartsetmouseputc(char *, int (*)(Queue *, int));
void uartputc(int);
void uartputs(char *, int);
void uartrecv(Uart *, char);
int uartstageoutput(Uart *);
void unbreak(Proc *);
void uncachepage(Page *);
long unionread(Chan *, void *, long);
/*@ requires \valid(l);
  @ terminates \true;
  @ assigns \nothing;
  @*/
void unlock(Lock *l);
uvlong us2fastticks(uvlong);
void userinit(void);
uintptr userpc(void);
long userwrite(char *, int);
void validaddr(uintptr, ulong, int);
/*@
  @ requires aname != \null;
  @ requires valid_string(aname);
  @ assigns \nothing;
  @ ensures p9_name_ok_slash(aname, slashok);
  @*/
void validname(char *aname, int slashok);
/*@
  @ requires aname != \null;
  @ requires valid_string(aname);
  @ assigns \result \from aname[0..];
  @ ensures \result != \null ==> valid_string(\result);
  @ ensures \result != \null ==> p9_name_ok_slash(\result, slashok);
  @*/
char *validnamedup(char *aname, int slashok);
void validstat(uchar *, int);
void *vmemchr(void *, int, ulong);
Proc *wakeup(Rendez *);
int walk(Chan **, char **, int, int, int *);
void wlock(RWLock *);
void wunlock(RWLock *);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc(ulong size);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc_raw(ulong size);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz(ulong size, int zero);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz_raw(ulong size, int zero);

/*@ terminates \true;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc_driver(ulong size);
void *xalloc_resident(ulong size);

/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size, zero;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz_driver(ulong size, int zero);

/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *smalloc_driver(ulong size);
void *smalloc_resident(ulong size);

/*@ terminates \true;
  @ exits \false;
  @ assigns \nothing;
*/
void xfree_driver(void *p);
void xfree_resident(void *p);
/*@ terminates \true;
  @ exits \false;
  @ assigns \nothing;
*/
void xfree(void *p);
void xhole(uintptr, uintptr);
void xinit(void);
int xmerge(void *, void *);
void *xspanalloc(ulong, int, ulong);
void xsummary(void);
void *bootstrap_alloc(ulong size);
void *bootstrap_alloc_aligned(ulong size, ulong alignment);
uintptr get_hhdm_offset(void);
void yield(void);
Page *fillpage(Page *, int);
void zeroprivatepages(void);
Segment *data2txt(Segment *);
Segment *dupseg(Segment **, int, int, Proc *);
Segment *newseg(int, uintptr, ulong);
Segment *seg(Proc *, uintptr, int);
Segment *txt2data(Segment *);
void hnputv(void *, uvlong);
void hnputl(void *, uint);
void hnputs(void *, ushort);
uvlong nhgetv(void *);
uint nhgetl(void *);
ushort nhgets(void *);
/* Frama-C struggles with the UTF-8 symbol here; provide an ASCII alias. */
#ifdef __FRAMAC__
ulong us(void);
#define µs us
#else
ulong µs(void);
#endif

long lcycles(void);
extern void (*cycles)(uvlong *);
void devmask(Pgrp *, int, char *);
int devallowed(Pgrp *, int);
int canmount(Pgrp *);

/* PCI configuration space access function pointers */
extern int (*pcicfgrw8)(int, int, int, int);
extern int (*pcicfgrw16)(int, int, int, int);
extern int (*pcicfgrw32)(int, int, int, int);

#pragma varargck argpos iprint 1
#pragma varargck argpos panic 1
#pragma varargck argpos pprint 1

/* Platform-specific address macros - must be provided by arch */
#ifndef KADDR
extern void *kaddr(uintptr);
#define KADDR(a) kaddr(a)
#endif

#ifndef PADDR
extern uintptr paddr(void *);
#define PADDR(a) paddr((void *)(a))
#endif

#ifndef evenaddr
#define evenaddr(x) /* x86 doesn't care about alignment */
#endif

#ifndef userureg
int userureg(Ureg *);
#endif

KMap *kmap(Page *);
void kunmap(KMap *);

#ifndef kmapinval
#define kmapinval() /* Invalidate kmap cache */
#endif

void setuppagetables(void); /* Setup kernel page tables */

/* Pebble primitives */
void pebbleinit(void);
void pebbleprocinit(Proc *);
void pebble_cleanup(Proc *);

/* Interrupt handling functions */
void intrdisable(int, void (*)(Ureg *, void *), void *, int, char *);
void intrenable(int, void (*)(Ureg *, void *), void *, int, char *);

/* Additional low-level functions */
void idlehands(void);
int tas(ulong *); /* Updated to match architecture-specific declaration */
/* coherence() is declared as function pointer in arch-specific fns.h */
extern void (*coherence)(void);
void SET(void *);
/* addarchfile() is declared in arch-specific fns.h with proper signature */
Dirtab *addarchfile(char *, int, long (*)(Chan *, void *, long, vlong),
                    long (*)(Chan *, void *, long, vlong));
/* I/O port access functions */
ushort ins(int port);              /* Input from I/O port */
void outs(int port, ushort value); /* Output to I/O port */

/* Memory and page ownership functions */
uintptr cankaddr(uintptr); /* Check if address in kernel address space - matches
                              arch signature */
#ifdef __FRAMAC__
int pageown_acquire(Proc *, uintptr, u64int); /* Acquire page ownership */
int pageown_release(Proc *, uintptr);         /* Release page ownership */
#else
enum PageOwnError pageown_acquire(Proc *, uintptr,
                                  u64int);          /* Acquire page ownership */
enum PageOwnError pageown_release(Proc *, uintptr); /* Release page ownership */

void pageown_cleanup_process(Proc *); /* Clean up page ownership for process */

/* Architecture-specific process functions - declarations handled in
 * arch-specific fns.h */
void procsave(Proc *);    /* Save process state */
void procrestore(Proc *); /* Restore process state */
void procsetup(Proc *);   /* Setup process state */
#endif
void procfork(Proc *);             /* Fork process state */
int proc_setup_p9page(Proc *);     /* Setup 9P exchange page */
void proc_teardown_p9page(Proc *); /* Release 9P exchange page */
int proc_setup_p9seg_stub(Proc *); /* Compatibility wrapper for old callers */
uintptr p9_pick_uaddr(Proc *, const UserCapability *);
void *kernel_setup_init_exchange(
    Proc *); /* Kernel boot: setup #X exchange channel for init */

/* MMU and page table functions */
uintptr *mmuwalk(uintptr *, uintptr, int, int); /* Walk page table */
u64int getcr3(void); /* Get CR3 register (page directory base) */
void putcr3(u64int); /* Set CR3 register (page directory base) */

/* Device registry and PCI framework functions */
void devregistry_init(void);
void pci_framework_init(void);
int pci_framework_enumerate(void);

/* Phase 4b: Benchmarking functions */
void benchmark_init(void);
void benchmark_enable(void);
void benchmark_disable(void);
void benchmark_boot_start(void);
void benchmark_boot_stage(int);
void benchmark_boot_end(void);
void benchmark_print_summary(void);
int validate_all(void);
uvlong rdtsc(void);

/* MMU virtual mapping (architecture-specific but commonly used) */
void *vmap(uvlong, vlong);
void vunmap(void *, vlong);

long kread(int, void *, long);
long kwrite(int, void *, long);
vlong kseek(int, vlong, int);

/* WASM runtime functions */
void wasm_runtime_init(void);
void wasm_arena_test(void);
struct Chan;       /* Forward declaration */
struct M3Function; /* Forward declaration for WASM3 */
int wasm_exec_compile(struct Chan *, struct M3Function **);
void wasm_exec_run(struct M3Function *);
void wasm_runtime_cleanup_process(Proc *);

extern int boot_verbose;

/* Bounded print for formal verification */
/*@ requires \valid_read(fmt);
  @ terminates \true;
  @ assigns \nothing;
  @*/
int bprint(const char *fmt, ...);

/*@ requires \valid_read(fmt);
  @ terminates \false;
  @*/
void bpanic(const char *fmt, ...) __attribute__((noreturn));

/* Buffered device I/O stubs (from mntrah_stub.c) */
long devbread(Chan *c, void *buf, long n, vlong off);
long devbwrite(Chan *c, void *buf, long n, vlong off);

/* Queue I/O forward declarations (from qio.c) */
Block *allocb(int size);
Block *iallocb(int size);
void freeb(Block *b);
void freeblist(Block *b);
Block *copyblock(Block *bp, int count);
Block *pullupqueue(Queue *q, int n);
Block *pullupblock(Block *bp, int n);
Block *qremove(Queue *q);
