#pragma once
#include "u.h"

void* malloc(ulong size);
void* mallocz(ulong size, int clear);
void free(void* ptr);
void* xalloc(ulong size);
void* xallocz(ulong size, int clear);
void xfree(void* ptr);
void* bootstrap_alloc(ulong size);

void lock(Lock *l);
void unlock(Lock *l);
void ilock(Lock *l);
void iunlock(Lock *l);
int canlock(Lock *l);

void error(char *s);
void nexterror(void);
void poperror(void);
int waserror(void);

void print(char *fmt, ...);
void panic(char *fmt, ...);
ulong getcallerpc(void *arg);

/* String/Mem functions */
void* memmove(void *dst, const void *src, ulong n);
void* memset(void *dst, int c, ulong n);
int memcmp(const void *s1, const void *s2, ulong n);
int strncmp(const char *s1, const char *s2, ulong n);

/* Time */
uvlong todget(void* a, void* b);
