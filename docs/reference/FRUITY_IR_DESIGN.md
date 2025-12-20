# Fruity IR Design and Implementation - The Lux9 Kernel's Language Agnostic Intermediate Representation

## 1. Introduction: What is Fruity IR?

Fruity Intermediate Representation (IR) is a core component of the Lux9 kernel's strategy for supporting multiple high-level languages natively. It acts as a specialized, Lux9-aware IR designed to bridge the gap between language-specific constructs (like Go assembly or Java bytecode) and the kernel's unique architectural features, particularly its capability-based memory management (Pebble) and transactional semantics.

Its primary purpose is to provide a common, well-defined target for language frontends. By translating various language source code into Fruity IR, Lux9 can then leverage a single, robust backend (QBE IL) to generate optimized native machine code. This enables "compile-once, run-anywhere-on-Lux9" for high-level languages.

## 2. Core Concepts and Purpose

*   **Pebble-Aware Design:** Unlike generic IRs, Fruity IR intrinsically understands and explicitly represents Lux9's Pebble memory model. It encodes memory operations (allocation, reference counting, transactions) directly into its instructions and basic block metadata.
*   **Language Agnostic Intermediate:** It provides a representation abstract enough to be a target for diverse high-level languages (e.g., CLR languages, Go, Java).
*   **Bridge to QBE:** Fruity IR serves as the direct input to the Fruity-to-QBE translator, which then feeds into the QBE machine code generation backend.
*   **Foundation for Security and Verification:** By embedding Lux9's memory and transactional semantics, Fruity IR facilitates static analysis, verification, and optimization passes crucial for the kernel's security and performance goals.

## 3. Key Data Structures (from `kernel/clr/fruity/fruity_ir.h`)

Fruity IR is built around an intrusive doubly-linked list structure, a common kernel pattern, allowing for flexible graph traversals and manipulations.

### 3.1. `fruity_instruction_t`

Represents a single operation in the IR.

```c
struct fruity_instruction {
    fruity_opcode_t opcode;       // The specific operation (e.g., LIME, ADD)
    fruity_operand_t operand;     // Data or reference for the operation

    /* Pebble effect annotations (for optimization and verification) */
    struct {
        int creates_white;        // Does this instruction issue a new white token?
        int burns_white;          // Does this instruction burn a white token?
        int may_free;             // Does this instruction potentially free memory?
        int is_speculative;       // Is this instruction part of a Red shadow transaction?
    } pebble_effects;

    /* Debug information (optional) */
    u32int msil_offset;
    u32int source_line;
    const char *source_file;

    // Doubly-linked list pointers
    fruity_instruction_t *next;
    fruity_instruction_t *prev;
};
```
*   **`fruity_operand_t`:** A union-based structure to hold various operand types (integers, floats, local/argument indices, type/method tokens, branch targets, tasklet IDs).

### 3.2. `fruity_basic_block_t`

Represents a basic block in the Control Flow Graph (CFG) of a function.

```c
struct fruity_basic_block {
    u32int block_id;

    fruity_instruction_t *instructions_head; // Head of the instruction list
    fruity_instruction_t *instructions_tail; // Tail of the instruction list
    ulong instruction_count;

    // Control Flow Graph edges
    fruity_basic_block_t **successors;
    fruity_basic_block_t **predecessors;
    ulong successor_count;
    ulong predecessor_count;

    // Dataflow analysis state, crucial for Pebble integration
    struct {
        ulong live_whites_in;   // White tokens live on entry to this block
        ulong live_whites_out;  // White tokens live on exit from this block
        int has_snapshot;       // Does this block involve a CHERRY (Red snapshot)?
        int in_transaction;     // Is this block inside a CHERRY...BERRY block?
    } pebble_state;

    // Doubly-linked list pointers
    fruity_basic_block_t *next;
    fruity_basic_block_t *prev;
};
```
*   **`pebble_state`:** This embedded structure is crucial. It allows Fruity IR to perform dataflow analysis specific to Lux9's Pebble memory model, tracking the lifecycle of white tokens and transactional states across basic blocks.

### 3.3. `fruity_function_t`

Represents a complete function, containing its CFG and metadata.

```c
struct fruity_function {
    u32int method_token;
    char *name;
    char *signature;

    fruity_basic_block_t *blocks_head;
    fruity_basic_block_t *blocks_tail;
    ulong block_count;

    // Locals and arguments type information
    ulong local_count;
    clr_value_type_t *local_types;
    ulong arg_count;
    clr_value_type_t *arg_types;
    clr_value_type_t return_type;

    // Pebble-specific metadata (computed during analysis)
    struct {
        int uses_exchange;      // Does this function use GRAPE (zero-copy IPC)?
        int is_transactional;   // Does this function use CHERRY/BERRY transactions?
        ulong max_white_tokens; // Maximum concurrent white tokens managed
        int has_loops;
    } pebble_metadata;

    // Optimization state
    struct {
        int in_ssa_form;
        int optimized;
    } opt_state;

    // Doubly-linked list pointers
    fruity_function_t *next;
    fruity_function_t *prev;
};
```
*   **`pebble_metadata`:** Tracks function-level properties related to Lux9's features, enabling higher-level analysis and optimization.

### 3.4. `fruity_module_t`

Represents a complete compilation unit (e.g., an assembly), containing multiple functions and module-level metadata.

```c
struct fruity_module {
    char *name;
    u32int version;

    fruity_function_t *functions_head;
    fruity_function_t *functions_tail;
    ulong function_count;

    void *metadata; // Opaque handle to language-specific metadata (e.g., ECMA-335)

    // Constants pool, statistics, etc.
};
```

## 4. The "Flavor System": Fruity Opcodes (from `kernel/clr/fruity/fruity_opcodes.h`)

Fruity IR's instruction set is explicitly designed to encapsulate Lux9's unique architectural semantics, particularly its capability-based memory management. The opcodes are categorized for clarity and purpose.

### 4.1. Key Categories

*   **Standard Operations (0x000-0x0FF):** Basic arithmetic, bitwise, comparison, constant loading (`LDC_I4`, `LDNULL`), and type conversions. These are common to most IRs.
*   **Pebble Memory Operations (0x100-0x1FF):** Directly map to Lux9's Pebble memory management primitives. This is where Fruity's uniqueness shines.
*   **Transactional Operations (0x200-0x2FF):** Support for Lux9's Red/Blue shadow copy transactional memory.
*   **IPC and Ownership Transfer (0x300-0x3FF):** Support for zero-copy inter-process communication (IPC) and fine-grained ownership transfer using Lux9's Exchange Page System.
*   **Control Flow (0x400-0x4FF):** Standard branching and function call/return.
*   **Stack and Local Operations (0x500-0x5FF):** Operations for managing local variables and arguments.
*   **Array Operations (0x600-0x6FF):** Support for array creation and element access.

### 4.2. Core "Flavors" (Pebble-Aware Opcodes)

These opcodes represent direct integration points with Lux9's core security and memory models:

*   **`FRUITY_LIME` (0x100):** Allocate (Black Pebble + first White Token).
    *   **Maps to:** `clr_object_alloc()` (which internally calls `pebble_black_alloc()`).
    *   **Semantic:** Creates a new secure memory region (Black Pebble) and issues the first reference (White Token).
    *   **Stack Effect:** `size → obj_ref`
*   **`FRUITY_VANILLA` (0x101):** Share reference (Issue White Token).
    *   **Maps to:** `clr_object_addref()` (issues a new white token).
    *   **Semantic:** Increments the reference count of a capability-backed object.
    *   **Stack Effect:** `obj_ref → obj_ref, obj_ref`
*   **`FRUITY_BURN` (0x102):** Release reference (Burn White Token).
    *   **Maps to:** `clr_object_release()` (burns a white token, potentially freeing if count is zero).
    *   **Semantic:** Decrements the reference count. If zero, the underlying memory is freed.
    *   **Stack Effect:** `obj_ref → ∅`
*   **`FRUITY_CHERRY` (0x200):** Create Red snapshot (Begin transaction).
    *   **Maps to:** `clr_object_snapshot()`.
    *   **Semantic:** Initiates a transactional memory block, creating a "Red shadow" for rollback.
    *   **Stack Effect:** `obj_ref → obj_ref`
*   **`FRUITY_BERRY` (0x201):** Commit transaction (Keep Blue changes).
    *   **Maps to:** `clr_object_commit()`.
    *   **Semantic:** Commits changes made during a transaction, discarding the Red shadow.
    *   **Stack Effect:** `obj_ref → obj_ref`
*   **`FRUITY_ROLLBACK` (0x202):** Abort transaction (Restore from Red).
    *   **Maps to:** `clr_object_rollback()`.
    *   **Semantic:** Discards changes and restores the object to its pre-transaction state from the Red shadow.
    *   **Stack Effect:** `obj_ref → obj_ref`
*   **`FRUITY_GRAPE` (0x300):** Zero-copy IPC transfer (Exchange).
    *   **Maps to:** `clr_msg_prepare()` + `clr_msg_send()`.
    *   **Semantic:** Transfers ownership of a capability-backed object to another process/tasklet via the Exchange Page System. Invalidates the object in the sender.
    *   **Stack Effect:** `obj_ref, dest_tasklet → ∅`
*   **`FRUITY_LEMON` (0x301):** Intra-process ownership transfer (Move).
    *   **Semantic:** Transfers ownership within the same process without changing white token counts, typically for return values or explicit moves.
    *   **Stack Effect:** `obj_ref_src → obj_ref_dst`

Each opcode can also carry `pebble_effects` annotations (`creates_white`, `burns_white`, `may_free`, `is_speculative`) to aid in static analysis and optimization passes.

## 5. Memory Model Integration and Verification

Fruity IR's design directly enforces Lux9's capability-based memory model:
*   **Explicit White Token Management:** Opcodes like LIME, VANILLA, and BURN directly manage the lifecycle of "white tokens" (references) to capability-backed memory regions.
*   **Transactional Memory:** CHERRY, BERRY, and ROLLBACK enable the use of Lux9's Red/Blue shadow copy transactional memory.
*   **Borrow Checker Semantics:** The `pebble_state` in `fruity_basic_block_t` and `pebble_metadata` in `fruity_function_t` allow for sophisticated dataflow analysis to ensure correct handling of references, preventing memory leaks, use-after-free, and other memory safety violations. Functions like `fruity_function_analyze_white_tokens` and `fruity_function_verify_white_balance` perform these critical checks.

## 6. Translation to QBE IL (from `kernel/clr/fruity/fruity_to_qbe.c`)

The `fruity_to_qbe` component translates Fruity IR into QBE Intermediate Language. This translation maps Fruity's high-level, Lux9-specific operations into QBE's low-level SSA-form instructions.
*   **Pebble Calls:** Fruity's Pebble opcodes (LIME, VANILLA, BURN, CHERRY, BERRY, ROLLBACK, GRAPE) are translated into QBE `call` instructions to specific runtime functions (e.g., `$lux_alloc`, `$lux_token_mint`, `$lux_snapshot`, `$lux_exchange_send`), which are part of Lux9's kernel ABI for managed runtimes.
*   **Type Mapping:** Basic CLR types (`CLR_INT32`, `CLR_REF`) are mapped to QBE types (`w` for word, `l` for long).

## 7. Serialization and Deserialization (from `kernel/clr/fruity/fruity_cbor.c`)

Fruity IR modules are serialized into CBOR (Concise Binary Object Representation) format for storage and transmission (e.g., via the `/dev/clr` device).
*   `fruity_module_to_cbor()`: Encodes a Fruity IR module into CBOR.
*   `fruity_module_from_cbor()`: Decodes CBOR data back into a Fruity IR module.
This mechanism enables the dynamic loading and compilation of managed code within the kernel.

## 8. Future Language Integration Strategy

The "Language -> Fruity IR -> QBE IL" pipeline is the cornerstone of Lux9's multi-language strategy:
*   **For Go:** A custom Go compiler frontend would translate Go Assembly (or Go's internal IR) directly into Fruity IR.
*   **For Java:** A custom Java compiler frontend would translate Java Bytecode directly into Fruity IR.

This approach ensures that all high-level language frontends benefit from Fruity IR's explicit Lux9-aware semantics, security properties, and leverage the proven QBE backend for efficient, native machine code generation. This makes it significantly easier to integrate nearly any language into the Lux9 ecosystem natively.
