/* main.c - llvm2fruity standalone tool
 *
 * Converts LLVM bitcode (.bc) to Fruity IR for CLR pipeline.
 * Usage: llvm2fruity input.bc -o output.dll
 *
 * This is a bootstrap tool for compiling C++/SYCL to .NET DLLs.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimal type definitions for standalone build */
typedef uint8_t u8int;
typedef uint16_t u16int;
typedef uint32_t u32int;
typedef uint64_t u64int;
typedef int64_t s64int;
typedef size_t ulong;
#define nil NULL

void *smalloc(size_t size) { return malloc(size); }
int snprint(char *buf, size_t size, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return r;
}
void print(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

#include "llvm_parser.h"

/* Forward declaration */
int llvm_compile_bitcode(u8int *bc_data, ulong bc_size, void *asm_page,
                         char *errbuf, ulong errbuf_size);

static void usage(const char *prog) {
  fprintf(stderr, "llvm2fruity - LLVM to Fruity IR compiler\n");
  fprintf(stderr, "Usage: %s input.bc [-o output.dll]\n\n", prog);
  fprintf(stderr, "Converts LLVM bitcode to .NET DLL via Fruity IR.\n");
  fprintf(stderr, "Used to bootstrap SYCL and C++ libraries for Lux9.\n");
}

int main(int argc, char **argv) {
  const char *input_file = NULL;
  const char *output_file = NULL;

  /* Parse arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      output_file = argv[++i];
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    } else if (argv[i][0] != '-') {
      input_file = argv[i];
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      usage(argv[0]);
      return 1;
    }
  }

  if (!input_file) {
    fprintf(stderr, "Error: No input file specified\n");
    usage(argv[0]);
    return 1;
  }

  /* Default output file */
  if (!output_file) {
    static char default_output[256];
    const char *dot = strrchr(input_file, '.');
    size_t base_len = dot ? (size_t)(dot - input_file) : strlen(input_file);
    snprintf(default_output, sizeof(default_output), "%.*s.dll", (int)base_len,
             input_file);
    output_file = default_output;
  }

  printf("llvm2fruity: %s -> %s\n", input_file, output_file);

  /* Read input file */
  FILE *f = fopen(input_file, "rb");
  if (!f) {
    perror("Failed to open input file");
    return 1;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  u8int *data = malloc(size);
  if (!data) {
    fprintf(stderr, "Out of memory\n");
    fclose(f);
    return 1;
  }

  if (fread(data, 1, size, f) != (size_t)size) {
    fprintf(stderr, "Failed to read file\n");
    free(data);
    fclose(f);
    return 1;
  }
  fclose(f);

  /* Parse LLVM bitcode */
  char errbuf[256];
  llvm_module_t *mod = NULL;

  if (llvm_parse_bitcode(data, size, &mod, errbuf, sizeof(errbuf)) < 0) {
    fprintf(stderr, "Parse error: %s\n", errbuf);
    free(data);
    return 1;
  }

  printf("Parsed LLVM module:\n");
  llvm_module_dump(mod);

  /* TODO: Translate to Fruity IR and emit .NET DLL */
  printf("\n[TODO] Fruity translation and PE emission not yet integrated.\n");
  printf("Module parsed successfully - %u functions\n", mod->function_count);

  llvm_module_destroy(mod);
  free(data);

  return 0;
}
