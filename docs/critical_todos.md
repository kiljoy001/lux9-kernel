# Critical TODOs in Lux9 Kernel

Last updated: 2025-12-15

**First-party kernel code only** (excluding tpm2-tss, qbe/, libmcu-cbor/, CIL-Interpreter/, test/)

---

## 🔴 CRITICAL - Device Drivers

| File | Line | Issue | Status |
|:-----|:-----|:------|:-------|
| `9front-port/devirq.c` | 404 | `intrdisable()` not implemented | ✅ Fixed |
| `9front-port/devmem.c` | 179 | I/O port read not implemented | ✅ Fixed |
| `9front-port/devmem.c` | 233 | I/O port write not implemented | ✅ Fixed |
| `9front-pc64/cga.c` | 177 | CGA screen memory not mapped yet | ⚪ N/A (VGA available, CGA is legacy) |

---

## ~~🔴 CRITICAL - devclr Stubs~~ ✅ RESOLVED

| File | Line | Issue | Status |
|:-----|:-----|:------|:-------|
| `9front-port/devclr.c` | 374 | Return placeholder stats | ✅ Real stats |
| `9front-port/devclr.c` | 411 | Control commands ignored | ✅ gc/reset/debug |
| `9front-port/devclr.c` | 445 | CIL bytecode load (stub) | ✅ Assembly registry |
| `9front-port/devclr.c` | 449 | Tasklet creation (stub) | ✅ Tasklet registry |
| `9front-port/devclr.c` | 453 | Channel creation (stub) | ✅ Channel registry |

---

## 🔴 CRITICAL - CLR Runtime

| File | Line | Issue | Status |
|:-----|:-----|:------|:-------|
| `clr/clr_runtime.c` | 156 | Resolve type token to get size | ✅ Fixed |
| `clr/clr_runtime.c` | 166 | Resolve element type size | ✅ Fixed |
| `clr/clr-kernel/clr_internal_calls.c` | 18 | String extraction from managed object | ✅ Fixed |
| `clr/clr-kernel/clr_internal_calls.c` | 37 | Get size from type token | ✅ Fixed |
| `clr/clr-kernel/clr_pebble_integration.c` | 307 | Redesign for token economy | ✅ Implemented |
| `clr/clr-kernel/clr_pebble_integration.h` | 227 | Implement Red-Blue API | ✅ Implemented |

---

## 🟡 MEDIUM - 9P Routing

| File | Line | Issue | Status |
|:-----|:-----|:------|:-------|
| `9p_router.c` | 436 | Handle MSGORD_CB_ROLLBACK | ✅ Fixed |
| `9p_router.c` | 1074 | Implement proper Tstat | ✅ Fixed |

---

## 🟡 MEDIUM - PCI/Family

| File | Line | Issue | Status |
|:-----|:-----|:------|:-------|
| `family/pci_9p.c` | 159 | Implement PCI bus listing | ✅ Fixed |
| `family/pci_9p.c` | 164 | Implement global PCI ctl | ✅ Fixed |
| `family/pci_family.c` | 452 | Implement channel cleanup | Implemented |
| `family/pci_resource_pool.c` | 30 | `exchange_cleanup` stub | ✅ Implemented |
| `family/pci_channel.c` | 100 | `exchange_unmap` stub | ✅ Implemented |
| `family/stubs.c` | 84 | Process stub | ✅ Implemented |
| `family/stubs.c` | 91 | Permission stub | ✅ Implemented |
| `family/stubs.c` | 98 | Missing family functions | ✅ Implemented |

---

## 🟡 MEDIUM - Kernel Core

| File | Line | Issue | Status |
|:-----|:-----|:------|:-------|
| `9front-pc64/main.c` | 378 | Fix devenv create path | ⚪ Boot order (not bug) |
| `9front-port/proc.c` | 794 | Implement /dev/sip for white tokens | ⚪ Future enhancement |
| `9front-port/sysproc.c` | 1969 | Store fruity_module_t* in devclr | ⚪ Future enhancement |
| `9front-pc64/globals.c` | 81 | fprint stub | ✅ Implemented |
| `9front-pc64/globals.c` | 209 | Swap system stubs | ✅ Implemented |
| `9front-pc64/globals.c` | 278 | VMX stubs | ✅ In devvmx.c |

---

## 🟡 MEDIUM - Device Stubs

| File | Line | Issue | Status |
|:-----|:-----|:------|:-------|
| `9front-port/devpebble.c` | 195 | Device entry points stubbed | ✅ Fully impl |
| `9front-port/sdio.c` | 69 | Simple stub for success | ✅ Intentional |
| `9front-port/sdio.c` | 80 | Device registration stub | ✅ Intentional |
| `9front-port/sdio.c` | 85 | LED function stubs | ✅ Intentional |

---

## 🟢 LOW - F# Compiler

| File | Line | Issue |
|:-----|:-----|:------|
| `clr/fsharp_compiler_zero/FSharpToAssembly.fs` | 69 | Convert F# AST to assembly |
| `clr/fsharp_compiler_zero/FSharpCompiler.fs` | 179 | AutomaticOwnershipInference |
| `clr/fsharp_compiler_zero/FSharpCompiler.fs` | 196 | Add optimization passes |
| `clr/fsharp_compiler_zero/FSharpCodeGen.fs` | 774 | Field/property access |
| `clr/fsharp_compiler_zero/FSharpCodeGen.fs` | 983 | TAIL_JMP offset |
| `clr/fsharp_compiler_zero/FSharpCodeGen.fs` | 996 | CALL function address |
| `clr/fsharp_compiler_zero/FSharpCodeGen.fs` | 1047 | Function pointer address |

---

## 🟢 LOW - Fruity/CLR Pipeline

| File | Line | Issue |
|:-----|:-----|:------|
| `clr/fruity/fruity_ir.c` | 493 | Module verification |
| `clr/fruity/fruity_ir.c` | 500 | Function verification |
| `clr/fruity/fruity_ir.c` | 507 | Block verification |
| `clr/fruity/fruity_to_qbe.c` | 115 | Call argument handling |
| `clr/fruity/fruity_to_qbe.c` | 191 | Argument handling |
| `clr/il_to_fruity.c` | 1592 | Get method token |
| `clr/qbe_compile.c` | 120 | Pebble white token integration |

---

## Summary

| Priority | Count |
|:---------|:------|
| 🔴 CRITICAL | 15 |
| 🟡 MEDIUM | 19 |
| 🟢 LOW | 14 |
| **Total** | **48** |

---

## Recommended Order

1. **I/O Port Access** (`devmem.c:179,233`)
2. **Interrupt Disable** (`devirq.c:404`)
3. **devclr implementation** (`devclr.c`)
4. **CLR Type Resolution** (`clr_runtime.c:156,166`)
5. **CGA screen mapping** (`cga.c:177`)
6. **9P stat/rollback** (`9p_router.c`)
