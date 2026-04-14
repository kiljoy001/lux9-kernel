/*
 * Exchange page system interface
 * Provides Singularity-style exchange heap semantics at page granularity
 */

#pragma once

#include "blind_ledger.h" // New include for UserCapability
#include "dat.h"

/* Exchange page handle - physical address of the page */
typedef UserCapability ExchangeHandle;

/* Error codes for exchange operations */
enum ExchangeError {
  EXCHANGE_OK = 0,
  EXCHANGE_EINVAL,       /* Invalid parameters */
  EXCHANGE_ENOTOWNER,    /* Not owner of the page */
  EXCHANGE_EBORROWED,    /* Page is currently borrowed */
  EXCHANGE_ENOMEM,       /* Out of memory */
  EXCHANGE_EALREADY,     /* Page already in use */
  EXCHANGE_ENOTEXCHANGE, /* Not an exchangeable page */
  EXCHANGE_EFULL,        /* Ring buffer full */
};

/* Exchange allocation types */
typedef enum {
  EXCHANGE_TYPE_LARGE = 0, /* Single large block (4KB) */
  EXCHANGE_TYPE_RING,      /* Small message in ring buffer */
  EXCHANGE_TYPE_AUTO,      /* Auto-detect based on size */
} ExchangeType;

/* Hybrid exchange request */
typedef struct {
  ulong size;          /* Requested message size */
  ExchangeType type;   /* Requested allocation type */
  char path[KNAMELEN]; /* Destination path for batching */
} ExchangeRequest;

/* Initialize exchange page system */
void exchangeinit(void);

/* Core exchange operations */
BlindLedgerError exchange_prepare(uintptr vaddr, ExchangeHandle *out_cap);
int exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles);
int exchange_accept(const ExchangeHandle *handle, uintptr dest_vaddr, int prot);
int exchange_cancel(const ExchangeHandle *handle);
int exchange_transfer(Proc *from, Proc *to, const ExchangeHandle *handle,
                      uintptr to_vaddr);

/* Transfer operations */
// No duplicate exchange_prepare_range here

/* Query operations */
int exchange_is_valid(const ExchangeHandle *handle);
Proc *exchange_get_owner(const ExchangeHandle *handle);

/* Syscall interface */
uintptr sys_exchange_prepare(void *list);
uintptr sys_exchange_prepare_range(void *list);
uintptr sys_exchange_accept(void *list);
uintptr sys_exchange_cancel(void *list);
uintptr sys_exchange_transfer(void *list);

/* Phase 3: Capability-Based Mapping */
uintptr exchange_map_by_cap(const UserCapability *cap);
int exchange_unmap_by_cap(const UserCapability *cap, uintptr va);
int exchange_verify_and_map(const UserCapability *cap, uintptr va, int prot);

/* TOCTOU Protection */
int exchange_lock_page(const UserCapability *cap);
void exchange_unlock_page(const UserCapability *cap);
