#include "../include/pe_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Function to parse PE header from binary data
pe_header_t* parse_pe_header(const uint8_t* data, size_t size) {
    // Always allocate memory for the header structure, even if data is NULL or too small
    pe_header_t* header = (pe_header_t*)malloc(sizeof(pe_header_t));
    if (header == NULL) {
        return NULL;
    }
    
    // Initialize the structure
    memset(header, 0, sizeof(pe_header_t));
    
    // Check if we have data to parse
    if (data == NULL || size < 2) {
        header->is_valid_pe = false;
        return header;
    }
    
    // Read DOS signature (first 2 bytes)
    header->dos_signature = (uint16_t)(data[0] | (data[1] << 8));
    
    // If we have enough data, try to read PE signature
    if (size >= 0x3E) {
        // Get the offset to PE header from DOS header (at offset 0x3C)
        uint32_t pe_offset = (uint32_t)(data[0x3C] | (data[0x3D] << 8));
        
        // Check if the offset is reasonable and we have enough data
        if (pe_offset > 0 && pe_offset < size && size >= pe_offset + 4) {
            header->pe_signature = (uint32_t)(data[pe_offset] | 
                                             (data[pe_offset + 1] << 8) | 
                                             (data[pe_offset + 2] << 16) | 
                                             (data[pe_offset + 3] << 24));
        }
    }
    
    // Check if signatures are valid
    if (header->dos_signature == DOS_SIGNATURE && 
        header->pe_signature == PE_SIGNATURE) {
        header->is_valid_pe = true;
    } else {
        header->is_valid_pe = false;
    }
    
    return header;
}

// Function to free PE header structure
void free_pe_header(pe_header_t* header) {
    if (header != NULL) {
        free(header);
    }
}