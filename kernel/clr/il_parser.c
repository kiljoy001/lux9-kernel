/* il_parser.c - .NET PE/COFF and CLI Metadata Parser
 *
 * Implementation of ECMA-335 compliant assembly parser.
 * Can read .NET DLLs/EXEs and extract IL bytecode.
 */

#include "il_parser.h"

/* ========== Kernel vs Userspace ========== */
#if defined(KERNEL) || defined(__PLAN9_KERNEL__)
/* Manual Plan 9 Types (avoiding include maze) */
#define _U_H_
#define nil ((void *)0)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
typedef u32int Rune;
#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#define USED(x)                                                                \
  if (x) {                                                                     \
  }

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;

#include "../../port/lib.h"
#include "../9front-pc64/mem.h"
#include "dat.h"
#include "fns.h"

#define IL_MALLOC(size) xalloc(size)
#define IL_FREE(ptr) xfree(ptr)
#define IL_PRINT(fmt, ...) print(fmt, ##__VA_ARGS__)

/* Provide kernel-compatible string functions */
#define strdup(s)                                                              \
  ({                                                                           \
    char *_d = xalloc(strlen(s) + 1);                                          \
    if (_d)                                                                    \
      strcpy(_d, s);                                                           \
    _d;                                                                        \
  })

#else
/* Userspace mode: use standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IL_MALLOC(size) malloc(size)
#define IL_FREE(ptr) free(ptr)
#define IL_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

/* ========== Helper Macros ========== */

#define READ_UINT16(ptr) (*(uint16_t *)(ptr))
#define READ_UINT32(ptr) (*(uint32_t *)(ptr))
#define READ_UINT64(ptr) (*(uint64_t *)(ptr))

/* ========== Error Handling ========== */

const char *il_error_string(il_error_t error) {
  switch (error) {
  case IL_OK:
    return "Success";
  case IL_ERROR_FILE_NOT_FOUND:
    return "File not found";
  case IL_ERROR_INVALID_PE:
    return "Invalid PE file";
  case IL_ERROR_INVALID_CLI:
    return "Invalid CLI header";
  case IL_ERROR_INVALID_METADATA:
    return "Invalid metadata";
  case IL_ERROR_METHOD_NOT_FOUND:
    return "Method not found";
  case IL_ERROR_OUT_OF_MEMORY:
    return "Out of memory";
  case IL_ERROR_INVALID_IL:
    return "Invalid IL bytecode";
  default:
    return "Unknown error";
  }
}

/* ========== RVA/Offset Conversion ========== */

pe_section_header_t *il_get_section_by_rva(il_assembly_t *assembly,
                                           uint32_t rva) {
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
  memmove(&assembly->coff_header, coff_ptr, sizeof(pe_coff_header_t));

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
    data_dir_ptr = p + 96; // PE32: data dirs at offset 96
  } else if (magic == 0x20B) {
    // PE32+: no base_of_data, 64-bit image_base
    assembly->optional_header.image_base = READ_UINT64(p + 24);
    data_dir_ptr = p + 112; // PE32+: data dirs at offset 112
  } else {
    return IL_ERROR_INVALID_PE;
  }

  // Read data directories (same for both formats after offset adjustment)
  for (int i = 0; i < PE_DIRECTORY_COUNT; i++) {
    assembly->optional_header.data_directories[i].virtual_address =
        READ_UINT32(data_dir_ptr + i * 8);
    assembly->optional_header.data_directories[i].size =
        READ_UINT32(data_dir_ptr + i * 8 + 4);
  }

  // Parse section headers
  assembly->section_count = assembly->coff_header.number_of_sections;
  assembly->sections =
      IL_MALLOC(sizeof(pe_section_header_t) * assembly->section_count);
  if (assembly->sections == NULL) {
    return IL_ERROR_OUT_OF_MEMORY;
  }

  uint8_t *section_ptr =
      opt_ptr + assembly->coff_header.size_of_optional_header;
  for (uint16_t i = 0; i < assembly->section_count; i++) {
    memmove(&assembly->sections[i], section_ptr, sizeof(pe_section_header_t));
    section_ptr += sizeof(pe_section_header_t);
  }

  return IL_OK;
}

/* ========== CLI Header Parsing ========== */

static il_error_t parse_cli_header(il_assembly_t *assembly) {
  // Get CLI header RVA from data directories
  pe_data_directory_t *cli_dir =
      &assembly->optional_header.data_directories[PE_DIRECTORY_CLR_RUNTIME];

  if (cli_dir->size == 0) {
    return IL_ERROR_INVALID_CLI; // Not a .NET assembly
  }

  uint32_t cli_offset = il_rva_to_offset(assembly, cli_dir->virtual_address);
  if (cli_offset == 0 || cli_offset + sizeof(cli_header_t) > assembly->size) {
    return IL_ERROR_INVALID_CLI;
  }

  memmove(&assembly->cli_header, &assembly->data[cli_offset],
          sizeof(cli_header_t));

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
  if (signature != 0x424A5342) { // "BSJB"
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
  assembly->streams =
      IL_MALLOC(sizeof(metadata_stream_t) * assembly->stream_count);
  if (assembly->streams == NULL) {
    return IL_ERROR_OUT_OF_MEMORY;
  }

  // Parse each stream header
  for (uint16_t i = 0; i < assembly->stream_count; i++) {
    assembly->streams[i].offset = READ_UINT32(metadata_ptr);
    assembly->streams[i].size = READ_UINT32(metadata_ptr + 4);
    metadata_ptr += 8;

    // Read stream name (null-terminated, aligned to 4 bytes)
    assembly->streams[i].name = (char *)metadata_ptr;
    size_t name_len = strlen((char *)metadata_ptr) + 1;
    size_t aligned_name_len = (name_len + 3) & ~3;
    metadata_ptr += aligned_name_len;

    // Calculate actual data pointer
    assembly->streams[i].data =
        &assembly->data[metadata_offset + assembly->streams[i].offset];

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

const char *il_get_string(il_assembly_t *assembly, uint32_t index) {
  if (index >= assembly->strings_heap_size) {
    return NULL;
  }
  return (const char *)&assembly->strings_heap[index];
}

uint32_t il_decode_compressed_uint(const uint8_t **data) {
  const uint8_t *ptr = *data;
  uint32_t val = 0;

  if ((*ptr & 0x80) == 0) {
    val = *ptr;
    *data += 1;
  } else if ((*ptr & 0xC0) == 0x80) {
    val = ((*ptr & 0x3F) << 8) | ptr[1];
    *data += 2;
  } else if ((*ptr & 0xE0) == 0xC0) {
    val = ((*ptr & 0x1F) << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    *data += 4;
  }
  return val;
}

const uint8_t *il_get_blob(il_assembly_t *assembly, uint32_t index,
                           uint32_t *size_out) {
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

const uint16_t *il_get_user_string_raw(il_assembly_t *assembly, uint32_t index,
                                       uint32_t *length) {
  if (index >= assembly->us_heap_size) {
    if (length)
      *length = 0;
    return NULL;
  }

  uint8_t *ptr = &assembly->us_heap[index];
  uint32_t bytes = 0;

  /* Decode compressed size */
  if ((ptr[0] & 0x80) == 0) {
    bytes = ptr[0];
    ptr += 1;
  } else if ((ptr[0] & 0xC0) == 0x80) {
    bytes = ((ptr[0] & 0x3F) << 8) | ptr[1];
    ptr += 2;
  } else if ((ptr[0] & 0xE0) == 0xC0) {
    bytes = ((ptr[0] & 0x1F) << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;
  }

  if (bytes == 0) {
    if (length)
      *length = 0;
    return (const uint16_t *)ptr; // Empty string
  }

  /* Last byte is terminal flag */
  if (bytes > 0)
    bytes--;

  if (length)
    *length = bytes / 2;
  return (const uint16_t *)ptr;
}

char *il_get_user_string(il_assembly_t *assembly, uint32_t index) {
  if (index >= assembly->us_heap_size) {
    return NULL;
  }

  uint8_t *ptr = &assembly->us_heap[index];
  uint32_t bytes = 0;

  /* Decode compressed size */
  if ((ptr[0] & 0x80) == 0) {
    bytes = ptr[0];
    ptr += 1;
  } else if ((ptr[0] & 0xC0) == 0x80) {
    bytes = ((ptr[0] & 0x3F) << 8) | ptr[1];
    ptr += 2;
  } else if ((ptr[0] & 0xE0) == 0xC0) {
    bytes = ((ptr[0] & 0x1F) << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;
  }

  if (bytes == 0)
    return strdup("");

  /* Last byte is the terminal byte (flag), ignore it for length */
  if (bytes > 0)
    bytes--;

  /* Convert UTF-16 buffer to ASCII/UTF-8 (char*) */
  /* US heap strings are UTF-16LE */
  uint16_t *chars = (uint16_t *)ptr;
  int char_count = bytes / 2;

  char *result = IL_MALLOC(char_count + 1);
  if (!result)
    return NULL;

  for (int i = 0; i < char_count; i++) {
    uint16_t c = chars[i];
    /* Simple truncation for now - full UTF-8 conversion would be better */
    result[i] = (c < 128) ? (char)c : '?';
  }
  result[char_count] = 0;

  return result;
}

/* ========== Metadata Table Helpers ========== */

static uint32_t get_coded_index_size(il_assembly_t *assembly, int tag_bits,
                                     metadata_table_kind_t *tables, int count) {
  uint32_t max_rows = 0;
  for (int i = 0; i < count; i++) {
    uint32_t rows = assembly->tables_header.row_counts[tables[i]];
    if (rows > max_rows)
      max_rows = rows;
  }
  return (max_rows < (1 << (16 - tag_bits))) ? 2 : 4;
}

static uint32_t get_table_row_size(il_assembly_t *assembly, int table_id) {
  int string_wide = (assembly->tables_header.heap_sizes & 0x01) != 0;
  int guid_wide = (assembly->tables_header.heap_sizes & 0x02) != 0;
  int blob_wide = (assembly->tables_header.heap_sizes & 0x04) != 0;

  switch (table_id) {
  case TABLE_MODULE: // 0x00
    return 2 + (string_wide ? 4 : 2) + (guid_wide ? 4 : 2) * 3;
  case TABLE_TYPEREF: // 0x01
  {
    metadata_table_kind_t refs[] = {TABLE_MODULE, 0x1A /*ModuleRef*/,
                                    TABLE_ASSEMBLYREF, TABLE_TYPEREF};
    uint32_t idx_size = get_coded_index_size(assembly, 2, refs, 4);
    return idx_size + (string_wide ? 4 : 2) * 2;
  }
  case TABLE_TYPEDEF: // 0x02
  {
    metadata_table_kind_t extends[] = {TABLE_TYPEDEF, TABLE_TYPEREF,
                                       TABLE_TYPESPEC};
    uint32_t extends_size = get_coded_index_size(assembly, 2, extends, 3);
    uint32_t field_size =
        (assembly->tables_header.row_counts[TABLE_FIELD] < 0x10000) ? 2 : 4;
    uint32_t method_size =
        (assembly->tables_header.row_counts[TABLE_METHODDEF] < 0x10000) ? 2 : 4;
    return 4 + (string_wide ? 4 : 2) * 2 + extends_size + field_size +
           method_size;
  }
  case TABLE_FIELD: // 0x04
    return 2 + (string_wide ? 4 : 2) + (blob_wide ? 4 : 2);
  case TABLE_METHODDEF: // 0x06
    return 8 + (string_wide ? 4 : 2) + (blob_wide ? 4 : 2) +
           ((assembly->tables_header.row_counts[TABLE_PARAM] < 0x10000) ? 2
                                                                        : 4);
  case TABLE_PARAM: // 0x08
    return 4 + (string_wide ? 4 : 2);
  case TABLE_MEMBERREF: // 0x0A
  {
    metadata_table_kind_t parents[] = {TABLE_TYPEDEF, TABLE_TYPEREF,
                                       TABLE_MODULE, TABLE_METHODDEF,
                                       TABLE_TYPESPEC};
    uint32_t parent_size = get_coded_index_size(assembly, 3, parents, 5);
    return parent_size + (string_wide ? 4 : 2) + (blob_wide ? 4 : 2);
  }
  case TABLE_TYPESPEC: // 0x1B
    return (blob_wide ? 4 : 2);
  case TABLE_STANDALONESIG: // 0x11
    return (blob_wide ? 4 : 2);
  default:
    return 0; // Unknown table
  }
}

static uint8_t *il_get_table_start(il_assembly_t *assembly,
                                   metadata_table_kind_t target_table) {
  uint8_t *ptr = assembly->tables_data;
  ptr += 24; // Skip header

  // Skip row counts
  for (int i = 0; i < 64; i++) {
    if (assembly->tables_header.valid_mask & (1ULL << i)) {
      ptr += 4;
    }
  }

  // Skip preceding tables
  for (int i = 0; i < 64; i++) {
    if (i == target_table) {
      if (assembly->tables_header.valid_mask & (1ULL << i))
        return ptr;
      else
        return NULL; // Table not present
    }

    if (assembly->tables_header.valid_mask & (1ULL << i)) {
      uint32_t rows = assembly->tables_header.row_counts[i];
      uint32_t row_size = get_table_row_size(assembly, i);
      if (row_size == 0) {
        // Warning: Unknown table size, cannot proceed accurately
        // For robustness, we might want to error out or guess
        // For now, return NULL if we can't skip past an unknown table
        return NULL;
      }
      ptr += rows * row_size;
    }
  }
  return NULL;
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

static typeref_row_t *parse_typeref_table(il_assembly_t *assembly,
                                          size_t *row_count_out) {
  size_t row_count = assembly->tables_header.row_counts[TABLE_TYPEREF];
  if (row_count == 0) {
    *row_count_out = 0;
    return NULL;
  }

  uint8_t *table_ptr = il_get_table_start(assembly, TABLE_TYPEREF);
  if (table_ptr == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  // Index sizes
  int string_wide = (assembly->tables_header.heap_sizes & 0x01) != 0;

  metadata_table_kind_t refs[] = {TABLE_MODULE, 0x1A /*ModuleRef*/,
                                  TABLE_ASSEMBLYREF, TABLE_TYPEREF};
  uint32_t scope_idx_size = get_coded_index_size(assembly, 2, refs, 4);
  int scope_wide = (scope_idx_size == 4);

  // Allocate
  typeref_row_t *rows = IL_MALLOC(sizeof(typeref_row_t) * row_count);
  if (rows == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  for (size_t i = 0; i < row_count; i++) {
    rows[i].resolution_scope = read_table_index(&table_ptr, scope_wide);
    rows[i].name_index = read_table_index(&table_ptr, string_wide);
    rows[i].namespace_index = read_table_index(&table_ptr, string_wide);
  }

  *row_count_out = row_count;
  return rows;
}

static typedef_row_t *parse_typedef_table(il_assembly_t *assembly,
                                          size_t *row_count_out) {
  size_t row_count = assembly->tables_header.row_counts[TABLE_TYPEDEF];
  if (row_count == 0) {
    *row_count_out = 0;
    return NULL;
  }

  uint8_t *table_ptr = il_get_table_start(assembly, TABLE_TYPEDEF);
  if (table_ptr == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  // Index sizes
  int string_wide = (assembly->tables_header.heap_sizes & 0x01) != 0;

  metadata_table_kind_t extends[] = {TABLE_TYPEDEF, TABLE_TYPEREF,
                                     TABLE_TYPESPEC};
  uint32_t extends_idx_size = get_coded_index_size(assembly, 2, extends, 3);
  int extends_wide = (extends_idx_size == 4);

  int field_wide = assembly->tables_header.row_counts[TABLE_FIELD] >= 0x10000;
  int method_wide =
      assembly->tables_header.row_counts[TABLE_METHODDEF] >= 0x10000;

  // Allocate
  typedef_row_t *rows = IL_MALLOC(sizeof(typedef_row_t) * row_count);
  if (rows == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  for (size_t i = 0; i < row_count; i++) {
    rows[i].flags = READ_UINT32(table_ptr);
    table_ptr += 4;
    rows[i].name_index = read_table_index(&table_ptr, string_wide);
    rows[i].namespace_index = read_table_index(&table_ptr, string_wide);
    rows[i].extends = read_table_index(&table_ptr, extends_wide);
    rows[i].field_list = read_table_index(&table_ptr, field_wide);
    rows[i].method_list = read_table_index(&table_ptr, method_wide);
  }

  *row_count_out = row_count;
  return rows;
}

static typespec_row_t *parse_typespec_table(il_assembly_t *assembly,
                                            size_t *row_count_out) {
  size_t row_count = assembly->tables_header.row_counts[TABLE_TYPESPEC];
  if (row_count == 0) {
    *row_count_out = 0;
    return NULL;
  }

  uint8_t *table_ptr = il_get_table_start(assembly, TABLE_TYPESPEC);
  if (table_ptr == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  // Index sizes
  int blob_wide = (assembly->tables_header.heap_sizes & 0x04) != 0;

  // Allocate
  typespec_row_t *rows = IL_MALLOC(sizeof(typespec_row_t) * row_count);
  if (rows == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  for (size_t i = 0; i < row_count; i++) {
    rows[i].signature = read_table_index(&table_ptr, blob_wide);
  }

  *row_count_out = row_count;
  return rows;
}

static standalonesig_row_t *parse_standalonesig_table(il_assembly_t *assembly,
                                                      size_t *row_count_out) {
  size_t row_count = assembly->tables_header.row_counts[TABLE_STANDALONESIG];
  if (row_count == 0) {
    *row_count_out = 0;
    return NULL;
  }

  uint8_t *table_ptr = il_get_table_start(assembly, TABLE_STANDALONESIG);
  if (table_ptr == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  // Index sizes
  int blob_wide = (assembly->tables_header.heap_sizes & 0x04) != 0;

  // Allocate
  standalonesig_row_t *rows = IL_MALLOC(sizeof(standalonesig_row_t) * row_count);
  if (rows == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  for (size_t i = 0; i < row_count; i++) {
    rows[i].signature = read_table_index(&table_ptr, blob_wide);
  }

  *row_count_out = row_count;
  return rows;
}

static methoddef_row_t *parse_methoddef_table(il_assembly_t *assembly,
                                              size_t *row_count_out) {
  size_t row_count = assembly->tables_header.row_counts[TABLE_METHODDEF];
  if (row_count == 0) {
    *row_count_out = 0;
    return NULL;
  }

  uint8_t *table_ptr = il_get_table_start(assembly, TABLE_METHODDEF);
  if (table_ptr == NULL) {
    *row_count_out = 0;
    return NULL;
  }

  // Determine index sizes
  int string_wide = (assembly->tables_header.heap_sizes & 0x01) != 0;
  int blob_wide = (assembly->tables_header.heap_sizes & 0x04) != 0;
  int param_wide = assembly->tables_header.row_counts[TABLE_PARAM] >= 0x10000;

  // Allocate rows
  methoddef_row_t *rows = IL_MALLOC(sizeof(methoddef_row_t) * row_count);
  if (rows == NULL) {
    *row_count_out = 0;
    return NULL;
  }

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

static il_method_t *parse_method(il_assembly_t *assembly, uint32_t rva,
                                 const char *name) {
  if (rva == 0) {
    return NULL; // Abstract or runtime-provided method
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
  uint8_t header_byte = method_ptr[0];
  uint8_t *code_end = NULL;

  if ((header_byte & 0x03) == 0x02) {
    // Tiny format - no exception handling
    method->flags = 0x02;
    method->max_stack = 8;
    method->il_code_size = header_byte >> 2;
    method->local_var_sig_token = 0;
    method->il_code = method_ptr + 1;
    IL_PRINT("IL_PARSER: TINY format method '%s' il_code=%p method_offset=0x%x\n",
             name, method->il_code, method_offset);
    method->exception_clauses = NULL;
    method->exception_clause_count = 0;
  } else if ((header_byte & 0x03) == 0x03) {
    // Fat format
    uint16_t fat_flags = READ_UINT16(method_ptr);
    method->flags = 0x03;
    uint16_t header_size = ((fat_flags >> 12) & 0x0F) * 4;
    method->max_stack = READ_UINT16(method_ptr + 2);
    method->il_code_size = READ_UINT32(method_ptr + 4);
    method->local_var_sig_token = READ_UINT32(method_ptr + 8);
    method->il_code = method_ptr + header_size;
    IL_PRINT("IL_PARSER: FAT format method '%s' il_code=%p method_offset=0x%x\n",
             name, method->il_code, method_offset);
    code_end = method->il_code + method->il_code_size;

    // Check for MoreSects flag (0x08 in low byte)
    if (fat_flags & 0x08) {
      // Exception handling data follows IL code (4-byte aligned)
      uint8_t *sect_ptr = code_end;
      uintptr_t alignment = ((uintptr_t)sect_ptr) & 3;
      if (alignment != 0) {
        sect_ptr += (4 - alignment);
      }

      // Parse exception sections
      while (1) {
        uint8_t sect_flags = sect_ptr[0];
        int is_fat_sect = (sect_flags & 0x40) != 0;
        int has_more = (sect_flags & 0x80) != 0;

        if ((sect_flags & 0x01) == 0) {
          // Not an exception handling section, skip
          break;
        }

        size_t clause_count;
        size_t sect_size;
        uint8_t *clause_ptr;

        if (is_fat_sect) {
          // Fat section header: 4 bytes (kind + 24-bit size)
          sect_size = (sect_ptr[1] | (sect_ptr[2] << 8) | (sect_ptr[3] << 16));
          clause_count = (sect_size - 4) / 24; // 24 bytes per fat clause
          clause_ptr = sect_ptr + 4;
        } else {
          // Small section header: 4 bytes (kind + 8-bit size + 2 reserved)
          sect_size = sect_ptr[1];
          clause_count = (sect_size - 4) / 12; // 12 bytes per small clause
          clause_ptr = sect_ptr + 4;
        }

        if (clause_count > 0) {
          method->exception_clauses =
              IL_MALLOC(sizeof(exception_clause_t) * clause_count);
          if (method->exception_clauses) {
            method->exception_clause_count = clause_count;

            for (size_t i = 0; i < clause_count; i++) {
              exception_clause_t *clause = &method->exception_clauses[i];

              if (is_fat_sect) {
                // Fat clause: 24 bytes
                clause->flags = READ_UINT32(clause_ptr);
                clause->try_offset = READ_UINT32(clause_ptr + 4);
                clause->try_length = READ_UINT32(clause_ptr + 8);
                clause->handler_offset = READ_UINT32(clause_ptr + 12);
                clause->handler_length = READ_UINT32(clause_ptr + 16);
                clause->class_token = READ_UINT32(clause_ptr + 20);
                clause_ptr += 24;
              } else {
                // Small clause: 12 bytes
                clause->flags = READ_UINT16(clause_ptr);
                clause->try_offset = READ_UINT16(clause_ptr + 2);
                clause->try_length = clause_ptr[4];
                clause->handler_offset = READ_UINT16(clause_ptr + 5);
                clause->handler_length = clause_ptr[7];
                clause->class_token = READ_UINT32(clause_ptr + 8);
                clause_ptr += 12;
              }
            }
          }
        }

        if (!has_more)
          break;
        sect_ptr += sect_size;
        // Align for next section
        alignment = ((uintptr_t)sect_ptr) & 3;
        if (alignment != 0) {
          sect_ptr += (4 - alignment);
        }
      }
    } else {
      method->exception_clauses = NULL;
      method->exception_clause_count = 0;
    }
  } else {
    IL_FREE(method);
    return NULL;
  }

  return method;
}

il_method_t *il_get_method_by_token(il_assembly_t *assembly, uint32_t token) {
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
    if (methods)
      IL_FREE(methods);
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
  if (method) {
    method->impl_flags = row->impl_flags;
    method->method_token = token; /* Store the token for later use */
  }

  IL_FREE(methods);
  return method;
}

il_method_t *il_get_method(il_assembly_t *assembly, const char *name) {
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

typeref_row_t *il_get_typeref(il_assembly_t *assembly, uint32_t rid) {
  if (rid == 0)
    return NULL;

  if (assembly->typerefs == NULL) {
    assembly->typerefs =
        parse_typeref_table(assembly, &assembly->typeref_count);
  }

  if (assembly->typerefs && rid <= assembly->typeref_count) {
    return &assembly->typerefs[rid - 1];
  }
  return NULL;
}

typedef_row_t *il_get_typedef(il_assembly_t *assembly, uint32_t rid) {
  if (rid == 0)
    return NULL;

  if (assembly->typedefs == NULL) {
    assembly->typedefs =
        parse_typedef_table(assembly, &assembly->typedef_count);
  }

  if (assembly->typedefs && rid <= assembly->typedef_count) {
    return &assembly->typedefs[rid - 1];
  }
  return NULL;
}

typespec_row_t *il_get_typespec(il_assembly_t *assembly, uint32_t rid) {
  if (rid == 0)
    return NULL;

  if (assembly->typespecs == NULL) {
    assembly->typespecs =
        parse_typespec_table(assembly, &assembly->typespec_count);
  }

  if (assembly->typespecs && rid <= assembly->typespec_count) {
    return &assembly->typespecs[rid - 1];
  }
  return NULL;
}

standalonesig_row_t *il_get_standalonesig(il_assembly_t *assembly, uint32_t rid) {
  if (rid == 0)
    return NULL;

  if (assembly->standalonesigs == NULL) {
    assembly->standalonesigs =
        parse_standalonesig_table(assembly, &assembly->standalonesig_count);
  }

  if (assembly->standalonesigs && rid <= assembly->standalonesig_count) {
    return &assembly->standalonesigs[rid - 1];
  }
  return NULL;
}

const char *il_get_method_parent_type_name(il_assembly_t *assembly,
                                           uint32_t method_token) {
  uint32_t row_index = method_token & 0x00FFFFFF;

  if (assembly->typedefs == NULL) {
    assembly->typedefs =
        parse_typedef_table(assembly, &assembly->typedef_count);
  }

  if (!assembly->typedefs)
    return NULL;

  for (size_t i = 0; i < assembly->typedef_count; i++) {
    uint32_t start = assembly->typedefs[i].method_list;
    uint32_t end;

    if (i + 1 < assembly->typedef_count) {
      end = assembly->typedefs[i + 1].method_list;
    } else {
      // Last type, extends to end of MethodDef table
      end = assembly->tables_header.row_counts[TABLE_METHODDEF] + 1;
    }

    if (row_index >= start && row_index < end) {
      // Found parent type
      return il_get_string(assembly, assembly->typedefs[i].name_index);
    }
  }
  return NULL;
}

/* ========== Assembly Parsing (Main Entry Point) ========== */

il_assembly_t *il_parse_assembly_memory(const uint8_t *data, size_t size,
                                        il_error_t *error) {
  IL_PRINT("IL_PARSER: il_parse_assembly_memory ENTER size=%ld\n", (long)size);
  il_assembly_t *assembly = IL_MALLOC(sizeof(il_assembly_t));
  if (assembly == NULL) {
    IL_PRINT("IL_PARSER: failed to allocate assembly struct\n");
    if (error)
      *error = IL_ERROR_OUT_OF_MEMORY;
    return NULL;
  }
  IL_PRINT("IL_PARSER: allocated assembly struct at %p\n", assembly);

  memset(assembly, 0, sizeof(il_assembly_t));
  assembly->data = (uint8_t *)data;
  assembly->size = size;
  IL_PRINT("IL_PARSER: assembly->data=%p (HHDM check: %s)\n",
           assembly->data,
           ((uintptr)assembly->data >= 0xffff800000000000ull) ? "YES" : "NO");

  // Parse PE/COFF headers
  IL_PRINT("IL_PARSER: calling parse_pe_header\n");
  il_error_t err = parse_pe_header(assembly);
  if (err != IL_OK) {
    IL_PRINT("IL_PARSER: parse_pe_header FAILED err=%d\n", err);
    if (error)
      *error = err;
    IL_FREE(assembly);
    return NULL;
  }
  IL_PRINT("IL_PARSER: parse_pe_header SUCCESS, %d sections\n",
           assembly->section_count);

  // Parse CLI header
  IL_PRINT("IL_PARSER: calling parse_cli_header\n");
  err = parse_cli_header(assembly);
  if (err != IL_OK) {
    IL_PRINT("IL_PARSER: parse_cli_header FAILED err=%d\n", err);
    if (error)
      *error = err;
    il_free_assembly(assembly);
    return NULL;
  }
  IL_PRINT("IL_PARSER: parse_cli_header SUCCESS, entry_point=0x%x\n",
           assembly->cli_header.entry_point_token);

  // Parse metadata
  IL_PRINT("IL_PARSER: calling parse_metadata_header\n");
  err = parse_metadata_header(assembly);
  if (err != IL_OK) {
    IL_PRINT("IL_PARSER: parse_metadata_header FAILED err=%d\n", err);
    if (error)
      *error = err;
    il_free_assembly(assembly);
    return NULL;
  }
  IL_PRINT("IL_PARSER: parse_metadata_header SUCCESS, %d streams\n",
           assembly->stream_count);

  // Parse metadata tables
  IL_PRINT("IL_PARSER: calling parse_metadata_tables\n");
  err = parse_metadata_tables(assembly);
  if (err != IL_OK) {
    IL_PRINT("IL_PARSER: parse_metadata_tables FAILED err=%d\n", err);
    if (error)
      *error = err;
    il_free_assembly(assembly);
    return NULL;
  }
  IL_PRINT("IL_PARSER: parse_metadata_tables SUCCESS\n");

  if (error)
    *error = IL_OK;
  IL_PRINT("IL_PARSER: il_parse_assembly_memory EXIT success\n");
  return assembly;
}

il_assembly_t *il_parse_assembly(const char *path, il_error_t *error) {
#if defined(KERNEL) || defined(__PLAN9_KERNEL__)
  /* In kernel mode, file I/O is not directly available */
  /* Use il_parse_assembly_memory instead with data from VFS */
  if (error)
    *error = IL_ERROR_FILE_NOT_FOUND;
  return nil;
#else
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    if (error)
      *error = IL_ERROR_FILE_NOT_FOUND;
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
    if (error)
      *error = IL_ERROR_OUT_OF_MEMORY;
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
#endif /* !KERNEL */
}

/* ========== Cleanup ========== */

void il_free_method(il_method_t *method) {
  if (method) {
    if (method->name) {
      IL_FREE(method->name);
    }
    if (method->exception_clauses) {
      IL_FREE(method->exception_clauses);
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
    if (assembly->typerefs)
      IL_FREE(assembly->typerefs);
    if (assembly->typedefs)
      IL_FREE(assembly->typedefs);
    if (assembly->typespecs)
      IL_FREE(assembly->typespecs);
    if (assembly->standalonesigs)
      IL_FREE(assembly->standalonesigs);

    if (assembly->data) {
      IL_FREE((void *)assembly->data);
    }
    IL_FREE(assembly);
  }
}

/* ========== Debug Utilities ========== */

void il_dump_assembly_info(il_assembly_t *assembly) {
  IL_PRINT("=== .NET Assembly Info ===\n");
  IL_PRINT("PE: %d sections\n", assembly->section_count);
  IL_PRINT("CLI Version: %d.%d\n", assembly->cli_header.major_runtime_version,
           assembly->cli_header.minor_runtime_version);
  IL_PRINT("Metadata Streams: %d\n", assembly->stream_count);
  for (uint16_t i = 0; i < assembly->stream_count; i++) {
    IL_PRINT("  [%d] %s (size=%d)\n", i, assembly->streams[i].name,
             assembly->streams[i].size);
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
    if ((i + 1) % 16 == 0)
      IL_PRINT("\n");
  }
  IL_PRINT("\n");
}
