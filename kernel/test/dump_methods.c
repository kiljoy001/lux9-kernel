/* dump_methods.c - Dump all methods from a .NET DLL */

#include "../clr/il_parser.h"
#include "../clr/il_disasm.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <dll>\n", argv[0]);
        return 1;
    }

    il_error_t error;
    il_assembly_t *assembly = il_parse_assembly(argv[1], &error);

    if (!assembly) {
        fprintf(stderr, "Error: %s\n", il_error_string(error));
        return 1;
    }

    printf("=== Assembly Info ===\n");
    il_dump_assembly_info(assembly);
    printf("\n");

    // Try tokens from 0x06000001 through 0x060000FF (MethodDef table)
    printf("=== Methods ===\n");
    for (uint32_t i = 1; i <= 255; i++) {
        uint32_t token = 0x06000000 | i;
        il_method_t *method = il_get_method_by_token(assembly, token);

        if (method) {
            printf("\n[Token 0x%08x] %s\n", token, method->name ? method->name : "<unnamed>");
            printf("  IL Code: %zu bytes, Max Stack: %u\n", method->il_code_size, method->max_stack);

            if (method->il_code_size > 0) {
                printf("  IL Disassembly:\n");
                il_disassemble_method(method);
            }

            il_free_method(method);
        }
    }

    il_free_assembly(assembly);
    return 0;
}
