#ifndef _STDINT_H_
#define _STDINT_H_

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;

#define UINT8_MAX  0xFF
#define INT8_MAX   0x7F
#define INT8_MIN   (-INT8_MAX - 1)
#define UINT16_MAX 0xFFFF
#define INT16_MAX  0x7FFF
#define INT16_MIN  (-INT16_MAX - 1)
#define UINT32_MAX 0xFFFFFFFFU
#define INT32_MAX  0x7FFFFFFF
#define INT32_MIN  (-INT32_MAX - 1)
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
#define INT64_MAX  0x7FFFFFFFFFFFFFFFLL
#define INT64_MIN  (-INT64_MAX - 1LL)

#endif
