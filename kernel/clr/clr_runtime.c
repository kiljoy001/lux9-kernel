/*
 * clr_runtime.c - CLR Runtime Entry Point (WASM Backend)
 *
 * Main entry point for loading and executing .NET assemblies.
 * Pipeline: IL → Fruity IR → WASM → Wasm3 Interpreter
 */

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
#ifdef USED
#undef USED
#endif
#define USED(x)                                                                \
  if (x) {                                                                     \
  }

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;
#include "../../port/lib.h"
#include "../9front-pc64/mem.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/uuid.h"
#include "clr_assemblies.h"
#include "fruity/fruity_ir.h"
#include "il_parser.h"
#include "wasm_backend/cil_opcodes.h"
#include "wasm_backend/cil_to_wasm.h"
#include "wasm_backend/fruity_to_wasm.h"
#include "wasm_backend/test_assembly.h"

/* WASM3 Runtime */
#include "wasm_runtime/wasm3/wasm3.h"

static void clr_dump_wasm_error(IM3Runtime runtime);

/* Forward declarations */
extern M3Result lux9_link_wasi(IM3Module module);

static const char *clr_map_lux9_method_name(const char *method_name) {
  if (!method_name)
    return NULL;

  if (strcmp(method_name, "Lux9Send9P") == 0 ||
      strcmp(method_name, "Send9P") == 0)
    return "lux9_send_9p";
  if (strcmp(method_name, "Lux9DebugPrint") == 0 ||
      strcmp(method_name, "Lux9Print") == 0 ||
      strcmp(method_name, "DebugPrint") == 0)
    return "lux9_debug_print";
  if (strcmp(method_name, "Lux9Yield") == 0 ||
      strcmp(method_name, "Yield") == 0)
    return "lux9_yield";
  if (strcmp(method_name, "Lux9Sleep") == 0 ||
      strcmp(method_name, "Sleep") == 0)
    return "lux9_sleep";
  if (strcmp(method_name, "Lux9Spawn") == 0 ||
      strcmp(method_name, "Spawn") == 0)
    return "lux9_spawn";
  if (strcmp(method_name, "Lux9Throw") == 0)
    return "lux9_throw";

  /* System.String properties */
  if (strcmp(method_name, "get_Length") == 0)
    return "clr_string_get_length";
  if (strcmp(method_name, "get_Chars") == 0)
    return "clr_string_get_char";

  return NULL;
}

static int clr_attach_token_to_import(fruity_module_t *mod, const char *name,
                                      u32int token) {
  fruity_function_t *imp = mod->functions_head;
  while (imp) {
    if (imp->name && strcmp(imp->name, name) == 0) {
      if (imp->method_token == 0 || imp->method_token == token) {
        imp->method_token = token;
      } else {
        /* Duplicate import name with a new token */
        fruity_function_t *dup = fruity_function_create(name, token);
        if (dup) {
          dup->import_info = imp->import_info;
          dup->next = mod->functions_head;
          if (mod->functions_head)
            mod->functions_head->prev = dup;
          mod->functions_head = dup;
          if (!mod->functions_tail)
            mod->functions_tail = dup;
          mod->function_count++;
          print("CLR: Duplicate import %s mapped to token %x\n", name, token);
        }
      }
      return 0;
    }
    imp = imp->next;
  }
  return -1;
}

static void clr_set_instr_mvid(fruity_instruction_t *instr,
                               il_assembly_t *assembly) {
  uuid_t mvid;

  if (!instr || !assembly)
    return;
  if (il_get_mvid(assembly, &mvid) == 0) {
    instr->method_mvid = mvid;
    instr->has_method_mvid = 1;
  }
}

static void clr_set_func_mvid(fruity_function_t *func,
                              il_assembly_t *assembly) {
  uuid_t mvid;

  if (!func || !assembly)
    return;
  if (il_get_mvid(assembly, &mvid) == 0) {
    func->mvid = mvid;
    func->has_mvid = 1;
  }
}

/* String literal table */
typedef struct {
  u32int us_index;
  char *cstr;
  void *managed_obj;
} clr_string_literal_t;

static clr_string_literal_t *string_literals;
static ulong string_literal_count;
static ulong string_literal_capacity;

il_assembly_t *current_assembly;

/* Helper to get user string (proper managed allocation) */
void *clr_string_from_literal(u32int us_index) {
  if (!current_assembly)
    return nil;

  /* Check cache first */
  for (ulong i = 0; i < string_literal_count; i++) {
    if (string_literals[i].us_index == us_index &&
        string_literals[i].managed_obj != nil) {
      return string_literals[i].managed_obj;
    }
  }

  /* Not in cache, get raw data */
  uint32_t char_count = 0;
  const uint16_t *chars =
      il_get_user_string_raw(current_assembly, us_index, &char_count);
  if (!chars)
    return nil;

  /* Allocate managed string object: 8 (header/length) + data */
  /* System.String layout: [int length][char firstChar...] */
  /* We use 8 bytes for length to match lux_alloc_array and alignment */
  ulong size = 8 + (char_count * 2);
  void *obj_data =
      xalloc(size); /* Using xalloc for kernel managed string allocation */
  if (!obj_data)
    return nil;

  /* Store length at start */
  *(u64int *)obj_data = (u64int)char_count;
  /* Copy characters */
  memmove((u8int *)obj_data + 8, chars, char_count * 2);

  /* Cache it */
  if (string_literal_count >= string_literal_capacity) {
    ulong new_cap = string_literal_capacity ? string_literal_capacity * 2 : 16;
    clr_string_literal_t *new_table =
        mallocz(sizeof(clr_string_literal_t) * new_cap, 1);
    if (new_table) {
      if (string_literals) {
        memmove(new_table, string_literals,
                sizeof(clr_string_literal_t) * string_literal_count);
        free(string_literals);
      }
      string_literals = new_table;
      string_literal_capacity = new_cap;
    }
  }

  if (string_literal_count < string_literal_capacity) {
    string_literals[string_literal_count].us_index = us_index;
    string_literals[string_literal_count].managed_obj = obj_data;
    string_literal_count++;
  }

  return obj_data;
}

u64int clr_string_get_length(void *ptr) {
  if (!ptr)
    return 0;
  return *(u64int *)ptr;
}

u64int clr_string_get_char(void *ptr, u64int index) {
  if (!ptr)
    return 0;
  u64int len = *(u64int *)ptr;
  if (index >= len)
    return 0;
  u16int *chars = (u16int *)((u8int *)ptr + 8);
  return chars[index];
}

ulong clr_get_type_size(u32int token) {
  /* Minimal stub - should use metadata to get real size */
  uint8_t kind = (token >> 24) & 0xFF;
  if (kind == TABLE_TYPEDEF || kind == TABLE_TYPEREF) {
    /* For now, just return a reasonable default for objects */
    return 32;
  }
  return 8;
}

#define CLR_STATIC_FIELD_SLOTS 256
static u64int clr_static_fields[CLR_STATIC_FIELD_SLOTS];

void *clr_get_static_field(u32int token) {
  u32int slot = token & 0xFF;
  if (slot >= CLR_STATIC_FIELD_SLOTS)
    return nil;
  return &clr_static_fields[slot];
}

/*
 * clr_compile_method_to_wasm
 * Converts IL Method -> Fruity -> WASM Binary
 */
/* Forward declarations */
static int clr_has_import(fruity_module_t *mod, const char *name);

/* Helper: Scan dependencies and add to module */
/* NOTE: This function is deprecated - WASM backend handles dependencies
 * directly */
static void clr_scan_dependencies(il_assembly_t *assembly, fruity_module_t *mod,
                                  fruity_function_t *func) {
  (void)assembly;
  (void)mod;
  (void)func;
  /* Stub - WASM backend compiles all methods upfront in
   * cil_to_wasm_build_module */
}

static int clr_has_import(fruity_module_t *mod, const char *name) {
  for (fruity_function_t *f = mod->functions_head; f; f = f->next) {
    if (f->name && name && strcmp(f->name, name) == 0)
      return 1;
  }
  return 0;
}

static void clr_add_import(fruity_module_t *mod, const char *name,
                           clr_value_type_t ret, const clr_value_type_t *args,
                           ulong arg_count) {
  if (!mod || !name || clr_has_import(mod, name))
    return;

  fruity_function_t *fn = fruity_function_create(name, 0);
  if (!fn)
    return;

  fn->import_info.is_import = 1;
  fn->import_info.module_name = xalloc(strlen("env") + 1);
  if (fn->import_info.module_name)
    strcpy(fn->import_info.module_name, "env");

  fn->import_info.function_name = xalloc(strlen(name) + 1);
  if (fn->import_info.function_name)
    strcpy(fn->import_info.function_name, name);

  fn->arg_count = arg_count;
  if (arg_count > 0) {
    fn->arg_types = xalloc(sizeof(clr_value_type_t) * arg_count);
    if (fn->arg_types)
      memmove(fn->arg_types, args, sizeof(clr_value_type_t) * arg_count);
    else
      fn->arg_count = 0;
  }
  fn->return_type = ret;

  /* Prepend so imports appear before locals */
  fn->next = mod->functions_head;
  if (mod->functions_head)
    mod->functions_head->prev = fn;
  mod->functions_head = fn;
  if (!mod->functions_tail)
    mod->functions_tail = fn;
  mod->function_count++;
}

static void clr_add_runtime_imports(fruity_module_t *mod) {
  const clr_value_type_t i32_args[] = {CLR_INT32};
  const clr_value_type_t i64_args[] = {CLR_INT64};
  const clr_value_type_t i64_i64_args[] = {CLR_INT64, CLR_INT64};
  const clr_value_type_t ref_args[] = {CLR_REF};
  const clr_value_type_t ref_ref_args[] = {CLR_REF, CLR_REF};
  const clr_value_type_t ref_ref_i64_args[] = {CLR_REF, CLR_REF, CLR_INT64};
  const clr_value_type_t ref_i32_args[] = {CLR_REF, CLR_INT32};
  const clr_value_type_t ref_i64_args[] = {CLR_REF, CLR_INT64};
  const clr_value_type_t ref_i64_i64[] = {CLR_REF, CLR_INT64, CLR_INT64};
  const clr_value_type_t i32_i64_args[] = {CLR_INT32, CLR_INT64};

  /* Lux9 system calls */
  clr_add_import(mod, "lux9_send_9p", CLR_INT32, ref_i32_args, 2);
  clr_add_import(mod, "lux9_debug_print", CLR_VOID, ref_i32_args, 2);
  clr_add_import(mod, "lux9_spawn", CLR_INT32, ref_args, 1);
  clr_add_import(mod, "lux9_sleep", CLR_VOID, i32_args, 1);
  clr_add_import(mod, "lux9_yield", CLR_VOID, nil, 0);
  clr_add_import(mod, "lux9_throw", CLR_VOID, ref_args, 1);

  /* Pebble memory management */
  clr_add_import(mod, "lux_alloc", CLR_REF, i64_i64_args, 2);
  clr_add_import(mod, "lux_addref", CLR_REF, ref_args, 1);
  clr_add_import(mod, "lux_release", CLR_VOID, ref_args, 1);
  clr_add_import(mod, "lux_snapshot", CLR_REF, ref_args, 1);
  clr_add_import(mod, "lux_commit", CLR_VOID, ref_args, 1);
  clr_add_import(mod, "lux_rollback", CLR_VOID, ref_args, 1);

  /* CLR runtime support */
  clr_add_import(mod, "clr_string_from_literal", CLR_REF, i32_args, 1);
  clr_add_import(mod, "clr_string_get_length", CLR_INT64, ref_args, 1);
  clr_add_import(mod, "clr_string_get_char", CLR_INT64, ref_i64_args, 2);
  clr_add_import(mod, "clr_get_type_size", CLR_INT64, i32_args, 1);
  clr_add_import(mod, "clr_get_static_field", CLR_REF, i32_args, 1);
  clr_add_import(mod, "clr_is_instance_of", CLR_INT32, ref_i32_args, 2);

  clr_add_import(mod, "clr_ptr_add", CLR_REF, ref_i64_args, 2);
  clr_add_import(mod, "clr_load_i64", CLR_INT64, ref_args, 1);
  clr_add_import(mod, "clr_store_i64", CLR_VOID, ref_i64_args, 2);
  clr_add_import(mod, "clr_memmove", CLR_VOID, ref_ref_i64_args, 3);
  clr_add_import(mod, "clr_memset", CLR_VOID, ref_i64_i64, 3);

  clr_add_import(mod, "clr_newobj", CLR_REF, i32_args, 1);
  clr_add_import(mod, "clr_newarr", CLR_REF, i32_i64_args, 2);
  clr_add_import(mod, "clr_array_len", CLR_INT64, ref_args, 1);
  clr_add_import(mod, "clr_array_get", CLR_INT64, ref_i64_args, 2);
  clr_add_import(mod, "clr_array_set", CLR_VOID, ref_i64_i64, 3);
  clr_add_import(mod, "clr_array_elem_addr", CLR_REF, ref_i64_args, 2);

  clr_add_import(mod, "clr_box", CLR_REF, i64_args, 1);
  clr_add_import(mod, "clr_unbox", CLR_REF, ref_args, 1);
  clr_add_import(mod, "clr_unbox_any", CLR_INT64, ref_args, 1);

  clr_add_import(mod, "clr_initobj", CLR_VOID, ref_i64_args, 2);
  clr_add_import(mod, "clr_cpobj", CLR_VOID, ref_ref_i64_args, 3);
  clr_add_import(mod, "clr_ldobj", CLR_INT64, ref_args, 1);
  clr_add_import(mod, "clr_stobj", CLR_VOID, ref_i64_args, 2);

  clr_add_import(mod, "clr_throw", CLR_VOID, ref_args, 1);
}

int clr_compile_method_to_wasm(il_assembly_t *assembly, const char *method_name,
                               void **wasm_bytes, u32int *wasm_len) {
  /* Find the entry method */
  il_method_t *main_m = il_get_method(assembly, method_name);
  if (!main_m) {
    print("CLR: Method '%s' not found\n", method_name);
    return -1;
  }

  print("CLR: Compiling method %s to WASM (direct CIL->WASM)...\n",
        method_name);

  /* Use direct CIL-to-WASM path (no Fruity IR conversion) */
  il_method_t *methods[1] = {main_m};
  int err =
      cil_to_wasm_build_module(methods, 1, method_name, wasm_bytes, wasm_len);
  if (err != 0) {
    print("CLR: cil_to_wasm_build_module failed: %d\n", err);
    return -1;
  }

  print("CLR: Direct CIL->WASM succeeded, %d bytes\n", *wasm_len);
  return 0;
}

static il_method_t *clr_find_entry_point(il_assembly_t *assembly,
                                         const char *name) {
  print("CLR: clr_find_entry_point name='%s'\n", name ? name : "nil");
  if (name) {
    il_method_t *m = il_get_method(assembly, name);
    if (m)
      return m;
  }
  il_method_t *kernel_entry = il_get_method(assembly, "KernelEntry");
  if (kernel_entry) {
    print("CLR: Found 'KernelEntry' entry point\n");
    return kernel_entry;
  }
  il_method_t *main = il_get_method(assembly, "Main");
  if (main) {
    print("CLR: Found 'Main' entry point\n");
    return main;
  }
  main = il_get_method(assembly, "main");
  if (main) {
    print("CLR: Found 'main' entry point\n");
    return main;
  }
  if (assembly->cli_header.entry_point_token) {
    print("CLR: Trying entry point token 0x%x\n",
          (unsigned int)assembly->cli_header.entry_point_token);
    return il_get_method_by_token(assembly,
                                  assembly->cli_header.entry_point_token);
  }
  print("CLR: No entry point found in assembly\n");
  return nil;
}

/*
 * clr_execute_assembly - Execute a .NET assembly
 * @dll_data: assembly bytes
 * @dll_size: assembly size
 * Returns: 0 on success, -1 on error
 * Entry point defaults to "Main" if not specified elsewhere
 */
int clr_execute_assembly_with_entry(void *dll_data, ulong dll_size,
                                    const char *entry_name);

int clr_execute_assembly(void *dll_data, ulong dll_size) {
  return clr_execute_assembly_with_entry(dll_data, dll_size, nil);
}

#include "clr_assemblies.h"

int clr_execute_assembly_with_entry(void *dll_data, ulong dll_size,
                                    const char *entry_name) {
  char errbuf[128];
  il_error_t err;
  const char *entry_point = nil; /* Default entry point lookup */

  /* Initialize Assembly Cache */
  clr_assemblies_init();

  print("CLR: Loading assembly...\n");
  il_assembly_t *assembly =
      il_parse_assembly_memory((u8int *)dll_data, dll_size, &err);
  if (!assembly) {
    print("CLR: Failed to parse assembly (%d)\n", err);
    return -1;
  }

  /* Add Main Assembly to Cache (TODO: Get name from assembly def) */
  /* For now, we assume it's the entry assembly, but let's try to get simple
     name if possible, or just rely on it being current. Better: Get AssemblyDef
     name.
  */
  // For now just add without name to ensure MVID index works?
  // Or assume "init" if entry_name is null?
  clr_assemblies_add(assembly, "init"); // Hardcoded for main assembly for now

  current_assembly = assembly;

  il_method_t *main = clr_find_entry_point(assembly, entry_name);

  if (!main) {
    print("CLR: No entry point found\n");
    return -1;
  }

  /* Compile to WASM */
  u32int wasm_size = 0;
  void *wasm_bytes = NULL;

  /* Collect ALL methods to ensure WASM indices match Metadata RIDs */
  u32int method_count = assembly->tables_header.row_counts[TABLE_METHODDEF];
  print("CLR: Assembly has %d methods. collecting...\n", method_count);

  il_method_t **all_methods = xalloc(sizeof(il_method_t *) * method_count);
  if (!all_methods) {
    print("CLR: Failed to allocate method array\n");
    return -1;
  }

  /* RIDs are 1-based */
  for (u32int i = 1; i <= method_count; i++) {
    u32int token = (TABLE_METHODDEF << 24) | i;
    il_method_t *m = il_get_method_by_token(assembly, token);
    if (!m) {
      print("CLR: Warning: Failed to parse method rid %d (token %x)\n", i,
            token);
    }
    all_methods[i - 1] = m;
  }

  /* Use direct CIL-to-WASM path (no Fruity IR conversion) */
  /* Set global assembly context for CIL-to-WASM translator */
  extern il_assembly_t *current_assembly;
  current_assembly = assembly;

  if (cil_to_wasm_build_module(all_methods, method_count, main->name,
                               &wasm_bytes, &wasm_size) != 0) {
    print("CLR: cil_to_wasm_build_module failed\n");
    return -1;
  }

  /* Instantiate WASM3 */
  print("CLR: Initializing WASM3...\n");

  /* Dump first 64 bytes of WASM binary for analysis */
  /* Dump full WASM binary for analysis */
  print("CLR: WASM binary hex dump (%d bytes):\n", (int)wasm_size);
  for (int i = 0; i < (int)wasm_size; i++) {
    if (i % 16 == 0)
      print("\n  %04x: ", i);
    print("%02x ", ((unsigned char *)wasm_bytes)[i]);
  }
  print("\n");

  print("CLR: DEBUG: About to call m3_NewEnvironment()\n");
  IM3Environment env = m3_NewEnvironment();
  print("CLR: DEBUG: m3_NewEnvironment() returned %p\n", env);
  if (!env) {
    print("m3_NewEnvironment failed\n");
    return -1;
  }

  print("CLR: DEBUG: About to call m3_NewRuntime()\n");
  IM3Runtime runtime = m3_NewRuntime(env, 64 * 1024, nil);
  print("CLR: DEBUG: m3_NewRuntime() returned %p\n", runtime);
  if (!runtime) {
    print("m3_NewRuntime failed\n");
    return -1;
  }

  unsigned long sp_val;
  asm("mov %%rsp, %0" : "=r"(sp_val));
  print("CLR: SP after m3_NewRuntime=%p\n", (void *)sp_val);

  print("CLR: Calling m3_ParseModule...\n");
  print("CLR: DEBUG: wasm_bytes=%p wasm_size=%d\n", wasm_bytes, (int)wasm_size);
  print("CLR: Address of m3_ParseModule=%p\n", m3_ParseModule);
  print("CLR: BEFORE m3_ParseModule call\n");

  IM3Module module = NULL;
  M3Result result = m3_ParseModule(env, &module, wasm_bytes, (u32int)wasm_size);

  print("CLR: AFTER m3_ParseModule call\n");
  print("CLR: m3_ParseModule() returned result=%p\n", result);
  if (result) {
    print("m3_ParseModule error: %s\n", result);
    return -1;
  }

  print("CLR: DEBUG: About to call m3_LoadModule()\n");
  result = m3_LoadModule(runtime, module);
  print("CLR: m3_LoadModule() returned result=%p\n", result);
  if (result) {
    print("m3_LoadModule error: %s\n", result);
    return -1;
  }

  /* Link Host Functions */
  result = lux9_link_wasi(module);
  if (result) {
    print("lux9_link_wasi error: %s\n", result);
    return -1;
  }

  /* Run Main (use main->name since that's what we exported) */
  IM3Function f;
  const char *lookup_name = main->name ? main->name : "main";
  print("CLR: Looking up function '%s'...\n", lookup_name);

  result = m3_FindFunction(&f, runtime, lookup_name);
  if (result) {
    print("m3_FindFunction error: %s\n", result);
    return -1;
  }

  u32int arg_count = m3_GetArgCount(f);
  print("CLR: Executing %s (args=%d, rets=%d)...\n", m3_GetFunctionName(f),
        arg_count, m3_GetRetCount(f));

  m3_ResetErrorInfo(runtime); /* Clear stale error info */
  if (arg_count == 1)
    result = m3_CallV(f, (u64int)0);
  else
    result = m3_CallV(f);

  if (result) {
    print("CLR: Execution failed: %s\n", result);
    clr_dump_wasm_error(runtime);
    return -1;
  }

  print("CLR: Execution finished successfully.\n");
  return 0;
}

/*
 * Test Function: Verifies Full Pipeline: IL(TestAdd.dll) -> Parser -> Fruity ->
 * WASM -> Execution
 */
void clr_test_wasm_pipeline(void) {
  print(
      "CLR-TEST: Starting Full WASM Pipeline Verification (TestAdd.dll)...\n");

  /* First, try the DIRECT path (bypasses Fruity) */
  print("CLR-TEST: === Testing DIRECT CIL->WASM path ===\n");

  il_error_t err;
  il_assembly_t *assembly = il_parse_assembly_memory(
      (u8int *)test_assembly_bytes, test_assembly_len, &err);
  if (!assembly) {
    print("CLR-TEST: Failed to parse TestAdd.dll: %d\n", err);
    return;
  }

  /* Get the methods we need */
  il_method_t *add_method = il_get_method(assembly, "Add");
  il_method_t *answer_method = il_get_method(assembly, "Answer");

  if (!add_method || !answer_method) {
    print("CLR-TEST: Could not find Add or Answer methods\n");
    return;
  }

  print("CLR-TEST: Found Add and Answer methods\n");

  /* Build WASM module using direct path */
  il_method_t *methods[2] = {add_method, answer_method};
  void *wasm_bytes = nil;
  u32int wasm_len = 0;

  int build_err =
      cil_to_wasm_build_module(methods, 2, "Answer", &wasm_bytes, &wasm_len);
  if (build_err != 0) {
    print("CLR-TEST: Direct build failed: %d\n", build_err);
    print("CLR-TEST: Falling back to Fruity path...\n");
    /* Fall back to old path */
    if (clr_execute_assembly_with_entry(test_assembly_bytes, test_assembly_len,
                                        "Answer") == 0) {
      print("CLR-TEST: *** FULL PIPELINE TEST PASSED (Fruity path) ***\n");
    } else {
      print("CLR-TEST: *** FULL PIPELINE TEST FAILED ***\n");
    }
    return;
  }

  print("CLR-TEST: Direct build succeeded, %d bytes\n", (int)wasm_len);

  /* Instantiate and run with WASM3 */
  IM3Environment env = m3_NewEnvironment();
  if (!env) {
    print("CLR-TEST: m3_NewEnvironment failed\n");
    return;
  }

  IM3Runtime runtime = m3_NewRuntime(env, 64 * 1024, nil);
  if (!runtime) {
    print("CLR-TEST: m3_NewRuntime failed\n");
    return;
  }

  IM3Module module = nil;
  M3Result result = m3_ParseModule(env, &module, wasm_bytes, wasm_len);
  if (result) {
    print("CLR-TEST: m3_ParseModule error: %s\n", result);
    return;
  }

  result = m3_LoadModule(runtime, module);
  if (result) {
    print("CLR-TEST: m3_LoadModule error: %s\n", result);
    return;
  }

  IM3Function f;
  result = m3_FindFunction(&f, runtime, "Answer");
  if (result) {
    print("CLR-TEST: m3_FindFunction error: %s\n", result);
    return;
  }

  print("CLR-TEST: Calling Answer()...\n");
  result = m3_CallV(f);
  if (result) {
    print("CLR-TEST: Execution failed: %s\n", result);
    clr_dump_wasm_error(runtime);
    print("CLR-TEST: *** DIRECT PATH TEST FAILED ***\n");
    return;
  }

  /* Get result */
  s64int ret_val = 0;
  m3_GetResultsV(f, &ret_val);
  print("CLR-TEST: Answer() returned %ld (expected 42)\n", (long)ret_val);

  if (ret_val == 42) {
    print("CLR-TEST: *** DIRECT PATH TEST PASSED ***\n");
  } else {
    print("CLR-TEST: *** DIRECT PATH TEST FAILED (wrong result) ***\n");
  }
}

static void clr_dump_wasm_error(IM3Runtime runtime) {
  M3ErrorInfo info;
  IM3BacktraceInfo bt;
  IM3BacktraceFrame frame;

  if (runtime == nil)
    return;

  m3_GetErrorInfo(runtime, &info);
  print("CLR: wasm error info: result=%s message=%s file=%s line=%lud\n",
        info.result ? info.result : "(nil)",
        info.message ? info.message : "(nil)", info.file ? info.file : "(nil)",
        (ulong)info.line);
  if (info.function) {
    print("CLR: wasm error in func=%s module=%s\n",
          m3_GetFunctionName(info.function),
          m3_GetModuleName(m3_GetFunctionModule(info.function)));
  }

  bt = m3_GetBacktrace(runtime);
  if (bt == nil || bt->frames == nil) {
    print("CLR: wasm backtrace: (none)\n");
    return;
  }

  print("CLR: wasm backtrace:\n");
  for (frame = bt->frames; frame != nil; frame = frame->next) {
    if (frame == M3_BACKTRACE_TRUNCATED) {
      print("CLR:  ... (truncated)\n");
      break;
    }
    if (frame->function) {
      print("CLR:  func=%s offset=0x%lux\n",
            m3_GetFunctionName(frame->function), (ulong)frame->moduleOffset);
    } else {
      print("CLR:  func=(nil) offset=0x%lux\n", (ulong)frame->moduleOffset);
    }
  }
}

void clr_init(void) {
  print("CLR (WASM Backend) Initialized\n");
  clr_test_wasm_pipeline();
}

/* Helpers for lux9_api.c */
u32int clr_get_field_rva(u32int token) {
  if (current_assembly)
    return il_get_field_rva(current_assembly, token);
  return 0;
}
u32int clr_rva_to_offset(u32int rva) {
  if (current_assembly)
    return il_rva_to_offset(current_assembly, rva);
  return 0;
}
void *clr_get_assembly_data(void) {
  if (current_assembly)
    return current_assembly->data;
  return NULL;
}
u32int clr_get_assembly_len(void) {
  if (current_assembly)
    return current_assembly->size;
  return 0;
}
