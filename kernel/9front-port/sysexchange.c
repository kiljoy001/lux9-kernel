/*
 * System calls for exchange page system
 * Provides userspace interface to exchange page functionality
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pageown.h"
#include "exchange.h"
#include <error.h>

/*
 * Prepare a page for exchange
 *
 * Usage: exchange_prepare(vaddr)
 *
 * Prepares a page for transfer to another process.
 * The current process loses access to the page.
 * Returns an exchange handle that can be passed to another process.
 */
long
sys_exchange_prepare(va_list list)
{
	u64int vaddr;
	ExchangeHandle handle;
	
	vaddr = va_arg(list, u64int);
	
	/* Validate parameters */
	if((vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);
	
	/* Prepare the page for exchange */
	handle = exchange_prepare(vaddr);
	if(handle == 0)
		error("exchange_prepare: failed to prepare page");
	
	return handle;
}

/*
 * Accept an exchange page
 *
 * Usage: exchange_accept(handle, dest_vaddr, prot)
 *
 * Accepts a page that was prepared for exchange by another process.
 * Maps the page at the specified virtual address with given permissions.
 */
long
sys_exchange_accept(va_list list)
{
	ExchangeHandle handle;
	u64int dest_vaddr;
	int prot;
	int result;
	
	handle = va_arg(list, ExchangeHandle);
	dest_vaddr = va_arg(list, u64int);
	prot = va_arg(list, int);
	
	/* Validate parameters */
	if((dest_vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);
	
	/* Accept the exchange page */
	result = exchange_accept(handle, dest_vaddr, prot);
	if(result != EXCHANGE_OK) {
		switch(result) {
		case EXCHANGE_EINVAL:
			error(Ebadarg);
		case EXCHANGE_ENOTEXCHANGE:
			error("exchange_accept: not an exchangeable page");
		case EXCHANGE_EALREADY:
			error("exchange_accept: page already owned");
		default:
			error("exchange_accept: failed");
		}
	}
	
	return 0;
}

/*
 * Cancel an exchange
 *
 * Usage: exchange_cancel(handle)
 *
 * Cancels a prepared exchange and returns the page to the original owner.
 */
long
sys_exchange_cancel(va_list list)
{
	ExchangeHandle handle;
	int result;
	
	handle = va_arg(list, ExchangeHandle);
	
	/* Cancel the exchange */
	result = exchange_cancel(handle);
	if(result != EXCHANGE_OK) {
		switch(result) {
		case EXCHANGE_EINVAL:
			error(Ebadarg);
		default:
			error("exchange_cancel: failed");
		}
	}
	
	return 0;
}

/*
 * Prepare a range of pages for exchange
 *
 * Usage: exchange_prepare_range(vaddr, len, handles)
 *
 * Prepares a range of pages for transfer to another process.
 * The current process loses access to the pages.
 * Returns the number of pages prepared.
 */
long
sys_exchange_prepare_range(va_list list)
{
	u64int vaddr;
	ulong len;
	ExchangeHandle *handles;
	int result;
	
	vaddr = va_arg(list, u64int);
	len = va_arg(list, ulong);
	handles = va_arg(list, ExchangeHandle*);
	
	/* Validate parameters */
	if((vaddr & (BY2PG-1)) != 0)
		error(Ebadarg);
	if(len == 0 || len > 1*GiB)
		error(Ebadarg);
	
	/* Prepare the pages for exchange */
	result = exchange_prepare_range(vaddr, len, handles);
	if(result < 0)
		error("exchange_prepare_range: failed to prepare pages");
	
	return result;
}