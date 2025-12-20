/*
 * Exchange page system - clean facade implementation
 * Integrates with borrow checker for safe page exchange
 * Provides Singularity-style exchange heap semantics at page granularity
 */

#include "exchange.h"
#include "../9front-pc64/fns.h"
#include "blind_ledger.h" // New include
#include "dat.h"
#include "lock_borrow.h"
#include "mem.h"
#include "pageown.h" // Will be deprecated/refactored
#include "portlib.h"
#include "u.h"
#include <error.h>

/* Track prepared pages for cancellation */
struct PreparedPage {
  UserCapability capability; // Changed from ExchangeHandle handle
  uintptr original_vaddr;
  Proc *owner;
  struct PreparedPage *next;
};

static struct PreparedPage *prepared_pages = nil;
static BorrowLock prepared_lock;
static LockDagNode lockdag_exchange_prepared =
    LOCKDAG_NODE("exchange-prepared");

/* Convert physical address to page frame number */
static inline ulong pa2pfn(uintptr pa) { return pa >> PGSHIFT; }

// Helper function to find a PreparedPage by UserCapability
static struct PreparedPage *
lookup_prepared_page_by_cap_locked(const UserCapability *cap) {
  struct PreparedPage *pp;
  for (pp = prepared_pages; pp != nil; pp = pp->next) {
    if (memcmp(pp->capability.hash, cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
      return pp;
    }
  }
  return nil;
}

/*
 * Initialize exchange page system
 */
void exchangeinit(void) {
  blind_ledger_init(); // Initialize Blind Ledger
  borrow_lock_init(&prepared_lock, (uintptr)&prepared_lock,
                   &lockdag_exchange_prepared);
  print("exchange: initialized\n");
}

/*
 * Prepare a page for exchange - remove from current process
 * Returns an exchange handle (physical address) that can be passed to another
 * process
 */
BlindLedgerError exchange_prepare(uintptr vaddr, ExchangeHandle *out_cap) {
  u64int *pte;
  uintptr pa;
  struct PreparedPage *pp = nil;
  BlindLedgerError ledger_err;
  enum BorrowError borrow_err;
  UserCapability existing_cap; // To hold the capability for the existing page
  BlindLedgerEntry existing_entry; // To hold details of the existing capability

  if (out_cap == nil)
    return BLIND_LEDGER_EINVAL;

  /* Validate virtual address alignment */
  if ((vaddr & (BY2PG - 1)) != 0)
    return BLIND_LEDGER_EINVAL;

  /* Get PTE for the virtual address */
  pte = mmuwalk(m->pml4, vaddr, 0, 0);
  if (pte == nil || (*pte & PTEVALID) == 0)
    return BLIND_LEDGER_EINVAL;

  /* Get physical address */
  pa = PADDR(*pte);

  // --- Verify current ownership via Blind Ledger ---
  // Lookup capability by physical address and owner
  // This verifies that 'up' actually owns the physical page 'pa' according to
  // the ledger
  ledger_err =
      ledger_lookup_by_pa_and_owner(pa, up, &existing_cap, &existing_entry);
  if (ledger_err != BLIND_LEDGER_OK) {
    /*
     * Failed to find valid ledger entry for this PA owned by current process.
     * This means the process mapped the page but doesn't handle the rights in
     * the ledger, or the ledger state is out of sync.
     */
    putcr3(getcr3()); // Flush to be safe
    return BLIND_LEDGER_EPERM;
  }
  // existing_cap and existing_entry are now populated and verified

  /* Unmap from current process */
  *pte = 0;
  /* Flush TLB */
  putcr3(getcr3());

  // --- Burn existing capability ---
  ledger_err = ledger_burn(&existing_cap, up);
  if (ledger_err != BLIND_LEDGER_OK) {
    // Critical consistency failure: Page is unmapped, ownership verified, but
    // burn failed. This implies internal ledger corruption or race condition.
    panic("exchange: critical ledger corruption - failed to burn verified "
          "capability");
    return BLIND_LEDGER_EFAULT;
  }

  // --- Release borrow checker ownership ---
  borrow_err = borrow_release(up, pa);
  if (borrow_err != BORROW_OK) {
    // Critical consistency failure: Ledger burn succeeded, but borrow release
    // failed. State is now inconsistent between Ledger and Borrow Checker.
    panic("exchange: critical borrow consistency failure - failed to release "
          "after burn");
    return BLIND_LEDGER_EFAULT;
  }

  // --- Mint new capability for prepared state ---
  // This represents the capability now owned by the exchange system, or for
  // transfer. Permissions might need to be adjusted based on the exchange
  // context.
  u8int mint_secret[BLIND_LEDGER_SECRET_SIZE];

  // Generate cryptographic secret via TPM-backed vault
  ledger_err = ledger_generate_secret(mint_secret);
  if (ledger_err != BLIND_LEDGER_OK) {
    // Failed to generate secret
    borrow_acquire(up, pa); // Re-acquire borrow
    return BLIND_LEDGER_EFAULT;
  }

  // The owner in ledger_mint should be the temporary owner, e.g., 'up' if
  // preparing for self, or a special kernel exchange process. For now, let's
  // keep 'up' as the conceptual owner until it's accepted by another process or
  // cancelled.
  ledger_err = ledger_mint(out_cap, pa, BY2PG, up,
                           CAP_PERM_TRANSFER | CAP_PERM_READ, mint_secret);
  if (ledger_err != BLIND_LEDGER_OK) {
    /*
     * Mint failed. We must rollback the state:
     * 1. Re-acquire borrow ownership
     * 2. We cannot easily un-burn the old capability as secrets are destroyed.
     *    However, the page is now in "limbo" - owned by no one in ledger, but
     * borrow released. This is an unrecoverable state for this page in terms of
     * user ownership. Panic is the safest option to avoid leaking untracked
     * memory.
     */
    panic("exchange: failed to mint new capability after burn - unrecoverable "
          "state");
    return BLIND_LEDGER_EFAULT;
  }
  // We don't acquire borrow_acquire here, as that's handled by
  // pebble_black_alloc normally. The physical page 'pa' is already controlled
  // by pebble/ledger now.

  /* Allocate tracking structure */
  pp = smalloc(sizeof(struct PreparedPage));
  if (pp == nil) {
    // Atomic rollback: burn the newly minted capability and re-acquire borrow
    // ownership
    ledger_burn(out_cap, up); // Burn the newly minted capability
    borrow_acquire(up, pa); // Re-acquire borrow ownership for original process
    // Note: The original capability is now gone, but the borrow checker state
    // is consistent
    return BLIND_LEDGER_ENOMEM;
  }

  /* Track the prepared page */
  pp->capability = *out_cap; // Store the UserCapability
  pp->original_vaddr = vaddr;
  pp->owner = up; // The original owner is the one preparing it.

  borrow_lock(&prepared_lock);
  pp->next = prepared_pages;
  prepared_pages = pp;
  borrow_unlock(&prepared_lock);

  return BLIND_LEDGER_OK;
}

/*
 * Accept an exchange page into current process
 * Maps the page at the specified virtual address with given permissions
 */
int exchange_accept(const ExchangeHandle *handle, uintptr dest_vaddr,
                    int prot) {
  uintptr pa;
  struct PreparedPage *pp;
  BlindLedgerEntry entry;
  BlindLedgerError ledger_err;
  enum BorrowError borrow_err;

  if (handle == nil || (dest_vaddr & (BY2PG - 1)) != 0)
    return EXCHANGE_EINVAL;

  // Verify capability with Blind Ledger and get physical address
  ledger_err = ledger_verify(handle, &entry);
  if (ledger_err != BLIND_LEDGER_OK) {
    return EXCHANGE_EINVAL; // Invalid or expired handle
  }
  pa = entry.physical_address;

  /* Remove from prepared pages list */
  borrow_lock(&prepared_lock); // Using borrow_lock for prepared_pages
  pp = lookup_prepared_page_by_cap_locked(handle); // Use new lookup
  if (pp == nil) {
    unlock(&prepared_lock);
    return EXCHANGE_EINVAL; // Not a prepared page
  }
  // Remove pp from the linked list
  struct PreparedPage **prev;
  for (prev = &prepared_pages; *prev != nil; prev = &(*prev)->next) {
    if (*prev == pp) {
      *prev = pp->next;
      break;
    }
  }
  unlock(&prepared_lock);

  /* Map the page into current process at the specified address */
  extern void userpmap(uintptr va, uintptr pa, int perms);
  userpmap(dest_vaddr, pa, prot);

  // Transfer ownership of the capability to the current process with rollback
  // support This also updates the ledger's internal state (process_hash, Merkle
  // root)
  LedgerRollbackToken rollback_token;
  UserCapability new_cap; // Will hold the new capability after transfer

  ledger_err =
      ledger_transfer_reversible(handle, pp->owner, up, &rollback_token);
  if (ledger_err != BLIND_LEDGER_OK) {
    // Transfer failed. Rollback mapping.
    u64int *pte = mmuwalk(m->pml4, dest_vaddr, 0, 0);
    if (pte && (*pte & PTEVALID))
      *pte = 0;
    putcr3(getcr3()); // Flush TLB
    // pp is not freed here, it remains valid in the list (we removed it? yes)
    // Wait, we removed it from the list at line 198-212.
    // We must put it back or free it and fail?
    // If we fail here, the handle is still owned by original owner.
    // But we lost the PreparedPage tracking if we free 'pp'.
    // So the original owner can never cancel it!
    // We should add it back to the list.

    borrow_lock(&prepared_lock);
    pp->next = prepared_pages;
    prepared_pages = pp;
    borrow_unlock(&prepared_lock);

    return EXCHANGE_EINVAL;
  }

  // Get the new capability after transfer (needed for potential rollback)
  ledger_err = ledger_verify(handle, &entry);
  if (ledger_err != BLIND_LEDGER_OK) {
    // This should not happen - we just transferred successfully
    u64int *pte = mmuwalk(m->pml4, dest_vaddr, 0, 0);
    if (pte && (*pte & PTEVALID))
      *pte = 0;
    putcr3(getcr3());
    ledger_rollback_transfer(&entry.capability, &rollback_token);
    free(pp);
    return EXCHANGE_EINVAL;
  }
  memmove(&new_cap, &entry.capability, sizeof(UserCapability));

  // Acquire borrow checker ownership for the current process
  borrow_err = borrow_acquire(up, pa);
  if (borrow_err != BORROW_OK) {
    // If acquire fails, unmap the page and rollback ledger_transfer
    u64int *pte = mmuwalk(m->pml4, dest_vaddr, 0, 0);
    if (pte && (*pte & PTEVALID))
      *pte = 0;
    putcr3(getcr3()); // Flush TLB

    // Atomic rollback: restore original owner
    ledger_rollback_transfer(&new_cap, &rollback_token);

    free(pp);
    return EXCHANGE_EALREADY;
  }

  free(pp); // Free the PreparedPage struct (using generic free which handles
            // smalloc)

  return EXCHANGE_OK;
}

/*
 * Cancel an exchange and return page to original owner
 * This undoes a prepare operation
 */
int exchange_cancel(const ExchangeHandle *handle) {
  uintptr pa;
  struct PreparedPage *pp;
  BlindLedgerEntry entry;
  BlindLedgerError ledger_err;
  enum BorrowError borrow_err;

  if (handle == nil)
    return EXCHANGE_EINVAL;

  // Find the prepared page
  borrow_lock(&prepared_lock);
  pp = lookup_prepared_page_by_cap_locked(handle); // Use new lookup
  if (pp == nil) {
    unlock(&prepared_lock);
    return EXCHANGE_EINVAL; // Not a prepared page
  }
  // Remove pp from the linked list
  struct PreparedPage **prev;
  for (prev = &prepared_pages; *prev != nil; prev = &(*prev)->next) {
    if (*prev == pp) {
      *prev = pp->next;
      break;
    }
  }
  unlock(&prepared_lock); // Unlock before external calls

  // Verify capability with Blind Ledger and get physical address
  ledger_err = ledger_verify(handle, &entry);
  if (ledger_err != BLIND_LEDGER_OK) {
    // This is a severe inconsistency. Prepared page found in tracking but
    // invalid in ledger.
    print("exchange: consistency warning - prepared page not in ledger\n");
    free(pp);
    return EXCHANGE_EINVAL;
  }
  pa = entry.physical_address;

  // --- Burn UserCapability via Blind Ledger ---
  ledger_err = ledger_burn(handle, pp->owner);
  if (ledger_err != BLIND_LEDGER_OK) {
    // Severe inconsistency. Cap valid but burn failed.
    panic("exchange: critical ledger failure - failed to burn during cancel");
    free(pp);
    return EXCHANGE_EINVAL;
  }

  // --- Release borrow checker ownership ---
  borrow_err = borrow_release(pp->owner, pa);
  if (borrow_err != BORROW_OK) {
    // Severe inconsistency. Borrow not released.
    panic(
        "exchange: critical borrow failure - failed to release during cancel");
    free(pp);
    return EXCHANGE_EINVAL;
  }

  /* Remap the page back to the original owner */
  extern void userpmap(uintptr va, uintptr pa, int perms);
  userpmap(pp->original_vaddr, pa, PTEVALID | PTEUSER | PTEWRITE);

  /* Free the tracking structure */
  free(pp);

  return EXCHANGE_OK;
}

/*
 * Transfer a page from one process to another
 * This is the core exchange operation
 */
int exchange_transfer(Proc *from, Proc *to, const ExchangeHandle *handle,
                      uintptr to_vaddr) {
  uintptr pa;
  BlindLedgerEntry entry;
  BlindLedgerError ledger_err;
  enum BorrowError borrow_err;

  if (from == nil || to == nil || handle == nil ||
      (to_vaddr & (BY2PG - 1)) != 0) // Changed from handle == 0
    return EXCHANGE_EINVAL;

  // Verify capability with Blind Ledger and get physical address
  ledger_err = ledger_verify(handle, &entry);
  if (ledger_err != BLIND_LEDGER_OK) {
    return EXCHANGE_EINVAL; // Invalid or expired handle
  }
  pa = entry.physical_address;

  // --- Transfer ownership via Blind Ledger with rollback support ---
  LedgerRollbackToken rollback_token;
  UserCapability new_cap; // Will hold the new capability after transfer

  ledger_err = ledger_transfer_reversible(handle, from, to, &rollback_token);
  if (ledger_err != BLIND_LEDGER_OK) {
    switch (ledger_err) {
    case BLIND_LEDGER_EPERM:
      return EXCHANGE_ENOTOWNER;
    case BLIND_LEDGER_EEXPIRED:
      return EXCHANGE_EINVAL;
    default:
      return EXCHANGE_EINVAL;
    }
  }

  // Get the new capability after transfer (needed for potential rollback)
  ledger_err = ledger_verify(handle, &entry);
  if (ledger_err != BLIND_LEDGER_OK) {
    // This should not happen - we just transferred successfully
    ledger_rollback_transfer(&entry.capability, &rollback_token);
    return EXCHANGE_EINVAL;
  }
  memmove(&new_cap, &entry.capability, sizeof(UserCapability));

  // --- Transfer ownership via borrow checker ---
  borrow_err = borrow_transfer(from, to, pa);
  if (borrow_err != BORROW_OK) {
    // CRITICAL ATOMIC ROLLBACK: Restore original owner in ledger
    BlindLedgerError rollback_err =
        ledger_rollback_transfer(&new_cap, &rollback_token);
    if (rollback_err != BLIND_LEDGER_OK) {
      // Rollback failed - system is now in inconsistent state
      print("PEBBLE: CRITICAL! Rollback failed after borrow_transfer failure "
            "for pa=%p: borrow_err=%d, rollback_err=%d\n",
            (void *)pa, borrow_err, rollback_err);
      // This is a severe consistency violation - may need to panic or mark
      // capability as corrupted
    }

    switch (borrow_err) {
    case BORROW_ENOTOWNER:
      return EXCHANGE_ENOTOWNER;
    case BORROW_EBORROWED:
      return EXCHANGE_EBORROWED;
    default:
      return EXCHANGE_EINVAL;
    }
  }

  /* Map into target process */
  extern void userpmap(uintptr va, uintptr pa, int perms);
  userpmap(to_vaddr, pa,
           PTEVALID | PTEUSER |
               PTEWRITE); // Permissions should come from the capability

  /* Flush TLB */
  putcr3(getcr3());

  return EXCHANGE_OK;
}

/*
 * Query if an exchange handle is valid
 */
int exchange_is_valid(const ExchangeHandle *handle) {
  BlindLedgerEntry entry;
  BlindLedgerError ledger_err;

  if (handle == nil)
    return 0; // Invalid if handle is nil

  ledger_err = ledger_verify(handle, &entry);
  if (ledger_err == BLIND_LEDGER_OK) {
    return 1; // Valid and active
  }
  return 0; // Not valid
}

/*
 * Get the owner of an exchange handle
 */
Proc *exchange_get_owner(const ExchangeHandle *handle) {
  BlindLedgerEntry entry;
  BlindLedgerError ledger_err;

  if (handle == nil)
    return nil;

  ledger_err = ledger_verify(handle, &entry);
  if (ledger_err == BLIND_LEDGER_OK) {
    return entry.owner;
  }
  return nil; // Not valid or not found
}

/*
 * Prepare a range of pages for exchange
 * Returns the number of pages prepared, or negative on error
 */
int exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles) {
  ulong offset;
  int npages = 0;
  BlindLedgerError ledger_err;

  /* Validate parameters */
  if ((vaddr & (BY2PG - 1)) != 0)
    return -EXCHANGE_EINVAL;
  if (len == 0 || len > 1 * GiB)
    return -EXCHANGE_EINVAL;
  if (handles == nil)
    return -EXCHANGE_EINVAL;

  /* Process each page in the range */
  for (offset = 0; offset < len; offset += BY2PG) {
    uintptr va = vaddr + offset;
    // Call updated exchange_prepare
    ledger_err = exchange_prepare(va, &handles[offset / BY2PG]);

    if (ledger_err != BLIND_LEDGER_OK) { // Check for BlindLedgerError
      /* Cancel any previously prepared pages */
      ulong cancel_offset;
      for (cancel_offset = 0; cancel_offset < offset; cancel_offset += BY2PG) {
        exchange_cancel(
            &handles[cancel_offset / BY2PG]); // Pass address of UserCapability
      }
      return -EXCHANGE_EINVAL; // Or a more specific error based on ledger_err
    }

    npages++;
  }

  return npages;
}
