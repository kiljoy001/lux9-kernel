/* il_parser.h - .NET PE/COFF and CLI Metadata Parser
 *
 * Parses .NET assemblies (DLL/EXE files) to extract:
 * - CLI metadata tables
 * - IL bytecode for methods
 * - Type definitions
 *
 * Based on ECMA-335 specification (CLI Common Language Infrastructure)
 */

#ifndef IL_PARSER_H
#define IL_PARSER_H

#ifdef USERSPACE_TEST
#include <stddef.h>
#include <stdint.h>
typedef uintptr_t uintptr;
#else
#include "qbe/kernel_compat.h"
#endif
/* #include <stddef.h> */
/* #include <stdint.h> */

/* ========== PE/COFF Structures ========== */

#define PE_SIGNATURE 0x00004550 // "PE\0\0"

typedef struct {
  uint16_t machine;
  uint16_t number_of_sections;
  uint32_t time_date_stamp;
  uint32_t pointer_to_symbol_table;
  uint32_t number_of_symbols;
  uint16_t size_of_optional_header;
  uint16_t characteristics;
} pe_coff_header_t;

typedef struct {
  uint32_t virtual_address;
  uint32_t size;
} pe_data_directory_t;

#define PE_DIRECTORY_COUNT 16
#define PE_DIRECTORY_CLR_RUNTIME 14

typedef struct {
  uint16_t magic;
  uint8_t major_linker_version;
  uint8_t minor_linker_version;
  uint32_t size_of_code;
  uint32_t size_of_initialized_data;
  uint32_t size_of_uninitialized_data;
  uint32_t address_of_entry_point;
  uint32_t base_of_code;
  uint64_t image_base;
  uint32_t section_alignment;
  uint32_t file_alignment;
  uint16_t major_os_version;
  uint16_t minor_os_version;
  uint16_t major_image_version;
  uint16_t minor_image_version;
  uint16_t major_subsystem_version;
  uint16_t minor_subsystem_version;
  uint32_t win32_version_value;
  uint32_t size_of_image;
  uint32_t size_of_headers;
  uint32_t checksum;
  uint16_t subsystem;
  uint16_t dll_characteristics;
  uint64_t size_of_stack_reserve;
  uint64_t size_of_stack_commit;
  uint64_t size_of_heap_reserve;
  uint64_t size_of_heap_commit;
  uint32_t loader_flags;
  uint32_t number_of_rva_and_sizes;
  pe_data_directory_t data_directories[PE_DIRECTORY_COUNT];
} pe_optional_header_t;

typedef struct {
  char name[8];
  uint32_t virtual_size;
  uint32_t virtual_address;
  uint32_t size_of_raw_data;
  uint32_t pointer_to_raw_data;
  uint32_t pointer_to_relocations;
  uint32_t pointer_to_linenumbers;
  uint16_t number_of_relocations;
  uint16_t number_of_linenumbers;
  uint32_t characteristics;
} pe_section_header_t;

/* ========== CLI Metadata Structures ========== */

typedef struct {
  uint32_t cb; // Size of CLI header
  uint16_t major_runtime_version;
  uint16_t minor_runtime_version;
  pe_data_directory_t metadata;
  uint32_t flags;
  uint32_t entry_point_token;
  pe_data_directory_t resources;
  pe_data_directory_t strong_name_signature;
  pe_data_directory_t code_manager_table;
  pe_data_directory_t vtable_fixups;
  pe_data_directory_t export_address_table_jumps;
  pe_data_directory_t managed_native_header;
} cli_header_t;

typedef struct {
  uint32_t signature; // Must be 0x424A5342 (BSJB)
  uint16_t major_version;
  uint16_t minor_version;
  uint32_t reserved;
  uint32_t version_length;
  char *version_string;
  uint16_t flags;
  uint16_t stream_count;
} metadata_header_t;

typedef struct {
  uint32_t offset;
  uint32_t size;
  char *name;
  uint8_t *data;
} metadata_stream_t;

/* Metadata table kinds */
typedef enum {
  TABLE_MODULE = 0x00,
  TABLE_TYPEREF = 0x01,
  TABLE_TYPEDEF = 0x02,
  TABLE_FIELD = 0x04,
  TABLE_METHODDEF = 0x06,
  TABLE_PARAM = 0x08,
  TABLE_MEMBERREF = 0x0A,
  TABLE_TYPESPEC = 0x1B,
  TABLE_ASSEMBLY = 0x20,
  TABLE_ASSEMBLYREF = 0x23
} metadata_table_kind_t;

typedef struct {
  uint32_t reserved;
  uint8_t major_version;
  uint8_t minor_version;
  uint8_t heap_sizes;
  uint8_t reserved2;
  uint64_t valid_mask;
  uint64_t sorted_mask;
  uint32_t *row_counts; // Array of row counts for each table
} metadata_tables_header_t;

/* ========== MethodDef Row ========== */

typedef struct {
  uint32_t rva; /* Relative virtual address */
  uint16_t impl_flags;
  uint16_t flags;
  uint32_t name_index;      /* Index into #Strings heap */
  uint32_t signature_index; /* Index into #Blob heap */
  uint32_t param_list;      /* Index into Param table */
} methoddef_row_t;

typedef struct {
  uint32_t resolution_scope; /* Index into Module, ModuleRef, AssemblyRef, or
                                TypeRef */
  uint32_t name_index;       /* Index into #Strings heap */
  uint32_t namespace_index;  /* Index into #Strings heap */
} typeref_row_t;

typedef struct {
  uint32_t flags;
  uint32_t name_index;      /* Index into #Strings heap */
  uint32_t namespace_index; /* Index into #Strings heap */
  uint32_t extends;         /* Index into TypeDef, TypeRef, or TypeSpec */
  uint32_t field_list;      /* Index into Field table */
  uint32_t method_list;     /* Index into MethodDef table */
} typedef_row_t;

typedef struct {
  uint32_t signature; /* Index into #Blob heap */
} typespec_row_t;

/* ========== Exception Clause Types (ECMA-335 II.25.4.6) ========== */

typedef enum {
  COR_ILEXCEPTION_CLAUSE_EXCEPTION = 0x0000, /* Catch handler */
  COR_ILEXCEPTION_CLAUSE_FILTER = 0x0001,    /* Filter-based handler */
  COR_ILEXCEPTION_CLAUSE_FINALLY = 0x0002,   /* Finally block */
  COR_ILEXCEPTION_CLAUSE_FAULT =
      0x0004, /* Fault block (finally that runs on exception only) */
} exception_clause_flags_t;

typedef struct {
  uint32_t flags;          /* Exception clause type */
  uint32_t try_offset;     /* Offset in IL where try block starts */
  uint32_t try_length;     /* Length of try block */
  uint32_t handler_offset; /* Offset where handler starts */
  uint32_t handler_length; /* Length of handler */
  uint32_t
      class_token; /* Catch: TypeRef/Def token; Filter: offset to filter code */
} exception_clause_t;

/* ========== Method Structures ========== */

typedef struct {
  char *name;
  uint8_t *il_code;
  size_t il_code_size;
  uint32_t max_stack;
  uint32_t local_var_sig_token;
  uint16_t impl_flags; // MethodImplAttributes
  uint8_t flags;       /* Tiny or fat format */

  /* Metadata token for this method (TABLE_METHODDEF | row_index) */
  uint32_t method_token;

  /* Exception handling */
  exception_clause_t *exception_clauses;
  size_t exception_clause_count;
} il_method_t;

/* ========== Assembly Structure ========== */

typedef struct {
  // Raw file data
  uint8_t *data;
  size_t size;

  // PE/COFF structures
  pe_coff_header_t coff_header;
  pe_optional_header_t optional_header;
  pe_section_header_t *sections;
  uint16_t section_count;

  // CLI structures
  cli_header_t cli_header;
  metadata_header_t metadata_header;
  metadata_stream_t *streams;
  uint16_t stream_count;

  // Metadata heaps
  uint8_t *strings_heap;
  size_t strings_heap_size;
  uint8_t *blob_heap;
  size_t blob_heap_size;
  uint8_t *guid_heap;
  size_t guid_heap_size;
  uint8_t *us_heap; // User strings
  size_t us_heap_size;

  // Metadata tables
  metadata_tables_header_t tables_header;
  uint8_t *tables_data;
  size_t tables_data_size;

  // Methods cache
  il_method_t *methods;
  size_t method_count;

  // Types cache
  typeref_row_t *typerefs;
  size_t typeref_count;
  typedef_row_t *typedefs;
  size_t typedef_count;
  typespec_row_t *typespecs;
  size_t typespec_count;
} il_assembly_t;

/* ========== Error Codes ========== */

typedef enum {
  IL_OK = 0,
  IL_ERROR_FILE_NOT_FOUND,
  IL_ERROR_INVALID_PE,
  IL_ERROR_INVALID_CLI,
  IL_ERROR_INVALID_METADATA,
  IL_ERROR_METHOD_NOT_FOUND,
  IL_ERROR_OUT_OF_MEMORY,
  IL_ERROR_INVALID_IL
} il_error_t;

/* ========== Public API ========== */

/* Parse a .NET assembly from file path */
il_assembly_t *il_parse_assembly(const char *path, il_error_t *error);

/* Parse a .NET assembly from memory */
il_assembly_t *il_parse_assembly_memory(const uint8_t *data, size_t size,
                                        il_error_t *error);

/* Get method by name */
il_method_t *il_get_method(il_assembly_t *assembly, const char *name);

/* Get method by MethodDef token */
il_method_t *il_get_method_by_token(il_assembly_t *assembly, uint32_t token);

/* Get the name of the type that owns the given method token */
const char *il_get_method_parent_type_name(il_assembly_t *assembly,
                                           uint32_t method_token);

/* Get TypeRef row (1-based index) */
typeref_row_t *il_get_typeref(il_assembly_t *assembly, uint32_t rid);

/* Get TypeDef row (1-based index) */
typedef_row_t *il_get_typedef(il_assembly_t *assembly, uint32_t rid);

/* Get TypeSpec row (1-based index) */
typespec_row_t *il_get_typespec(il_assembly_t *assembly, uint32_t rid);

/* Get string from #Strings heap */
const char *il_get_string(il_assembly_t *assembly, uint32_t index);

/* Get blob from #Blob heap */
const uint8_t *il_get_blob(il_assembly_t *assembly, uint32_t index,
                           uint32_t *size);

/* Get user string from #US heap (returns newly allocated char*) */
char *il_get_user_string(il_assembly_t *assembly, uint32_t index);

/* Get raw UTF-16 user string from #US heap */
const uint16_t *il_get_user_string_raw(il_assembly_t *assembly, uint32_t index,
                                       uint32_t *length);

/* Decode compressed unsigned integer from blob */
uint32_t il_decode_compressed_uint(const uint8_t **data);

/* Free assembly */
void il_free_assembly(il_assembly_t *assembly);

/* Free method */
void il_free_method(il_method_t *method);

/* Error handling */
const char *il_error_string(il_error_t error);

/* ========== Utility Functions ========== */

/* Convert RVA (relative virtual address) to file offset */
uint32_t il_rva_to_offset(il_assembly_t *assembly, uint32_t rva);

/* Get section by RVA */
pe_section_header_t *il_get_section_by_rva(il_assembly_t *assembly,
                                           uint32_t rva);

/* Dump assembly info (for debugging) */
void il_dump_assembly_info(il_assembly_t *assembly);

/* Dump method info (for debugging) */
void il_dump_method(il_method_t *method);

#endif /* IL_PARSER_H */
