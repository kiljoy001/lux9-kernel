#pragma once
#include <u.h>
#include <libc.h>

/* Internal syscall wrapper */
long __syscall(long n, long a1, long a2, long a3, long a4, long a5, long a6);

/* Pebble extensions */
void* pebble_issue_white(ulong size);
int pebble_black_alloc(ulong size, void **handle);
int pebble_black_free(void *handle);

typedef ulong ExchangeHandle;
ExchangeHandle exchange_prepare(ulong vaddr);
int exchange_accept(ExchangeHandle handle, ulong dest_vaddr, int prot);
