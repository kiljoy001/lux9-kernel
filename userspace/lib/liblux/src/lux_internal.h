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

/* Packing Macros */
#define GBIT8(p) (((uchar *)(p))[0])
#define GBIT16(p) (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8))
#define GBIT32(p)                                                              \
  (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) |  \
   (((uchar *)(p))[3] << 24))
#define GBIT64(p)                                                              \
  ((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) |                     \
            (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) |           \
   ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) |                    \
             (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24))            \
    << 32))

#define PBIT8(p, v)                                                            \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
  } while (0)
#define PBIT16(p, v)                                                           \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
    ((uchar *)(p))[1] = (uchar)((v) >> 8);                                     \
  } while (0)
#define PBIT32(p, v)                                                           \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
    ((uchar *)(p))[1] = (uchar)((v) >> 8);                                     \
    ((uchar *)(p))[2] = (uchar)((v) >> 16);                                    \
    ((uchar *)(p))[3] = (uchar)((v) >> 24);                                    \
  } while (0)
#define PBIT64(p, v)                                                           \
  do {                                                                         \
    ((uchar *)(p))[0] = (uchar)(v);                                            \
    ((uchar *)(p))[1] = (uchar)((v) >> 8);                                     \
    ((uchar *)(p))[2] = (uchar)((v) >> 16);                                    \
    ((uchar *)(p))[3] = (uchar)((v) >> 24);                                    \
    ((uchar *)(p))[4] = (uchar)((v) >> 32);                                    \
    ((uchar *)(p))[5] = (uchar)((v) >> 40);                                    \
    ((uchar *)(p))[6] = (uchar)((v) >> 48);                                    \
    ((uchar *)(p))[7] = (uchar)((v) >> 56);                                    \
  } while (0)

/* Fixed length sizes of 9P fields */
#define BIT8SZ 1
#define BIT16SZ 2
#define BIT32SZ 4
#define BIT64SZ 8
#define QIDSZ (BIT8SZ + BIT32SZ + BIT64SZ)

#endif
