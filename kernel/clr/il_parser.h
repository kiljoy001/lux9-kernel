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
typedef uint32_t u32int;
typedef uint8_t u8int;
typedef uint64_t u64int;
typedef unsigned long ulong;
#else
#include "il_compat.h"
#endif

/* UUID Support */
#include "../include/uuid.h"

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
  TABLE_CONSTANT = 0x0B,
  TABLE_CUSTOMATTRIBUTE = 0x0C,
  TABLE_FIELDMARSHAL = 0x0D,
  TABLE_DECLSECURITY = 0x0E,
  TABLE_CLASSLAYOUT = 0x0F,
  TABLE_FIELDLAYOUT = 0x10,
  TABLE_STANDALONESIG = 0x11,
  TABLE_EVENTMAP = 0x12,
  TABLE_EVENT = 0x14,
  TABLE_PROPERTYMAP = 0x15,
  TABLE_PROPERTY = 0x17,
  TABLE_METHODSEMANTICS = 0x18,
  TABLE_METHODIMPL = 0x19,
  TABLE_MODULEREF = 0x1A,
  TABLE_TYPESPEC = 0x1B,
  TABLE_IMPLMAP = 0x1C,
  TABLE_FIELDRVA = 0x1D,
  TABLE_ASSEMBLY = 0x20,
  TABLE_ASSEMBLYPROCESSOR = 0x21,
  TABLE_ASSEMBLYOS = 0x22,
  TABLE_ASSEMBLYREF = 0x23,
  TABLE_ASSEMBLYREFPROCESSOR = 0x24,
  TABLE_ASSEMBLYREFOS = 0x25,
  TABLE_FILE = 0x26,
  TABLE_EXPORTEDTYPE = 0x27,
  TABLE_MANIFESTRESOURCE = 0x28,
  TABLE_NESTEDCLASS = 0x29,
  TABLE_GENERICPARAM = 0x2A,
  TABLE_METHODSPEC = 0x2B,
  TABLE_GENERICPARAMCONSTRAINT = 0x2C
} metadata_table_kind_t;

/* ... structs ... */

typedef struct {
  uint32_t method;        /* MethodDefOrRef encoded index */
  uint32_t instantiation; /* Blob index */
} methodspec_row_t;

/* ... inside il_assembly_t ... */
/* I can't easily inject inside the struct without replacing the whole struct.
   For now I'll just add the row definition and API.
   The assembly struct update requires replacing the whole struct block.
*/

/* ... API ... */

/* ... API moved to end ... */

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

  /* Capability binding (runtime) */
  struct clr_monotonic_capability *capability;
} typedef_row_t;

typedef struct {
  uint32_t signature; /* Index into #Blob heap */
} typespec_row_t;

typedef struct {
  uint32_t signature; /* Index into #Blob heap */
} standalonesig_row_t;

typedef struct {
  uint16_t flags;
  uint32_t name_index; /* Index into #Strings heap */
  uint32_t signature;  /* Index into #Blob heap */
} field_row_t;

/* AssemblyRef row */
typedef struct {
  uint16_t major_version;
  uint16_t minor_version;
  uint16_t build_number;
  uint16_t revision_number;
  uint32_t flags;
  uint32_t public_key_or_token; /* Index into #Blob heap */
  uint32_t name_index;          /* Index into #Strings heap */
  uint32_t culture_index;       /* Index into #Strings heap */
  uint32_t hash_value;          /* Index into #Blob heap */
} assemblyref_row_t;

/* MemberRef row - references to members (methods/fields) in other assemblies */
typedef struct {
  uint32_t class_index; /* Coded index: TypeRef, ModuleRef, MethodDef, TypeSpec,
                           TypeDef */
  uint32_t name_index;  /* Index into #Strings heap */
  uint32_t signature;   /* Index into #Blob heap */
} memberref_row_t;

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
  uint32_t signature_index; /* Index into #Blob heap for MethodDef signature */
  uint16_t impl_flags;      // MethodImplAttributes
  uint8_t flags;            /* Tiny or fat format */

  /* Metadata token for this method (TABLE_METHODDEF | row_index) */
  uint32_t method_token;

  /* WASM function index assigned during CIL->WASM compilation */
  uint32_t wasm_func_idx;

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

  /* UUID (MVID) */
  uuid_t mvid;
  int mvid_loaded; /* 0=not loaded, 1=loaded */

  // Types cache
  typeref_row_t *typerefs;
  size_t typeref_count;
  typedef_row_t *typedefs;
  size_t typedef_count;
  typespec_row_t *typespecs;
  size_t typespec_count;
  standalonesig_row_t *standalonesigs;
  size_t standalonesig_count;
  memberref_row_t *memberrefs;
  size_t memberref_count;

  /* Capability binding (created during assembly load) */
  /* See clr_capability.h for full type definition */
  struct clr_monotonic_capability *capability;

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

/* Get MethodDef name and RVA without parsing body (returns 0 on success) */
int il_get_methoddef_info(il_assembly_t *assembly, uint32_t token,
                          char *name_out, size_t name_len, uint32_t *rva_out);
int il_find_methoddef_by_name(il_assembly_t *assembly, const char *type_name,
                              const char *method_name, uint32_t *token_out);

/* Get the name of the type that owns the given method token */
const char *il_get_method_parent_type_name(il_assembly_t *assembly,
                                           uint32_t method_token);

/* Get TypeRef row (1-based index) */
typeref_row_t *il_get_typeref(il_assembly_t *assembly, uint32_t rid);

/* Get TypeDef row (1-based index) */
typedef_row_t *il_get_typedef(il_assembly_t *assembly, uint32_t rid);

/* Get TypeSpec row (1-based index) */
typespec_row_t *il_get_typespec(il_assembly_t *assembly, uint32_t rid);

/* Get StandAloneSig row (1-based index) */
standalonesig_row_t *il_get_standalonesig(il_assembly_t *assembly,
                                          uint32_t rid);

/* Get MemberRef row (1-based index) */
memberref_row_t *il_get_memberref(il_assembly_t *assembly, uint32_t rid);

/* Get AssemblyRef row (1-based index) */
assemblyref_row_t *il_get_assemblyref(il_assembly_t *assembly, uint32_t rid);

/* Resolve a MemberRef token to its class/type name, method name, AND scope
 * (assembly name) Returns 0 on success, -1 on failure Caller provides buffers;
 * names are copied into them */
int il_resolve_memberref(il_assembly_t *assembly, uint32_t token,
                         char *type_name, size_t type_len, char *method_name,
                         size_t method_len, char *scope_name, size_t scope_len);

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

/* ========== Type Metadata Functions ========== */

/* Get size in bytes of a CLR type from its metadata token */
ulong clr_get_type_size(u32int token);

/* Get MethodSpec row (1-based index) */
methodspec_row_t *il_get_methodspec(il_assembly_t *assembly, uint32_t rid);

/* Resolve a MethodSpec token to its underlying method definition
 * Returns 0 on success, -1 on failure
 * Caller provides buffers; names are copied into them */
int il_resolve_methodspec(il_assembly_t *assembly, uint32_t token,
                          char *type_name_out, size_t type_buf_len,
                          char *method_name_out, size_t method_buf_len);

/* Get StandAloneSig row (1-based index) */
standalonesig_row_t *il_get_standalonesig(il_assembly_t *assembly,
                                          uint32_t rid);

/* Get Field row (1-based index) */
field_row_t *il_get_field(il_assembly_t *assembly, uint32_t rid);

/* Get Module MVID (UUID) */
int il_get_mvid(il_assembly_t *assembly, uuid_t *out_uuid);

/* Get P/Invoke info for a method token.
 * Returns 0 if found and populated, -1 otherwise.
 */
int il_get_pinvoke_info(il_assembly_t *assembly, uint32_t method_token,
                        char *module_out, size_t module_len, char *func_out,
                        size_t func_len);

/* Get signature token for a MethodDef token.
 * Returns 0 on success, -1 on failure. */
int il_get_method_signature_token(il_assembly_t *assembly, uint32_t token,
                                  uint32_t *sig_out);
int il_load_all_methods(il_assembly_t *assembly);

/* Field RVA Lookup */
u32int il_get_field_rva(il_assembly_t *assembly, u32int field_token);

#endif // IL_PARSER_H
