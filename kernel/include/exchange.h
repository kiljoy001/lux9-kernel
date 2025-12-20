/*
 * Exchange page system interface
 * Provides Singularity-style exchange heap semantics at page granularity
 */

#pragma once

#include "dat.h"
#include "blind_ledger.h" // New include for UserCapability

/* Exchange page handle - physical address of the page */
typedef UserCapability ExchangeHandle;

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
BlindLedgerError	exchange_prepare(uintptr vaddr, ExchangeHandle *out_cap);
int		exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles);
int		exchange_accept(const ExchangeHandle *handle, uintptr dest_vaddr, int prot);
int		exchange_cancel(const ExchangeHandle *handle);

/* Transfer operations */
// No duplicate exchange_prepare_range here

/* Query operations */
int		exchange_is_valid(const ExchangeHandle *handle);
Proc*		exchange_get_owner(const ExchangeHandle *handle);

/* Syscall interface */
long	sys_exchange_prepare(va_list list);
long	sys_exchange_prepare_range(va_list list);
long	sys_exchange_accept(va_list list);
long	sys_exchange_cancel(va_list list);