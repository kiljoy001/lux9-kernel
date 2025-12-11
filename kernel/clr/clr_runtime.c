/*
 * clr_runtime.c - CLR Runtime Entry Point
 *
 * Main entry point for loading and executing .NET assemblies.
 * Wires together: il_parser → il_to_fruity → fruity_to_qbe → qbe_compile
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

/* Forward declarations */
extern int fruity_to_qbe(fruity_module_t *module, uintptr out_handle,
                         char *errorbuf, ulong errorbuf_size);
extern int qbe_compile_page(char *qbe_text, ulong qbe_len, void *out_page,
                            ulong page_size);

/* String literal table (populated during compilation) */
typedef struct {
  u32int us_index;   /* Index into #US heap */
  char *cstr;        /* C string value */
  void *managed_obj; /* Managed System.String object (allocated at runtime) */
} clr_string_literal_t;

static clr_string_literal_t *string_literals;
static ulong string_literal_count;
static ulong string_literal_capacity;

/* Compiled method cache */
typedef struct {
  u32int method_token;
  char *name;
  void *native_code; /* Pointer to compiled native code */
  ulong code_size;
} clr_compiled_method_t;

static clr_compiled_method_t *compiled_methods;
static ulong compiled_method_count;
static ulong compiled_method_capacity;

/* Current assembly */
static il_assembly_t *current_assembly;

/*
 * clr_string_from_literal - Create managed string from #US heap index
 * Called by compiled code for LDSTR instruction.
 */
void *clr_string_from_literal(u32int us_index) {
  /* Search string literal table */
  for (ulong i = 0; i < string_literal_count; i++) {
    if (string_literals[i].us_index == us_index) {
      /* Already created, return cached object */
      if (string_literals[i].managed_obj)
        return string_literals[i].managed_obj;

      /* Create managed string object */
      char *cstr = string_literals[i].cstr;
      ulong len = strlen(cstr);

      /* Allocate: [s32int length][u16int chars...] */
      ulong size = sizeof(s32int) + len * sizeof(u16int);
      void *data = xalloc(size);
      if (data == nil)
        return nil;

      /* Fill in length */
      *(s32int *)data = (s32int)len;

      /* Fill in chars (ASCII to UTF-16LE) */
      u16int *chars = (u16int *)((s32int *)data + 1);
      for (ulong j = 0; j < len; j++) {
        chars[j] = (u16int)(uchar)cstr[j];
      }

      /* Create managed object wrapper */
      /* For MVP, just return the data pointer */
      /* Full impl would use pebble_alloc + CLR object header */
      string_literals[i].managed_obj = data;
      return data;
    }
  }

  /* Not found - extract from assembly */
  if (current_assembly == nil)
    return nil;

  char *cstr = il_get_user_string(current_assembly, us_index);
  if (cstr == nil)
    return nil;

  /* Add to table */
  if (string_literal_count >= string_literal_capacity) {
    ulong new_cap = string_literal_capacity ? string_literal_capacity * 2 : 16;
    clr_string_literal_t *new_table =
        xalloc(new_cap * sizeof(clr_string_literal_t));
    if (new_table == nil)
      return nil;

    if (string_literals) {
      memmove(new_table, string_literals,
              string_literal_count * sizeof(clr_string_literal_t));
      xfree(string_literals);
    }
    string_literals = new_table;
    string_literal_capacity = new_cap;
  }

  string_literals[string_literal_count].us_index = us_index;
  string_literals[string_literal_count].cstr = cstr;
  string_literals[string_literal_count].managed_obj = nil;
  string_literal_count++;

  /* Recurse to create the object */
  return clr_string_from_literal(us_index);
}

/*
 * clr_find_entry_point - Find the Main() method in an assembly
 */
static il_method_t *clr_find_entry_point(il_assembly_t *assembly) {
  /* First try: Look for "Main" */
  il_method_t *main = il_get_method(assembly, "Main");
  if (main)
    return main;

  /* Second try: Look for "<Main>$" (top-level statements) */
  main = il_get_method(assembly, "<Main>$");
  if (main)
    return main;

  /* Third try: Use entry point token from CLI header */
  if (assembly->cli_header.entry_point_token) {
    main = il_get_method_by_token(assembly,
                                  assembly->cli_header.entry_point_token);
    if (main)
      return main;
  }

  return nil;
}

/*
 * clr_compile_method - Compile a single IL method to native code
 * Pipeline: IL → Fruity IR → QBE text → x86-64 binary
 */
static void *clr_compile_method(il_assembly_t *assembly, il_method_t *method,
                                ulong *out_size) {
  il_to_fruity_error_t err;

  /* Step 1: IL → Fruity IR */
  fruity_function_t *func = il_to_fruity_convert_method(assembly, method, &err);
  if (func == nil) {
    print("clr: IL→Fruity failed for %s: error %d\n", method->name, err);
    return nil;
  }

  /* Wrap in a temporary module for fruity_to_qbe */
  fruity_module_t temp_mod;
  memset(&temp_mod, 0, sizeof(temp_mod));
  temp_mod.name = method->name;
  temp_mod.functions_head = func;
  temp_mod.functions_tail = func;
  temp_mod.function_count = 1;

  /* Step 2: Fruity IR → QBE text */
  /* Allocate a page for QBE text output */
  void *qbe_page = xalloc(4096);
  if (qbe_page == nil) {
    fruity_free_function(func);
    return nil;
  }

  char errbuf[128];
  int qbe_result = fruity_to_qbe(&temp_mod, (uintptr)PADDR(qbe_page), errbuf,
                                 sizeof(errbuf));
  if (qbe_result < 0) {
    print("clr: Fruity→QBE failed for %s: %s\n", method->name, errbuf);
    xfree(qbe_page);
    fruity_free_function(func);
    return nil;
  }

  ulong qbe_len = strlen((char *)qbe_page);

  /* Step 3: QBE text → x86-64 binary */
  ulong code_page_size = 4096; /* 4KB page for code */
  void *code_page = xalloc(code_page_size);
  if (code_page == nil) {
    xfree(qbe_page);
    fruity_free_function(func);
    return nil;
  }

  int code_size =
      qbe_compile_page((char *)qbe_page, qbe_len, code_page, code_page_size);
  if (code_size <= 0) {
    print("clr: QBE→x86-64 failed for %s\n", method->name);
    xfree(code_page);
    xfree(qbe_page);
    fruity_free_function(func);
    return nil;
  }

  /* Cleanup intermediate buffers */
  xfree(qbe_page);
  fruity_free_function(func);

  if (out_size)
    *out_size = (ulong)code_size;
  return code_page;
}

/*
 * clr_load_assembly - Load a .NET assembly from memory
 */
il_assembly_t *clr_load_assembly(void *data, ulong size, char *errbuf,
                                 ulong errlen) {
  il_error_t err;
  il_assembly_t *assembly = il_parse_assembly_memory((u8int *)data, size, &err);

  if (assembly == nil) {
    if (errbuf) {
      snprint(errbuf, errlen, "failed to parse assembly: %s",
              il_error_string(err));
    }
    return nil;
  }

  current_assembly = assembly;
  return assembly;
}

/*
 * clr_execute_assembly - Load, compile, and execute .NET assembly
 * Returns the exit code from Main().
 */
int clr_execute_assembly(void *dll_data, ulong dll_size) {
  char errbuf[128];

  /* Load assembly */
  il_assembly_t *assembly =
      clr_load_assembly(dll_data, dll_size, errbuf, sizeof(errbuf));
  if (assembly == nil) {
    print("clr: %s\n", errbuf);
    return -1;
  }

  /* Find entry point */
  il_method_t *main = clr_find_entry_point(assembly);
  if (main == nil) {
    print("clr: no entry point found\n");
    il_free_assembly(assembly);
    return -1;
  }

  print("clr: compiling entry point: %s\n", main->name);

  /* Compile entry point */
  ulong code_size;
  void *code = clr_compile_method(assembly, main, &code_size);
  if (code == nil) {
    print("clr: failed to compile entry point\n");
    il_free_assembly(assembly);
    return -1;
  }

  print("clr: compiled %s (%ld bytes), executing...\n", main->name, code_size);

  /* Execute! */
  /* Cast to function pointer and call */
  /* For init: void Main() or int Main() */
  typedef int (*main_func_t)(void);
  main_func_t main_fn = (main_func_t)code;

  int result = main_fn();

  print("clr: Main() returned %d\n", result);

  /* Cleanup */
  xfree(code);
  il_free_assembly(assembly);
  current_assembly = nil;

  return result;
}

/*
 * clr_init - Initialize the CLR runtime
 * Called during kernel boot.
 */
void clr_init(void) {
  string_literals = nil;
  string_literal_count = 0;
  string_literal_capacity = 0;

  compiled_methods = nil;
  compiled_method_count = 0;
  compiled_method_capacity = 0;

  current_assembly = nil;

  print("CLR runtime initialized\n");
}
