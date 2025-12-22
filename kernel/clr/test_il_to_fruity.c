/* test_il_to_fruity.c - Test IL → Fruity IR converter
 *
 * Compile:
 *   gcc -o test_il_to_fruity test_il_to_fruity.c il_to_fruity.c il_parser.c
 * il_disasm.c -I. -I./fruity
 *
 * Test:
 *   fsc test_hello.fs
 *   ./test_il_to_fruity test_hello.dll
 */

#include "fruity/fruity_ir.h"
#include "il_disasm.h"
#include "il_parser.h"
#include "il_to_fruity.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Kernel Stubs */
void *xalloc(size_t size) { return malloc(size); }
void *xallocz(size_t size, int zero) { return calloc(1, size); }
void xfree(void *ptr) { free(ptr); }

int print(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vprintf(fmt, args);
  va_end(args);
  return ret;
}

int snprint(char *buf, int len, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(buf, len, fmt, args);
  va_end(args);
  return ret;
}

void uartputs(const char *s) { printf("%s", s); }
void panic(char *msg) {
  printf("PANIC: %s\n", msg);
  exit(1);
}

/* Print Fruity instruction */
void print_fruity_instruction(fruity_instruction_t *instr) {
  printf("  [IL_%04x] ", instr->msil_offset);

  switch (instr->opcode) {
  case FRUITY_NOP:
    printf("FRUITY_NOP");
    break;
  case FRUITY_LDC_I4:
    printf("FRUITY_LDC_I4 %d", instr->operand.value.i32);
    break;
  case FRUITY_LDC_I8:
    printf("FRUITY_LDC_I8 %ld", instr->operand.value.i64);
    break;
  case FRUITY_LDNULL:
    printf("FRUITY_LDNULL");
    break;
  case FRUITY_DUP:
    printf("FRUITY_DUP");
    break;
  case FRUITY_POP:
    printf("FRUITY_POP");
    break;
  case FRUITY_LOAD_LOCAL:
    printf("FRUITY_LOAD_LOCAL %u", instr->operand.value.index);
    break;
  case FRUITY_STORE_LOCAL:
    printf("FRUITY_STORE_LOCAL %u", instr->operand.value.index);
    break;
  case FRUITY_LOAD_ARG:
    printf("FRUITY_LOAD_ARG %u", instr->operand.value.index);
    break;
  case FRUITY_ADD:
    printf("FRUITY_ADD");
    break;
  case FRUITY_SUB:
    printf("FRUITY_SUB");
    break;
  case FRUITY_MUL:
    printf("FRUITY_MUL");
    break;
  case FRUITY_DIV:
    printf("FRUITY_DIV");
    break;
  case FRUITY_REM:
    printf("FRUITY_REM");
    break;
  case FRUITY_AND:
    printf("FRUITY_AND");
    break;
  case FRUITY_OR:
    printf("FRUITY_OR");
    break;
  case FRUITY_XOR:
    printf("FRUITY_XOR");
    break;
  case FRUITY_NOT:
    printf("FRUITY_NOT");
    break;
  case FRUITY_NEG:
    printf("FRUITY_NEG");
    break;
  case FRUITY_LOAD_STRING:
    printf("FRUITY_LOAD_STRING 0x%08x", instr->operand.value.i32);
    break;
  case FRUITY_NEWOBJ:
    printf("FRUITY_NEWOBJ 0x%08x", instr->operand.value.token);
    break;
  case FRUITY_NEWARR:
    printf("FRUITY_NEWARR 0x%08x", instr->operand.value.token);
    break;
  case FRUITY_CALL:
    printf("FRUITY_CALL 0x%08x", instr->operand.value.token);
    break;
  case FRUITY_RET:
    printf("FRUITY_RET");
    break;
  case FRUITY_LIME:
    printf("FRUITY_LIME 0x%08x", instr->operand.value.token);
    break;
  case FRUITY_JUMP:
    printf("FRUITY_JUMP");
    break;
  case FRUITY_SWITCH: {
    printf("FRUITY_SWITCH (%u targets)",
           instr->operand.value.switch_targets->count);
    for (u32int i = 0; i < instr->operand.value.switch_targets->count; i++) {
      printf(" -> Block %u",
             instr->operand.value.switch_targets->targets[i]->block_id);
    }
    break;
  }
  default:
    printf("UNKNOWN(%d)", instr->opcode);
    break;
  }
  printf("\n");
}

/* Print Fruity basic block */
void print_fruity_block(fruity_basic_block_t *block) {
  printf("\nBlock %u: (%lu instructions)\n", block->block_id,
         block->instruction_count);
  for (fruity_instruction_t *instr = block->instructions_head; instr != NULL;
       instr = instr->next) {
    print_fruity_instruction(instr);
  }
}

/* Print Fruity function */
void print_fruity_function(fruity_function_t *func) {
  printf("\n=== Fruity Function: %s ===\n", func->name);
  printf("Method token: 0x%08x\n", func->method_token);
  printf("Block count: %lu\n", func->block_count);

  for (fruity_basic_block_t *block = func->blocks_head; block != NULL;
       block = block->next) {
    print_fruity_block(block);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <assembly.dll>\n", argv[0]);
    return 1;
  }

  const char *path = argv[1];

  printf("=== IL Parser Test ===\n");
  printf("Parsing: %s\n\n", path);

  /* Parse IL assembly */
  il_error_t il_error;
  il_assembly_t *assembly = il_parse_assembly(path, &il_error);
  if (assembly == NULL) {
    fprintf(stderr, "Error parsing assembly: %s\n", il_error_string(il_error));
    return 1;
  }

  printf("Assembly parsed successfully!\n");
  printf("PE sections: %u\n", assembly->section_count);
  printf("CLI version: %u.%u\n", assembly->cli_header.major_runtime_version,
         assembly->cli_header.minor_runtime_version);
  printf(
      "ImplMap count: %lu\n",
      (unsigned long)assembly->tables_header.row_counts[0x1C]); // TABLE_IMPLMAP
  printf("Entry point token: 0x%08x\n\n",
         assembly->cli_header.entry_point_token);

  /* Get entry point method */
  il_method_t *method = NULL;
  if (assembly->cli_header.entry_point_token != 0) {
    method = il_get_method_by_token(assembly,
                                    assembly->cli_header.entry_point_token);
    if (method == NULL) {
      /* Try getting main method by name */
      method = il_get_method(assembly, "main");
    }
  }

  if (method == NULL) {
    fprintf(stderr, "Could not find entry point method\n");
    il_free_assembly(assembly);
    return 1;
  }

  printf("=== Entry Point Method ===\n");
  printf("Name: %s\n", method->name ? method->name : "(null)");
  printf("Max stack: %u\n", method->max_stack);
  printf("IL code size: %lu bytes\n", method->il_code_size);
  printf("Flags: 0x%02x\n\n", method->flags);

  /* Disassemble IL */
  printf("=== IL Disassembly ===\n");
  il_disassemble_method(method);

  /* Pre-load all methods for assembly conversion */
  il_load_all_methods(assembly);

  /* Convert to Fruity IR (Full Assembly) */
  printf("\n=== Converting Assembly to Fruity IR ===\n");
  il_to_fruity_error_t fruity_error;
  fruity_module_t *module =
      il_to_fruity_convert_assembly(assembly, &fruity_error);

  if (module == NULL) {
    fprintf(stderr, "Error converting assembly: %s\n",
            il_to_fruity_error_string(fruity_error));
    il_free_method(method);
    il_free_assembly(assembly);
    return 1;
  }

  /* Print all functions to see imports */
  printf("Conversion successful! Functions: %lu\n", module->function_count);
  for (fruity_function_t *f = module->functions_head; f; f = f->next) {
    if (f->import_info.is_import) {
      printf("IMPORT: Token=0x%x Name=%s Module=%s Func=%s Args=%lu\n",
             f->method_token, f->name, f->import_info.module_name,
             f->import_info.function_name, f->arg_count);
    } else {
      printf("FUNC: Token=0x%x Name=%s\n", f->method_token, f->name);
    }
  }

  printf("Conversion successful!\n");

  /* Cleanup */
  // fruity_module_destroy(module); // Not implemented in userspace test fully?
  // Just free assembly/method
  il_free_method(method);
  il_free_assembly(assembly);

  printf("\n=== Test Complete ===\n");
  return 0;
}
