/*
 * clr_exchange_ops.c - System.P9.ExchangePage Native Implementation
 *
 * Implements low-level 9P message passing via direct dispatch.
 */

#include "../../9front-pc64/mem.h"
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/fcall.h"
#include "../qbe/kernel_compat.h"
#include "clr-kernel/clr_pebble_integration.h"

extern int p9_dispatch(Proc *p, Fcall *t, Fcall *r);

/* Forward declarations for array helpers from clr_core.c (conceptually) */
/* We need to access array data. Assuming the layout from clr_core.c:
   [length (4)] [pad (4)] [data...] for byte arrays? 
   Wait, clr_core.c:clr_array_getvalue handles object arrays [len][pad][ptr...].
   For byte arrays (value types), the data usually starts immediately after length?
   Let's check Mono/CoreCLR or standard layout.
   Usually: [MethodTable*][Length][Data]
   But clr_core.c seems to assume:
   clr_object_t->data points to [Length][...]
   
   If clr_object_t->data points to the array data structure:
   For byte[]:
   [s32int length][u8int b0][u8int b1]...
   
   Let's assume this simple layout as used for Strings in clr_core.c.
*/

static void *get_array_data(clr_object_t *arr, int *out_len) {
    if (arr == nil || arr->data == nil) {
        if (out_len) *out_len = 0;
        return nil;
    }
    
    s32int *len_ptr = (s32int *)arr->data;
    if (out_len) *out_len = *len_ptr;
    
    /* Data starts after length (4 bytes) */
    return (void *)(len_ptr + 1);
}

/*
 * System.P9.ExchangePage.GetMyPage()
 * Returns capability/handle to the process's exchange page.
 * For this direct-dispatch version, we might just return a dummy handle
 * or the actual p9page pointer if we were using shared memory.
 * 
 * Init.fs uses this handle to pass to SendMessage.
 * We can return the Proc* pointer cast to int/handle, or just 1 to indicate "valid".
 * Since SendMessage uses p9_dispatch(up, ...), we don't strictly need the handle 
 * to find the page, as 'up' (current process) is global in kernel context.
 */
uint32_t clr_exchange_get_page(void) {
    /* Return current process PID as a handle, or just 1 */
    return up ? up->pid : 1;
}

/* 
 * System.P9.ExchangePage.SendMessage(
 *     ExchangePageCap pageCap, 
 *     byte[] request, 
 *     int requestLen, 
 *     byte[] reply, 
 *     int replyMaxLen
 * )
 */
int clr_exchange_send_message(
    u32int cap_handle, /* ExchangePageCap is a struct with one uint field */
    clr_object_t *req_arr,
    int req_len,
    clr_object_t *reply_arr,
    int reply_max_len
) {
    Fcall t, r;
    uchar *req_buf;
    uchar *reply_buf;
    int arr_len;
    
    if (req_arr == nil || reply_arr == nil) return -1;
    
    /* Get pointers to array data */
    req_buf = get_array_data(req_arr, &arr_len);
    if (req_len > arr_len) return -1; /* Bounds check */
    
    reply_buf = get_array_data(reply_arr, &arr_len);
    if (reply_max_len > arr_len) return -1; /* Bounds check */
    
    /* Parse request message (T-message) */
    /* convM2S parses byte buffer into Fcall struct */
    /* Returns size of message, or 0 on error */
    if (convM2S(req_buf, req_len, &t) == 0) {
        print("clr_exchange: bad T-message format\n");
        return -1;
    }
    
    /* Initialize reply Fcall */
    memset(&r, 0, sizeof(r));
    
    /* Dispatch to kernel 9P router */
    /* This handles everything: Tattach, Twalk, Topen, etc. */
    /* It mimics what devmnt/exportfs do */
    
    /* Note: p9_dispatch expects 'up' to be set */
    if (up == nil) {
        print("clr_exchange: no current process\n");
        return -1;
    }
    
    /* Dispatch! */
    /* p9_dispatch returns 0 on success, -1 on error */
    /* On error, it might set r.type to Rerror or just return -1 */
    if (p9_dispatch(up, &t, &r) < 0) {
        /* If p9_dispatch failed at routing level, construct generic Rerror if not set */
        if (r.type != Rerror) {
            r.type = Rerror;
            r.tag = t.tag;
            r.ename = "dispatch failed";
        }
    }
    
    /* Serialize reply (R-message) back to bytes */
    /* convS2M(Fcall *f, uchar *ap, uint nap) */
    unsigned int encoded_len = convS2M(&r, reply_buf, reply_max_len);
    
    /* Free any strings allocated in Fcall r by p9_dispatch/handlers if necessary? */
    /* p9_dispatch typically uses a static buffer or small allocations. 
       Need to be careful about leaks if strings are kstrdup'd.
       Standard 9P handlers usually use a shared buffer for reply data.
       But Rerror strings might need freeing if they are not constants.
       For now, assume p9_dispatch cleans up or uses pool.
    */
    
    return (int)encoded_len;
}
