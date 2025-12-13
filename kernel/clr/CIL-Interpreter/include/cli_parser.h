#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// CLI Header structure (ECMA-335 II.25.3.3)
typedef struct {
    uint32_t cb;                    // Size of the header in bytes
    uint16_t major_runtime_version; // Major version of the runtime
    uint16_t minor_runtime_version; // Minor version of the runtime
    uint32_t meta_data_rva;         // RVA to the metadata root
    uint32_t meta_data_size;        // Size of the metadata root
    uint32_t flags;                 // CLI header flags
    uint32_t entry_point_token;     // Entry point token (MethodDef or File)
    uint32_t resources_rva;         // RVA to resources
    uint32_t resources_size;        // Size of resources
    uint32_t strong_name_rva;       // RVA to strong name signature
    uint32_t strong_name_size;      // Size of strong name signature
    uint32_t code_manager_table_rva; // RVA to code manager table
    uint32_t code_manager_table_size; // Size of code manager table
    uint32_t vtable_fixups_rva;     // RVA to Vtable fixups
    uint32_t vtable_fixups_size;    // Size of Vtable fixups
    uint32_t export_address_table_jumps_rva; // RVA to export address table jumps
    uint32_t export_address_table_jumps_size; // Size of export address table jumps
    uint32_t managed_native_header_rva; // RVA to managed native header
    uint32_t managed_native_header_size; // Size of managed native header
    bool is_valid;                  // Flag indicating if this is a valid CLI header
} cli_header_t;

// CLI header flags
#define COMIMAGE_FLAGS_ILONLY           0x00000001
#define COMIMAGE_FLAGS_32BITREQUIRED    0x00000002
#define COMIMAGE_FLAGS_IL_LIBRARY       0x00000004
#define COMIMAGE_FLAGS_STRONGNAMESIGNED 0x00000008
#define COMIMAGE_FLAGS_NATIVE_ENTRYPOINT 0x00000010
#define COMIMAGE_FLAGS_TRACKDEBUGDATA   0x00010000

// Function prototypes
cli_header_t* parse_cli_header(const uint8_t* data, size_t size);
void free_cli_header(cli_header_t* header);

// Helper functions to check flags
bool cli_header_is_il_only(const cli_header_t* header);
bool cli_header_is_32bit_required(const cli_header_t* header);
bool cli_header_is_library(const cli_header_t* header);
bool cli_header_is_strong_name_signed(const cli_header_t* header);
bool cli_header_has_native_entrypoint(const cli_header_t* header);

#endif // CLI_PARSER_H