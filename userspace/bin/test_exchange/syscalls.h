/* Syscall wrappers for exchange operations */

#include <stdint.h>
#include <stddef.h>

/* Syscall numbers from kernel/include/sys.h */
#define EXCHANGE_PREPARE	59
#define EXCHANGE_ACCEPT		60
#define EXCHANGE_CANCEL		61
#define EXCHANGE_PREPARE_RANGE	62

/* Memory permissions */
#define PTEVALID	0x001
#define PTEWRITE	0x002
#define PTEUSER		0x004

/* Page size */
#define BY2PG		4096

/* Syscall function - implemented in kernel */
extern long syscall(int num, ...);

/* Exchange syscall wrappers */
uintptr_t
exchange_prepare(uintptr_t vaddr)
{
	return syscall(EXCHANGE_PREPARE, vaddr);
}

int
exchange_accept(uintptr_t handle, uintptr_t dest_vaddr, int prot)
{
	return syscall(EXCHANGE_ACCEPT, handle, dest_vaddr, prot);
}

int
exchange_cancel(uintptr_t handle)
{
	return syscall(EXCHANGE_CANCEL, handle);
}

int
exchange_prepare_range(uintptr_t vaddr, unsigned long len, uintptr_t *handles)
{
	return syscall(EXCHANGE_PREPARE_RANGE, vaddr, len, handles);
}

/* Simple printf for debugging */
int printf(const char *fmt, ...);

/* Simple sbrk for memory allocation */
extern void* sbrk(intptr_t increment);