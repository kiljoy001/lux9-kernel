// runtime_host.c - Linux Host Runtime for Testing AOT Code
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Typedefs matching the AOT assumptions
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned long uintptr_t;

// ---- Object Layouts ----
typedef struct {
  uintptr_t header;
  uint32_t length;
  uint16_t first_char;
} StringObject;

// ---- Allocator ----
void *lux_alloc(size_t size, uint32_t token) { return calloc(1, size); }

// ---- CLR Internals ----

// Entry Point Token from Init.Main
extern void method_100663301(void *arg);

// Internal Calls
int clr_string_get_length(StringObject *str) {
  if (!str)
    return 0;
  return str->length;
}

void *clr_thread_get_current() { return NULL; }
void clr_p9_write(int fd, void *buf, int count) {}
void clr_p9_read() {}
void clr_p9_attach() {}
void clr_p9_clunk() {}
void clr_p9_stat() {}

void clr_write(StringObject *s) {
  if (!s)
    return;

  // Print to stdout
  int len = s->length;
  for (int i = 0; i < len; i++) {
    putchar((char)((uint16_t *)&s->first_char)[i]);
  }
}

void *clr_newobj(uint32_t token) { return malloc(64); }

StringObject *clr_string_from_literal(int id) {
  char *msg = "";
  if (id == 1)
    msg = "Hello AOT World!\n";
  if (id == 37)
    msg = "Kernel AOT Init Shim Loaded.\n";
  if (id == 97)
    msg = "Math Test: 10 + 20...\n";
  if (id == 143)
    msg = "Math Check OK.\n";
  if (id == 175)
    msg = "Math Check FAILED.\n";

  size_t len = strlen(msg);
  StringObject *obj = malloc(sizeof(StringObject) + len * 2 + 2);
  obj->length = len;
  uint16_t *chars = &obj->first_char;
  for (size_t i = 0; i < len; i++)
    chars[i] = msg[i];
  return obj;
}

int main() {
  printf("--- HOST TEST START ---\n");
  method_100663301(NULL);
  printf("--- HOST TEST END ---\n");
  return 0;
}
