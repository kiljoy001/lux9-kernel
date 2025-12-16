#ifndef COMPAT_H
#define COMPAT_H
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define u8int uint8_t
#define u16int uint16_t
#define u32int uint32_t
#define u64int uint64_t
#define usize size_t
#define uintptr uintptr_t
#define nil NULL
#define KADDR(x) ((void *)(x))
#define snprint snprintf

#define USED(x) (void)(x)

#endif
