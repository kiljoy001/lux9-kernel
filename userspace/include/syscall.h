#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stddef.h>
#include <stdarg.h>

typedef long ssize_t;

/* Syscall numbers - must match kernel definitions */
#define	SYSR1		0
#define	_ERRSTR		1
#define	BIND		2
#define	CHDIR		3
#define	CLOSE		4
#define	DUP		5
#define	ALARM		6
#define	EXEC		7
#define	EXITS		8
#define	_FSESSION	9
#define	FAUTH		10
#define	_FSTAT		11
#define	SEGBRK		12
#define	_MOUNT		13
#define	OPEN		14
#define	_READ		15
#define	OSEEK		16
#define	SLEEP		17
#define	_STAT		18
#define	RFORK		19
#define	_WRITE		20
#define	PIPE		21
#define	CREATE		22
#define	FD2PATH		23
#define	BRK_		24
#define	REMOVE		25
#define	_WSTAT		26
#define	_FWSTAT		27
#define	NOTIFY		28
#define	NOTED		29
#define	SEGATTACH	30
#define	SEGDETACH	31
#define	SEGFREE		32
#define	SEGFLUSH	33
#define	RENDEZVOUS	34
#define	UNMOUNT		35
#define	_WAIT		36
#define	SEMACQUIRE	37
#define	SEMRELEASE	38
#define	SEEK		39
#define	FVERSION	40
#define	ERRSTR		41
#define	STAT		42
#define	FSTAT		43
#define	WSTAT		44
#define	FWSTAT		45
#define	MOUNT		46
#define	AWAIT		47
#define	PREAD		50
#define	PWRITE		51
#define	TSEMACQUIRE	52
#define _NSEC		53
#define VMEXCHANGE	54
#define VMLEND_SHARED	55
#define VMLEND_MUT	56
#define VMRETURN	57
#define VMOWNINFO	58
#define PEBBLE_BLACK_ALLOC	59
#define PEBBLE_BLACK_FREE	60
#define PEBBLE_WHITE_ISSUE	61
#define PEBBLE_WHITE_VERIFY	62
#define PEBBLE_RED_COPY	63
#define PEBBLE_BLUE_DISCARD	64

/* Syscall interface */
extern long syscall0(long n);
extern long syscall1(long n, long a1);
extern long syscall2(long n, long a1, long a2);
extern long syscall3(long n, long a1, long a2, long a3);
extern long syscall4(long n, long a1, long a2, long a3, long a4);
extern long syscall5(long n, long a1, long a2, long a3, long a4, long a5);
extern long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);

/* Process management */
extern int fork(void);
extern int exec(const char *path, char *const argv[]);
extern void exit(int status);
extern int wait(int *status);
extern int rfork(int flags);

/* File operations */
extern int open(const char *path, int flags, ...);
extern int close(int fd);
extern ssize_t read(int fd, void *buf, size_t count);
extern ssize_t write(int fd, const void *buf, size_t count);
extern int seek(int fd, long offset, int whence);
extern int pipe(int fd[2]);
extern int dup(int fd);
extern int mount(const char *path, int server_pid, const char *proto);
extern int unmount(const char *path, const char *spec);

/* Memory */
extern void* brk(void *addr);

/* Sleep and timing */
extern void sleep(int seconds);
extern void sleep_ms(int ms);
extern int alarm(int seconds);

/* Strings and I/O */
extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t len, const char *fmt, ...);
extern int puts(const char *s);
extern int putchar(int c);

/* String functions */
extern char* strcpy(char *dest, const char *src);
extern char* strncpy(char *dest, const char *src, size_t n);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern size_t strlen(const char *s);
extern char* strcat(char *dest, const char *src);
extern char* strncat(char *dest, const char *src, size_t n);

/* Memory functions */
extern void* malloc(size_t size);
extern void free(void *ptr);
extern void* calloc(size_t nmemb, size_t size);
extern void* realloc(void *ptr, size_t size);

#endif /* _SYSCALL_H_ */