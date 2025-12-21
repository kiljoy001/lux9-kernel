/* lux9_api.c - Lux9 System Interface for WASM
 *
 * Implements the Host Functions (Imports) that WASM modules use to talk to the kernel.
 * The primary interface is 9P over Exchange Pages.
 */

#include "../../include/u.h"
#include "../../include/portlib.h"
#include "../../include/mem.h"
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../wasm_runtime/wasm3/wasm3.h"
#include "../wasm_runtime/wasm3/m3_core.h"

/* Error checking macro for WASM linking */
#define _(x) { M3Result _res = (x); if (_res) return _res; }

/* Forward declaration for 9P router hook */
extern long p9_route_message(int pid, void *msg, ulong len);

/*
 * lux9_send_9p(ptr: i32, len: i32) -> i32
 *
 * Copies 'len' bytes from WASM memory at 'ptr' to the process's Exchange Page.
 * Then calls the kernel 9P router.
 */
m3ApiRawFunction(lux9_send_9p) {
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(u8int *, msg_ptr);
    m3ApiGetArg(u32int, msg_len);

    if (msg_len > 4096) { /* Exchange page size limit */
        m3ApiReturn(-1);
    }

    /* Check if process has an exchange page */
    if (!up->p9page) {
        /* Lazy allocation could happen here, or fail */
        m3ApiReturn(-2); 
    }

    /* Copy from WASM memory to Kernel Exchange Page */
    /* Note: msg_ptr is already a pointer into the WASM linear memory */
    
    memmove(up->p9page, msg_ptr, msg_len);

    /* Route the message */
    long res = p9_route_message(up->pid, up->p9page, msg_len);

    m3ApiReturn((u32int)res);
}

/*
 * lux9_yield() -> void
 *
 * Yields the CPU.
 */
m3ApiRawFunction(lux9_yield) {
    m3ApiReturnType(void)
    sched();
    m3ApiSuccess();
}

/*
 * lux9_debug_print(ptr: i32, len: i32) -> void
 *
 * Prints to kernel console (kprint).
 */
m3ApiRawFunction(lux9_debug_print) {
    m3ApiReturnType(void)
    m3ApiGetArgMem(char *, str);
    m3ApiGetArg(u32int, len);

    if (len > 256) len = 256;
    
    char buf[257];
    memmove(buf, str, len);
    buf[len] = 0;
    
    print("%s", buf);
    
    m3ApiSuccess();
}

/* Linker function to bind these to a module */
M3Result lux9_link_wasi(IM3Module module) {
    M3Result result = m3Err_none;

    _ (m3_LinkRawFunction(module, "env", "lux9_send_9p",     "i(ii)",  &lux9_send_9p));
    _ (m3_LinkRawFunction(module, "env", "lux9_yield",       "v()",    &lux9_yield));
    _ (m3_LinkRawFunction(module, "env", "lux9_debug_print", "v(ii)",  &lux9_debug_print));

    return result;
}
