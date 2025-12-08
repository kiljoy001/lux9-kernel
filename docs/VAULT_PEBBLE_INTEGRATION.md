# Secure Vault Integration with Pebble System

## Priority: Pebble Software Path First, Factotum Later

The secure vault is a **foundational component** for the Pebble system's software authentication path. Before building higher-level services like factotum, we need to ensure proper integration with Pebble.

## Pebble Context

### What is Pebble?

From the codebase, Pebble is a capability-based memory management system with colored allocations:

- **Black** - Non-swappable, secure memory (our vault uses this)
- **White** - Swappable, normal memory
- **Red** - Device/MMIO memory
- **Blue** - Shared memory regions

### Current Vault-Pebble Integration

The vault already uses Pebble Black allocation:

```c
// kernel/9front-port/devram.c:152-167

if(pebble_black_alloc(secure_rd.size, &handle) == 0){
    PebbleState *ps = pebble_state();
    if(ps && (pb = pebble_lookup_black(ps, handle)) != nil){
        secure_rd.data = pb->addr;
        secure_rd.pebble_handle = handle;
        print("ramdisk: secure vault %lud bytes allocated via Pebble at %p\n",
              secure_rd.size, secure_rd.data);
    }
}
```

## Why Vault is Critical for Pebble Software Path

### 1. Secure Credential Storage

Pebble needs to authenticate software components. The vault provides:
- **Secure key storage** for code signing keys
- **Certificate storage** for capability tokens
- **Password/secret management** for software attestation

### 2. Capability Token Management

Pebble's capability system needs secure storage for:
- **White tokens** (capability proofs)
- **Process credentials** (authentication tokens)
- **Delegation chains** (signed capability transfers)

### 3. Software Attestation Path

The vault enables the software authentication chain:

```
1. Boot → Secure vault initialized with password
2. Kernel → Loads signing keys from vault
3. Software → Verified against keys in vault
4. Pebble → Issues capabilities based on verified software
5. Processes → Receive White tokens for memory access
```

## Missing Integration Points

### What We Have ✅

1. **Vault device**: `/dev/secureram` and `/dev/secureram.ctl`
2. **Pebble Black allocation**: Non-swappable memory
3. **Encryption**: XChaCha20 for data at rest
4. **Password protection**: Argon2id key derivation
5. **Secure wipe**: 7-pass DoD cleanup

### What We Need 🔧

1. **Pebble Budget Integration**
   - Track vault usage against process secure budgets
   - Enforce per-process secure memory limits
   - Account for Black allocations

2. **Capability-Based Access**
   - Only processes with appropriate capabilities can access vault
   - Integrate with Pebble White token verification
   - Enforce capability checks on vault operations

3. **Process-Specific Vault Regions**
   - Allow multiple processes to have isolated vault regions
   - Each region protected by separate password
   - Prevent one process from accessing another's vault data

4. **Audit Trail**
   - Log all vault access operations
   - Track which process accessed what and when
   - Integrate with Pebble's security event logging

5. **Key Derivation from Pebble State**
   - Derive vault keys from Pebble measurement chain
   - Bind vault unlock to specific system state
   - Prevent unlock if Pebble state is compromised

## Pebble Software Path Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Boot Sequence                            │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│    Initialize Vault with Root Password                      │
│    - Derive master key from password                        │
│    - Generate per-subsystem keys                            │
│    - Store in Pebble Black allocation                       │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│    Load Code Signing Keys from Vault                        │
│    - Public keys for signature verification                 │
│    - Root CA certificates                                   │
│    - Certificate revocation lists                           │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│    Verify Software Components                               │
│    - Check signatures on kernel modules                     │
│    - Verify userspace binaries                              │
│    - Validate library integrity                             │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│    Issue Pebble Capabilities                                │
│    - Generate White tokens for verified processes           │
│    - Assign memory budgets                                  │
│    - Grant Black allocation rights to trusted code          │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│    Store Runtime Secrets in Vault                           │
│    - Session keys                                           │
│    - Authentication tokens                                  │
│    - Delegation credentials                                 │
└─────────────────────────────────────────────────────────────┘
```

## Implementation Priority

### Phase 1: Basic Integration (Current)
- ✅ Vault uses Pebble Black allocation
- ✅ Password-protected storage
- ✅ XChaCha20 encryption

### Phase 2: Pebble Budget Tracking (Next)
- Track vault allocations against Pebble budgets
- Enforce secure memory limits per process
- Integrate with `devpebble.c` accounting

### Phase 3: Capability-Based Access (After Phase 2)
- Require Pebble White tokens to access vault
- Implement capability checks in vault operations
- Add per-process vault isolation

### Phase 4: Audit and Attestation (After Phase 3)
- Implement audit trail
- Add Pebble state measurement
- Bind vault unlock to system integrity

### Phase 5: Software Verification (After Phase 4)
- Store code signing keys in vault
- Implement signature verification
- Integrate with Pebble capability issuance

## Why Wait on Factotum

Factotum is a **higher-level service** that builds on the vault. Before implementing it, we need:

1. **Solid vault-Pebble integration** - Budget tracking, capability checks
2. **Process isolation** - Multiple processes using vault safely
3. **Audit trail** - Security event logging
4. **Software verification path** - Code signing infrastructure

Once these are in place, factotum becomes the **authentication frontend** that uses the vault for key storage. But the vault must first be properly integrated into Pebble's security model.

## Current Status

**Vault Implementation**: ✅ Complete
- Encryption: XChaCha20
- Password: Argon2id
- Storage: Pebble Black
- Wipe: 7-pass DoD

**Pebble Integration**: 🔧 In Progress
- Black allocation: ✅ Done
- Budget tracking: ❌ TODO
- Capability checks: ❌ TODO
- Process isolation: ❌ TODO

**Factotum Integration**: 🔮 Future
- Depends on complete Pebble integration
- Will use vault as secure key store
- Will integrate with capability system

## Next Steps

1. **Complete devpebble.c allocation I/O** - Make Pebble operations fully functional
2. **Add budget tracking to vault** - Enforce secure memory limits
3. **Implement capability checks** - Require Pebble tokens for vault access
4. **Add audit trail** - Log all vault operations
5. **Then** build factotum on top of solid foundation

## Conclusion

The vault is the **secure storage grain** that will enable:
- **Pebble software path** (immediate priority)
- **Factotum authentication** (future goal)
- **Secure software execution** (long-term vision)

By focusing on Pebble integration first, we ensure the vault is properly embedded in the system's security architecture before adding higher-level services like factotum.
