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

/*
 * clr_compile_method_to_wasm
 * Converts IL Method -> Fruity -> WASM Binary
 */
static void *clr_compile_method_to_wasm(il_assembly_t *assembly,
                                        il_method_t *method, ulong *out_size) {
  il_to_fruity_error_t err;

  print("CLR: Compiling method %s to WASM...\n", method->name);

  /* 1. IL -> Fruity */
  fruity_function_t *func = il_to_fruity_convert_method(assembly, method, &err);
  if (!func) {
    print("CLR: IL->Fruity failed (err=%d)\n", err);
    return nil;
  }

  fruity_module_t temp_mod;
  memset(&temp_mod, 0, sizeof(temp_mod));
  temp_mod.name = method->name;
  temp_mod.functions_head = func;

  /* 2. Fruity -> WASM */
  fruity_wasm_result_t wasm_res;
  if (fruity_compile_to_wasm(&temp_mod, &wasm_res) != 0) {
    print("CLR: Fruity->WASM failed\n");
    fruity_free_function(func);
    return nil;
  }

  fruity_free_function(func);

  print("CLR: WASM binary generated (%ld bytes)\n", wasm_res.wasm_size);
  if (out_size)
    *out_size = wasm_res.wasm_size;
  return wasm_res.wasm_binary;
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
 * clr_execute_assembly
 */
int clr_execute_assembly(void *dll_data, ulong dll_size,
                         const char *entry_point) {
  char errbuf[128];
  il_error_t err;

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
  ulong wasm_size;
  void *wasm_bytes = clr_compile_method_to_wasm(assembly, main, &wasm_size);
  if (!wasm_bytes) {
    return -1;
  }

  /* Instantiate WASM3 */
  print("CLR: Initializing WASM3...\n");
  IM3Environment env = m3_NewEnvironment();
  if (!env) {
    print("m3_NewEnvironment failed\n");
    return -1;
  }

  IM3Runtime runtime = m3_NewRuntime(env, 64 * 1024, nil);
  if (!runtime) {
    print("m3_NewRuntime failed\n");
    return -1;
  }

  IM3Module module;
  M3Result result = m3_ParseModule(env, &module, wasm_bytes, (u32int)wasm_size);
  if (result) {
    print("m3_ParseModule: %s\n", result);
    return -1;
  }

  result = m3_LoadModule(runtime, module);
  if (result) {
    print("m3_LoadModule: %s\n", result);
    return -1;
  }

  /* Link Host Functions */
  result = lux9_link_wasi(module);
  if (result) {
    print("lux9_link_wasi: %s\n", result);
    return -1;
  }

  /* Run Main */
  IM3Function f;
  result = m3_FindFunction(&f, runtime, "Main");
  if (result) {
    print("m3_FindFunction: %s\n", result);
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
  /* Target Method: "Answer" (which calls Add(20, 22) -> 42) */

  if (clr_execute_assembly(test_assembly_bytes, test_assembly_len, "Answer") ==
      0) {
    print("CLR-TEST: *** FULL PIPELINE TEST PASSED ***\n");
  } else {
    print("CLR-TEST: *** FULL PIPELINE TEST FAILED ***\n");
  }
}

void clr_init(void) {
  print("CLR (WASM Backend) Initialized\n");
  clr_test_wasm_pipeline();
}