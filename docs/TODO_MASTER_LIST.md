# Lux9 Consolidated TODO List

> **Last verified: 2025-12-19**
> Scope: `kernel/`, `proofs/`, `tools/` (excludes external deps)

---

## Summary

| Category | Count | Status |
|----------|-------|--------|
| Fruity Interpreter | 17 | Active |
| Boot & Runtime | 3 | Active |
| Fruity→QBE | 1 | Active |
| IL→Fruity | 0 | ✅ Complete |
| QBE Exec | 0 | ✅ Complete |
| Admitted Proofs | 17 | Active |
| **Total** | **38** | |

---

## 🔴 Fruity Interpreter (`fruity_interp.c`)

| Line | Issue |
|------|-------|
| 72 | Revoke white token when API available |
| 81 | Red snapshot implementation |
| 86 | Blue commit implementation |
| 91 | Red rollback implementation |
| 323 | Overflow arithmetic check |
| 543 | Load string from metadata |
| 576 | Zero-copy IPC transfer |
| 801 | vtable lookup for LDVIRTFTN |
| 898 | Static field access |
| 905 | Static field store |
| 955 | Type checking (CASTCLASS) |
| 1127 | Exception dispatch (RETHROW) |
| 1143 | Varargs support |
| 1149 | Tail call optimization |
| 1153 | Check finite float |
| 1161 | Typed references |
| 1212 | Args conversion |

---

## 🔴 Boot & Runtime

| File:Line | Issue |
|-----------|-------|
| `userinit.c:226` | Load CLR from /boot/boot |
| `userinit.c:317-319` | **FIXME**: m→pml4 hang |
| `execution_engine.c:4582` | System.String token |

---

## 🟡 Fruity→QBE (`fruity_to_qbe.c`)

| Line | Issue |
|------|-------|
| 167,172,180,188 | Overflow checks (emit runtime) |

---

## ✅ Completed (2025-12-19)

- `il_to_fruity.c:740` — Type-aware flavor injection ✅
- `fruity_interp.c:689` — Method invocation (CALL) ✅
- `fruity_interp.c:695` — Indirect call (CALLI) ✅
- `fruity_interp.c:703` — Load function pointer (LDFTN) ✅
- `fruity_interp.c:925` — Object construction (NEWOBJ) ✅
- `qbe_exec.c:472` — AOT cache lookup ✅
- `qbe_exec.c:482` — AOT precompilation ✅

---

## ⚠️ Admitted Proofs (17)

| File | Count | Notes |
|------|-------|-------|
| `ramdisk_wipe.v` | 5 | Memory fill properties |
| `fsharp_complete_typechecker.v` | 3 | Type preservation |
| `fsharp_fully_proven.v` | 3 | Type soundness |
| `fsharp_full_language.v` | 3 | Complex cases |
| `fsharp_zero_admits_final.v` | 1 | Substitution lemma |
| `fsharp_proven_zero_admits.v` | 1 | Weak substitution |
| `fsharp_truly_minimal.v` | 1 | Minimal system |

> Note: `pebble_clr.v:step_preserves_memory_safety` and `il_semantics.v:il_step_deterministic` are now **fully proven**.

---

## Top 5 Priorities

1. **`userinit.c:317`** — m→pml4 hang (boot stability)
2. **`fruity_interp.c:898`** — Static field access
3. **`fruity_interp.c:543`** — String metadata loading
4. **`fruity_interp.c:955`** — Type checking (CASTCLASS)
5. **`ramdisk_wipe.v`** — Complete memory wipe proofs
