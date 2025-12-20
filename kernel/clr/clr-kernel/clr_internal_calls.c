/* clr_internal_calls.c - Internal Call Dispatcher
 *
 * Maps managed InternalCall methods to native C kernel functions.
 */

/* #include "clr_kernel_architecture.h" - Removed to avoid dependency on Mach
 * headers */
#include "../../include/dat.h"
/* #include "clr_kernel_architecture.h" - Removed to avoid dependency on Mach
 * headers */
#include "../../include/dat.h"
#include "../../include/fns.h"
#include "../../include/u.h"
#include "../qbe/kernel_compat.h"

/* Forward declarations of kernel functions */
extern void panic(char *fmt, ...);
extern int print(char *fmt, ...);

/* Helper to convert managed string to C string
 * Managed string layout: [int32 length][uint16 chars...]
 * We convert UTF-16LE to ASCII for kernel use.
 */
static char extracted_buffer[256];
static char *extract_string(void *managed_str) {
  if (!managed_str)
    return "(null)";

  /* Read length (first 4 bytes) */
  int32_t len = *(int32_t *)managed_str;
  if (len <= 0 || len >= 255)
    len = 255;

  /* Read UTF-16LE chars and convert to ASCII */
  uint16_t *chars = (uint16_t *)((uint8_t *)managed_str + 4);
  for (int i = 0; i < len && i < 255; i++) {
    /* Simple ASCII extraction (ignore high byte) */
    extracted_buffer[i] = (char)(chars[i] & 0xFF);
  }
  extracted_buffer[len] = '\0';

  return extracted_buffer;
}

/* Lux9.Kernel.Kernel::Panic */
void Lux9_Kernel_Kernel_Panic(void *str_obj) {
  char *msg = extract_string(str_obj);
  panic("CLR PANIC: %s", msg);
}

/* Lux9.Kernel.Kernel::Print */
void Lux9_Kernel_Kernel_Print(void *str_obj) {
  char *msg = extract_string(str_obj);
  print("%s", msg);
}

/* CorElementType sizes for type token */
static uint32_t get_element_type_size(uint8_t element_type) {
  switch (element_type) {
  case 0x02:
    return 1; /* BOOLEAN */
  case 0x03:
    return 2; /* CHAR */
  case 0x04:
    return 1; /* I1 */
  case 0x05:
    return 1; /* U1 */
  case 0x06:
    return 2; /* I2 */
  case 0x07:
    return 2; /* U2 */
  case 0x08:
    return 4; /* I4 */
  case 0x09:
    return 4; /* U4 */
  case 0x0A:
    return 8; /* I8 */
  case 0x0B:
    return 8; /* U8 */
  case 0x0C:
    return 4; /* R4 */
  case 0x0D:
    return 8; /* R8 */
  case 0x18:
    return 8; /* I (native int) */
  case 0x19:
    return 8; /* U (native uint) */
  default:
    return 8; /* Reference types = pointer size */
  }
}

/* Lux9.Kernel.Pebble::Allocate */
void *Lux9_Kernel_Pebble_Allocate(uint32_t type_token) {
  /* Extract element type from token for sizing */
  uint8_t element_type = type_token & 0xFF;
  uint32_t size = get_element_type_size(element_type);
  if (size < 16)
    size = 16; /* Minimum object size */

  /* Use kernel allocator (Pebble integration would go here) */
  extern void *xalloc(unsigned long);
  return xalloc(size);
}

/* Internal Call Table Entry */
typedef struct {
  const char *class_name;
  const char *method_name;
  void *native_func;
} internal_call_entry_t;

static internal_call_entry_t internal_calls[] = {
    {"Lux9.Kernel.Kernel", "Panic", Lux9_Kernel_Kernel_Panic},
    {"Lux9.Kernel.Kernel", "Print", Lux9_Kernel_Kernel_Print},
    // Add others...
    {NULL, NULL, NULL}};

/* Resolver */
void *clr_resolve_internal_call(const char *cls, const char *method) {
  for (int i = 0; internal_calls[i].class_name; i++) {
    if (strcmp((char *)internal_calls[i].class_name, (char *)cls) == 0 &&
        strcmp((char *)internal_calls[i].method_name, (char *)method) == 0) {
      return internal_calls[i].native_func;
    }
  }
  return NULL;
}
