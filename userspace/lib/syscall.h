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

/**
 * Invoke the kernel syscall specified by `n` with a single argument.
 * 
 * @param n Syscall number to invoke.
 * @param a1 First argument passed to the syscall (placed in `rdi`).
 * @returns The raw return value produced by the syscall (value from `rax`); negative values represent kernel error codes. 
 */
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

/**
 * Perform a system call with two arguments and return its result.
 *
 * @param n Syscall number to invoke.
 * @param a1 First syscall argument (passed in RDI).
 * @param a2 Second syscall argument (passed in RSI).
 * @returns The raw return value from the syscall (value returned in RAX).
 */
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

/**
 * Invoke a system call with three arguments.
 *
 * @param n System call number.
 * @param a1 First argument passed to the syscall.
 * @param a2 Second argument passed to the syscall.
 * @param a3 Third argument passed to the syscall.
 * @returns The value returned by the syscall; a negative value indicates an error code. 
 */
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

/**
 * Invoke a syscall identified by `n` with four integer/pointer arguments.
 *
 * @param n   Syscall number to invoke.
 * @param a1  First argument (passed in RDI).
 * @param a2  Second argument (passed in RSI).
 * @param a3  Third argument (passed in RDX).
 * @param a4  Fourth argument (passed in R10).
 * @returns The raw syscall return value (RAX). On error this typically contains a negative errno-style value.
 */
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

/**
 * Issue a system call with five arguments.
 *
 * @param n System call number.
 * @param a1 First syscall argument (placed in RDI).
 * @param a2 Second syscall argument (placed in RSI).
 * @param a3 Third syscall argument (placed in RDX).
 * @param a4 Fourth syscall argument (placed in R10).
 * @param a5 Fifth syscall argument (placed in R8).
 * @returns Result of the system call (value returned in RAX).
 */
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

/**
 * Invoke a system call with up to six arguments.
 *
 * @param n System call number.
 * @param a1 First argument to the system call.
 * @param a2 Second argument to the system call.
 * @param a3 Third argument to the system call.
 * @param a4 Fourth argument to the system call.
 * @param a5 Fifth argument to the system call.
 * @param a6 Sixth argument to the system call.
 * @returns The raw value returned by the kernel: a non-negative result on success or a negative error code on failure.
 */
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