#ifndef LUX_H
#define LUX_H

/* Basic types - matching kernel conventions */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned long ulong;
typedef long long vlong;
typedef unsigned long long uvlong;
typedef unsigned long usize;
typedef long isize;
typedef void* uintptr;

/* Syscall numbers - from kernel/include/sys.h */
#define SYS_SYSR1 0
#define SYS_ERRSTR 1
#define SYS_BIND 2
#define SYS_CHDIR 3
#define SYS_CLOSE 4
#define SYS_DUP 5
#define SYS_ALARM 6
#define SYS_EXEC 7
#define SYS_EXITS 8
#define SYS_OPEN 14
#define SYS_READ 15
#define SYS_SLEEP 17
#define SYS_RFORK 19
#define SYS_WRITE 20
#define SYS_PIPE 21
#define SYS_CREATE 22
#define SYS_BRK 24
#define SYS_MOUNT 53
#define SYS_AWAIT 54

/* File mode flags */
#define OREAD   0
#define OWRITE  1
#define ORDWR   2
#define OEXEC   3
#define OTRUNC  0x0010
#define OCEXEC  0x0020
#define ORCLOSE 0x0040

/* Mount flags */
#define MREPL   0
#define MBEFORE 1
#define MAFTER  2
#define MCREATE 4

/* Rfork flags */
#define RFNAMEG  (1<<0)
#define RFENVG   (1<<1)
#define RFFDG    (1<<2)
#define RFNOTEG  (1<<3)
#define RFPROC   (1<<4)
#define RFMEM    (1<<5)
#define RFNOWAIT (1<<6)

/* System call interface - implemented in syscall.S */
extern long _syscall(long num, ...);

/* Core I/O functions */
int open(const char *path, int mode);
int close(int fd);
long read(int fd, void *buf, long n);
long write(int fd, const void *buf, long n);
int bind(const char *name, const char *old, int flag);
int mount(int fd, int afd, const char *old, int flag, const char *aname);
long brk(void *addr);
void exits(const char *msg);

/* Process control */
int rfork(int flags);
int await(char *status, int nstatus);

/* Utility functions */
void *memset(void *s, int c, ulong n);
void *memcpy(void *dest, const void *src, ulong n);
ulong strlen(const char *s);
char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);

/* Minimal print support - writes to stderr (fd 2) */
int print(const char *fmt, ...);

#endif /* LUX_H */
