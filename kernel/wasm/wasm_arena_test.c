/* wasm_arena_test.c - Test Pebble Arena Branch Integration
 *
 * This test validates that the WASM-as-processes refactoring correctly
 * integrates Pebble arena branches for per-WASM resource accounting.
 */

#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/fcall.h"
#include "../include/pebble.h"

extern void wasm_runtime_init(void);
extern int sys_wasm_compile(Fcall *tx, Fcall *rx);
extern int sys_wasm_execute(Fcall *tx, Fcall *rx);
extern int sys_wasm_destroy(Fcall *tx, Fcall *rx);

/* Forward declaration */
static void wasm_arena_test_main(void);

/* Arena test kernel process wrapper */
/*@
  @ requires  == \null || \valid();
  @ assigns \nothing;
  @*/
static void wasm_arena_test_proc(void *) {
    print("=== WASM Arena Test Starting (in kproc) ===\n");
    wasm_arena_test_main();
}

/* Arena test: Load and execute arena_test.wasm */
/*@
  @ assigns \nothing;
  @*/
static void wasm_arena_test_main(void) {
    print("=== WASM Arena Test - Loading Module ===\n");

    /* Read arena_test.wasm from initrd */
    Chan *c = namec("/boot/arena_test.wasm", Aopen, OREAD, 0);
    if (c == nil) {
        print("WASM_ARENA_TEST: Failed to open arena_test.wasm\n");
        return;
    }

    /* Get file size */
    Dir d;
    devtab[c->type]->stat(c, (uchar *)&d, sizeof(Dir));
    u32int module_size = d.length;

    print("WASM_ARENA_TEST: Loading %u byte module\n", module_size);

    /* Allocate buffer and read module */
    u8int *module_bytes = mallocz(module_size, 1);
    if (module_bytes == nil) {
        print("WASM_ARENA_TEST: Failed to allocate module buffer\n");
        cclose(c);
        return;
    }

    devtab[c->type]->read(c, module_bytes, module_size, 0);
    cclose(c);

    /* Build Tsyscall(SYS_WASM_COMPILE) message */
    Fcall tx, rx;
    memset(&tx, 0, sizeof(Fcall));
    memset(&rx, 0, sizeof(Fcall));

    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 160; /* SYS_WASM_COMPILE */
    tx.scount = 4 + module_size;

    /* Format: [module_size:4] [module_bytes:n] */
    u8int *sdata = mallocz(tx.scount, 1);
    *(u32int *)sdata = module_size;
    memmove(sdata + 4, module_bytes, module_size);
    tx.sdata = sdata;

    print("WASM_ARENA_TEST: Compiling WASM module (arena should init with 1MB)\n");
    print("WASM_ARENA_TEST: Expected: local_colorless = 131072 tokens (1048576 bytes)\n");

    /* Compile (this calls arena_branch_init) */
    int result = sys_wasm_compile(&tx, &rx);
    free(module_bytes);
    free(sdata);

    if (result != 0) {
        print("WASM_ARENA_TEST: Compile failed: %s\n", rx.ename);
        return;
    }

    print("WASM_ARENA_TEST: Compile succeeded, pid=%llu\n", *(u64int *)rx.sdata);
    print("WASM_ARENA_TEST: Check arena_branch_init debug output above\n");

    /* Run tests */
    const char *tests[] = {
        "test_progressive_alloc",
        "test_large_alloc",
        "test_exact_budget",
        "test_refill",
        "test_conservation"
    };

      /*@ loop invariant 0 <= i <= 5;
    @ loop assigns i;
    @ loop variant 5 - i;
    @*/
  for (int i = 0; i < 5; i++) {
        print("\n=== Test %d: %s ===\n", i+1, tests[i]);

        /* Build Tsyscall(SYS_WASM_EXECUTE) */
        memset(&tx, 0, sizeof(Fcall));
        memset(&rx, 0, sizeof(Fcall));

        tx.type = Tsyscall;
        tx.tag = 1;
        tx.scallnr = 161; /* SYS_WASM_EXECUTE */

        u32int func_name_len = strlen(tests[i]);
        tx.scount = 4 + func_name_len;

        sdata = mallocz(tx.scount, 1);
        *(u32int *)sdata = func_name_len;
        memmove(sdata + 4, tests[i], func_name_len);
        tx.sdata = sdata;

        /* Execute test */
        result = sys_wasm_execute(&tx, &rx);
        free(sdata);

        if (result != 0) {
            print("WASM_ARENA_TEST: %s FAILED: %s\n", tests[i], rx.ename);
        } else {
            u64int retval = *(u64int *)rx.sdata;
            print("WASM_ARENA_TEST: %s returned %llu\n", tests[i], retval);
        }
    }

    /* Cleanup (this calls arena_branch_drain) */
    print("\n=== Cleanup: arena_branch_drain ===\n");
    print("WASM_ARENA_TEST: Expected: tokens returned to process colorless_bank\n");

    memset(&tx, 0, sizeof(Fcall));
    memset(&rx, 0, sizeof(Fcall));
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 162; /* SYS_WASM_DESTROY */

    sys_wasm_destroy(&tx, &rx);

    print("WASM_ARENA_TEST: Check arena_branch_drain debug output above\n");
    print("=== WASM Arena Test Complete ===\n");
}

/* Public entry point - spawns test as kproc */
/*@
  @ assigns \nothing;
  @*/
void wasm_arena_test(void) {
    kproc("wasm_arena_test", wasm_arena_test_proc, nil);
    print("=== WASM Arena Test - kproc spawned ===\n");
}
