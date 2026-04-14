/* list_methods.c - List all methods in a .NET DLL */

#include "../clr/il_parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <dll>\n", argv[0]);
        return 1;
    }

    il_error_t error;
    il_assembly_t *assembly = il_parse_assembly(argv[1], &error);

    if (assembly == NULL) {
        fprintf(stderr, "Error: %s\n", il_error_string(error));
        return 1;
    }

    printf("Successfully parsed: %s\n\n", argv[1]);
    printf("Methods found:\n");

    // Iterate through all methods in the MethodDef table
    uint32_t method_count = assembly->metadata_tables.methoddef_count;
    printf("Total methods: %u\n\n", method_count);

    for (uint32_t i = 0; i < method_count; i++) {
        // Tokens are 1-indexed, table is 0-indexed
        uint32_t token = 0x06000001 + i;  // 0x06 = MethodDef table

        il_method_t *method = il_get_method_by_token(assembly, token);
        if (method) {
            printf("  [%u] %s\n", i+1, method->name ? method->name : "<unnamed>");
            printf("      Token: 0x%08x\n", token);
            printf("      RVA: 0x%08x\n", method->rva);
            printf("      IL code size: %u bytes\n", method->code_size);

            if (method->code_size > 0 && method->code) {
                printf("      IL bytes: ");
                for (uint32_t j = 0; j < (method->code_size < 16 ? method->code_size : 16); j++) {
                    printf("%02x ", method->code[j]);
                }
                if (method->code_size > 16) printf("...");
                printf("\n");
            }
            printf("\n");

            il_free_method(method);
        }
    }

    il_free_assembly(assembly);
    return 0;
}
