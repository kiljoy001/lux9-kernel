# TPM2-TSS Porting Notes for Lux9

## What We Just Did

We've imported the complete TPM2 Software Stack's MU and SAPI layers into the Lux9 kernel. This gives us the full TPM 2.0 command set with proper marshaling.

## Files Imported

- **145 C source files**
- **28 header files**
- **~29,000 lines of code total**

### Structure

```
kernel/tpm2-tss/
├── LICENSE (BSD-2-Clause)
├── README.md
├── include/          # 25 TSS2 headers
│   ├── tss2_mu.h
│   ├── tss2_sys.h
│   ├── tss2_tpm2_types.h
│   ├── tss2_common.h
│   └── tss2_endian.h
├── mu/               # 7 marshaling implementation files (~4,300 LOC)
│   ├── base-types.c
│   ├── tpm2b-types.c
│   ├── tpma-types.c
│   ├── tpml-types.c
│   ├── tpms-types.c
│   ├── tpmt-types.c
│   └── tpmu-types.c
├── sapi/             # 137+ SAPI command implementations
│   ├── api/
│   │   ├── Tss2_Sys_NV_DefineSpace.c
│   │   ├── Tss2_Sys_NV_Write.c
│   │   ├── Tss2_Sys_NV_Read.c
│   │   ├── Tss2_Sys_NV_UndefineSpace.c
│   │   └── ... (133 more TPM2 commands)
│   ├── sysapi_util.c
│   └── sysapi_util.h
└── util/
    ├── tss2_endian.h
    └── log.h
```

## Next Steps

### 1. Adaptation for Kernel

These files currently have dependencies on:
- Standard C library (stdint.h, string.h, etc.)
- Logging macros
- Config headers

We need to:
1. Replace `#include <stdint.h>` with our kernel types (`u.h`)
2. Replace logging with kernel `print()` or disable
3. Remove/stub out unnecessary features (async, complex error handling)

### 2. Build Integration

Add to GNUmakefile:
```make
TPM2_TSS_MU = $(wildcard kernel/tpm2-tss/mu/*.c)
TPM2_TSS_SAPI = $(wildcard kernel/tpm2-tss/sapi/*.c kernel/tpm2-tss/sapi/api/*.c)
TPM2_TSS_OBJS = $(patsubst %.c,%.o,$(TPM2_TSS_MU) $(TPM2_TSS_SAPI))
```

### 3. Create 9P Interface

Build `/dev/tpm/` filesystem on top of SAPI:
```
/dev/tpm/
├── ctl          # Control file for commands
├── random       # Read TPM random bytes
├── pcr/         # PCR operations
│   ├── 0
│   ├── 1
│   └── ...
└── nv/          # NV storage
    ├── 0x1500001
    └── ...
```

### 4. Integration with Blind Ledger

Once 9P interface is ready, the blind ledger can use TPM for:
- Secret generation: `tpm_nv_write(index, secret, 64)`
- Secret storage: Hardware-backed NVRAM
- Secret destruction: `tpm_nv_delete(index)`

## Why This Approach Works

1. **No Reinventing**: We leverage 10+ years of TPM2 development
2. **Full Feature Set**: All 137 TPM2 commands available
3. **Proper Marshaling**: MU layer handles all endianness correctly
4. **Clean Layering**: SAPI provides simple command construction
5. **9P Integration**: Fits perfectly with Plan 9 philosophy

## Estimate

- Adaptation work: ~2-4 hours (replace headers, fix logging, test build)
- 9P interface: ~3-5 hours (devtpm.c device driver)
- Integration: ~1-2 hours (connect blind_ledger to /dev/tpm)

**Total**: ~6-11 hours to have complete, production-ready TPM 2.0 support.

Compare to writing from scratch: **Months** of work studying specs, debugging protocol issues, etc.

## Open Source Leverage FTW

Just like the TIS driver took 17 minutes instead of months, this port will take hours instead of months.

**That's the power of open source.**
