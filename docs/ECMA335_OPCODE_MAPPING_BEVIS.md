# ECMA-335 CIL to Fruity/QBE Mapping Lexicon

This document serves as the master translation guide for the GHOSTDAG-OS CLR JIT. It maps ECMA-335 CIL opcodes to Fruity high-level semantics and finally to QBE intermediate representation.

**Key:**
*   **CIL:** The .NET bytecode.
*   **Stack:** Effect on the evaluation stack (Before -> After).
*   **Fruity (Semantic Layer):** Verification, GC barriers, and "Softwar" (PoW) checks inserted here.
*   **QBE (Low-Level):** The resulting SSA instructions.

---

## 1. Basic Data Operations

| Opcode | Stack | Fruity Logic | QBE IR Generation |
| :--- | :--- | :--- | :--- |
| `nop` (0x00) | `... -> ...` | No-op. | (None) |
| `ldarg.0` (0x02) | `... -> ..., val` | Load argument 0. Verify type. | `%v =w loadw %arg0` (if int) |
| `ldloc.0` (0x06) | `... -> ..., val` | Load local 0. Verify init. | `%v =w loadw %loc0` |
| `stloc.0` (0x0A) | `..., val -> ...` | Store local 0. | `storew %val, %loc0` |
| `ldc.i4.0` (0x16) | `... -> ..., 0` | Push constant 0. | `%v =w copy 0` |
| `dup` (0x25) | `..., val -> ..., val, val` | Duplicate top. | `%t =w copy %val`<br>`(Stack tracks %t)` |

---

## 2. Arithmetic & Logic

| Opcode | Stack | Fruity Logic | QBE IR Generation |
| :--- | :--- | :--- | :--- |
| `add` (0x58) | `..., a, b -> ..., res` | Check overflow if `add.ovf`. | `%res =w add %a, %b` |
| `sub` (0x59) | `..., a, b -> ..., res` | Check overflow if `sub.ovf`. | `%res =w sub %a, %b` |
| `mul` (0x5A) | `..., a, b -> ..., res` | Check overflow. | `%res =w mul %a, %b` |
| `div` (0x5B) | `..., a, b -> ..., res` | **Check b != 0.** Trap if 0. | `%chk =w ceqw %b, 0`<br>`jnz %chk, @divide_by_zero, @ok`<br>`@ok: %res =w div %a, %b` |
| `and` (0x5F) | `..., a, b -> ..., res` | Bitwise AND. | `%res =w and %a, %b` |
| `shl` (0x62) | `..., val, amt -> ..., res` | Shift Left. | `%res =w shl %val, %amt` |

---

## 3. Object Model (The "Softwar" Integration)

This is where the thermodynamic economy hooks in.

| Opcode | Stack | Fruity Logic (Safety + Physics) | QBE IR Generation |
| :--- | :--- | :--- | :--- |
| `newobj` (0x73) | `..., [args] -> ..., obj` | 1. Calculate Size.<br>2. **Check BEVIS PoW (`up->pow_nonce`).**<br>3. Call `pebble_issue_white`.<br>4. Call Ctor. | `%size =w copy 32` (example)<br>`%tok =l call $pebble_issue_white(%pebble, 0, %size)`<br>`%check =l ceql %tok, 0`<br>`jnz %check, @oom_error, @alloc_ok`<br>`@alloc_ok:`<br>`%obj =l call $pebble_white_verify(%tok)`<br>`call $Ctor(%obj, args...)` |
| `ldfld` (0x7B) | `..., obj -> ..., val` | 1. Null Check (`obj != null`).<br>2. Type Check (Field offset). | `%chk =l ceql %obj, 0`<br>`jnz %chk, @null_ref, @ok`<br>`@ok: %ptr =l add %obj, $offset`<br>`%val =w loadw %ptr` |
| `stfld` (0x7D) | `..., obj, val -> ...` | 1. Null Check.<br>2. **GC Write Barrier** (if ref type). | `%chk =l ceql %obj, 0`<br>`jnz %chk, @null_ref, @ok`<br>`@ok: %ptr =l add %obj, $offset`<br>`storew %val, %ptr`<br>*(If ref: call $gc_write_barrier(%obj))* |
| `callvirt` (0x6F) | `..., obj, [args] -> ..., [ret]` | 1. Null Check.<br>2. VTable Lookup.<br>3. Indirect Call. | `%chk =l ceql %obj, 0`<br>`jnz %chk, @null_ref, @ok`<br>`@ok: %vtable =l loadl %obj`<br>`%func =l loadl %vtable`<br>`call %func(%obj, args...)` |

---

## 4. Control Flow

| Opcode | Stack | Fruity Logic | QBE IR Generation |
| :--- | :--- | :--- | :--- |
| `br` (0x38) | `... -> ...` | Unconditional Branch. | `jmp @label` |
| `brfalse` (0x2B) | `..., val -> ...` | Branch if 0/null. | `%chk =w ceqw %val, 0`<br>`jnz %chk, @target, @fallthrough` |
| `beq` (0x2E) | `..., a, b -> ...` | Branch if equal. | `%chk =w ceqw %a, %b`<br>`jnz %chk, @target, @fallthrough` |
| `ret` (0x2A) | `..., [val] -> ...` | Return from method. | `ret %val` |

---

## 5. Advanced / Unsafe (Pebble Specific)

| Opcode | Stack | Fruity Logic | QBE IR Generation |
| :--- | :--- | :--- | :--- |
| `localloc` (0xFE 0x0F) | `..., size -> ..., ptr` | **Stack Allocation? NO.**<br>In GHOSTDAG-OS, verify stack limit or use Pebble. | *Preferably mapped to safe stack bump, or trap if stack rigid.* |
| `cpblk` (0xFE 0x17) | `..., dst, src, len -> ...` | Memory Copy.<br>**Check Bounds.** | `call $memcpy(%dst, %src, %len)` |

---

## 6. Implementation Strategy for `il_to_fruity.c`

To implement the JIT, the parser loop must:

1.  **Decode Opcode:** Read the byte stream.
2.  **Abstract Stack:** Maintain a compile-time stack of `FruityValue` (tracking types and temporary registers).
3.  **Emit Blocks:**
    *   **Pre-computation:** Emit null checks/bound checks (Fruity Layer).
    *   **Operation:** Emit the QBE math/logic.
    *   **Barriers:** Emit GC write barriers or PoW checks.

This mapping ensures that every CIL instruction is not just translated, but **hardened** with the OS's thermodynamic policies.