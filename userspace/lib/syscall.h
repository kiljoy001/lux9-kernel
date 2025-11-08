/*
 * Syscall bridge for Lux9 userspace
 * Provides interface between userspace programs and kernel syscalls
 */

#pragma once

#include <stdint.h>
#include <stdarg.h>

/* Syscall numbers - must match kernel definitions */
#define SYS_RFORK        25   /* rfork */
#define SYS_EXEC         14   /* exec */
#define SYS_EXITS        15   /* exits */
#define SYS_WAIT         42   /* wait */
#define SYS_OPEN         20   /* open */
#define SYS_CLOSE        21   /* close */
#define SYS_READ         22   /* read */
#define SYS_WRITE        26   /* write */
#define SYS_MOUNT        50   /* mount */
#define SYS_UNMOUNT      38   /* unmount */

/* Syscall interface functions */
static inline long syscall1(long n, long a1) {
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    __asm__ volatile (
        "syscall"
        : "=r"(rax)
        : "r"(rax), "r"(rdi)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline long syscall2(long n, long a1, long a2) {
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    __asm__ volatile (
        "syscall"
        : "=r"(rax)
        : "r"(rax), "r"(rdi), "r"(rsi)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = a3;
    __asm__ volatile (
        "syscall"
        : "=r"(rax)
        : "r"(rax), "r"(rdi), "r"(rsi), "r"(rdx)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4) {
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = a3;
    register long r10 __asm__("r10") = a4;
    __asm__ volatile (
        "syscall"
        : "=r"(rax)
        : "r"(rax), "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = a3;
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    __asm__ volatile (
        "syscall"
        : "=r"(rax)
        : "r"(rax), "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = a3;
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    register long r9 __asm__("r9") = a6;
    __asm__ volatile (
        "syscall"
        : "=r"(rax)
        : "r"(rax), "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory"
    );
    return rax;
}

/* Error handling */
#define IS_ERR(x) ((unsigned long)(x) >= (unsigned long)-4095)
#define PTR_ERR(x) ((long)(x))
#define ERR_CAST(x) ((void*)(long)(long)(x))