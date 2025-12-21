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
  ulong wasm_size;
  void *wasm_bytes = clr_compile_method_to_wasm(assembly, main, &wasm_size);
  if (!wasm_bytes) {
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

  print("CLR: Calling m3_ParseModule...\n");
  print("CLR: DEBUG: wasm_bytes=%p wasm_size=%lu\n", wasm_bytes, wasm_size);
  IM3Module module = NULL;
  M3Result result = m3_ParseModule(env, &module, wasm_bytes, (u32int)wasm_size);
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
