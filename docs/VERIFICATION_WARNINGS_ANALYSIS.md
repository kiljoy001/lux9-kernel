# Verification Warnings Analysis

**Date:** January 23, 2026
**Status:** Investigation of Frama-C `⚠ WARN` statuses.

## 1. Root Causes Identified

The initial investigation revealed that many "Warnings" reported by `verification_manager.py` were actually **failures** (syntax errors or missing type definitions) that were misclassified because the script treated any non-zero exit code without "Proved goals" as a warning if "User Error" was absent.

### A. Preprocessing Artifacts (`va_list`)
*   **Issue:** The `scripts/framac_plan9_v2.sh` script was stripping `typedef ... va_list` and renaming `__builtin_va_list` to `va_list` without defining `va_list` itself. This caused syntax errors in `struct Fmt` (defined in `portlib.h` and widely used).
*   **Fix:** Updated `scripts/framac_plan9_v2.sh` to stop modifying `va_list`. Frama-C's GCC machdep handles the standard definitions correctly.
*   **Affected Files:** `borrow_enforce.c` and potentially 40+ others including `portlib.h`.

### B. Missing Headers in Frama-C Mode (`devram.c`)
*   **Issue:** `kernel/9front-port/devram.c` explicitly excluded `monocypher.h` when `__FRAMAC__` was defined. This caused `crypto_argon2_config` to be undefined, leading to syntax errors.
*   **Fix:** Added `#include "monocypher.h"` to the `__FRAMAC__` block in `devram.c`.

### C. Missing Type Definitions in Standalone Files (`msgord_verified.c`)
*   **Issue:** `kernel/msgord_verified.c` appears to be a self-contained verification target with inline typedefs, but it was missing `BString` (used in a function prototype `newpath(BString)`).
*   **Fix:** Added `typedef struct BString { char *data; int len; } BString;` to `msgord_verified.c`.

### D. Static Assertions
*   **Issue:** The preprocessing script removed `_Static_assert` lines but left dangling string literals like `"ulong must match pointer size");` on the next line, causing syntax errors.
*   **Fix:** Added sed filters to remove these specific assertion strings.

## 2. Remaining Warnings (True Verification Gaps)

After fixing the syntax/parsing errors, the remaining warnings are legitimate verification gaps that need to be addressed in future work.

### `kernel/borrow_enforce.c`
*   **Status:** Parsing OK.
*   **Issues:**
    *   Timeouts on `typed_borrow_checked_memset_call_memset_requires`.
    *   Missing `assigns` clauses for `borrow_checked_memmove`.
    *   Missing specs for `panic`, `print`.

### `kernel/msgord_verified.c`
*   **Status:** Parsing OK.
*   **Issues:**
    *   Type mismatch warning: `expected 'Rendez *' but got argument of type 'Rendez **'`. This should be investigated in the code.
    *   Missing specs for `lock`, `unlock`, `xfree`, `wakeup`, `memset`.

### `kernel/9front-port/devram.c`
*   **Status:** Parsing OK (mostly).
*   **Issues:**
    *   `Invalid infinite range password_1+(0..)`: Needs `requires \valid` specification for the password buffer length.
    *   Missing specs for `crypto_wipe`, `genrandom`, `xalloc`.

## 3. Recommendations

1.  **Iterative Spec Addition:** Add minimal ACSL contracts (`assigns \nothing; terminates \true;` or similar) for common kernel functions like `panic`, `print`, `lock`, `unlock` in a central stub file or header to reduce noise.
2.  **Fix Code Bugs:** Investigate the `Rendez**` vs `Rendez*` mismatch in `msgord_verified.c`.
3.  **Refine Ranges:** Fix the "Invalid infinite range" error in `devram.c` by adding length preconditions.
