/* Minimal libc compatibility for Plan 9 code */
#ifndef _LIBC_H_
#define _LIBC_H_

#include <stdint.h>
#include <stddef.h>

typedef uint8_t		uchar;
typedef uint16_t	ushort;
typedef uint32_t	uint;
typedef uint32_t	u32int;
typedef uint64_t	uvlong;
typedef int64_t		vlong;

#define nil NULL

typedef struct Fmt Fmt;
typedef struct Qid Qid;
typedef struct Dir Dir;

struct Qid {
	uvlong	path;
	u32int	vers;
	uchar	type;
};

struct Dir {
	/* system-modified data */
	ushort	type;	/* server type */
	uint	dev;	/* server subtype */
	/* file data */
	Qid	qid;	/* unique id from server */
	u32int	mode;	/* permissions */
	u32int	atime;	/* last read time */
	u32int	mtime;	/* last write time */
	vlong	length;	/* file length */
	char	*name;	/* last element of path */
	char	*uid;	/* owner name */
	char	*gid;	/* group name */
	char	*muid;	/* last modifier name */
};

/* Memory functions */
void*	malloc(size_t);
void*	realloc(void*, size_t);
void	free(void*);
void*	memset(void*, int, size_t);
void*	memmove(void*, const void*, size_t);
int	memcmp(const void*, const void*, size_t);

/* String functions */
size_t	strlen(const char*);
char*	strcpy(char*, const char*);
char*	strncpy(char*, const char*, size_t);
int	strcmp(const char*, const char*);
int	strncmp(const char*, const char*, size_t);
char*	strdup(const char*);
char*	strcat(char*, const char*);

#endif
