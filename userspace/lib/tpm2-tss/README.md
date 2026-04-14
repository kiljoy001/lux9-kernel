# TPM2-TSS Port to Lux9

This directory contains a port of the TPM2 Software Stack (TSS2) to the Lux9 kernel.

## Source

Adapted from: https://github.com/tpm2-software/tpm2-tss
License: BSD-2-Clause
Original Authors: Intel Corporation, Fraunhofer SIT

## What We're Porting

### 1. MU Layer (Marshaling/Unmarshaling) - `mu/`
- Converts TPM2 data structures to/from big-endian wire format
- ~4,300 lines of code
- Minimal dependencies (just endian conversion)

### 2. SAPI Layer (System API) - `sapi/`
- 1-to-1 mapping of TPM2 commands
- Command construction and response parsing
- Uses MU layer for serialization

### 3. NOT Porting
- **ESAPI** - Too complex, session management not needed in kernel
- **FAPI** - High-level API with JSON/filesystem dependencies
- **TCTI** - We already have TIS driver (`tpm2_driver.c`)

## Integration

Stack:
```
[9P TPM Interface] (/dev/tpm/...)
        ↓
   [SAPI Layer] (command construction)
        ↓
    [MU Layer] (marshaling)
        ↓
  [TIS Driver] (hardware I/O) - existing tpm2_driver.c
```

## Build

MU and SAPI will be compiled into the kernel. Headers in `include/` provide the TPM2 type definitions and function prototypes.

## License

This port maintains BSD-2-Clause licensing from the original tpm2-tss project.
The TIS driver (`../9front-port/tpm2_driver.c`) is GPL-2.0 (adapted from Linux).
