/*
 * GCC compatibility header for AHCI driver
 * Provides standard C definitions and Plan 9 compatibility
 */

#ifndef _GCC_COMPAT_H_
#define _GCC_COMPAT_H_

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

/* Plan 9 style types using standard C types */
typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t uint;
typedef uint64_t ulong;
typedef int64_t vlong;
typedef uint64_t uvlong;

/* Plan 9 fixed-width types (already defined in stdint.h) */
typedef uint8_t u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef uint64_t u64int;
typedef int8_t s8int;
typedef int16_t s16int;
typedef int32_t s32int;
typedef int64_t s64int;

typedef u32int Rune;

/* Plan 9 macros */
#define nelem(x) (sizeof(x)/sizeof((x)[0]))
#define offsetof(s, m) (uintptr_t)(&(((s*)0)->m))
#define nil ((void*)0)

/* USED macro to suppress unused warnings */
#define USED(...) if(__VA_ARGS__){}

/* HZ definition for timing */
#define HZ 100

/* MACHP macro */
#define MACHP(n) (&mach)

/* Simple mach structure for timing */
typedef struct {
    ulong ticks;
} Mach;

extern Mach mach;

/* Forward declarations for structures */
struct Lock;
struct QLock;
struct Rendez;
struct SDev;
struct SDunit;
struct SDreq;
struct Sfis;
struct Afis;
struct Alist;
struct Actab;
struct Aport;
struct Ahba;
struct Aportm;
struct Aportc;
struct Aenc;
struct Pcidev;
struct Ureg;

/* Locking structures */
typedef struct Lock Lock;
typedef struct QLock QLock;
typedef struct Rendez Rendez;

struct Lock {
    ulong key;
    ulong sr;
    uintptr_t pc;
    void *p;
    void *m;
    ushort isilock;
    long lockcycles;
};

struct QLock {
    Lock use;
    void *head;
    void *tail;
    uintptr_t pc;
    int locked;
};

struct Rendez {
    Lock lock;
    void *p;
};

/* PCI structures */
typedef struct Pcidev Pcidev;
struct Pcidev {
    int tbdf;
    ushort vid;
    ushort did;
    ushort pcr;
    uchar rid;
    uchar ccrp;
    uchar ccru;
    uchar ccrb;
    uchar cls;
    uchar ltr;
    uchar intl;
    struct {
        uvlong bar;
        vlong size;
    } mem[6];
    struct {
        uvlong bar;
        vlong size;
    } rom;
    struct {
        uvlong bar;
        vlong size;
    } ioa, mema;
    struct {
        uvlong bar;
        vlong size;
    } prefa;
    Pcidev *list;
    Pcidev *link;
    Pcidev *parent;
    Pcidev *bridge;
    int pmrb;
    int msi;
};

/* Ureg structure */
typedef struct Ureg Ureg;
struct Ureg {
    u64int ax;
    u64int bx;
    u64int cx;
    u64int dx;
    u64int si;
    u64int di;
    u64int bp;
    u64int r8;
    u64int r9;
    u64int r10;
    u64int r11;
    u64int r12;
    u64int r13;
    u64int r14;
    u64int r15;
    u16int ds;
    u16int es;
    u16int fs;
    u16int gs;
    u64int type;
    u64int error;
    u64int pc;
    u64int cs;
    u64int flags;
    u64int sp;
    u64int ss;
};

/* SD (Storage Device) structures */
typedef struct SDev SDev;
typedef struct SDunit SDunit;
typedef struct SDreq SDreq;
typedef struct SDifc SDifc;

struct SDreq {
    SDunit *unit;
    int lun;
    char write;
    char proto;
    char ataproto;
    uchar cmd[0x20];
    int clen;
    void *data;
    int dlen;
    int flags;
    int status;
    long rlen;
    uchar sense[32];
};

struct SDunit {
    SDev *dev;
    int subno;
    uchar inquiry[255];
    uchar sense[18];
    uchar rsense[18];
    uchar haversense;
    char *name;
    char *user;
    ulong perm;
    QLock ctl;
    uvlong sectors;
    ulong secsize;
    void *part;
    int npart;
    ulong vers;
    void *ctlperm;
    QLock raw;
    ulong rawinuse;
    int state;
    SDreq *req;
    void *rawperm;
    void *efile[5];
    int nefile;
};

struct SDev {
    void *r;
    SDifc *ifc;
    void *ctlr;
    int idno;
    char name[8];
    SDev *next;
    QLock qlock;
    int enabled;
    int nunit;
    QLock unitlock;
    char *unitflg;
    SDunit **unit;
};

struct SDifc {
    char *name;
    SDev *(*pnp)(void);
    int (*enable)(SDev *);
    int (*disable)(SDev *);
    int (*verify)(SDunit *);
    int (*online)(SDunit *);
    int (*rio)(SDreq *);
    char *(*rctl)(SDunit *, char *, char *);
    int (*wctl)(SDunit *, void *);
    long (*bio)(SDunit *, int, int, void *, long, uvlong);
    SDev *(*probe)(void *);
    void (*clear)(SDev *);
    char *(*rtopctl)(SDev *, char *, char *);
    int (*wtopctl)(SDev *, void *);
    int (*ataio)(SDreq *);
};

/* FIS structures */
typedef struct Sfis Sfis;
struct Sfis {
    ushort feat;
    uchar udma;
    uchar speeds;
    uint sig;
    uint lsectsz;
    uint physshift;
    uint physalign;
    uint c;
    uint h;
    uint s;
};

/* AHCI structures */
typedef struct Afis Afis;
typedef struct Alist Alist;
typedef struct Actab Actab;
typedef struct Aport Aport;
typedef struct Ahba Ahba;
typedef struct Aportm Aportm;
typedef struct Aportc Aportc;
typedef struct Aenc Aenc;

struct Afis {
    uchar *base;
    uchar *d;
    uchar *p;
    uchar *r;
    uchar *u;
    ulong *devicebits;
};

struct Alist {
    ulong flags;
    ulong len;
    ulong ctab;
    ulong ctabhi;
    uchar reserved[16];
};

struct Actab {
    uchar cfis[0x40];
    uchar atapi[0x10];
    uchar pad[0x30];
    struct {
        ulong dba;
        ulong dbahi;
        ulong pad;
        ulong count;
    } prdt;
};

struct Aport {
    ulong list;
    ulong listhi;
    ulong fis;
    ulong fishi;
    ulong isr;
    ulong ie;
    ulong cmd;
    ulong res1;
    ulong task;
    ulong sig;
    ulong scr0;
    ulong scr2;
    ulong scr1;
    ulong scr3;
    ulong ci;
    ulong scr4;
    ulong fbs;
    ulong res2[11];
    ulong vendor[4];
};

struct Ahba {
    ulong cap;
    ulong ghc;
    ulong isr;
    ulong pi;
    ulong ver;
    ulong ccc;
    ulong cccports;
    ulong emloc;
    ulong emctl;
    ulong cap2;
    ulong bios;
};

struct Aportm {
    QLock qlock;
    Rendez rendez;
    uchar flag;
    Sfis sfis;
    Afis fis;
    Alist *list;
    Actab *ctab;
    uchar feat;
    uchar udma;
    uchar physshift;
    uchar physalign;
};

struct Aportc {
    Aport *p;
    Aportm *m;
};

struct Aenc {
    uint encsz;
    ulong *enctx;
    ulong *encrx;
};

/* AHCI constants from ahci.h */
enum {
    Abar = 5,
    
    /* cap bits */
    H64a = 1<<31,
    Hncq = 1<<30,
    Hsntf = 1<<29,
    Hmps = 1<<28,
    Hss = 1<<27,
    Halp = 1<<26,
    Hal = 1<<25,
    Hclo = 1<<24,
    Hiss = 1<<20,
    Ham = 1<<18,
    Hpm = 1<<17,
    Hfbs = 1<<16,
    Hpmb = 1<<15,
    Hssc = 1<<14,
    Hpsc = 1<<13,
    Hncs = 1<<8,
    Hcccs = 1<<7,
    Hems = 1<<6,
    Hxs = 1<<5,
    Hnp = 1<<0,
    
    /* ghc bits */
    Hae = 1<<31,
    Hie = 1<<1,
    Hhr = 1<<0,
    
    /* cap2 bits */
    Apts = 1<<2,
    Nvmp = 1<<1,
    Boh = 1<<0,
    
    /* bios bits */
    Bos = 1<<0,
    Oos = 1<<1,
    
    /* emctl bits */
    Pm = 1<<27,
    Alhd = 1<<26,
    Xonly = 1<<25,
    Smb = 1<<24,
    Esgpio = 1<<19,
    Eses2 = 1<<18,
    Esafte = 1<<17,
    Elmt = 1<<16,
    Emrst = 1<<9,
    Tmsg = 1<<8,
    Mr = 1<<0,
    Emtype = Esgpio | Eses2 | Esafte | Elmt,
    
    /* interrupt bits */
    Acpds = 1<<31,
    Atfes = 1<<30,
    Ahbfs = 1<<29,
    Ahbds = 1<<28,
    Aifs = 1<<27,
    Ainfs = 1<<26,
    Aofs = 1<<24,
    Aipms = 1<<23,
    Aprcs = 1<<22,
    Adpms = 1<<7,
    Apcs = 1<<6,
    Adps = 1<<5,
    Aufs = 1<<4,
    Asdbs = 1<<3,
    Adss = 1<<2,
    Apio = 1<<1,
    Adhrs = 1<<0,
    
    IEM = Acpds|Atfes|Ahbds|Ahbfs|Ahbds|Aifs|Ainfs|Aprcs|Apcs|Adps|Aufs|Asdbs|Adss|Adhrs,
    Ifatal = Ahbfs|Ahbds|Aifs,
    
    /* serror bits */
    SerrX = 1<<26,
    SerrF = 1<<25,
    SerrT = 1<<24,
    SerrS = 1<<23,
    SerrH = 1<<22,
    SerrC = 1<<21,
    SerrD = 1<<20,
    SerrB = 1<<19,
    SerrW = 1<<18,
    SerrI = 1<<17,
    SerrN = 1<<16,
    ErrE = 1<<11,
    ErrP = 1<<10,
    ErrC = 1<<9,
    ErrT = 1<<8,
    ErrM = 1<<1,
    ErrI = 1<<0,
    ErrAll = ErrE|ErrP|ErrC|ErrT|ErrM|ErrI,
    SerrAll = SerrX|SerrF|SerrT|SerrS|SerrH|SerrC|SerrD|SerrB|SerrW|SerrI|SerrN|ErrAll,
    SerrBad = 0x7f<<19,
    
    /* cmd register bits */
    Aicc = 1<<28,
    Aasp = 1<<27,
    Aalpe = 1<<26,
    Adlae = 1<<25,
    Aatapi = 1<<24,
    Apste = 1<<23,
    Afbsc = 1<<22,
    Aesp = 1<<21,
    Acpd = 1<<20,
    Ampsp = 1<<19,
    Ahpcp = 1<<18,
    Apma = 1<<17,
    Acps = 1<<16,
    Acr = 1<<15,
    Afr = 1<<14,
    Ampss = 1<<13,
    Accs = 1<<8,
    Afre = 1<<4,
    Aclo = 1<<3,
    Apod = 1<<2,
    Asud = 1<<1,
    Ast = 1<<0,
    
    Arun = Ast|Acr|Afre|Afr,
    Apwr = Apod|Asud,
    
    /* ctl register bits */
    Aipm = 1<<8,
    Aspd = 1<<4,
    Adet = 1<<0,
    
    /* sstatus register bits */
    Smissing = 0<<0,
    Spresent = 1<<0,
    Sphylink = 3<<0,
    Sbist = 4<<0,
    Smask = 7<<0,
    
    Gmissing = 0<<4,
    Gi = 1<<4,
    Gii = 2<<4,
    Giii = 3<<4,
    Gmask = 7<<4,
    
    Imissing = 0<<8,
    Iactive = 1<<8,
    Isleepy = 2<<8,
    Islumber = 6<<8,
    Imask = 7<<8,
    
    SImask = Smask | Imask,
    SSmask = Smask | Isleepy,
    
    /* FIS constants */
    Lprdtl = 1<<16,
    Lpmp = 1<<12,
    Lclear = 1<<10,
    Lbist = 1<<9,
    Lreset = 1<<8,
    Lpref = 1<<7,
    Lwrite = 1<<6,
    Latapi = 1<<5,
    Lcfl = 1<<0,
    
    /* Error constants */
    Emed = 1<<0,
    Enm = 1<<1,
    Eabrt = 1<<2,
    Emcr = 1<<3,
    Eidnf = 1<<4,
    Emc = 1<<5,
    Eunc = 1<<6,
    Ewp = 1<<6,
    Eicrc = 1<<7,
    Efatal = Eidnf|Eicrc,
    
    /* Status constants */
    ASerr = 1<<0,
    ASdrq = 1<<3,
    ASdf = 1<<5,
    ASdrdy = 1<<6,
    ASbsy = 1<<7,
    ASobs = 1<<1|1<<2|1<<4,
    
    /* FIS types */
    H2dev = 0x27,
    D2host = 0x34,
    Fiscmd = 0x80,
    Ataobs = 0xa0,
    Atalba = 0x40,
    Fissize = 0x20,
    
    /* FIS layout */
    Ftype = 0,
    Fflags = 1,
    Fcmd = 2,
    Ffeat = 3,
    Flba0 = 4,
    Flba8 = 5,
    Flba16 = 6,
    Fdev = 7,
    Flba24 = 8,
    Flba32 = 9,
    Flba40 = 10,
    Ffeat8 = 11,
    Fsc = 12,
    Fsc8 = 13,
    Ficc = 14,
    Fcontrol = 15,
    
    Fioport = 1,
    Fstatus = 2,
    Frerror = 3,
    
    /* Protocol types */
    Pnd = 0<<0,
    Pin = 1<<0,
    Pout = 2<<0,
    Pdatam = 3<<0,
    Ppio = 1<<2,
    Pdma = 2<<2,
    Pdmq = 3<<2,
    Preset = 4<<2,
    Pdiag = 5<<2,
    Ppkt = 6<<2,
    Pprotom = 7<<2,
    P48 = 0<<5,
    P28 = 1<<5,
    Pcmdszm = 1<<5,
    Pssn = 0<<6,
    P512 = 1<<6,
    Pssm = 1<<6,
    
    /* Feature flags */
    Dlba = 1<<0,
    Dllba = 1<<1,
    Dsmart = 1<<2,
    Dpower = 1<<3,
    Dnop = 1<<4,
    Datapi = 1<<5,
    Datapi16 = 1<<6,
    Data8 = 1<<7,
    Dsct = 1<<8,
    Dnflag = 9,
    
    Pspinup = 1<<0,
    Pidready = 1<<1,
    
    /* Aenc constants */
    Aled = 1<<0,
    Locled = 1<<3,
    Errled = 1<<6,
    Ledoff = 0,
    Ledon = 1,
    
    /* Aportm flags */
    Ferror = 1,
    Fdone = 2,
};

#define sstatus scr0
#define sctl scr2
#define serror scr1
#define sactive scr3
#define ntf scr4

/* Error constants */
extern char Enoerror[];
extern char Emount[];
extern char Eunmount[];
extern char Eismtpt[];
extern char Eunion[];
extern char Emountrpc[];
extern char Eshutdown[];
extern char Enocreate[];
extern char Enonexist[];
extern char Eexist[];
extern char Ebadsharp[];
extern char Enotdir[];
extern char Eisdir[];
extern char Ebadchar[];
extern char Efilename[];
extern char Eperm[];
extern char Ebadusefd[];
extern char Ebadarg[];
extern char Einuse[];
extern char Eio[];
extern char Etoobig[];
extern char Etoosmall[];
extern char Enoport[];
extern char Ehungup[];
extern char Ebadctl[];
extern char Enodev[];
extern char Eprocdied[];
extern char Enochild[];
extern char Eioload[];
extern char Enovmem[];
extern char Ebadfd[];
extern char Enofd[];
extern char Eisstream[];
extern char Ebadexec[];
extern char Etimedout[];
extern char Econrefused[];
extern char Econinuse[];
extern char Eintr[];
extern char Enomem[];
extern char Esoverlap[];
extern char Emouseset[];
extern char Eshort[];
extern char Egreg[];
extern char Ebadspec[];
extern char Enoreg[];
extern char Enoattach[];
extern char Eshortstat[];
extern char Ebadstat[];
extern char Enegoff[];
extern char Ecmdargs[];
extern char Ebadip[];
extern char Edirseek[];
extern char Etoolong[];
extern char Echange[];

/* Plan 9 kernel function stubs for GCC compilation */
#ifdef __GNUC__

/* Basic printing functions */
extern int print(const char *fmt, ...);
extern int iprint(const char *fmt, ...);
extern void panic(const char *fmt, ...);

/* Memory management */
extern void* xspanalloc(ulong size, int align, ulong);
#define PCIWADDR(x) ((uvlong)(uintptr_t)(x))

/* Timing and delays */
extern void delay(int ms);
extern void microdelay(int us);
extern void esleep(int ms);

/* Process and scheduling */
extern int waserror(void);
extern void poperror(void);
extern void tsleep(void *r, int (*f)(void*), void *arg, long ms);
extern void* getcallerpc(void *arg);

/* Locking */
extern void ilock(void *lock);
extern void iunlock(void *lock);
extern void qlock(void *qlock);
extern void qunlock(void *qlock);
extern void wakeup(void *rendez);

/* String functions */
extern char* seprint(char *buf, char *e, const char *fmt, ...);
extern int snprint(char *buf, int len, const char *fmt, ...);
extern int strcmp(const char *s1, const char *s2);
extern char* strcpy(char *s1, const char *s2);
extern size_t strlen(const char *s);

/* Configuration */
extern char* getconf(char *name);

/* PCI functions */
extern void pcienable(void *pcidev);
extern void pcisetbme(void *pcidev);
extern int pcicfgr16(void *pcidev, int offset);
extern void pcicfgw16(void *pcidev, int offset, int value);
extern int pcicfgr8(void *pcidev, int offset);
extern void pcicfgw8(void *pcidev, int offset, int value);

/* Memory mapping */
extern void* vmap(uvlong pa, ulong size);
extern void vunmap(void *va, ulong size);

/* Device management */
extern void* malloc(size_t size);
extern void free(void *ptr);

/* SD (storage device) functions */
extern void sdadddevs(void *sdev);
extern void sdaddfile(void *unit, char *name, int perm, char *user, 
                     long (*read)(void*, void*, long, vlong),
                     long (*write)(void*, void*, long, vlong));
extern int sdsetsense(void *req, int status, int key, int asc, int ascq);

/* SCSI functions */
extern int scsiverify(void *unit);
extern int scsionline(void *unit);
extern int scsibio(void *unit, int lun, int write, void *data, long count, uvlong lba);

/* Error handling */
extern void error(char *err);

/* Function pointer for coherence */
extern void (*coherence)(void);

/* Return0 function for sleep */
static inline int return0(void *a) { (void)a; return 0; }

/* FIS functions */
extern void setfissig(Sfis*, uint);
extern int txmodefis(Sfis*, uchar*, uchar);
extern int atapirwfis(Sfis*, uchar*, uchar*, int, int);
extern int featfis(Sfis*, uchar*, uchar);
extern int flushcachefis(Sfis*, uchar*);
extern int identifyfis(Sfis*, uchar*);
extern int nopfis(Sfis*, uchar*, int);
extern int rwfis(Sfis*, uchar*, int, int, uvlong);
extern void skelfis(uchar*);
extern void sigtofis(Sfis*, uchar*);
extern uvlong fisrw(Sfis*, uchar*, int*);
extern void idmove(char*, ushort*, int);
extern vlong idfeat(Sfis*, ushort*);
extern uvlong idwwn(Sfis*, ushort*);
extern int idss(Sfis*, ushort*);
extern int idpuis(ushort*);
extern ushort id16(ushort*, int);
extern uint id32(ushort*, int);
extern uvlong id64(ushort*, int);
extern char *pflag(char*, char*, Sfis*);
extern uint fistosig(uchar*);

#endif /* __GNUC__ */

#endif /* _GCC_COMPAT_H_ */
