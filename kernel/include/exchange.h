/*
 * Exchange page system interface
 * Provides Singularity-style exchange heap semantics at page granularity
 */

#pragma once

#include "dat.h"

/* Exchange page handle - physical address of the page */
typedef uintptr ExchangeHandle;

/* Error codes for exchange operations */
enum ExchangeError {
	EXCHANGE_OK = 0,
	EXCHANGE_EINVAL,		/* Invalid parameters */
	EXCHANGE_ENOTOWNER,		/* Not owner of the page */
	EXCHANGE_EBORROWED,		/* Page is currently borrowed */
	EXCHANGE_ENOMEM,		/* Out of memory */
	EXCHANGE_EALREADY,		/* Page already in use */
	EXCHANGE_ENOTEXCHANGE,		/* Not an exchangeable page */
};

/* Initialize exchange page system */
void	exchangeinit(void);

/* Core exchange operations */
ExchangeHandle	exchange_prepare(uintptr vaddr);
int		exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles);
int		exchange_accept(ExchangeHandle handle, uintptr dest_vaddr, int prot);
int		exchange_cancel(ExchangeHandle handle);

/* Transfer operations */
int		exchange_transfer(Proc *from, Proc *to, ExchangeHandle handle, uintptr to_vaddr);
int		exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles);

/* Query operations */
int		exchange_is_valid(ExchangeHandle handle);
Proc*		exchange_get_owner(ExchangeHandle handle);

/* Syscall interface */
long	sys_exchange_prepare(va_list list);
long	sys_exchange_prepare_range(va_list list);
long	sys_exchange_accept(va_list list);
long	sys_exchange_cancel(va_list list);