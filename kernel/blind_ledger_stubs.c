#include "blind_ledger.h"
#include <lib.h>
#include <u.h>

/* Stubs for missing Blind Ledger RB-Tree implementation */

void *ledger_tree_search(const u8int *hash) { return nil; }

void ledger_uuid_tree_insert(void *node) { /* No-op */ }

void *ledger_uuid_tree_search(const u8int *uuid) { return nil; }

/* Stub for lux9_error */
void lux9_error(char *fmt, ...) { print("LUX9 ERROR: %s\n", fmt); }
