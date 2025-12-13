#ifndef PE_PARSER_H
#define PE_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// PE file signature "PE\0\0"
#define PE_SIGNATURE 0x00004550

// MS-DOS signature "MZ"
#define DOS_SIGNATURE 0x5A4D

// Structure to represent a PE file header
typedef struct {
    uint16_t dos_signature;     // "MZ" signature
    uint32_t pe_signature;      // "PE\0\0" signature
    bool is_valid_pe;           // Flag indicating if this is a valid PE file
} pe_header_t;

// Function prototypes
pe_header_t* parse_pe_header(const uint8_t* data, size_t size);
void free_pe_header(pe_header_t* header);

#endif // PE_PARSER_H