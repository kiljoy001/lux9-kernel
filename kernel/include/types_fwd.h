/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */

#ifndef _TYPES_FWD_H_
#define _TYPES_FWD_H_

/* Process type */
#ifndef _PROC_DEFINED
#define _PROC_DEFINED
typedef struct Proc Proc;
#endif

/* Machine/CPU type */
#ifndef _MACH_DEFINED
#define _MACH_DEFINED
typedef struct Mach Mach;
#endif

/* Label type (context switch state) */
#ifndef _LABEL_DEFINED
#define _LABEL_DEFINED
typedef struct Label Label;
#endif

/* Lock type */
#ifndef _LOCK_DEFINED
#define _LOCK_DEFINED
typedef struct Lock Lock;
#endif

/* Channel type */
#ifndef _CHAN_DEFINED
#define _CHAN_DEFINED
typedef struct Chan Chan;
#endif

/* 9P message type */
#ifndef _FCALL_DEFINED
#define _FCALL_DEFINED
typedef struct Fcall Fcall;
#endif

/* User registers type */
#ifndef _UREG_DEFINED
#define _UREG_DEFINED
typedef struct Ureg Ureg;
#endif

/* Rendezvous type */
#ifndef _RENDEZ_DEFINED
#define _RENDEZ_DEFINED
typedef struct Rendez Rendez;
#endif

/* 9P Qid type */
#ifndef _QID_DEFINED
#define _QID_DEFINED
typedef struct Qid Qid;
#endif

/* 9P Dir type */
#ifndef _DIR_DEFINED
#define _DIR_DEFINED
typedef struct Dir Dir;
#endif

/* Dirtab type */
#ifndef _DIRTAB_DEFINED
#define _DIRTAB_DEFINED
typedef struct Dirtab Dirtab;
#endif

/* Devgen type */
#ifndef _DEVGEN_DEFINED
#define _DEVGEN_DEFINED
typedef struct Chan Chan;
typedef struct Dirtab Dirtab;
typedef struct Dir Dir;
typedef int Devgen(Chan *, char *, Dirtab *, int, int, Dir *);
#endif

/* QLock type */
#ifndef _QLOCK_DEFINED
#define _QLOCK_DEFINED
typedef struct QLock QLock;
#endif

/* RWLock type */
#ifndef _RWLOCK_DEFINED
#define _RWLOCK_DEFINED
typedef struct RWLock RWLock;
#endif

/* Ref record */
#ifndef _REF_DEFINED
#define _REF_DEFINED
typedef struct Ref Ref;
#endif

/* Fruity IR forward decls */
#ifndef _FRUITY_TYPES_DEFINED
#define _FRUITY_TYPES_DEFINED
typedef struct fruity_module fruity_module_t;
typedef struct fruity_function fruity_function_t;
typedef struct fruity_basic_block fruity_basic_block_t;
typedef struct fruity_instruction fruity_instruction_t;
#endif

#endif /* _TYPES_FWD_H_ */
