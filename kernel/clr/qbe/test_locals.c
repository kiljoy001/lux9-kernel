#define USERSPACE_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Forward declarations/Includes for types */
#include "../il_parser.h"
#include "qbe_exec.h"
#include "../fruity/fruity_to_qbe.h"
#include "../il_to_fruity.h"
#include "fruity_sljit.h"

/* Stubbing Fruity/JIT functions to satisfy linker/compiler */

const char *il_to_fruity_error_string(il_to_fruity_error_t error) { return "stub"; }

fruity_function_t *il_to_fruity_convert_method(il_assembly_t *assembly, il_method_t *method, il_to_fruity_error_t *error) { return NULL; }

int fruity_interp_execute(fruity_function_t *func, void **args, int arg_count, void *result) { return -1; }

void fruity_free_function(fruity_function_t *func) {}

fruity_jit_ctx_t *fruity_jit_create(void) { return NULL; }

int fruity_jit_compile(fruity_jit_ctx_t *ctx, fruity_function_t *func, fruity_jit_result_t *result) { return -1; }

void fruity_jit_destroy(fruity_jit_ctx_t *ctx) {}

int fruity_jit_execute(fruity_jit_result_t *result, int64_t *args, int arg_count, int64_t *retval) { return -1; }

void fruity_jit_free_code(fruity_jit_result_t *result) {}

int fruity_aot_compile(fruity_jit_ctx_t *ctx, fruity_function_t *func, void **buffer, size_t *size) { return -1; }

int fruity_aot_load(void *buffer, size_t size, fruity_jit_result_t *result) { return -1; }

/* Mock xalloc/xfree */
void *xalloc(size_t size) {
    void *p = calloc(1, size);
    if (!p) { fprintf(stderr, "Allocation failed\n"); exit(1); }
    return p;
}
void xfree(void *p) { free(p); }

/* Include source files directly to access static functions and avoid makefile complexity */
#include "../il_parser.c"
/* We need to undefine these to avoid conflicts if they are redefined in qbe_exec.c */
#undef nil
#undef snprint
#include "qbe_exec.c"

void test_locals_decoding() {
    printf("[TEST] Locals Decoding from StandAloneSig\n");

    /* 1. Setup Assembly */
    il_assembly_t *assembly = xalloc(sizeof(il_assembly_t));
    
    /* Setup Blob Heap */
    /* Blob format: [Compressed Length] [Data] */
    /* We want Data to be: 0x07 (LOCAL_SIG) 0x05 (Count=5) 0x00 (Constraint) 0x00 (ByRef) ... */
    /* So Length is 4 bytes (example). */
    /* Heap: 0x04 (Len), 0x07, 0x05, 0x00, 0x00 */
    uint8_t blob[] = { 0x04, 0x07, 0x05, 0x00, 0x00 }; 
    assembly->blob_heap = xalloc(sizeof(blob));
    memcpy(assembly->blob_heap, blob, sizeof(blob));
    assembly->blob_heap_size = sizeof(blob);

    /* Setup StandAloneSig Table */
    /* We mock the parsed rows directly since il_get_standalonesig calls parse_standalonesig_table */
    /* But il_get_standalonesig checks if rows are NULL and parses if so. */
    /* We need to mock the Tables Header and Data to allow parsing to work, 
       OR just manually populate the cache. Manual population is easier. */
    
    assembly->standalonesig_count = 1;
    assembly->standalonesigs = xalloc(sizeof(standalonesig_row_t) * 1);
    assembly->standalonesigs[0].signature = 0; /* Index 0 in blob heap */

    /* 2. Setup Method */
    il_method_t *method = xalloc(sizeof(il_method_t));
    method->il_code = (uint8_t*)"\x00"; /* NOP */
    method->il_code_size = 1;
    
    /* local_var_sig_token = 0x11000001 (Table 0x11, Row 1) */
    method->local_var_sig_token = (TABLE_STANDALONESIG << 24) | 1;

    /* 3. Setup Context */
    qbe_exec_ctx_t *ctx = qbe_exec_create();

    /* 4. Execute (Intercept vm_execute_method via modified qbe_exec.c or verify log?) */
    /* Wait, vm_execute_method in qbe_exec.c (USERSPACE_TEST) is static. 
       I can't easily hook it unless I modify qbe_exec.c to print or store the locals_count.
       
       However, the USERSPACE_TEST version of vm_execute_method in qbe_exec.c is:
       static int vm_execute_method(..., uint32_t lc) { ... }
       
       I can redefine it here? No, it's static in included file.
       
       But I included qbe_exec.c. I can modify qbe_exec.c to be more testable?
       Or, since I can edit qbe_exec.c (I just did), I can add a global variable there for testing?
       
       Actually, let's just print the locals_count in qbe_exec.c's USERSPACE_TEST vm_execute_method.
       But I don't want to modify the code just for printing.
       
       Alternative: Use a debugger? No.
       
       Let's look at qbe_exec.c again. 
       static int vm_execute_method(...) { ... }
       
       It does nothing with 'lc'.
       
       I should modify qbe_exec.c's stub vm_execute_method to store the last 'lc' in a global
       variable so I can assert on it.
    */
    
    qbe_exec_result_t result;
    int ret = qbe_exec_method(ctx, assembly, method, QBE_EXEC_INTERPRET_CIL, NULL, 0, &result);
    
    if (ret != 0) {
        printf("FAILED: qbe_exec_method returned %d (%s)\n", ret, result.error_msg);
        exit(1);
    }

    /* Check the global variable from qbe_exec.c */
    extern uint32_t last_locals_count;
    printf("last_locals_count: %d\n", last_locals_count);
    assert(last_locals_count == 5);
    printf("PASSED: last_locals_count == 5\n");
    
    qbe_exec_destroy(ctx);
    xfree(method);
    xfree(assembly->standalonesigs);
    xfree(assembly->blob_heap);
    xfree(assembly);
}

int main() {
    test_locals_decoding();
    printf("Test finished (verification incomplete without stored 'lc')\n");
    return 0;
}
