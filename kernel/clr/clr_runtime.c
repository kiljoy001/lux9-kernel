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
                         ulong *out_size, char *errorbuf, ulong errorbuf_size);
/* qbe_compile_page: takes physical addresses of QBE text page and output code
 * page */
extern int qbe_compile_page(uintptr qbe_page, uintptr asm_page, char *errorbuf,
                            usize errorbuf_size);

/* Pebble API forward declarations */
typedef struct UserCapability UserCapability;
extern int pebble_black_alloc(ulong size, UserCapability *out_cap);
extern int pebble_black_free(const UserCapability *cap);
extern void *pebble_get_black_addr(const UserCapability *cap);

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

/* Static field table */
typedef struct {
  u32int field_token;
  void *data_ptr;
  ulong size;
} clr_static_field_t;

static clr_static_field_t *static_fields;
static ulong static_field_count;
static ulong static_field_capacity;

/*
 * clr_get_static_field - Get pointer to static field data by token
 */
void *clr_get_static_field(u32int token) {
  for (ulong i = 0; i < static_field_count; i++) {
    if (static_fields[i].field_token == token)
      return static_fields[i].data_ptr;
  }
  return nil;
}

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
 * clr_get_type_size - Get size of a type from its metadata token
 * Token format: upper byte = table index, lower 3 bytes = row index
 * Tables: 0x01=TypeRef, 0x02=TypeDef, 0x1B=TypeSpec
 */
ulong clr_get_type_size(u32int token) {
  u8int table = (token >> 24) & 0xFF;
  u32int row = token & 0x00FFFFFF;

  USED(table);
  USED(row);

  /* Common primitive type tokens (from System namespace) */
  /* These are based on CorElementType values often embedded in signatures */
  switch (token & 0xFF) {
  case 0x01:
    return 0; /* ELEMENT_TYPE_VOID */
  case 0x02:
    return 1; /* ELEMENT_TYPE_BOOLEAN */
  case 0x03:
    return 2; /* ELEMENT_TYPE_CHAR */
  case 0x04:
    return 1; /* ELEMENT_TYPE_I1 */
  case 0x05:
    return 1; /* ELEMENT_TYPE_U1 */
  case 0x06:
    return 2; /* ELEMENT_TYPE_I2 */
  case 0x07:
    return 2; /* ELEMENT_TYPE_U2 */
  case 0x08:
    return 4; /* ELEMENT_TYPE_I4 */
  case 0x09:
    return 4; /* ELEMENT_TYPE_U4 */
  case 0x0A:
    return 8; /* ELEMENT_TYPE_I8 */
  case 0x0B:
    return 8; /* ELEMENT_TYPE_U8 */
  case 0x0C:
    return 4; /* ELEMENT_TYPE_R4 */
  case 0x0D:
    return 8; /* ELEMENT_TYPE_R8 */
  case 0x0E:
    return 8; /* ELEMENT_TYPE_STRING (ptr) */
  case 0x18:
    return 8; /* ELEMENT_TYPE_I (native int) */
  case 0x19:
    return 8; /* ELEMENT_TYPE_U (native uint) */
  case 0x1C:
    return 8; /* ELEMENT_TYPE_OBJECT (ptr) */
  default:
    return 64; /* Unknown - use default object size */
  }
}

/*
 * clr_newobj - Allocate new object
 */
void *clr_newobj(u32int token) {
  ulong size = clr_get_type_size(token);
  if (size < 16)
    size = 16; /* Minimum object size (header + vtable) */
  return xalloc(size);
}

/*
 * clr_newarr - Allocate new array
 */
void *clr_newarr(u32int token, u32int count) {
  ulong element_size = clr_get_type_size(token);
  if (element_size == 0)
    element_size = 8; /* Default to pointer size */

  /* Array layout: [length:4][type_ptr:8][elements...] */
  ulong header_size = sizeof(u32int) + sizeof(uintptr);
  ulong total = header_size + count * element_size;

  void *arr = xalloc(total);
  if (arr) {
    *(u32int *)arr = count; /* Store length */
  }
  return arr;
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
  extern void uartputs(char *, int);
  char debug_buf[128];
  il_to_fruity_error_t err;

  print("CLR: clr_compile_method ENTERED method=%s\n", method->name);
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr_compile_method ENTER method=%s\n", method->name);
  uartputs(debug_buf, strlen(debug_buf));

  /* Step 1: IL → Fruity IR */
  print("CLR: Step 1 - calling il_to_fruity_convert_method\n");
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr Step 1: IL->Fruity\n");
  uartputs(debug_buf, strlen(debug_buf));
  fruity_function_t *func = il_to_fruity_convert_method(assembly, method, &err);
  print("CLR: il_to_fruity_convert_method returned %p\n", func);
  if (func == nil) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr IL->Fruity FAILED err=%d\n", err);
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: IL→Fruity failed for %s: error %d\n", method->name, err);
    return nil;
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr IL->Fruity SUCCESS func=%p\n", func);
  uartputs(debug_buf, strlen(debug_buf));

  /* Wrap in a temporary module for fruity_to_qbe */
  print("CLR: Wrapping function in temp module\n");
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr wrapping in temp module\n");
  uartputs(debug_buf, strlen(debug_buf));
  fruity_module_t temp_mod;
  memset(&temp_mod, 0, sizeof(temp_mod));
  temp_mod.name = method->name;
  temp_mod.functions_head = func;
  temp_mod.functions_tail = func;
  temp_mod.function_count = 1;

  /* Step 2: Fruity IR → QBE text (Two-pass with Pebble allocation) */
  char errbuf[128];

  /* Pass 1: Query required size */
  print("CLR: Step 2a - Querying QBE IL size\n");
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr Step 2a: Querying QBE IL size\n");
  uartputs(debug_buf, strlen(debug_buf));
  ulong qbe_il_size = 0;
  int qbe_result =
      fruity_to_qbe(&temp_mod, 0, &qbe_il_size, errbuf, sizeof(errbuf));
  print("CLR: fruity_to_qbe returned %d, size=%ld\n", qbe_result, qbe_il_size);
  if (qbe_result < 0 || qbe_il_size == 0) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr fruity_to_qbe size query FAILED: %s\n", errbuf);
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: Failed to query QBE IL size for %s: %s\n", method->name,
          errbuf);
    fruity_free_function(func);
    return nil;
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr QBE IL size query: %lud bytes\n", qbe_il_size);
  uartputs(debug_buf, strlen(debug_buf));

  /* Pass 2: Allocate Pebble black token with exact size */
  print("CLR: Step 2b - Allocating Pebble black token (%ld bytes)\n",
        qbe_il_size);
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr Step 2b: Allocating Pebble black token (%lud bytes)\n",
          qbe_il_size);
  uartputs(debug_buf, strlen(debug_buf));
  UserCapability qbe_cap;
  if (pebble_black_alloc(qbe_il_size, &qbe_cap) != 0) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr pebble_black_alloc FAILED\n");
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: pebble_black_alloc(%lud) failed for %s\n", qbe_il_size,
          method->name);
    fruity_free_function(func);
    return nil;
  }
  print("CLR: pebble_black_alloc succeeded\n");

  void *qbe_page = pebble_get_black_addr(&qbe_cap);
  if (qbe_page == nil) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr pebble_get_black_addr FAILED\n");
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: pebble_get_black_addr failed for %s\n", method->name);
    pebble_black_free(&qbe_cap);
    fruity_free_function(func);
    return nil;
  }
  print("CLR: qbe_page=%p\n", qbe_page);
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr allocated qbe_page=%p\n",
          qbe_page);
  uartputs(debug_buf, strlen(debug_buf));

  /* Pass 3: Fill the page with actual QBE IL */
  print("CLR: Step 2c - Generating QBE IL to page\n");
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr Step 2c: Generating QBE IL to page\n");
  uartputs(debug_buf, strlen(debug_buf));
  qbe_result = fruity_to_qbe(&temp_mod, (uintptr)PADDR(qbe_page), &qbe_il_size,
                             errbuf, sizeof(errbuf));
  print("CLR: fruity_to_qbe (gen) returned %d\n", qbe_result);
  if (qbe_result < 0) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr fruity_to_qbe FAILED: %s\n", errbuf);
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: Fruity→QBE failed for %s: %s\n", method->name, errbuf);
    pebble_black_free(&qbe_cap);
    fruity_free_function(func);
    return nil;
  }
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr fruity_to_qbe SUCCESS\n");
  uartputs(debug_buf, strlen(debug_buf));

  ulong qbe_len = strlen((char *)qbe_page);
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr qbe_len=%ld\n", qbe_len);
  uartputs(debug_buf, strlen(debug_buf));
  uartputs("DEBUG: Generated QBE IL:\n", 25);
  uartputs((char *)qbe_page, qbe_len);
  uartputs("DEBUG: End of QBE IL\n", 21);

  /* Step 3: QBE text → x86-64 binary */
  print("CLR: Step 3 - QBE to x86-64\n");
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr Step 3: QBE->x86-64, allocating code page\n");
  uartputs(debug_buf, strlen(debug_buf));
  ulong code_page_size = 4096; /* 4KB page for code */
  void *code_page = xalloc(code_page_size);
  if (code_page == nil) {
    pebble_black_free(&qbe_cap);
    fruity_free_function(func);
    return nil;
  }
  print("CLR: code_page=%p\n", code_page);
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr allocated code_page=%p\n",
          code_page);
  uartputs(debug_buf, strlen(debug_buf));

  print("CLR: calling qbe_compile_page\n");
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr calling qbe_compile_page\n");
  uartputs(debug_buf, strlen(debug_buf));
  char qbe_errbuf[128];
  /* qbe_compile_page expects physical addresses */
  int result = qbe_compile_page(PADDR(qbe_page), PADDR(code_page), qbe_errbuf,
                                sizeof(qbe_errbuf));
  print("CLR: qbe_compile_page returned %d\n", result);
  if (result < 0) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr qbe_compile_page FAILED: %s\n", qbe_errbuf);
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: QBE→x86-64 failed for %s: %s\n", method->name, qbe_errbuf);
    xfree(code_page);
    pebble_black_free(&qbe_cap);
    fruity_free_function(func);
    return nil;
  }
  /* result is 0 on success - need to calculate actual code size differently */
  int code_size = (int)code_page_size; /* Use full page for now */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr qbe_compile_page SUCCESS\n");
  uartputs(debug_buf, strlen(debug_buf));

  /* Cleanup intermediate buffers */
  pebble_black_free(&qbe_cap); /* Safe - no header corruption with Pebble */
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
  extern void uartputs(char *, int);
  char errbuf[128];
  char debug_buf[128];

  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr_execute_assembly ENTERED dll_size=%ld\n", dll_size);
  uartputs(debug_buf, strlen(debug_buf));

  /* Load assembly */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr calling clr_load_assembly\n");
  uartputs(debug_buf, strlen(debug_buf));
  il_assembly_t *assembly =
      clr_load_assembly(dll_data, dll_size, errbuf, sizeof(errbuf));
  if (assembly == nil) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr_load_assembly FAILED: %s\n", errbuf);
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: %s\n", errbuf);
    return -1;
  }
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr_load_assembly returned %p\n", assembly);
  uartputs(debug_buf, strlen(debug_buf));

  /* Find entry point */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr calling clr_find_entry_point\n");
  uartputs(debug_buf, strlen(debug_buf));
  il_method_t *main = clr_find_entry_point(assembly);
  if (main == nil) {
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: clr_find_entry_point FAILED\n");
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: no entry point found\n");
    il_free_assembly(assembly);
    return -1;
  }
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr found entry point: %s\n",
          main->name);
  uartputs(debug_buf, strlen(debug_buf));

  /* Compile entry point */
  snprint(debug_buf, sizeof(debug_buf),
          "DEBUG: clr calling clr_compile_method\n");
  uartputs(debug_buf, strlen(debug_buf));
  ulong code_size;
  void *code = clr_compile_method(assembly, main, &code_size);
  if (code == nil) {
    snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr_compile_method FAILED\n");
    uartputs(debug_buf, strlen(debug_buf));
    print("clr: failed to compile entry point\n");
    il_free_assembly(assembly);
    return -1;
  }
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr compiled %ld bytes at %p\n",
          code_size, code);
  uartputs(debug_buf, strlen(debug_buf));
  print("CLR: compiled entry point, code size=%ld at %p\n", code_size, code);

  /* Execute! */
  /* Cast to function pointer and call */
  /* For init: void Main() or int Main() */
  print("CLR: about to execute Main()\n");
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr about to execute Main()\n");
  uartputs(debug_buf, strlen(debug_buf));
  typedef int (*main_func_t)(void);
  main_func_t main_fn = (main_func_t)code;

  print("CLR: calling main_fn at %p\n", main_fn);
  int result = main_fn();

  snprint(debug_buf, sizeof(debug_buf), "DEBUG: clr Main() returned %d\n",
          result);
  uartputs(debug_buf, strlen(debug_buf));
  print("clr: Main() returned %d\n", result);

  /* Cleanup - temporarily disabled due to allocator mismatch causing xfree
   * panic.
   * TODO: Fix allocator consistency across all CLR modules.
   * This is a small memory leak but allows init to complete successfully. */
  /* xfree(code); */
  /* il_free_assembly(assembly); */
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

/*
 * Implementation of System.Threading.Thread.Sleep(int ms)
 */
void clr_thread_sleep(int ms) {
  if (up == nil)
    return;

  if (ms < 0) {
    /* Infinite sleep not supported yet, default to 1 min to allow break */
    ms = 60000;
  }

  /* tsleep for ms milliseconds
   * return0 means "condition never met", so it waits for timeout or interrupt
   */
  tsleep(&up->sleep, return0, 0, (ulong)ms);
}

/*
 * Process creation support
 */
#define RFPROC (1 << 4)
#define RFFDG (1 << 2)
#define RFNOTEG (1 << 3)

extern uintptr sysrfork(void *);
extern uintptr sysexec(void *);
extern void pexit(char *, int);

int clr_process_start(char *cmd, char *args_str) {
  /* fork */
  ulong fork_args[1];
  fork_args[0] = RFPROC | RFFDG | RFNOTEG;

  int pid = (int)sysrfork(fork_args);
  /* sysrfork returns pid in parent, 0 in child.
     Note: In kernel mode, sysrfork returns directly. */

  if (pid < 0)
    return pid;

  if (pid == 0) {
    /* Child process */
    /* Construct argv. We need to split args_str if possible,
       but for now simple [cmd, args_str, nil] */

    char *argv[4]; /* cmd, arg1, arg2, nil */
    int argc = 0;

    argv[argc++] = cmd;
    if (args_str && *args_str) {
      /* TODO: Improper tokenize, just treating whole string as one arg */
      argv[argc++] = args_str;
    }
    argv[argc] = nil;

    ulong exec_args[2];
    exec_args[0] = (ulong)cmd;
    exec_args[1] = (ulong)argv;

    sysexec(exec_args);

    /* If sysexec returns, it failed */
    pexit("exec failed", 1);
  }

  return pid;
}
