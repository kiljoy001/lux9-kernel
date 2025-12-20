# CLR Testing Plan

## Testing Strategy

We'll test the pipeline in stages, building up from simple to complex.

## Stage 1: Component Unit Tests (Can Do NOW)

### Test 1.1: Fruity IR Creation
**Status**: ✅ Can test immediately

Create a minimal Fruity IR module in code and verify structure:

```c
// test/test_fruity_ir.c
#include "fruity_ir.h"

int main() {
    fruity_module_t *mod = fruity_module_create("test");
    fruity_function_t *func = fruity_function_create(mod, "add");
    fruity_basic_block_t *bb = fruity_block_create(func);

    // Add instruction: return 42
    fruity_instruction_t *ret_instr = fruity_instruction_create(FRUITY_RET);
    fruity_instruction_t *const_instr = fruity_instruction_create(FRUITY_CONST_I32);
    const_instr->const_i32 = 42;

    fruity_block_add_instruction(bb, const_instr);
    fruity_block_add_instruction(bb, ret_instr);

    // Verify structure
    assert(mod->function_count == 1);
    assert(func->block_count == 1);
    assert(bb->instruction_count == 2);

    print("✓ Fruity IR test passed\n");
    return 0;
}
```

**To build**: Standalone test using fruity_standalone.h

### Test 1.2: Fruity → QBE Translation
**Status**: ✅ Can test immediately

```c
// test/test_fruity_to_qbe.c
int main() {
    fruity_module_t *mod = create_simple_module(); // from above

    // Allocate output buffer (simulating a page)
    u8int page[4096];
    uintptr fake_pa = (uintptr)page - get_hhdm_offset();

    char errbuf[256];
    int ret = fruity_to_qbe(mod, fake_pa, errbuf, sizeof(errbuf));

    if(ret != 0) {
        print("Error: %s\n", errbuf);
        return 1;
    }

    // Verify QBE IL was generated
    char *qbe_il = (char*)page;
    print("Generated QBE IL:\n%s\n", qbe_il);

    // Check for expected strings
    assert(strstr(qbe_il, "export function") != NULL);
    assert(strstr(qbe_il, "$test") != NULL);

    print("✓ Fruity→QBE test passed\n");
    return 0;
}
```

### Test 1.3: QBE → x86-64 Compilation
**Status**: ✅ Can test immediately

```c
// test/test_qbe_compile.c
int main() {
    // Create minimal QBE IL
    char qbe_il[4096];
    snprintf(qbe_il, sizeof(qbe_il),
        "export function w $test() {\n"
        "@start\n"
        "    ret 42\n"
        "}\n");

    u8int code_page[4096];
    uintptr qbe_pa = (uintptr)qbe_il - get_hhdm_offset();
    uintptr code_pa = (uintptr)code_page - get_hhdm_offset();

    char errbuf[256];
    int ret = qbe_compile_page(qbe_pa, code_pa, errbuf, sizeof(errbuf));

    if(ret != 0) {
        print("Error: %s\n", errbuf);
        return 1;
    }

    // Dump generated machine code (hex)
    print("Generated x86-64 code:\n");
    for(int i = 0; i < 32; i++) {
        print("%02x ", code_page[i]);
        if((i+1) % 16 == 0) print("\n");
    }

    // Verify prologue exists
    assert(code_page[0] == 0x55);  // push rbp

    print("✓ QBE→x86-64 test passed\n");
    return 0;
}
```

### Test 1.4: CBOR Serialization
**Status**: ✅ Can test immediately

```c
// test/test_cbor.c
int main() {
    fruity_module_t *mod = create_simple_module();

    // Serialize to CBOR
    u8int cbor_buf[4096];
    char errbuf[256];
    ulong cbor_len = fruity_module_to_cbor(mod, cbor_buf, sizeof(cbor_buf),
                                           errbuf, sizeof(errbuf));

    if(cbor_len == 0) {
        print("Serialization error: %s\n", errbuf);
        return 1;
    }

    print("CBOR size: %lu bytes\n", cbor_len);

    // Deserialize back
    fruity_module_t *mod2 = fruity_module_from_cbor(cbor_buf, cbor_len,
                                                     errbuf, sizeof(errbuf));

    if(mod2 == NULL) {
        print("Deserialization error: %s\n", errbuf);
        return 1;
    }

    // Verify basic structure
    assert(strcmp(mod2->name, "test_module") == 0);

    print("✓ CBOR serialization test passed\n");
    return 0;
}
```

## Stage 2: Integration Tests (Requires Kernel Boot)

### Test 2.1: Syscall Path Test

Boot kernel and test syscall interface:

```c
// userspace/test_clr_syscall.c
#include <u.h>
#include <libc.h>

int main() {
    // Minimal .NET assembly header (fake for now)
    uchar fake_dll[256];
    memset(fake_dll, 0, sizeof(fake_dll));

    // PE header signature
    fake_dll[0] = 'M';
    fake_dll[1] = 'Z';

    // Try to compile (will fail gracefully since IL parser is stubbed)
    int fd = open("#c/clr/compile", OWRITE);
    if(fd < 0) {
        print("Failed to open /dev/clr: %r\n");
        exits("fail");
    }

    long n = write(fd, fake_dll, sizeof(fake_dll));
    print("Wrote %ld bytes to CLR compiler\n", n);

    // Read result (will be error since IL is fake)
    char result[1024];
    seek(fd, 0, 0);
    n = read(fd, result, sizeof(result));
    print("CLR result: %.*s\n", (int)n, result);

    close(fd);
    exits(nil);
}
```

**Expected output**: Error message about invalid IL, proving syscall path works

### Test 2.2: Hand-Crafted Fruity IR Test

Skip IL parsing, create Fruity IR directly in kernel:

```c
// kernel/9front-port/devclr.c - add test function
static fruity_module_t*
create_test_module(void)
{
    fruity_module_t *mod = fruity_module_create("kernel_test");
    fruity_function_t *func = fruity_function_create(mod, "return42");
    fruity_basic_block_t *bb = fruity_block_create(func);

    // Instruction: const 42
    fruity_instruction_t *const_instr = mallocz(sizeof(*const_instr), 1);
    const_instr->opcode = FRUITY_CONST_I32;
    const_instr->const_i32 = 42;
    fruity_block_add_instruction(bb, const_instr);

    // Instruction: ret
    fruity_instruction_t *ret_instr = mallocz(sizeof(*ret_instr), 1);
    ret_instr->opcode = FRUITY_RET;
    ret_instr->has_value = 1;
    fruity_block_add_instruction(bb, ret_instr);

    return mod;
}

// In sys_clrcompile(), add test path:
if(DEBUGGING) {
    module = create_test_module();
} else {
    module = il_to_fruity(dll_data, dll_len);  // Real path
}
```

**Test procedure**:
1. Boot kernel with DEBUGGING=1
2. Call sys_clrcompile()
3. Verify QBE IL is generated correctly
4. Verify x86-64 code is generated
5. Map page as executable
6. Call function pointer
7. Verify return value is 42

### Test 2.3: Code Execution Test

Execute generated code in userspace:

```c
// userspace/test_clr_exec.c
typedef int (*fn_ptr)(void);

int main() {
    // Get compiled code from kernel (via test module above)
    int fd = open("#c/clr/compile", ORDWR);

    // Write test request
    write(fd, "TEST", 4);

    // Read back code page handle
    uintptr code_handle;
    read(fd, &code_handle, sizeof(code_handle));

    // Map code page as executable
    void *code = segattach(0, "clr_code", (void*)code_handle, 4096);
    if(code == (void*)-1) {
        print("Failed to map code: %r\n");
        exits("fail");
    }

    // Execute!
    fn_ptr func = (fn_ptr)code;
    int result = func();

    print("Function returned: %d\n", result);

    if(result == 42) {
        print("✓ Code execution test PASSED!\n");
        exits(nil);
    } else {
        print("✗ Expected 42, got %d\n", result);
        exits("fail");
    }
}
```

## Stage 3: Real IL Parser Integration (Next Priority)

### Test 3.1: Simple F# Program

```fsharp
// test.fsx
module Test

let add x y = x + y
let answer = 42
```

Compile with F# compiler:
```bash
fsharpc --target:library test.fsx -o test.dll
```

Load into kernel:
```c
// Read test.dll into buffer
uchar *dll = readfile("test.dll", &dll_len);

// Compile via syscall
int fd = open("#c/clr/compile", OWRITE);
write(fd, dll, dll_len);

// Get compiled code handle
uintptr handle;
read(fd, &handle, sizeof(handle));

// Execute
void *code = segattach(0, "clr", (void*)handle, 4096);
int (*answer_fn)(void) = find_symbol(code, "Test.answer");
int result = answer_fn();
assert(result == 42);
```

### Test 3.2: Pebble Operations

```fsharp
module PebbleTest

open System

// Allocate Pebble memory (LIME)
let allocTest () =
    let ptr = Pebble.alloc 64
    ptr

// Token operations (VANILLA/BURN)
let tokenTest ptr =
    let token = Pebble.share ptr  // VANILLA
    // use token
    Pebble.release token          // BURN
```

Verify generated code calls Pebble runtime:
```
objdump -d test_pebble.o | grep lux_alloc
objdump -d test_pebble.o | grep lux_token_mint
```

## Stage 4: Performance Testing

### Test 4.1: Compilation Speed

Compile 100 functions and measure time:
```c
for(int i = 0; i < 100; i++) {
    u64 start = fastticks(nil);
    compile_function(func);
    u64 end = fastticks(nil);
    print("Function %d: %lld cycles\n", i, end - start);
}
```

**Target**: <2ms per function on modern CPU

### Test 4.2: Execution Speed

Compare F# code vs native C:
```c
// C version
int fib(int n) {
    if(n <= 1) return n;
    return fib(n-1) + fib(n-2);
}

// F# version (compiled via CLR)
let rec fib n =
    if n <= 1 then n
    else fib(n-1) + fib(n-2)
```

**Target**: Within 2x of native C (acceptable for high-level language)

## Testing Infrastructure

### Build Tests
```bash
# In kernel/ directory
make tests

# Runs:
# - test_fruity_ir
# - test_fruity_to_qbe
# - test_qbe_compile
# - test_cbor
```

### Kernel Tests
```bash
# Boot kernel with test flag
make run TEST=1

# Kernel runs internal tests at boot
# Output:
# clr: running self-tests...
# ✓ Fruity IR test passed
# ✓ Fruity→QBE test passed
# ✓ QBE→x86-64 test passed
# ✓ Code execution test passed
# clr: all tests passed
```

### Userspace Tests
```bash
# In userspace/
make test_clr

# Runs test suite:
./test_clr_syscall    # Test syscall interface
./test_clr_exec       # Test code execution
./test_clr_pebble     # Test Pebble integration
```

## Debugging Tools

### 1. QBE IL Dumper
```c
// Add to sys_clrcompile():
if(getconf("clr_debug")) {
    // Dump QBE IL to console
    print("=== QBE IL ===\n%s\n", (char*)KADDR(qbe_page));
}
```

### 2. x86-64 Disassembler
```bash
# Extract compiled code page
dd if=/dev/clr/code bs=4096 count=1 of=code.bin

# Disassemble
objdump -D -b binary -m i386:x86-64 code.bin
```

### 3. GDB Integration
```bash
# Debug compiled code
gdb kernel/lux9.elf
(gdb) break sys_clrcompile
(gdb) continue
(gdb) x/20i $code_page   # Examine generated code
```

## Current Test Status

### Can Run NOW:
- ✅ Component unit tests (Stage 1)
- ✅ Build system tests
- ⚠️  Integration tests (need kernel boot - currently have boot loader issue)

### Blocked:
- ❌ Stage 2 tests - need kernel to boot
- ❌ Stage 3 tests - need IL parser integration
- ❌ Stage 4 tests - need working execution

## Immediate Next Steps

1. **Fix kernel boot** (PVH ELF Note issue)
2. **Run Stage 1 unit tests** - verify each component works
3. **Add test path to sys_clrcompile()** - create Fruity IR directly
4. **Test code generation** - verify x86-64 output
5. **Test code execution** - call generated function
6. **Integrate IL parser** - connect to real .NET DLLs
7. **Full end-to-end test** - F# → .NET DLL → Native → Execute

## Success Criteria

The CLR is "working" when we can:

1. ✅ Compile kernel with CLR components
2. ⏳ Boot kernel (blocked on boot loader)
3. ⏳ Call sys_clrcompile() from userspace
4. ⏳ Generate valid x86-64 code
5. ⏳ Execute generated code
6. ⏳ Return correct result (42)
7. ❌ Load real .NET DLL
8. ❌ Execute F# code

**Current Score**: 1/8 complete

But foundation is solid - once kernel boots, we can rapidly test the rest.
