#ifndef _LUX_INTERNAL_H_
#define _LUX_INTERNAL_H_

#include "../inc/lux.h"

/* Fixed address from 9p_router.h */
#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_MSG_OFFSET 0x000
#define P9_MSG_SIZE 0xF00
#define P9_CONTROL_OFFSET 0xF00

/* Assembly doorbell trigger */
long _syscall(void);

#endif
