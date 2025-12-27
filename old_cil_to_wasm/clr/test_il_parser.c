/* test_il_parser.c - Test IL parser with .NET assembly
 *
 * Compile:
 *   gcc -o test_il_parser test_il_parser.c il_parser.c il_disasm.c -I.
 *
 * Test:
 *   fsc test_hello.fs  # Compile F# to test_hello.dll
 *   ./test_il_parser test_hello.dll
 */

#include "il_parser.h"
#include "il_disasm.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <assembly.dll>\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];

    printf("Parsing: %s\n", path);

    il_error_t error;
    il_assembly_t *assembly = il_parse_assembly(path, &error);

    if (assembly == NULL) {
        fprintf(stderr, "Error: %s\n", il_error_string(error));
        return 1;
    }

    printf("Success! Assembly parsed.\n\n");

    // Dump assembly info
    il_dump_assembly_info(assembly);

    // Dump sections
    printf("\n=== Section Headers ===\n");
    for (int i = 0; i < assembly->section_count; i++) {
        pe_section_header_t *sect = &assembly->sections[i];
        printf("Section %d: %.8s\n", i, sect->name);
        printf("  VirtualAddress:   0x%08x\n", sect->virtual_address);
        printf("  VirtualSize:      0x%08x\n", sect->virtual_size);
        printf("  PointerToRawData: 0x%08x\n", sect->pointer_to_raw_data);
        printf("  SizeOfRawData:    0x%08x\n", sect->size_of_raw_data);
    }

    // Try to get entry point method
    printf("\n=== Entry Point ===\n");
    printf("Token: 0x%08x\n", assembly->cli_header.entry_point_token);

    // Get entry point method by token
    if (assembly->cli_header.entry_point_token != 0) {
        il_method_t *entry_method = il_get_method_by_token(assembly, assembly->cli_header.entry_point_token);
        if (entry_method) {
            printf("\n=== Entry Point Method ===\n");
            il_dump_method(entry_method);

            printf("\n=== Disassembly ===\n");
            il_disassemble_method(entry_method);

            il_free_method(entry_method);
        } else {
            printf("Failed to get entry point method\n");
        }
    }

    // Try to get method named "main"
    printf("\n=== Looking for 'main' method ===\n");
    il_method_t *main_method = il_get_method(assembly, "main");
    if (main_method) {
        il_dump_method(main_method);

        printf("\n=== Disassembly ===\n");
        il_disassemble_method(main_method);

        il_free_method(main_method);
    } else {
        printf("'main' method not found\n");
    }

    // Clean up
    il_free_assembly(assembly);

    printf("\nTest complete!\n");
    return 0;
}
