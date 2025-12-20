# CLR Implementation - Next Steps

## What We Just Completed ✅

**Created IL Parser** (`il_parser.c`, `il_parser.h`)
- Parses .NET PE/COFF files
- Extracts CLI metadata
- Reads metadata streams (#Strings, #Blob, #GUID, #US, #~)
- Parses metadata tables header
- ~600 lines of C code

## How to Test the IL Parser

### Step 1: Install F# Compiler

```bash
# On Ubuntu/Debian
sudo apt-get install fsharp

# Or use .NET SDK
wget https://dot.net/v1/dotnet-install.sh
bash dotnet-install.sh
```

### Step 2: Compile F# Test Program

```bash
cd kernel/clr
fsc test_hello.fs
# Produces: test_hello.dll
```

### Step 3: Compile IL Parser Test

```bash
gcc -o test_il_parser test_il_parser.c il_parser.c -I.
```

### Step 4: Run Test

```bash
./test_il_parser test_hello.dll
```

**Expected output:**
```
Parsing: test_hello.dll
Success! Assembly parsed.

=== .NET Assembly Info ===
PE: 3 sections
CLI Version: 2.5
Metadata Streams: 5
  [0] #~ (size=XXX)
  [1] #Strings (size=XXX)
  [2] #US (size=XXX)
  [3] #GUID (size=XXX)
  [4] #Blob (size=XXX)
Metadata Tables: valid_mask=0xXXXXXXXXXXXXXXXX

=== Entry Point ===
Token: 0x06000001

Test complete!
```

## What's Still Missing

### 1. MethodDef Table Parser (CRITICAL)

**Current issue:** We can parse the tables header, but not the actual MethodDef rows.

**Need to add to `il_parser.c`:**

```c
typedef struct {
    uint32_t rva;
    uint16_t impl_flags;
    uint16_t flags;
    uint32_t name_index;
    uint32_t signature_index;
    uint32_t param_list;
} methoddef_row_t;

static methoddef_row_t* parse_methoddef_table(il_assembly_t *asm, size_t *row_count_out) {
    size_t row_count = asm->tables_header.row_counts[TABLE_METHODDEF];
    if (row_count == 0) {
        return NULL;
    }

    methoddef_row_t *rows = IL_MALLOC(sizeof(methoddef_row_t) * row_count);

    // Parse each row from tables_data
    // (need to calculate offset based on previous tables)

    *row_count_out = row_count;
    return rows;
}
```

### 2. Implement `il_get_method()` (CRITICAL)

**Current status:** Stub that returns NULL

**Need to implement:**

```c
il_method_t* il_get_method(il_assembly_t *asm, const char *name) {
    // 1. Parse MethodDef table
    size_t method_count;
    methoddef_row_t *methods = parse_methoddef_table(asm, &method_count);

    // 2. Search for method by name
    for (size_t i = 0; i < method_count; i++) {
        const char *method_name = il_get_string(asm, methods[i].name_index);
        if (strcmp(method_name, name) == 0) {
            // Found it!
            il_method_t *method = parse_method(asm, methods[i].rva, name);
            IL_FREE(methods);
            return method;
        }
    }

    IL_FREE(methods);
    return NULL;
}
```

### 3. IL Disassembler (for debugging)

**Would be very useful:**

```c
void il_disassemble_method(il_method_t *method) {
    uint8_t *il = method->il_code;
    size_t offset = 0;

    while (offset < method->il_code_size) {
        uint8_t opcode = il[offset];
        printf("IL_%04x: ", offset);

        switch (opcode) {
            case 0x00: printf("nop\n"); offset++; break;
            case 0x02: printf("ldarg.0\n"); offset++; break;
            case 0x03: printf("ldarg.1\n"); offset++; break;
            case 0x06: printf("ldloc.0\n"); offset++; break;
            case 0x0A: printf("stloc.0\n"); offset++; break;
            case 0x16: printf("ldc.i4.0\n"); offset++; break;
            case 0x17: printf("ldc.i4.1\n"); offset++; break;
            case 0x20: printf("ldc.i4.s %d\n", (int8_t)il[offset+1]); offset+=2; break;
            case 0x58: printf("add\n"); offset++; break;
            case 0x59: printf("sub\n"); offset++; break;
            case 0x5A: printf("mul\n"); offset++; break;
            case 0x72: {
                uint32_t token = *(uint32_t*)&il[offset+1];
                printf("ldstr <token 0x%08x>\n", token);
                offset += 5;
                break;
            }
            case 0x28: {
                uint32_t token = *(uint32_t*)&il[offset+1];
                printf("call <token 0x%08x>\n", token);
                offset += 5;
                break;
            }
            case 0x2A: printf("ret\n"); offset++; break;
            default:
                printf("unknown opcode 0x%02x\n", opcode);
                offset++;
                break;
        }
    }
}
```

## Updated Test (with method extraction)

Once we implement `il_get_method()`, test becomes:

```c
int main(int argc, char **argv) {
    // ... parse assembly ...

    // Get main method
    il_method_t *main_method = il_get_method(asm, "main");
    if (main_method) {
        printf("\n=== Main Method ===\n");
        il_dump_method(main_method);

        printf("\n=== Disassembly ===\n");
        il_disassemble_method(main_method);

        il_free_method(main_method);
    } else {
        printf("main method not found\n");
    }

    il_free_assembly(asm);
    return 0;
}
```

**Expected output:**
```
=== Main Method ===
Max stack: 8
IL code size: 15 bytes

=== Disassembly ===
IL_0000: ldstr <token 0x70000001>
IL_0005: call <token 0x0A000001>
IL_000a: ldc.i4.0
IL_000b: ret
```

## Next Priority Tasks

### Task 1: Implement MethodDef Table Parser
**File:** `kernel/clr/il_parser.c`

**Add function:** `parse_methoddef_table()`

**Test:** Can extract all methods from assembly

### Task 2: Implement il_get_method()
**File:** `kernel/clr/il_parser.c`

**Implement:** Search MethodDef table by name

**Test:** Can get specific method by name

### Task 3: Add IL Disassembler
**File:** `kernel/clr/il_disasm.c` (new file)

**Implement:** Print IL opcodes in human-readable format

**Test:** Disassemble F# main method

### Task 4: IL → Fruity IR Converter
**File:** `kernel/clr/il_to_fruity.c` (new file)

**Implement:** Convert IL bytecode to Fruity IR

**Test:** Convert simple add function

## Immediate Next Session

**Priority 1:** Finish IL parser
- [ ] Implement MethodDef table parsing
- [ ] Implement il_get_method()
- [ ] Test with F# assembly

**Priority 2:** IL disassembler
- [ ] Create il_disasm.c
- [ ] Implement basic IL opcodes
- [ ] Print human-readable IL

**Priority 3:** IL → Fruity IR
- [ ] Create il_to_fruity.c
- [ ] Map IL opcodes to Fruity opcodes
- [ ] Handle stack semantics

**After that:** Connect to QBE and test full F# → native pipeline!
