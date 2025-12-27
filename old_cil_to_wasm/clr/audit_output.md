Found 207 defined opcodes.
# Full CLR Opcode Audit Report

**Total Defined:** 207
**Implemented:** 173
**Stubbed/NOP:** 5
**Missing:** 29

## 🔴 Missing Opcodes (Defined in header, missing in C)
| Opcode | Name | Hex |
| :--- | :--- | :--- |
| `IL_ARGLIST` | ARGLIST | 0xFE00 |
| `IL_CEQ` | CEQ | 0xFE01 |
| `IL_CGT` | CGT | 0xFE02 |
| `IL_CGT_UN` | CGT_UN | 0xFE03 |
| `IL_CLT` | CLT | 0xFE04 |
| `IL_CLT_UN` | CLT_UN | 0xFE05 |
| `IL_CONSTRAINED` | CONSTRAINED | 0xFE16 |
| `IL_CPBLK` | CPBLK | 0xFE17 |
| `IL_DIV_UN` | DIV_UN | 0x5C |
| `IL_INITBLK` | INITBLK | 0xFE18 |
| `IL_INITOBJ` | INITOBJ | 0xFE15 |
| `IL_LDARG` | LDARG | 0xFE09 |
| `IL_LDARGA` | LDARGA | 0xFE0A |
| `IL_LDC_R4` | LDC_R4 | 0x22 |
| `IL_LDC_R8` | LDC_R8 | 0x23 |
| `IL_LDFTN` | LDFTN | 0xFE06 |
| `IL_LDLOC` | LDLOC | 0xFE0C |
| `IL_LDLOCA` | LDLOCA | 0xFE0D |
| `IL_LDVIRTFTN` | LDVIRTFTN | 0xFE07 |
| `IL_LOCALLOC` | LOCALLOC | 0xFE0F |
| `IL_NO` | NO | 0xFE19 |
| `IL_READONLY` | READONLY | 0xFE1E |
| `IL_REFANYTYPE` | REFANYTYPE | 0xFE1D |
| `IL_REM_UN` | REM_UN | 0x5E |
| `IL_RETHROW` | RETHROW | 0xFE1A |
| `IL_SHR_UN` | SHR_UN | 0x64 |
| `IL_SIZEOF` | SIZEOF | 0xFE1C |
| `IL_STARG` | STARG | 0xFE0B |
| `IL_STLOC` | STLOC | 0xFE0E |

## 🟡 Stubbed / NOP Implementations
| Opcode | Name | Hex | Note |
| :--- | :--- | :--- | :--- |
| `IL_JMP` | JMP | 0x27 | Mapped to `FRUITY_NOP` |
| `IL_LDFLDA` | LDFLDA | 0x7C | Mapped to `FRUITY_NOP` |
| `IL_LDSFLDA` | LDSFLDA | 0x7F | Mapped to `FRUITY_NOP` |
| `IL_MUL_OVF` | MUL_OVF | 0xD8 | Mapped to `FRUITY_NOP` |
| `IL_MUL_OVF_UN` | MUL_OVF_UN | 0xD9 | Mapped to `FRUITY_NOP` |
