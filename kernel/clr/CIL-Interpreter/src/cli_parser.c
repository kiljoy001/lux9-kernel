#include "../include/cli_parser.h"
#include <stdlib.h>
#include <string.h>

// Function to parse CLI header from binary data
cli_header_t* parse_cli_header(const uint8_t* data, size_t size) {
    // Check if we have enough data for basic CLI header
    if (data == NULL || size < sizeof(uint32_t)) {
        // Return NULL for completely invalid input
        if (data == NULL) {
            return NULL;
        }
        // For small size, still allocate structure but mark as invalid
        cli_header_t* header = (cli_header_t*)malloc(sizeof(cli_header_t));
        if (header == NULL) {
            return NULL;
        }
        memset(header, 0, sizeof(cli_header_t));
        header->is_valid = false;
        return header;
    }
    
    // Allocate memory for the header structure
    cli_header_t* header = (cli_header_t*)malloc(sizeof(cli_header_t));
    if (header == NULL) {
        return NULL;
    }
    
    // Initialize the structure
    memset(header, 0, sizeof(cli_header_t));
    
    // Check if we have enough data for the full CLI header
    if (size < 72) {  // Minimum size for CLI header
        header->is_valid = false;
        return header;
    }
    
    // Parse CLI header fields (little-endian format)
    header->cb = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
    header->major_runtime_version = (uint16_t)(data[4] | (data[5] << 8));
    header->minor_runtime_version = (uint16_t)(data[6] | (data[7] << 8));
    header->meta_data_rva = (uint32_t)(data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24));
    header->meta_data_size = (uint32_t)(data[12] | (data[13] << 8) | (data[14] << 16) | (data[15] << 24));
    header->flags = (uint32_t)(data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24));
    header->entry_point_token = (uint32_t)(data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24));
    header->resources_rva = (uint32_t)(data[24] | (data[25] << 8) | (data[26] << 16) | (data[27] << 24));
    header->resources_size = (uint32_t)(data[28] | (data[29] << 8) | (data[30] << 16) | (data[31] << 24));
    header->strong_name_rva = (uint32_t)(data[32] | (data[33] << 8) | (data[34] << 16) | (data[35] << 24));
    header->strong_name_size = (uint32_t)(data[36] | (data[37] << 8) | (data[38] << 16) | (data[39] << 24));
    header->code_manager_table_rva = (uint32_t)(data[40] | (data[41] << 8) | (data[42] << 16) | (data[43] << 24));
    header->code_manager_table_size = (uint32_t)(data[44] | (data[45] << 8) | (data[46] << 16) | (data[47] << 24));
    header->vtable_fixups_rva = (uint32_t)(data[48] | (data[49] << 8) | (data[50] << 16) | (data[51] << 24));
    header->vtable_fixups_size = (uint32_t)(data[52] | (data[53] << 8) | (data[54] << 16) | (data[55] << 24));
    header->export_address_table_jumps_rva = (uint32_t)(data[56] | (data[57] << 8) | (data[58] << 16) | (data[59] << 24));
    header->export_address_table_jumps_size = (uint32_t)(data[60] | (data[61] << 8) | (data[62] << 16) | (data[63] << 24));
    header->managed_native_header_rva = (uint32_t)(data[64] | (data[65] << 8) | (data[66] << 16) | (data[67] << 24));
    header->managed_native_header_size = (uint32_t)(data[68] | (data[69] << 8) | (data[70] << 16) | (data[71] << 24));
    
    // Mark as valid
    header->is_valid = true;
    
    return header;
}

// Function to free CLI header structure
void free_cli_header(cli_header_t* header) {
    if (header != NULL) {
        free(header);
    }
}

// Helper functions to check flags
bool cli_header_is_il_only(const cli_header_t* header) {
    return header != NULL && header->is_valid && (header->flags & COMIMAGE_FLAGS_ILONLY) != 0;
}

bool cli_header_is_32bit_required(const cli_header_t* header) {
    return header != NULL && header->is_valid && (header->flags & COMIMAGE_FLAGS_32BITREQUIRED) != 0;
}

bool cli_header_is_library(const cli_header_t* header) {
    return header != NULL && header->is_valid && (header->flags & COMIMAGE_FLAGS_IL_LIBRARY) != 0;
}

bool cli_header_is_strong_name_signed(const cli_header_t* header) {
    return header != NULL && header->is_valid && (header->flags & COMIMAGE_FLAGS_STRONGNAMESIGNED) != 0;
}

bool cli_header_has_native_entrypoint(const cli_header_t* header) {
    return header != NULL && header->is_valid && (header->flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT) != 0;
}