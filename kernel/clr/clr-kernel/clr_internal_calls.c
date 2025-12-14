/* clr_internal_calls.c - Internal Call Dispatcher
 *
 * Maps managed InternalCall methods to native C kernel functions.
 */

#include "clr_kernel_architecture.h"
#include "../../include/u.h"
#include "../../include/dat.h"
#include "../../include/fns.h"

/* Forward declarations of kernel functions */
extern void panic(char *fmt, ...);
extern int print(char *fmt, ...);

/* Helper to convert managed string to C string */
static char *extract_string(void *managed_str) {
    if (!managed_str) return "null";
    /* TODO: Implement proper string extraction from managed object */
    /* For now, assume it's just a raw pointer to data for simple testing */
    return (char *)((uint8_t *)managed_str + 4); /* Skip header/length */
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

/* Lux9.Kernel.Pebble::Allocate */
void *Lux9_Kernel_Pebble_Allocate(uint32_t size) {
    /* TODO: Get size from type token */
    void *obj;
    /* Use Pebble allocator */
    // pebble_alloc(size, &obj);
    return obj;
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
    {NULL, NULL, NULL}
};

/* Resolver */
void *clr_resolve_internal_call(const char *cls, const char *method) {
    for (int i = 0; internal_calls[i].class_name; i++) {
        if (strcmp(internal_calls[i].class_name, cls) == 0 &&
            strcmp(internal_calls[i].method_name, method) == 0) {
            return internal_calls[i].native_func;
        }
    }
    return NULL;
}
