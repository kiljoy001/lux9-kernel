/*
 * Kernel compatibility stubs for GCC compilation
 * Provides minimal implementations of Plan 9 kernel functions
 */

#define _POSIX_C_SOURCE 199309L
#include "gcc_compat.h"
#include <time.h>

/* Define the mach structure */
Mach mach = {0};

/* Basic printing functions */
int print(const char *fmt, ...) {
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}

int iprint(const char *fmt, ...) {
    va_list args;
    int ret;
    va_start(args, fmt);
    ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}

void panic(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "PANIC: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

/* Memory management */
void* xspanalloc(ulong size, int align, ulong) {
    /* Simple malloc for now - in real kernel this would be page-aligned */
    (void)align;
    return calloc(1, size);
}

/* Timing and delays - simplified for compilation */
void delay(int ms) {
    (void)ms;
    /* No-op for now */
}

void microdelay(int us) {
    (void)us;
    /* No-op for now */
}

void esleep(int ms) {
    (void)ms;
    /* No-op for now */
}

/* Process and scheduling */
int waserror(void) {
    return 0; /* Always succeed for now */
}

void poperror(void) {
    /* No-op */
}

void tsleep(void *r, int (*f)(void*), void *arg, long ms) {
    /* Simple sleep - in real kernel this would be proper scheduling */
    (void)r; (void)f; (void)arg; (void)ms;
    /* No-op for now */
}

void* getcallerpc(void *arg) {
    (void)arg;
    return nil; /* Return nil for now */
}

/* Locking */
void ilock(void *lock) {
    (void)lock;
    /* No-op for compilation */
}

void iunlock(void *lock) {
    (void)lock;
    /* No-op for compilation */
}

void qlock(void *qlock) {
    (void)qlock;
    /* No-op for compilation */
}

void qunlock(void *qlock) {
    (void)qlock;
    /* No-op for compilation */
}

void wakeup(void *rendez) {
    (void)rendez;
    /* No-op for compilation */
}

/* String functions */
char* seprint(char *buf, char *e, const char *fmt, ...) {
    va_list args;
    int len = e - buf;
    if (len <= 0) return buf;
    
    va_start(args, fmt);
    int ret = vsnprintf(buf, len, fmt, args);
    va_end(args);
    
    if (ret < 0) return buf;
    if (ret >= len) return buf + len - 1;
    return buf + ret;
}

int snprint(char *buf, int len, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, len, fmt, args);
    va_end(args);
    return ret;
}

/* Configuration */
char* getconf(char *name) {
    (void)name;
    return nil; /* Return nil for now */
}

/* PCI functions */
void pcienable(void *pcidev) {
    (void)pcidev;
    /* No-op for compilation */
}

void pcisetbme(void *pcidev) {
    (void)pcidev;
    /* No-op for compilation */
}

int pcicfgr16(void *pcidev, int offset) {
    (void)pcidev; (void)offset;
    return 0; /* Return 0 for now */
}

void pcicfgw16(void *pcidev, int offset, int value) {
    (void)pcidev; (void)offset; (void)value;
    /* No-op for compilation */
}

int pcicfgr8(void *pcidev, int offset) {
    (void)pcidev; (void)offset;
    return 0; /* Return 0 for now */
}

void pcicfgw8(void *pcidev, int offset, int value) {
    (void)pcidev; (void)offset; (void)value;
    /* No-op for compilation */
}

/* Memory mapping */
void* vmap(uvlong pa, ulong size) {
    (void)pa;
    return calloc(1, size); /* Simple malloc for now */
}

void vunmap(void *va, ulong size) {
    (void)size;
    free(va); /* Simple free for now */
}

/* SD (storage device) functions */
void sdadddevs(void *sdev) {
    (void)sdev;
    /* No-op for compilation */
}

void sdaddfile(void *unit, char *name, int perm, char *user, 
               long (*read)(void*, void*, long, vlong),
               long (*write)(void*, void*, long, vlong)) {
    (void)unit; (void)name; (void)perm; (void)user; (void)read; (void)write;
    /* No-op for compilation */
}

int sdsetsense(void *req, int status, int key, int asc, int ascq) {
    (void)req; (void)key; (void)asc; (void)ascq;
    return status; /* Return status for now */
}

/* SCSI functions */
int scsiverify(void *unit) {
    (void)unit;
    return 0; /* Success for now */
}

int scsionline(void *unit) {
    (void)unit;
    return 1; /* Online for now */
}

int scsibio(void *unit, int lun, int write, void *data, long count, uvlong lba) {
    (void)unit; (void)lun; (void)write; (void)data; (void)lba;
    return count; /* Return count for now */
}

/* Error handling */
void error(char *err) {
    fprintf(stderr, "ERROR: %s\n", err);
    exit(1);
}

/* Coherence function */
void (*coherence)(void) = NULL;

/* FIS functions */
void setfissig(Sfis *sfis, uint sig) {
    (void)sfis; (void)sig;
    /* No-op for compilation */
}

int txmodefis(Sfis *sfis, uchar *fis, uchar udma) {
    (void)sfis; (void)fis; (void)udma;
    return 0; /* Success for now */
}

int atapirwfis(Sfis *sfis, uchar *fis, uchar *data, int write, int count) {
    (void)sfis; (void)fis; (void)data; (void)write; (void)count;
    return count; /* Return count for now */
}

int featfis(Sfis *sfis, uchar *fis, uchar feat) {
    (void)sfis; (void)fis; (void)feat;
    return 0; /* Success for now */
}

int flushcachefis(Sfis *sfis, uchar *fis) {
    (void)sfis; (void)fis;
    return 0; /* Success for now */
}

int identifyfis(Sfis *sfis, uchar *fis) {
    (void)sfis; (void)fis;
    return 0; /* Success for now */
}

int nopfis(Sfis *sfis, uchar *fis, int interrupt) {
    (void)sfis; (void)fis; (void)interrupt;
    return 0; /* Success for now */
}

int rwfis(Sfis *sfis, uchar *fis, int write, int count, uvlong lba) {
    (void)sfis; (void)fis; (void)write; (void)count; (void)lba;
    return count; /* Return count for now */
}

void skelfis(uchar *fis) {
    (void)fis;
    /* No-op for compilation */
}

void sigtofis(Sfis *sfis, uchar *fis) {
    (void)sfis; (void)fis;
    /* No-op for compilation */
}

uvlong fisrw(Sfis *sfis, uchar *fis, int *count) {
    (void)sfis; (void)fis; (void)count;
    return 0; /* Return 0 for now */
}

void idmove(char *dst, ushort *src, int len) {
    (void)dst; (void)src; (void)len;
    /* No-op for compilation */
}

vlong idfeat(Sfis *sfis, ushort *data) {
    (void)sfis; (void)data;
    return 0; /* Return 0 for now */
}

uvlong idwwn(Sfis *sfis, ushort *data) {
    (void)sfis; (void)data;
    return 0; /* Return 0 for now */
}

int idss(Sfis *sfis, ushort *data) {
    (void)sfis; (void)data;
    return 512; /* Return standard sector size for now */
}

int idpuis(ushort *data) {
    (void)data;
    return 0; /* Return 0 for now */
}

ushort id16(ushort *data, int offset) {
    (void)data; (void)offset;
    return 0; /* Return 0 for now */
}

uint id32(ushort *data, int offset) {
    (void)data; (void)offset;
    return 0; /* Return 0 for now */
}

uvlong id64(ushort *data, int offset) {
    (void)data; (void)offset;
    return 0; /* Return 0 for now */
}

char *pflag(char *buf, char *ebuf, Sfis *sfis) {
    (void)sfis;
    if (buf < ebuf) *buf = '\0';
    return buf; /* Return buffer for now */
}

uint fistosig(uchar *fis) {
    (void)fis;
    return 0; /* Return 0 for now */
}

/* Error constants */
char Enoerror[] = "no error";
char Emount[] = "inconsistent mount";
char Eunmount[] = "not mounted";
char Eismtpt[] = "is a mount point";
char Eunion[] = "not in union";
char Emountrpc[] = "mount rpc error";
char Eshutdown[] = "device shut down";
char Enocreate[] = "mounted directory forbids creation";
char Enonexist[] = "file does not exist";
char Eexist[] = "file already exists";
char Ebadsharp[] = "unknown device in # filename";
char Enotdir[] = "not a directory";
char Eisdir[] = "file is a directory";
char Ebadchar[] = "bad character in file name";
char Efilename[] = "file name syntax";
char Eperm[] = "permission denied";
char Ebadusefd[] = "inappropriate use of fd";
char Ebadarg[] = "bad arg in system call";
char Einuse[] = "device or object already in use";
char Eio[] = "i/o error";
char Etoobig[] = "read or write too large";
char Etoosmall[] = "read or write too small";
char Enoport[] = "network port not available";
char Ehungup[] = "i/o on hungup channel";
char Ebadctl[] = "bad process or channel control request";
char Enodev[] = "no free devices";
char Eprocdied[] = "process exited";
char Enochild[] = "no living children";
char Eioload[] = "i/o error in demand load";
char Enovmem[] = "virtual memory allocation failed";
char Ebadfd[] = "fd out of range or not open";
char Enofd[] = "no free file descriptors";
char Eisstream[] = "seek on a stream";
char Ebadexec[] = "exec header invalid";
char Etimedout[] = "connection timed out";
char Econrefused[] = "connection refused";
char Econinuse[] = "connection in use";
char Eintr[] = "interrupted";
char Enomem[] = "kernel allocate failed";
char Esoverlap[] = "segments overlap";
char Emouseset[] = "mouse type already set";
char Eshort[] = "i/o count too small";
char Egreg[] = "the front fell off";
char Ebadspec[] = "bad attach specifier";
char Enoreg[] = "process has no saved registers";
char Enoattach[] = "mount/attach disallowed";
char Eshortstat[] = "stat buffer too small";
char Ebadstat[] = "malformed stat buffer";
char Enegoff[] = "negative i/o offset";
char Ecmdargs[] = "wrong #args in control message";
char Ebadip[] = "bad ip address syntax";
char Edirseek[] = "seek in directory";
char Etoolong[] = "name too long";
char Echange[] = "media or partition has changed";
