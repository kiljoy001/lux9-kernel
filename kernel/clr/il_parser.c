/* il_parser.c - .NET PE/COFF and CLI Metadata Parser
 *
 * Implementation of ECMA-335 compliant assembly parser.
 * Can read .NET DLLs/EXEs and extract IL bytecode.
 */

#include "il_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== Kernel vs Userspace ========== */
#ifdef KERNEL
#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#define IL_MALLOC(size) xalloc(size)
#define IL_FREE(ptr) free(ptr)
#define IL_PRINT(fmt, ...) print(fmt, ##__VA_ARGS__)
#else
#define IL_MALLOC(size) malloc(size)
#define IL_FREE(ptr) free(ptr)
#define IL_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

/* ========== Helper Macros ========== */

#define READ_UINT16(ptr) (*(uint16_t*)(ptr))
#define READ_UINT32(ptr) (*(uint32_t*)(ptr))
#define READ_UINT64(ptr) (*(uint64_t*)(ptr))

/* ========== Error Handling ========== */

const char* il_error_string(il_error_t error) {
    switch (error) {
        case IL_OK: return "Success";
        case IL_ERROR_FILE_NOT_FOUND: return "File not found";
        case IL_ERROR_INVALID_PE: return "Invalid PE file";
        case IL_ERROR_INVALID_CLI: return "Invalid CLI header";
        case IL_ERROR_INVALID_METADATA: return "Invalid metadata";
        case IL_ERROR_METHOD_NOT_FOUND: return "Method not found";
        case IL_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case IL_ERROR_INVALID_IL: return "Invalid IL bytecode";
        default: return "Unknown error";
    }
}

/* ========== RVA/Offset Conversion ========== */

pe_section_header_t* il_get_section_by_rva(il_assembly_t *assembly, uint32_t rva) {
    for (uint16_t i = 0; i < assembly->section_count; i++) {
        pe_section_header_t *section = &assembly->sections[i];
        uint32_t section_start = section->virtual_address;
        uint32_t section_end = section_start + section->virtual_size;

        if (rva >= section_start && rva < section_end) {
            return section;
        }
    }
    return NULL;
}

uint32_t il_rva_to_offset(il_assembly_t *assembly, uint32_t rva) {
    pe_section_header_t *section = il_get_section_by_rva(assembly, rva);
    if (section == NULL) {
        return 0;
    }

    uint32_t offset_in_section = rva - section->virtual_address;
    return section->pointer_to_raw_data + offset_in_section;
}

/* ========== PE/COFF Parsing ========== */

static il_error_t parse_pe_header(il_assembly_t *assembly) {
    uint8_t *data = assembly->data;
    size_t size = assembly->size;

    // Check DOS header
    if (size < 0x40) {
        return IL_ERROR_INVALID_PE;
    }

    // DOS signature: "MZ"
    if (data[0] != 'M' || data[1] != 'Z') {
        return IL_ERROR_INVALID_PE;
    }

    // Get PE header offset from DOS header
    uint32_t pe_offset = READ_UINT32(&data[0x3C]);
    if (pe_offset + sizeof(uint32_t) + sizeof(pe_coff_header_t) > size) {
        return IL_ERROR_INVALID_PE;
    }

    // Check PE signature
    uint32_t pe_sig = READ_UINT32(&data[pe_offset]);
    if (pe_sig != PE_SIGNATURE) {
        return IL_ERROR_INVALID_PE;
    }

    // Parse COFF header
    uint8_t *coff_ptr = &data[pe_offset + 4];
    memcpy(&assembly->coff_header, coff_ptr, sizeof(pe_coff_header_t));

    // Parse optional header (handle both PE32 and PE32+)
    uint8_t *opt_ptr = coff_ptr + sizeof(pe_coff_header_t);

    // Read magic to determine PE32 vs PE32+
    uint16_t magic = READ_UINT16(opt_ptr);
    assembly->optional_header.magic = magic;

    // Manually parse fields based on format
    uint8_t *p = opt_ptr;
    assembly->optional_header.major_linker_version = p[2];
    assembly->optional_header.minor_linker_version = p[3];
    assembly->optional_header.size_of_code = READ_UINT32(p + 4);
    assembly->optional_header.size_of_initialized_data = READ_UINT32(p + 8);
    assembly->optional_header.size_of_uninitialized_data = READ_UINT32(p + 12);
    assembly->optional_header.address_of_entry_point = READ_UINT32(p + 16);
    assembly->optional_header.base_of_code = READ_UINT32(p + 20);

    // Data directories location depends on format
    uint8_t *data_dir_ptr;
    if (magic == 0x10B) {
        // PE32: has base_of_data field, 32-bit image_base, offsets differ
        uint32_t base_of_data = READ_UINT32(p + 24);
        (void)base_of_data; // unused
        assembly->optional_header.image_base = READ_UINT32(p + 28);
        data_dir_ptr = p + 96;  // PE32: data dirs at offset 96
    } else if (magic == 0x20B) {
        // PE32+: no base_of_data, 64-bit image_base
        assembly->optional_header.image_base = READ_UINT64(p + 24);
        data_dir_ptr = p + 112;  // PE32+: data dirs at offset 112
    } else {
        return IL_ERROR_INVALID_PE;
    }

    // Read data directories (same for both formats after offset adjustment)
    for (int i = 0; i < PE_DIRECTORY_COUNT; i++) {
        assembly->optional_header.data_directories[i].virtual_address =
            READ_UINT32(data_dir_ptr + i*8);
        assembly->optional_header.data_directories[i].size =
            READ_UINT32(data_dir_ptr + i*8 + 4);
    }

    // Parse section headers
    assembly->section_count = assembly->coff_header.number_of_sections;
    assembly->sections = IL_MALLOC(sizeof(pe_section_header_t) * assembly->section_count);
    if (assembly->sections == NULL) {
        return IL_ERROR_OUT_OF_MEMORY;
    }

    uint8_t *section_ptr = opt_ptr + assembly->coff_header.size_of_optional_header;
    for (uint16_t i = 0; i < assembly->section_count; i++) {
        memcpy(&assembly->sections[i], section_ptr, sizeof(pe_section_header_t));
        section_ptr += sizeof(pe_section_header_t);
    }

    return IL_OK;
}

/* ========== CLI Header Parsing ========== */

static il_error_t parse_cli_header(il_assembly_t *assembly) {
    // Get CLI header RVA from data directories
    pe_data_directory_t *cli_dir = &assembly->optional_header.data_directories[PE_DIRECTORY_CLR_RUNTIME];

    if (cli_dir->size == 0) {
        return IL_ERROR_INVALID_CLI;  // Not a .NET assembly
    }

    uint32_t cli_offset = il_rva_to_offset(assembly, cli_dir->virtual_address);
    if (cli_offset == 0 || cli_offset + sizeof(cli_header_t) > assembly->size) {
        return IL_ERROR_INVALID_CLI;
    }

    memcpy(&assembly->cli_header, &assembly->data[cli_offset], sizeof(cli_header_t));

    return IL_OK;
}

/* ========== Metadata Parsing ========== */

static il_error_t parse_metadata_header(il_assembly_t *assembly) {
    // Get metadata RVA from CLI header
    uint32_t metadata_rva = assembly->cli_header.metadata.virtual_address;
    uint32_t metadata_offset = il_rva_to_offset(assembly, metadata_rva);

    if (metadata_offset == 0) {
        return IL_ERROR_INVALID_METADATA;
    }

    uint8_t *metadata_ptr = &assembly->data[metadata_offset];

    // Parse metadata header
    uint32_t signature = READ_UINT32(metadata_ptr);
    if (signature != 0x424A5342) {  // "BSJB"
        return IL_ERROR_INVALID_METADATA;
    }

    assembly->metadata_header.signature = signature;
    assembly->metadata_header.major_version = READ_UINT16(metadata_ptr + 4);
    assembly->metadata_header.minor_version = READ_UINT16(metadata_ptr + 6);
    assembly->metadata_header.reserved = READ_UINT32(metadata_ptr + 8);
    assembly->metadata_header.version_length = READ_UINT32(metadata_ptr + 12);

    // Skip version string (aligned to 4 bytes)
    uint32_t version_len = assembly->metadata_header.version_length;
    uint32_t aligned_version_len = (version_len + 3) & ~3;

    metadata_ptr += 16 + aligned_version_len;

    // Parse stream headers
    assembly->metadata_header.flags = READ_UINT16(metadata_ptr);
    assembly->metadata_header.stream_count = READ_UINT16(metadata_ptr + 2);
    assembly->stream_count = assembly->metadata_header.stream_count;

    metadata_ptr += 4;

    // Allocate stream array
    assembly->streams = IL_MALLOC(sizeof(metadata_stream_t) * assembly->stream_count);
    if (assembly->streams == NULL) {
        return IL_ERROR_OUT_OF_MEMORY;
    }

    // Parse each stream header
    for (uint16_t i = 0; i < assembly->stream_count; i++) {
        assembly->streams[i].offset = READ_UINT32(metadata_ptr);
        assembly->streams[i].size = READ_UINT32(metadata_ptr + 4);
        metadata_ptr += 8;

        // Read stream name (null-terminated, aligned to 4 bytes)
        assembly->streams[i].name = (char*)metadata_ptr;
        size_t name_len = strlen((char*)metadata_ptr) + 1;
        size_t aligned_name_len = (name_len + 3) & ~3;
        metadata_ptr += aligned_name_len;

        // Calculate actual data pointer
        assembly->streams[i].data = &assembly->data[metadata_offset + assembly->streams[i].offset];

        // Identify heap types
        if (strcmp(assembly->streams[i].name, "#Strings") == 0) {
            assembly->strings_heap = assembly->streams[i].data;
            assembly->strings_heap_size = assembly->streams[i].size;
        } else if (strcmp(assembly->streams[i].name, "#Blob") == 0) {
            assembly->blob_heap = assembly->streams[i].data;
            assembly->blob_heap_size = assembly->streams[i].size;
        } else if (strcmp(assembly->streams[i].name, "#GUID") == 0) {
            assembly->guid_heap = assembly->streams[i].data;
            assembly->guid_heap_size = assembly->streams[i].size;
        } else if (strcmp(assembly->streams[i].name, "#US") == 0) {
            assembly->us_heap = assembly->streams[i].data;
            assembly->us_heap_size = assembly->streams[i].size;
        } else if (strcmp(assembly->streams[i].name, "#~") == 0 ||
                   strcmp(assembly->streams[i].name, "#-") == 0) {
            // Metadata tables stream
            assembly->tables_data = assembly->streams[i].data;
            assembly->tables_data_size = assembly->streams[i].size;
        }
    }

    return IL_OK;
}

/* ========== Metadata Tables Parsing ========== */

static il_error_t parse_metadata_tables(il_assembly_t *assembly) {
    if (assembly->tables_data == NULL) {
        return IL_ERROR_INVALID_METADATA;
    }

    uint8_t *tables_ptr = assembly->tables_data;

    // Parse tables header
    assembly->tables_header.reserved = READ_UINT32(tables_ptr);
    assembly->tables_header.major_version = tables_ptr[4];
    assembly->tables_header.minor_version = tables_ptr[5];
    assembly->tables_header.heap_sizes = tables_ptr[6];
    assembly->tables_header.reserved2 = tables_ptr[7];
    assembly->tables_header.valid_mask = READ_UINT64(tables_ptr + 8);
    assembly->tables_header.sorted_mask = READ_UINT64(tables_ptr + 16);

    tables_ptr += 24;

    // Count tables and allocate row counts
    int table_count = 0;
    for (int i = 0; i < 64; i++) {
        if (assembly->tables_header.valid_mask & (1ULL << i)) {
            table_count++;
        }
    }

    assembly->tables_header.row_counts = IL_MALLOC(sizeof(uint32_t) * 64);
    if (assembly->tables_header.row_counts == NULL) {
        return IL_ERROR_OUT_OF_MEMORY;
    }

    memset(assembly->tables_header.row_counts, 0, sizeof(uint32_t) * 64);

    // Read row counts
    for (int i = 0; i < 64; i++) {
        if (assembly->tables_header.valid_mask & (1ULL << i)) {
            assembly->tables_header.row_counts[i] = READ_UINT32(tables_ptr);
            tables_ptr += 4;
        }
    }

    return IL_OK;
}

/* ========== String/Blob Heap Access ========== */

const char* il_get_string(il_assembly_t *assembly, uint32_t index) {
    if (index >= assembly->strings_heap_size) {
        return NULL;
    }
    return (const char*)&assembly->strings_heap[index];
}

const uint8_t* il_get_blob(il_assembly_t *assembly, uint32_t index, uint32_t *size_out) {
    if (index >= assembly->blob_heap_size) {
        return NULL;
    }

    uint8_t *blob_ptr = &assembly->blob_heap[index];

    // Blob size encoding (compressed integer)
    uint32_t size = 0;
    if ((blob_ptr[0] & 0x80) == 0) {
        size = blob_ptr[0];
        blob_ptr += 1;
    } else if ((blob_ptr[0] & 0xC0) == 0x80) {
        size = ((blob_ptr[0] & 0x3F) << 8) | blob_ptr[1];
        blob_ptr += 2;
    } else if ((blob_ptr[0] & 0xE0) == 0xC0) {
        size = ((blob_ptr[0] & 0x1F) << 24) | (blob_ptr[1] << 16) |
               (blob_ptr[2] << 8) | blob_ptr[3];
        blob_ptr += 4;
    }

    if (size_out) {
        *size_out = size;
    }

    return blob_ptr;
}

/* ========== MethodDef Table Parsing ========== */

static uint32_t read_table_index(uint8_t **ptr, int wide) {
    uint32_t value;
    if (wide) {
        value = READ_UINT32(*ptr);
        *ptr += 4;
    } else {
        value = READ_UINT16(*ptr);
        *ptr += 2;
    }
    return value;
}

static methoddef_row_t* parse_methoddef_table(il_assembly_t *assembly, size_t *row_count_out) {
    size_t row_count = assembly->tables_header.row_counts[TABLE_METHODDEF];
    if (row_count == 0) {
        *row_count_out = 0;
        return NULL;
    }

    // Determine index sizes based on heap_sizes byte
    int string_wide = (assembly->tables_header.heap_sizes & 0x01) != 0;
    int blob_wide = (assembly->tables_header.heap_sizes & 0x04) != 0;
    int param_wide = assembly->tables_header.row_counts[TABLE_PARAM] >= 0x10000;

    // Allocate rows
    methoddef_row_t *rows = IL_MALLOC(sizeof(methoddef_row_t) * row_count);
    if (rows == NULL) {
        *row_count_out = 0;
        return NULL;
    }

    // Find MethodDef table start in tables stream
    // Tables are stored in order, so we need to skip previous tables
    uint8_t *table_ptr = assembly->tables_data;

    // Skip tables header (24 bytes + row counts)
    table_ptr += 24;
    for (int i = 0; i < 64; i++) {
        if (assembly->tables_header.valid_mask & (1ULL << i)) {
            table_ptr += 4;  // Skip row count
        }
    }

    // Now skip all tables before MethodDef (table 0x06)
    for (int table_id = 0; table_id < TABLE_METHODDEF; table_id++) {
        if (!(assembly->tables_header.valid_mask & (1ULL << table_id))) {
            continue;  // Table doesn't exist
        }

        size_t table_rows = assembly->tables_header.row_counts[table_id];
        size_t row_size = 0;

        // Calculate row size for each table type
        switch (table_id) {
            case TABLE_MODULE:  // 0x00
                row_size = 2 + (string_wide ? 4 : 2) * 3;
                break;
            case TABLE_TYPEREF:  // 0x01
                row_size = 2 + (string_wide ? 4 : 2) * 2;
                break;
            case TABLE_TYPEDEF:  // 0x02
                row_size = 4 + (string_wide ? 4 : 2) * 2 + 2 + 2;
                break;
            case TABLE_FIELD:  // 0x04
                row_size = 2 + (string_wide ? 4 : 2) + (blob_wide ? 4 : 2);
                break;
            // Add more tables as needed
            default:
                // Unknown table, skip conservatively
                row_size = 8;
                break;
        }

        table_ptr += row_size * table_rows;
    }

    // Now we're at the MethodDef table
    // MethodDef row format:
    //   RVA (4 bytes)
    //   ImplFlags (2 bytes)
    //   Flags (2 bytes)
    //   Name (string index - 2 or 4 bytes)
    //   Signature (blob index - 2 or 4 bytes)
    //   ParamList (param index - 2 or 4 bytes)

    for (size_t i = 0; i < row_count; i++) {
        rows[i].rva = READ_UINT32(table_ptr);
        table_ptr += 4;

        rows[i].impl_flags = READ_UINT16(table_ptr);
        table_ptr += 2;

        rows[i].flags = READ_UINT16(table_ptr);
        table_ptr += 2;

        rows[i].name_index = read_table_index(&table_ptr, string_wide);
        rows[i].signature_index = read_table_index(&table_ptr, blob_wide);
        rows[i].param_list = read_table_index(&table_ptr, param_wide);
    }

    *row_count_out = row_count;
    return rows;
}

/* ========== Method Parsing ========== */

static il_method_t* parse_method(il_assembly_t *assembly, uint32_t rva, const char *name) {
    if (rva == 0) {
        return NULL;  // Abstract or runtime-provided method
    }

    uint32_t method_offset = il_rva_to_offset(assembly, rva);
    if (method_offset == 0) {
        return NULL;
    }

    uint8_t *method_ptr = &assembly->data[method_offset];
    il_method_t *method = IL_MALLOC(sizeof(il_method_t));
    if (method == NULL) {
        return NULL;
    }

    memset(method, 0, sizeof(il_method_t));

    // Copy method name
    method->name = IL_MALLOC(strlen(name) + 1);
    if (method->name) {
        strcpy(method->name, name);
    }

    // Parse method header
    uint8_t flags = method_ptr[0];

    if ((flags & 0x03) == 0x02) {
        // Tiny format
        method->flags = 0x02;
        method->max_stack = 8;
        method->il_code_size = flags >> 2;
        method->local_var_sig_token = 0;
        method->il_code = method_ptr + 1;
    } else if ((flags & 0x03) == 0x03) {
        // Fat format
        method->flags = 0x03;
        uint16_t header_size = (flags & 0xF0) >> 4;
        method->max_stack = READ_UINT16(method_ptr + 2);
        method->il_code_size = READ_UINT32(method_ptr + 4);
        method->local_var_sig_token = READ_UINT32(method_ptr + 8);
        method->il_code = method_ptr + (header_size * 4);
    } else {
        IL_FREE(method);
        return NULL;
    }

    return method;
}

il_method_t* il_get_method_by_token(il_assembly_t *assembly, uint32_t token) {
    // Token format: [table_kind:8][row_index:24]
    uint8_t table_kind = (token >> 24) & 0xFF;
    uint32_t row_index = token & 0x00FFFFFF;

    if (table_kind != TABLE_METHODDEF) {
        return NULL;
    }

    // Parse MethodDef table
    size_t method_count;
    methoddef_row_t *methods = parse_methoddef_table(assembly, &method_count);
    if (methods == NULL || row_index == 0 || row_index > method_count) {
        if (methods) IL_FREE(methods);
        return NULL;
    }

    // Get row (1-indexed)
    methoddef_row_t *row = &methods[row_index - 1];

    // Get method name from #Strings heap
    const char *method_name = il_get_string(assembly, row->name_index);
    if (method_name == NULL) {
        IL_FREE(methods);
        return NULL;
    }

    // Parse method body
    il_method_t *method = parse_method(assembly, row->rva, method_name);

    IL_FREE(methods);
    return method;
}

il_method_t* il_get_method(il_assembly_t *assembly, const char *name) {
    // Parse MethodDef table
    size_t method_count;
    methoddef_row_t *methods = parse_methoddef_table(assembly, &method_count);
    if (methods == NULL) {
        return NULL;
    }

    // Search for method by name
    for (size_t i = 0; i < method_count; i++) {
        const char *method_name = il_get_string(assembly, methods[i].name_index);
        if (method_name && strcmp(method_name, name) == 0) {
            // Found it!
            il_method_t *method = parse_method(assembly, methods[i].rva, name);
            IL_FREE(methods);
            return method;
        }
    }

    IL_FREE(methods);
    return NULL;
}

/* ========== Assembly Parsing (Main Entry Point) ========== */

il_assembly_t* il_parse_assembly_memory(const uint8_t *data, size_t size, il_error_t *error) {
    il_assembly_t *assembly = IL_MALLOC(sizeof(il_assembly_t));
    if (assembly == NULL) {
        if (error) *error = IL_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    memset(assembly, 0, sizeof(il_assembly_t));
    assembly->data = (uint8_t*)data;
    assembly->size = size;

    // Parse PE/COFF headers
    il_error_t err = parse_pe_header(assembly);
    if (err != IL_OK) {
        if (error) *error = err;
        IL_FREE(assembly);
        return NULL;
    }

    // Parse CLI header
    err = parse_cli_header(assembly);
    if (err != IL_OK) {
        if (error) *error = err;
        il_free_assembly(assembly);
        return NULL;
    }

    // Parse metadata
    err = parse_metadata_header(assembly);
    if (err != IL_OK) {
        if (error) *error = err;
        il_free_assembly(assembly);
        return NULL;
    }

    // Parse metadata tables
    err = parse_metadata_tables(assembly);
    if (err != IL_OK) {
        if (error) *error = err;
        il_free_assembly(assembly);
        return NULL;
    }

    if (error) *error = IL_OK;
    return assembly;
}

il_assembly_t* il_parse_assembly(const char *path, il_error_t *error) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        if (error) *error = IL_ERROR_FILE_NOT_FOUND;
        return NULL;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Read file data
    uint8_t *data = IL_MALLOC(size);
    if (data == NULL) {
        fclose(f);
        if (error) *error = IL_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    fread(data, 1, size, f);
    fclose(f);

    // Parse assembly
    il_assembly_t *assembly = il_parse_assembly_memory(data, size, error);
    if (assembly == NULL) {
        IL_FREE(data);
        return NULL;
    }

    return assembly;
}

/* ========== Cleanup ========== */

void il_free_method(il_method_t *method) {
    if (method) {
        if (method->name) {
            IL_FREE(method->name);
        }
        IL_FREE(method);
    }
}

void il_free_assembly(il_assembly_t *assembly) {
    if (assembly) {
        if (assembly->sections) {
            IL_FREE(assembly->sections);
        }
        if (assembly->streams) {
            IL_FREE(assembly->streams);
        }
        if (assembly->tables_header.row_counts) {
            IL_FREE(assembly->tables_header.row_counts);
        }
        if (assembly->methods) {
            for (size_t i = 0; i < assembly->method_count; i++) {
                il_free_method(&assembly->methods[i]);
            }
            IL_FREE(assembly->methods);
        }
        if (assembly->data) {
            IL_FREE((void*)assembly->data);
        }
        IL_FREE(assembly);
    }
}

/* ========== Debug Utilities ========== */

void il_dump_assembly_info(il_assembly_t *assembly) {
    IL_PRINT("=== .NET Assembly Info ===\n");
    IL_PRINT("PE: %d sections\n", assembly->section_count);
    IL_PRINT("CLI Version: %d.%d\n",
             assembly->cli_header.major_runtime_version,
             assembly->cli_header.minor_runtime_version);
    IL_PRINT("Metadata Streams: %d\n", assembly->stream_count);
    for (uint16_t i = 0; i < assembly->stream_count; i++) {
        IL_PRINT("  [%d] %s (size=%d)\n",
                 i, assembly->streams[i].name, assembly->streams[i].size);
    }
    IL_PRINT("Metadata Tables: valid_mask=0x%llx\n",
             (unsigned long long)assembly->tables_header.valid_mask);
}

void il_dump_method(il_method_t *method) {
    IL_PRINT("=== Method: %s ===\n", method->name ? method->name : "<unknown>");
    IL_PRINT("Max stack: %d\n", method->max_stack);
    IL_PRINT("IL code size: %d bytes\n", (int)method->il_code_size);
    IL_PRINT("IL bytecode:\n");

    for (size_t i = 0; i < method->il_code_size && i < 64; i++) {
        IL_PRINT("%02x ", method->il_code[i]);
        if ((i + 1) % 16 == 0) IL_PRINT("\n");
    }
    IL_PRINT("\n");
}
