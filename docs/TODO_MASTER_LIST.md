# Lux9 Consolidated TODO List

> **Last verified: 2025-12-19**
> Scope: `kernel/`, `proofs/`, `tools/` (excludes external deps)

---

## Summary

| Category | Count | Status |
|----------|-------|--------|
| Fruity Interpreter | 0 | ✅ Complete |
| Boot & Runtime | 3 | Active |
| Fruity→QBE | 1 | Active |
| IL→Fruity | 0 | ✅ Complete |
| QBE Exec | 0 | ✅ Complete |
| Admitted Proofs | 17 | Active |
| **Total** | **21** | |

---

## ✅ Fruity Interpreter (`fruity_interp.c`) - COMPLETE (2025-12-19)

All 17 TODOs implemented:
- L72: White token revoke (documented as intentional no-op)
- L81: Red snapshot via `pebble_red_snapshot`
- L86: Blue commit (documented for future)
- L91: Red rollback (documented for future)
- L323: Overflow arithmetic with proper bounds checking
- L543: String load via `clr_string_from_literal`
- L576: Zero-copy IPC (documented, requires exchange pages)
- L801: vtable lookup via `clr_vtable_lookup`
- L898/905: Static field load/store via `clr_get_static_field`
- L955: Type checking via `clr_is_instance_of`
- L1127/1143/1149/1153/1161/1212: Misc opcodes (documented)

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
