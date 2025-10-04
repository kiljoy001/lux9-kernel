# GCC Plugin for Plan 9 C Dialect

## Overview

A GCC plugin to support Plan 9 C extensions, allowing native Plan 9 source code to be compiled with GCC without manual conversion.

## Plan 9 C Extensions

### 1. Anonymous Struct Embedding

**Plan 9 Syntax:**
```c
struct Lock {
    int locked;
    int key;
};

struct Timer {
    Tval twhen;
    Timer *tnext;
};

struct Proc {
    Lock;      // Anonymous embedding - no field name
    Timer;     // Embeds all Timer fields
    int pid;
};
```

**What it means:**
All fields from Lock and Timer are copied directly into Proc's layout:
```c
struct Proc {
    int locked;      // from Lock
    int key;         // from Lock
    Tval twhen;      // from Timer
    Timer *tnext;    // from Timer
    int pid;         // Proc's own field
};
```

**Access:**
```c
Proc *p;
p->locked = 1;    // Direct access, no p->lock.locked
p->twhen = 100;   // Direct access, no p->timer.twhen
```

**Plugin Implementation:**
1. Parse struct declarations
2. When encountering a type name with no field name (e.g., `Lock;`)
3. Look up the type definition
4. Insert all fields from that type into the current struct
5. Track field offsets to maintain proper layout

### 2. Pragma Extensions

**varargck - Variadic Argument Type Checking:**
```c
#pragma varargck argpos print 1
#pragma varargck type "s" char*
#pragma varargck type "lld" vlong
```

Tells the compiler:
- Which argument position to start checking (argpos)
- What C type corresponds to format specifiers (type)

**incomplete - Forward Declarations:**
```c
#pragma incomplete Tzone
```

Marks a type as intentionally incomplete (opaque pointer).

**Plugin Implementation:**
1. Register pragma handlers with GCC
2. Parse pragma arguments
3. Store type information for format string checking
4. Integrate with GCC's -Wformat warnings

### 3. UTF-8 Identifiers

**Plan 9 allows:**
```c
ulong µs(void);    // Microsecond timer function
```

**Status:** Already supported in GCC with `-fextended-identifiers` (default in C99+)

### 4. Struct-Variable Declaration

**Plan 9 syntax:**
```c
struct {
    Lock lk;
    uint n;
    char buf[16384];
} kmesg;
```

Simultaneously declares:
1. An anonymous struct type
2. A global variable `kmesg` of that type

**Problem with GCC:**
When this appears in a header file included by multiple .c files, each compilation unit creates its own copy in BSS, causing "multiple definition" linker errors.

**Plugin Solution:**
1. Detect struct-variable declarations in headers
2. Rewrite as: type definition + `extern` declaration
3. Generate a .c file with the actual definition

**Manual fix (what we did):**
```c
// Header file
struct Kmesg {
    Lock lk;
    uint n;
    char buf[16384];
};
extern struct Kmesg kmesg;

// One .c file (globals.c)
struct Kmesg kmesg;
```

## Plugin Architecture

### Implementation Approach

**Plugin Type:** GCC GIMPLE plugin

**Hook Points:**
- `PLUGIN_START_UNIT` - Parse source before GIMPLE generation
- `PLUGIN_FINISH_TYPE` - Rewrite struct definitions
- `PLUGIN_PRAGMAS` - Register custom pragma handlers

### Code Structure

```c
/* gcc-plan9-plugin.c */
#include "gcc-plugin.h"
#include "plugin-version.h"
#include "tree.h"
#include "gimple.h"
#include "tree-iterator.h"
#include "cp/cp-tree.h"

int plugin_is_GPL_compatible;

/* Expand anonymous struct embeddings */
static void expand_anonymous_structs(tree type) {
    tree field;

    for (field = TYPE_FIELDS(type); field; field = TREE_CHAIN(field)) {
        /* If field has type but no DECL_NAME, it's anonymous */
        if (TREE_CODE(field) == FIELD_DECL && !DECL_NAME(field)) {
            tree embed_type = TREE_TYPE(field);

            /* Insert all fields from embed_type here */
            tree embed_field;
            for (embed_field = TYPE_FIELDS(embed_type);
                 embed_field;
                 embed_field = TREE_CHAIN(embed_field)) {

                /* Clone field and insert into parent */
                tree new_field = copy_node(embed_field);
                TREE_CHAIN(new_field) = TREE_CHAIN(field);
                TREE_CHAIN(field) = new_field;
            }

            /* Remove the anonymous field marker */
            /* Update field offsets */
        }
    }
}

/* Handle #pragma varargck */
static void handle_pragma_varargck(cpp_reader *pfile) {
    tree arg1 = pragma_lex();  /* "argpos" or "type" */
    tree arg2 = pragma_lex();  /* function or format spec */
    tree arg3 = pragma_lex();  /* position or type */

    /* Store in format checking tables */
    register_format_spec(arg2, arg3);
}

/* Plugin initialization */
int plugin_init(struct plugin_name_args *info,
                struct plugin_gcc_version *version) {

    /* Check GCC version compatibility */
    if (!plugin_default_version_check(version, &gcc_version))
        return 1;

    /* Register callbacks */
    register_callback(info->base_name,
                     PLUGIN_FINISH_TYPE,
                     expand_anonymous_structs,
                     NULL);

    register_callback(info->base_name,
                     PLUGIN_PRAGMAS,
                     handle_pragma_varargck,
                     NULL);

    return 0;
}
```

### Build Process

**Compile plugin:**
```bash
gcc -I$(gcc -print-file-name=plugin)/include \
    -fPIC -shared \
    -o plan9.so \
    gcc-plan9-plugin.c
```

**Use in build:**
```bash
gcc -fplugin=./plan9.so \
    -fplugin-arg-plan9-verbose \
    mycode.c
```

**Integrate in build system:**
```makefile
PLAN9_PLUGIN := $(shell pwd)/plan9.so
CFLAGS += -fplugin=$(PLAN9_PLUGIN)
```

## Benefits

### For 9front Port
- **No source modification** - use original Plan 9 code as-is
- **Maintainability** - easier to merge upstream changes
- **Type safety** - proper format string checking with varargck
- **Portability** - plugin works on any GCC-supported platform

### For Plan 9 Community
- **Access to GCC ecosystem** - optimizations, sanitizers, analyzers
- **Cross-compilation** - build Plan 9 code for embedded targets
- **Modern tooling** - LSP, static analysis, debuggers
- **Performance** - GCC's optimization passes

## Implementation Complexity

### Easy (1-2 days)
- UTF-8 identifiers (already supported)
- Basic pragma parsing
- Struct-variable detection

### Medium (1 week)
- varargck integration with GCC format checking
- Anonymous struct field flattening
- Proper offset calculation

### Hard (2-3 weeks)
- Correct handling of nested anonymous structs
- Preserve alignment and padding
- Debug info generation for embedded fields
- Interaction with other GCC features (LTO, sanitizers)

## Testing Strategy

### Test Suite
```c
/* test-anonymous.c */
struct A { int x; int y; };
struct B { long z; };

struct C {
    A;
    B;
    int w;
};

int main(void) {
    struct C c;
    c.x = 1;  /* Should compile */
    c.y = 2;
    c.z = 3;
    c.w = 4;

    /* Check layout */
    assert(sizeof(struct C) == sizeof(int)*3 + sizeof(long));
    assert(&c.x == &c);  /* First field at offset 0 */
}
```

**Validation:**
1. Compile with Plan 9 native compiler
2. Compile with GCC + plugin
3. Compare struct layouts (offsets, sizes, alignment)
4. Compare generated assembly
5. Run Plan 9 test suite

## Related Work

### Similar Projects
- **9front's gcc patches** - Minimal GCC modifications for Plan 9 userspace
- **clang -fplan9-extensions** - Clang has similar support
- **Go's struct embedding** - Language-level feature inspired by Plan 9

### Differences from This Project
This project (lux9-kernel) chose **manual conversion** instead of a GCC plugin because:
1. One-time effort for kernel port
2. Explicit control over every transformation
3. No plugin dependency for builds
4. Educational value in understanding differences

A GCC plugin would be valuable for:
- Building 9front userspace with GCC
- Ongoing development with original sources
- Cross-platform Plan 9 development

## Future Enhancements

### Phase 2 Features
- **Automatic header splitting** - separate kernel/userspace headers
- **Plan 9 calling convention** - support RARG pseudo-registers
- **Section attributes** - map Plan 9 sections to ELF
- **Symbol namespacing** - handle Plan 9's flat namespace

### Integration with Build Systems
- **Meson support** - plan9_c = custom_target(plugin=...)
- **CMake module** - find_package(Plan9GCC)
- **mk integration** - transparent fallback to native compiler

## References

- **GCC Plugin Manual:** https://gcc.gnu.org/onlinedocs/gccint/Plugins.html
- **GIMPLE Documentation:** https://gcc.gnu.org/onlinedocs/gccint/GIMPLE.html
- **Plan 9 C Compilers:** http://doc.cat-v.org/plan_9/4th_edition/papers/compiler
- **9front Source:** https://git.9front.org/plan9front/plan9front

## Status

**Current:** Documented idea, not implemented

**This project (lux9-kernel):** Used manual conversion for one-time kernel port

**Recommended for:** Teams wanting to maintain large Plan 9 codebases with GCC tooling
