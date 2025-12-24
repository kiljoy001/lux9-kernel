/**
 * Frama-C Missing Types for Plan 9
 *
 * This header defines ONLY types that are incomplete or missing
 * after Plan 9 preprocessing. Does NOT redefine existing types.
 */

#ifndef FRAMAC_MISSING_TYPES_H
#define FRAMAC_MISSING_TYPES_H

// Fmt structure - often incomplete in preprocessed output
#ifndef _FMT_DEFINED
#define _FMT_DEFINED

typedef struct Fmt Fmt;
typedef int (*Fmts)(Fmt *);

struct Fmt {
    unsigned char runes;
    void *start;
    void *to;
    void *stop;
    int (*flush)(Fmt *);
    void *farg;
    int nfmt;
    void *args;
    int r;
    int width;
    int prec;
    unsigned long flags;
};

#endif /* _FMT_DEFINED */

// Incomplete types that may be missing
struct Waitmsg;
struct Mnt;
struct Mount;
struct Mhead;
struct Mntrpc;
struct Mntcache;
struct Dev;
struct Cmdbuf;
struct Cmdtab;

#endif /* FRAMAC_MISSING_TYPES_H */
