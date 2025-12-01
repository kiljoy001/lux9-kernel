#pragma once
#include <u.h>
#include "syscall.h" // for __syscall declaration

static inline long syscall1(long n, long a1) {
    return __syscall(n, a1, 0, 0, 0, 0, 0);
}

static inline long syscall2(long n, long a1, long a2) {
    return __syscall(n, a1, a2, 0, 0, 0, 0);
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    return __syscall(n, a1, a2, a3, 0, 0, 0);
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4) {
    return __syscall(n, a1, a2, a3, a4, 0, 0);
}

static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    return __syscall(n, a1, a2, a3, a4, a5, 0);
}
