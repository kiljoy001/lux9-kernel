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

#endif /* _TYPES_FWD_H_ */
