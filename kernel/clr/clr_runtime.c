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

#include "fruity/fruity_ir.h"
#include "il_parser.h"
#include "il_to_fruity.h"
#include "wasm_backend/fruity_to_wasm.h"
#include "wasm_backend/test_assembly.h"

/* WASM3 Runtime */
#include "wasm_runtime/wasm3/wasm3.h"

/* Forward declarations */
extern M3Result lux9_link_wasi(IM3Module module);

/* String literal table */
typedef struct {
  u32int us_index;
  char *cstr;
  void *managed_obj;
} clr_string_literal_t;

static clr_string_literal_t *string_literals;
static ulong string_literal_count;
static ulong string_literal_capacity;

static il_assembly_t *current_assembly;

/* Helper to get user string (stub for now) */
void *clr_string_from_literal(u32int us_index) {
  if (!current_assembly)
    return nil;
  char *s = il_get_user_string(current_assembly, us_index);
  return s;
}

ulong clr_get_type_size(u32int token) {
  /* Minimal stub */
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
/* Helper: Scan dependencies and add to module */
static void clr_scan_dependencies(il_assembly_t *assembly, fruity_module_t *mod,
                                  fruity_function_t *func) {
  if (!func || !func->blocks_head)
    return;

  fruity_basic_block_t *bb = func->blocks_head;
  while (bb) {
    fruity_instruction_t *instr = bb->instructions_head;
    while (instr) {
      if (instr->operand.type == FRUITY_OP_METHOD) {
        u32int token = instr->operand.value.token;

        /* Check if exists */
        int found = 0;
        fruity_function_t *f = mod->functions_head;
        while (f) {
          if (f->method_token == token) {
            found = 1;
            break;
          }
          f = f->next;
        }

        if (!found) {
          char modname[64] = {0};
          char funcname[64] = {0};

          if (il_get_pinvoke_info(assembly, token, modname, 64, funcname, 64) ==
              0) {
            /* Import */
            fruity_function_t *imp = fruity_function_create(funcname, token);
            if (imp) {
              print("CLR: Dependency Import Found: %s.%s (Token %x) -> New "
                    "Func %p\n",
                    modname, funcname, token, imp);
              imp->import_info.is_import = 1;
              /* Simple duplicate because strdup might not be avail */
              imp->import_info.module_name = xalloc(strlen(modname) + 1);
              if (imp->import_info.module_name)
                strcpy(imp->import_info.module_name, modname);

              imp->import_info.function_name = xalloc(strlen(funcname) + 1);
              if (imp->import_info.function_name)
                strcpy(imp->import_info.function_name, funcname);

              if (strcmp(modname, "env") == 0) {
                if (strcmp(funcname, "lux9_send_9p") == 0) {
                  imp->arg_count = 2;
                  imp->arg_types = xalloc(sizeof(clr_value_type_t) * 2);
                  if (imp->arg_types) {
                    imp->arg_types[0] = CLR_REF;
                    imp->arg_types[1] = CLR_INT32;
                  } else {
                    imp->arg_count = 0;
                  }
                  imp->return_type = CLR_INT32;
                } else if (strcmp(funcname, "lux9_debug_print") == 0) {
                  imp->arg_count = 2;
                  imp->arg_types = xalloc(sizeof(clr_value_type_t) * 2);
                  if (imp->arg_types) {
                    imp->arg_types[0] = CLR_REF;
                    imp->arg_types[1] = CLR_INT32;
                  } else {
                    imp->arg_count = 0;
                  }
                  imp->return_type = CLR_VOID;
                } else if (strcmp(funcname, "lux9_spawn") == 0) {
                  imp->arg_count = 1;
                  imp->arg_types = xalloc(sizeof(clr_value_type_t));
                  if (imp->arg_types)
                    imp->arg_types[0] = CLR_REF;
                  else
                    imp->arg_count = 0;
                  imp->return_type = CLR_INT32;
                } else if (strcmp(funcname, "lux9_sleep") == 0) {
                  imp->arg_count = 1;
                  imp->arg_types = xalloc(sizeof(clr_value_type_t));
                  if (imp->arg_types)
                    imp->arg_types[0] = CLR_INT32;
                  else
                    imp->arg_count = 0;
                  imp->return_type = CLR_VOID;
                } else if (strcmp(funcname, "lux9_yield") == 0) {
                  imp->arg_count = 0;
                  imp->return_type = CLR_VOID;
                } else {
                  print("CLR: Warning: unknown env import %s, defaulting to "
                        "void()\n",
                        funcname);
                }
              }

              /* Prepend (WASM requirement: Imports first) */
              imp->next = mod->functions_head;
              if (mod->functions_head)
                mod->functions_head->prev = imp;
              mod->functions_head = imp;
              if (!mod->functions_tail)
                mod->functions_tail = imp;
              mod->function_count++;

              print("CLR: List Update (Prepend): Head=%p Tail=%p\n",
                    mod->functions_head, mod->functions_tail);
            }
          } else {
            /* Local */
            il_method_t *m = il_get_method_by_token(assembly, token);
            if (m) {
              print("CLR: Dependency Local Found: Token %x -> Compiling...\n",
                    token);
              il_to_fruity_error_t err;
              fruity_function_t *new_func =
                  il_to_fruity_convert_method(assembly, m, &err);
              if (new_func) {
                print("CLR: New Local Func %p\n", new_func);
                /* Append */
                if (mod->functions_tail) {
                  mod->functions_tail->next = new_func;
                  new_func->prev = mod->functions_tail;
                  mod->functions_tail = new_func;
                } else {
                  mod->functions_head = mod->functions_tail = new_func;
                }
                mod->function_count++;

                print("CLR: List Update (Append): Head=%p Tail=%p\n",
                      mod->functions_head, mod->functions_tail);

                /* Recurse */
                clr_scan_dependencies(assembly, mod, new_func);
              } else {
                print("CLR: Failed to compile local dep %x. Error: %d (%s)\n",
                      token, err, il_to_fruity_error_string(err));
              }
            }
          }
        }
      }
      instr = instr->next;
    }
    bb = bb->next;
  }
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

  clr_add_import(mod, "lux_alloc", CLR_REF, i64_i64_args, 2);
  clr_add_import(mod, "lux_addref", CLR_REF, ref_args, 1);
  clr_add_import(mod, "lux_release", CLR_VOID, ref_args, 1);
  clr_add_import(mod, "lux_snapshot", CLR_REF, ref_args, 1);
  clr_add_import(mod, "lux_commit", CLR_VOID, ref_args, 1);
  clr_add_import(mod, "lux_rollback", CLR_VOID, ref_args, 1);

  clr_add_import(mod, "clr_string_from_literal", CLR_REF, i32_args, 1);
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
  /* 1. Compile Main */
  il_method_t *main_m = il_get_method(assembly, method_name);
  if (!main_m)
    return -1;

  print("CLR: Compiling method %s to WASM...\n", method_name);

  il_to_fruity_error_t err;
  fruity_function_t *main_func =
      il_to_fruity_convert_method(assembly, main_m, &err);
  if (!main_func) {
    print("CLR: Main compilation failed: %d\n", err);
    return -1;
  }

  /* 2. Build module */
  fruity_module_t temp_mod;
  memset(&temp_mod, 0, sizeof(temp_mod));
  temp_mod.functions_head = main_func;
  temp_mod.functions_tail = main_func;
  temp_mod.function_count = 1;

  /* 3. Scan dependencies */
  clr_scan_dependencies(assembly, &temp_mod, main_func);

  /* 4. Add runtime imports */
  clr_add_runtime_imports(&temp_mod);

  /* 5. Emit WASM */
  fruity_wasm_result_t wasm_res;
  print("CLR: Calling fruity_compile_to_wasm...\n");
  if (fruity_compile_to_wasm(&temp_mod, &wasm_res) != 0) {
    print("CLR: fruity_compile_to_wasm failed\n");
    return -1;
  }
  print("CLR: fruity_compile_to_wasm returned %d bytes\n", wasm_res.wasm_size);

  *wasm_bytes = wasm_res.wasm_binary;
  *wasm_len = wasm_res.wasm_size;

  return 0;
}

static il_method_t *clr_find_entry_point(il_assembly_t *assembly,
                                         const char *name) {
  if (name) {
    il_method_t *m = il_get_method(assembly, name);
    if (m)
      return m;
  }
  il_method_t *main = il_get_method(assembly, "Main");
  if (main)
    return main;
  if (assembly->cli_header.entry_point_token) {
    return il_get_method_by_token(assembly,
                                  assembly->cli_header.entry_point_token);
  }
  return nil;
}

/*
 * clr_execute_assembly - Execute a .NET assembly
 * @dll_data: assembly bytes
 * @dll_size: assembly size
 * Returns: 0 on success, -1 on error
 * Entry point defaults to "Main" if not specified elsewhere
 */
int clr_execute_assembly(void *dll_data, ulong dll_size) {
  char errbuf[128];
  il_error_t err;
  const char *entry_point = nil; /* Default entry point lookup */

  print("CLR: Loading assembly...\n");
  il_assembly_t *assembly =
      il_parse_assembly_memory((u8int *)dll_data, dll_size, &err);
  if (!assembly) {
    print("CLR: Failed to parse assembly (%d)\n", err);
    return -1;
  }
  current_assembly = assembly;

  il_method_t *main = clr_find_entry_point(assembly, entry_point);
  if (!main) {
    print("CLR: No entry point found\n");
    return -1;
  }

  /* Compile to WASM */
  u32int wasm_size = 0;
  void *wasm_bytes = NULL;

  if (clr_compile_method_to_wasm(assembly, main->name, &wasm_bytes,
                                 &wasm_size) != 0) {
    print("CLR: Compilation failed\n");
    return -1;
  }

  /* Instantiate WASM3 */
  print("CLR: Initializing WASM3...\n");

  /* Dump first 64 bytes of WASM binary for analysis */
  print("CLR: WASM binary hex dump (first 64 bytes):\n");
  for (int i = 0; i < 64 && i < (int)wasm_size; i++) {
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

  /* Run Main */
  IM3Function f;
  result = m3_FindFunction(&f, runtime, "Main");
  if (result) {
    print("m3_FindFunction error: %s\n", result);
    return -1;
  }

  print("CLR: Executing Main...\n");
  result = m3_CallV(f);

  if (result) {
    print("CLR: Execution failed: %s\n", result);
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

  /* Use embedded TestAdd.dll byte array */
  /* Target Method: Will use default entry point (Main) */

  if (clr_execute_assembly(test_assembly_bytes, test_assembly_len) == 0) {
    print("CLR-TEST: *** FULL PIPELINE TEST PASSED ***\n");
  } else {
    print("CLR-TEST: *** FULL PIPELINE TEST FAILED ***\n");
  }
}

void clr_init(void) {
  print("CLR (WASM Backend) Initialized\n");
  clr_test_wasm_pipeline();
}
