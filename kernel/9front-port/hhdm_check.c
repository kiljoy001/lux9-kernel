/*
 * HHDM offset validation and dynamic detection
 */
#include "u.h"
#include "portlib.h"
#include "../../limine.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

extern uintptr saved_limine_hhdm_offset;
extern struct limine_hhdm_request *limine_hhdm;

/* Validate HHDM offset is working correctly */
static int
hhdm_offset_valid(uintptr offset)
{
    /* Basic sanity checks */
    if (offset == 0) {
        return 0;  /* Invalid offset */
    }
    
    /* Check if it's in expected HHDM range for x86_64 */
    if (offset < 0xffff800000000000ULL || offset > 0xffffffff80000000ULL) {
        return 0;  /* Outside expected HHDM range */
    }
    
    return 1;  /* Valid */
}

/* Get HHDM offset dynamically from Limine */
uintptr
get_hhdm_offset(void)
{
    /* First try saved offset */
    if (hhdm_offset_valid(saved_limine_hhdm_offset)) {
        return saved_limine_hhdm_offset;
    }
    
    /* Fall back to direct Limine query if saved is invalid */
    if (limine_hhdm && limine_hhdm->response) {
        uintptr direct_offset = limine_hhdm->response->offset;
        if (hhdm_offset_valid(direct_offset)) {
            /* Update saved offset for future use */
            saved_limine_hhdm_offset = direct_offset;
            return direct_offset;
        }
    }
    
    /* Last resort fallback */
    print("WARNING: Using fallback HHDM offset - memory corruption may occur\n");
    return 0xffff800000000000UL;
}
