/* test_clr_pipeline.c - Unit test for CLR compilation pipeline
 *
 * This tests the core components in isolation without needing kernel boot
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

/* Standalone mode - use standard types */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long u64int;
typedef long vlong;
typedef unsigned long ulong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef long intptr;

#define nil NULL
#define USED(x) (void)(x)
#define BY2PG 4096

/* Mock kernel functions */
static uintptr mock_hhdm_offset = 0xffff800000000000UL;

uintptr get_hhdm_offset(void) { return mock_hhdm_offset; }
void* mallocz(usize n, int clr) { (void)clr; return calloc(1, n); }
void free(void *p) { if(p) ::free(p); }
int snprint(char *buf, int len, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, len, fmt, ap);
    va_end(ap);
    return n;
}
void* memmove(void *dst, void *src, usize n) { return ::memmove(dst, src, n); }
void* memset(void *dst, int c, usize n) { return ::memset(dst, c, n); }
usize strlen(char *s) { return ::strlen(s); }
char* strdup(char *s) { return ::strdup(s); }
#define KADDR(pa) ((void*)((pa) + mock_hhdm_offset))

/* Include Fruity IR (need to adapt paths) */
#include "../clr/fruity/fruity_opcodes.h"
#include "../clr/fruity/fruity_types.h"
#include "../clr/fruity/fruity_ir.h"
#include "../clr/fruity/fruity_to_qbe.h"
#include "../clr/qbe_compile.h"

/* Test 1: Create and verify Fruity IR structure */
static int
test_fruity_ir(void)
{
    printf("Test 1: Fruity IR creation...\n");

    /* Create module */
    fruity_module_t *mod = mallocz(sizeof(*mod), 1);
    mod->name = strdup("test_module");
    mod->version = 1;

    /* Create function */
    fruity_function_t *func = mallocz(sizeof(*func), 1);
    func->name = strdup("return42");
    mod->functions_head = func;
    mod->functions_tail = func;
    mod->function_count = 1;

    /* Create basic block */
    fruity_basic_block_t *bb = mallocz(sizeof(*bb), 1);
    bb->block_id = 0;
    func->blocks_head = bb;
    func->blocks_tail = bb;
    func->block_count = 1;

    /* Create instructions */
    fruity_instruction_t *instr1 = mallocz(sizeof(*instr1), 1);
    instr1->opcode = FRUITY_RET;

    bb->instructions_head = instr1;
    bb->instructions_tail = instr1;
    bb->instruction_count = 1;

    /* Verify structure */
    assert(mod->function_count == 1);
    assert(func->block_count == 1);
    assert(bb->instruction_count == 1);
    assert(strcmp(mod->name, "test_module") == 0);
    assert(strcmp(func->name, "return42") == 0);

    printf("  ✓ Module has %lu functions\n", mod->function_count);
    printf("  ✓ Function '%s' has %lu blocks\n", func->name, func->block_count);
    printf("  ✓ Block has %lu instructions\n", bb->instruction_count);
    printf("  ✓ Fruity IR test PASSED\n\n");

    return 0;
}

/* Test 2: Fruity → QBE translation */
static int
test_fruity_to_qbe(void)
{
    printf("Test 2: Fruity → QBE translation...\n");

    /* Create simple module */
    fruity_module_t *mod = mallocz(sizeof(*mod), 1);
    mod->name = strdup("test");
    mod->version = 1;

    fruity_function_t *func = mallocz(sizeof(*func), 1);
    func->name = strdup("test_func");
    mod->functions_head = func;
    mod->function_count = 1;

    fruity_basic_block_t *bb = mallocz(sizeof(*bb), 1);
    bb->block_id = 0;
    func->blocks_head = bb;
    func->block_count = 1;

    fruity_instruction_t *ret = mallocz(sizeof(*ret), 1);
    ret->opcode = FRUITY_RET;
    bb->instructions_head = ret;
    bb->instruction_count = 1;

    /* Allocate page and translate */
    u8int page[4096];
    memset(page, 0, sizeof(page));

    /* Simulate physical address */
    uintptr fake_pa = (uintptr)page - mock_hhdm_offset;

    char errbuf[256];
    int result = fruity_to_qbe(mod, fake_pa, errbuf, sizeof(errbuf));

    if(result != 0) {
        printf("  ✗ Translation failed: %s\n", errbuf);
        return -1;
    }

    /* Verify QBE IL was generated */
    char *qbe_il = (char*)page;
    printf("  Generated QBE IL:\n");
    printf("  %s\n", qbe_il);

    assert(strstr(qbe_il, "export function") != NULL);
    assert(strstr(qbe_il, "$test_func") != NULL);
    assert(strstr(qbe_il, "ret") != NULL);

    printf("  ✓ QBE IL contains 'export function'\n");
    printf("  ✓ QBE IL contains function name\n");
    printf("  ✓ QBE IL contains 'ret'\n");
    printf("  ✓ Fruity→QBE test PASSED\n\n");

    return 0;
}

/* Test 3: QBE → x86-64 compilation */
static int
test_qbe_compile(void)
{
    printf("Test 3: QBE → x86-64 compilation...\n");

    /* Create minimal QBE IL */
    char qbe_il[4096];
    snprintf(qbe_il, sizeof(qbe_il),
        "export function w $test() {\n"
        "@start\n"
        "    ret 42\n"
        "}\n");

    u8int code_page[4096];
    memset(code_page, 0, sizeof(code_page));

    uintptr qbe_pa = (uintptr)qbe_il - mock_hhdm_offset;
    uintptr code_pa = (uintptr)code_page - mock_hhdm_offset;

    char errbuf[256];
    int result = qbe_compile_page(qbe_pa, code_pa, errbuf, sizeof(errbuf));

    if(result != 0) {
        printf("  ✗ Compilation failed: %s\n", errbuf);
        return -1;
    }

    /* Dump generated code */
    printf("  Generated x86-64 code (hex):\n  ");
    for(int i = 0; i < 32; i++) {
        printf("%02x ", code_page[i]);
        if((i+1) % 16 == 0 && i < 31) printf("\n  ");
    }
    printf("\n");

    /* Verify prologue */
    assert(code_page[0] == 0x55);  // push rbp
    assert(code_page[1] == 0x48);  // REX.W
    assert(code_page[2] == 0x89);  // mov rbp, rsp (part 1)
    assert(code_page[3] == 0xE5);  // mov rbp, rsp (part 2)

    printf("  ✓ Code starts with 'push rbp' (0x55)\n");
    printf("  ✓ Followed by 'mov rbp, rsp' (0x48 0x89 0xE5)\n");
    printf("  ✓ QBE→x86-64 test PASSED\n\n");

    return 0;
}

/* Main test driver */
int
main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("========================================\n");
    printf("CLR Compilation Pipeline Unit Tests\n");
    printf("========================================\n\n");

    int failures = 0;

    if(test_fruity_ir() != 0) failures++;
    if(test_fruity_to_qbe() != 0) failures++;
    if(test_qbe_compile() != 0) failures++;

    printf("========================================\n");
    if(failures == 0) {
        printf("✓ ALL TESTS PASSED (%d/3)\n", 3);
        printf("========================================\n");
        return 0;
    } else {
        printf("✗ %d TESTS FAILED\n", failures);
        printf("========================================\n");
        return 1;
    }
}
