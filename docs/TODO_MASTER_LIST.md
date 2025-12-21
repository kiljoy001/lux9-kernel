# Lux9 Consolidated TODO List

> **Last verified: 2025-12-20**
> **Strategic Pivot:** Transitioning CLR execution from Native Code (QBE/SLJIT) to WebAssembly (WASM).
> Scope: `kernel/`, `proofs/`, `tools/`, `userspace/`

---

## Summary

| Category | Count | Status |
|----------|-------|--------|
| **Phase 1: WASM Backend** | 4 | 🚀 New Priority |
| **Phase 2: WASM Runtime** | 3 | 🚀 New Priority |
| **Phase 3: System Interface** | 3 | 🚀 New Priority |
| Boot & Runtime | 2 | 🔴 Blocked |
| F# Init | 2 | 🟠 Active |
| BCL Gaps | 3 | 🟡 Active |
| Admitted Proofs | 12 | Active |
| **Total** | **29** | |

---

## 🚀 Phase 1: CIL -> WASM Translation (`kernel/clr/wasm_backend/`)

The new compiler backend to replace QBE. Uses Fruity IR as input.

| Task | Description |
|------|-------------|
| **Create `fruity_to_wasm.c`** | Implement the translation logic. Map Fruity opcodes (LDC, ADD) to WASM bytes (i32.const, i32.add). |
| **Implement Loop-Switch** | Implement the "Loop-Switch" pattern to handle unstructured CIL control flow (goto) within structured WASM blocks. |
| **WASM Binary Emitter** | Implement the logic to serialize the translated instructions into a valid `.wasm` binary format (header, sections). |
| **Integrate with `il_parser.c`** | Hook the new backend into the existing metadata parser. |

---

## 🚀 Phase 2: WASM Runtime Integration (`kernel/clr/wasm_runtime/`)

Replacing `sljit` with a lightweight interpreter.

| Task | Description |
|------|-------------|
| **Integrate `wasm3`** | Import `wasm3.c` and `wasm3.h` into the kernel build. Ensure it compiles without libc dependencies. |
| **Pebble Memory Manager** | Implement a custom WASM memory allocator that uses the Pebble system (tokens) for linear memory growth (`memory.grow`). |
| **Runtime Hook** | Modify `execution_engine.c` to instantiate the WASM VM instead of jumping to JITed machine code. |

---

## 🚀 Phase 3: System Interface (Lux9-WASI)

Defining how WASM modules talk to the kernel.

| Task | Description |
|------|-------------|
| **Define Host Functions** | Create the C implementation of imports: `lux9_send_9p`, `lux9_yield`, `lux9_debug_print`. |
| **9P Bridge** | Wire `lux9_send_9p` to the internal `9p_router.c` via Exchange Pages. |
| **Init Wrapper** | Update `userspace/init9p/Init.fs` to import these host functions instead of P/Invoke calls. |

---

## 🛑 Deprecated / Removed (Architecture Shift)

These items are no longer relevant due to the WASM pivot.

- ~~`kernel/clr/qbe`~~ (Replaced by `wasm_backend`)
- ~~`kernel/clr/sljit`~~ (Replaced by `wasm3`)
- ~~`fruity_to_qbe.c`~~
- ~~`fruity_sljit.c`~~

---

## 🔴 Boot & Runtime (BLOCKING)

| File:Line | Issue |
|-----------|-------|
| `userinit.c:226` | Load CLR from /boot/boot (Update to load WASM module or trigger translation) |
| `userinit.c:317-319` | **FIXME**: m→pml4 hang |

---

## 🟠 F# Init (`userspace/init9p/Init.fs`)

| File:Line | Issue |
|-----------|-------|
| `Init.fs:238` | TODO: Implement wait() via 9P for child reaping |
| `Init.fs:237-241` | Reap loop doesn't reap - needs kernel wait() |

---

## 🟡 BCL Gaps (Blocking Shell)

| File | Issue |
|------|-------|
| `System.Console.cs` | Console.ReadLine not implemented |
| `Process.cs` | Process.WaitForExit not implemented |
| `Process.cs` | Environment.CurrentDirectory not implemented |

---

## ⚠️ Admitted Proofs (12)

| File | Count | Notes |
|------|-------|-------|
| `ramdisk_wipe.v` | 0 | ✅ Complete |
| `fsharp_complete_typechecker.v` | 3 | Type preservation |
| `fsharp_fully_proven.v` | 3 | Type soundness |
| `fsharp_full_language.v` | 3 | Complex cases |
| `fsharp_zero_admits_final.v` | 1 | Substitution lemma |
| `fsharp_proven_zero_admits.v` | 1 | Weak substitution |
| `fsharp_truly_minimal.v` | 1 | Minimal system |

---

## Top 5 Priorities

1. **Phase 1:** Implement `fruity_to_wasm.c` (Loop-Switch Control Flow).
2. **Phase 2:** Integrate `wasm3` into the kernel build.
3. **Phase 3:** Define `lux9_send_9p` host function for 9P communication.
4. **`userinit.c:317`** — m→pml4 hang (boot stability).
5. **`Init.fs`** — Refactor to use WASM Imports.