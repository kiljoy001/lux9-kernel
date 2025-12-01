#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H
#include <u.h>
typedef ssize ssize_t;
// size_t is usually in stddef.h, but if missing:
#ifndef _SIZE_T
#define _SIZE_T
typedef usize size_t;
#endif
#endif
