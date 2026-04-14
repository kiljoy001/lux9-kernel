# BrunnenG-Lux9 Integration Architecture

**Status**: Design Document
**Date**: 2025-12-04
**Version**: 1.0

## Executive Summary

BrunnenG provides decentralized identity infrastructure for Lux9 through:
- **TPM 2.0** hardware-rooted private keys
- **USB carriers** for portable public identities
- **Alfis blockchain** for distributed verification
- **W3C DIDs** for standards compliance
- **9P protocol** for universal access

This creates a distributed Active Directory replacement with hardware security, blockchain anchoring, and zero-trust architecture.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Lux9 Kernel                          │
│              (9P mux, Pebble capabilities)              │
└─────────────────────────────────────────────────────────┘
                          ↑ 9P protocol
         ┌────────────────┼────────────────┐
         │                │                │
    ┌────▼─────┐    ┌────▼─────┐    ┌────▼─────┐
    │ devbrunnen│    │ devalfis │    │  devtpm  │
    │ (identity)│    │(blockchain)│   │ (crypto) │
    └─────┬─────┘    └─────┬─────┘    └─────┬────┘
          │                │                │
          │                │                │
     Yggdrasil ←────── Alfis ←──────── TPM 2.0
      Mesh             Chain            Hardware
```

## Core Components

### 1. Identity Model: TPM + USB + Alfis

**Identity Equation:**
```
IDstable = H(DIH || H(YUBIpk))
where DIH = H(TPMpk || H(DILsig))
```

**Storage Distribution:**
- **TPM**: Private key (sealed, never exported)
- **USB Drive**: Public data (certificate, merkle proof, TPM bindings)
- **Alfis Blockchain**: Truth source (merkle roots, DID documents, revocations)

### 2. DID Format

```
DID Syntax: did:brunnen:<identifier>

Examples:
did:brunnen:0xabc123456789...  (IDstable hash)
did:brunnen:alice.ygg          (human-readable alias)
```

### 3. File Hierarchy (/dev/brunnen)

```
/dev/brunnen/
    clone                # Open to get new identity FD

    /dev/brunnen/0/      # Identity session 0
        ctl              # Control: "auth", "register", "bind"
        identity         # Read: current DID
        idstable         # Read: raw IDstable hash
        cert             # Read: X.509 certificate
        sign             # Write: data → Read: signature
        verify           # Write: "sig data" → Read: "valid"

        did/
            document     # Read: full DID document
            resolve      # Write: DID → Read: DID document
            methods      # Read: list of verification methods

        credentials/
            new          # Write: credential JSON to issue
            list         # Read: list of held credentials
            present      # Write: presentation JSON
            verify       # Write: credential → Read: valid/invalid

        namespace/
            layout       # Read: namespace JSON from Alfis
            mount        # Write: "path uri" to bind mount
            umount       # Write: path to unbind
            grants       # Read: current access grants

        groups/
            list         # Read: groups this identity is in
            join         # Write: "group proof" to join
            leave        # Write: group name
            verify       # Write: "group member" → Read: valid/invalid
```

## Identity Operations

### Registration (New Identity)

```bash
# 1. Open identity session
brunnen=/dev/brunnen/clone
read -n 2 $brunnen
id=/dev/brunnen/0

# 2. Create identity
echo "register alice.ygg" > $id/ctl
# TPM generates key, Alfis publishes DID document

# 3. Write to USB drive
usb=/dev/sdb1
echo "export $usb" > $id/ctl
# USB now contains portable identity bundle
```

### Authentication (Local Machine)

```bash
# 1. Insert USB drive with identity
echo "bind /dev/sdb1" > $id/ctl

# 2. Read identity
cat $id/identity              # did:brunnen:alice.ygg
cat $id/did/document          # Full DID document

# 3. Sign challenge
echo "challenge123" > $id/sign
sig=`{cat $id/sign}           # TPM signature
```

### Remote Verification (Different Machine)

```bash
# 1. Bob's machine receives Alice's DID
alice_did="did:brunnen:alice.ygg"

# 2. Resolve via Alfis
echo $alice_did > $id/did/resolve
doc=`{cat $id/did/resolve}

# 3. Verify signature
echo "$sig challenge123" > $id/verify
cat $id/verify                # "valid" or "invalid"

# No pre-configuration needed - pure blockchain verification
```

### TPM Re-binding (New Machine)

```bash
# 1. Insert USB on different machine
echo "bind /dev/sdb1" > $id/ctl
# Warning: TPM mismatch

# 2. Verify old TPM signature (from USB)
# Verify merkle proof (from Alfis)
# Both valid → Alice's identity confirmed

# 3. Create new TPM binding
echo "rebind" > $id/ctl
# New TPM signs identity, updates USB
# Publishes new binding to Alfis
```

## Alfis Integration

### DID Document Structure

```json
{
  "@context": [
    "https://www.w3.org/ns/did/v1",
    "https://w3id.org/security/suites/ed25519-2020/v1"
  ],
  "id": "did:brunnen:0xabc123...",
  "alsoKnownAs": ["did:brunnen:alice.ygg"],

  "verificationMethod": [
    {
      "id": "did:brunnen:0xabc123...#tpm-workstation",
      "type": "TPM2.0-2024",
      "controller": "did:brunnen:0xabc123...",
      "publicKeyJwk": {
        "kty": "RSA",
        "n": "0vx7agoebG...",
        "e": "AQAB"
      },
      "machine": "workstation.alice.ygg",
      "attestation": "0x111222...",
      "bound": "2025-01-15T10:30:00Z"
    },
    {
      "id": "did:brunnen:0xabc123...#dilithium",
      "type": "Dilithium3-2024",
      "controller": "did:brunnen:0xabc123...",
      "publicKeyMultibase": "zDNae6k2P...",
      "purpose": "post-quantum"
    }
  ],

  "authentication": [
    "did:brunnen:0xabc123...#tpm-workstation"
  ],

  "service": [
    {
      "id": "did:brunnen:0xabc123...#namespace",
      "type": "BrunnenNamespace",
      "serviceEndpoint": "ygg://alice.ygg:567"
    }
  ],

  "proof": {
    "type": "BrunnenMerkleProof2024",
    "created": "2025-12-04T14:22:00Z",
    "verificationMethod": "did:brunnen:0xabc123...#dilithium",
    "merkleRoot": "0xdef456...",
    "merkleProof": ["0xaaa...", "0xbbb...", "0xccc..."]
  }
}
```

### Alfis Record Types

**1. Trust Records (Identities):**
```
trust:alice.ygg → DID document
```

**2. Group Records (P256):**
```json
{
  "name": "group:admins.corp.ygg",
  "type": "TXT",
  "value": {
    "p256_group_pub": "0x555666...",
    "members": [
      {
        "idstable": "0xabc123...",
        "member_pub": "0x777888...",
        "proof": "0x999aaa...",
        "joined": "2025-11-01"
      }
    ],
    "merkle_root": "0xccc..."
  }
}
```

**3. Namespace Records:**
```json
{
  "name": "ns:alice.ygg",
  "type": "TXT",
  "value": {
    "owner": "0xabc123...",
    "layout": {
      "/blog": "hyper://abc123.../",
      "/code": "9p://git.alice.ygg:564",
      "/data": "ipfs://QmXyz.../",
      "/secrets": "brunnen://encrypted/alice"
    },
    "capabilities": {
      "black_budget": 2097152,
      "white_tokens": 200,
      "red_snapshots": 20
    }
  }
}
```

## Verifiable Credentials

### Group Membership Credential

```json
{
  "@context": [
    "https://www.w3.org/2018/credentials/v1"
  ],
  "id": "urn:uuid:3978344f-8596-4c3a-a978-8fcaba3903c5",
  "type": ["VerifiableCredential", "GroupMembership"],

  "issuer": "did:brunnen:corp.ygg",
  "issuanceDate": "2025-11-01T00:00:00Z",
  "expirationDate": "2026-11-01T00:00:00Z",

  "credentialSubject": {
    "id": "did:brunnen:alice.ygg",
    "group": "group:admins.corp.ygg",
    "role": "administrator",
    "capabilities": {
      "black_budget": 10485760,
      "white_tokens": 1000,
      "red_snapshots": 100
    }
  },

  "proof": {
    "type": "P256Signature2024",
    "created": "2025-11-01T00:00:00Z",
    "proofPurpose": "assertionMethod",
    "verificationMethod": "did:brunnen:corp.ygg#group-admin",
    "p256Signature": "0x555666..."
  }
}
```

### Presenting Credentials

```bash
# Alice presents admin credential
cat /tmp/admin-credential.json > $id/credentials/present

# BrunnenG verifies and grants capabilities
cat $id/capabilities
# black_budget: 10485760 (from credential)
# white_tokens: 1000
# red_snapshots: 100
```

## Namespace Integration

### Namespace as Identity

Each DID has an associated namespace layout stored in Alfis:

```bash
# Query Alice's namespace layout
echo "ns:alice.ygg" > /dev/alfis/0/query
layout=`{cat /dev/alfis/0/query}

# Namespace JSON defines:
# /blog → hyper://feed
# /code → 9p://git server
# /data → ipfs://hash
# /secrets → encrypted storage
```

### Mounting Remote Namespaces

```bash
# Mount Alice's namespace
srv ygg!alice.ygg!567 /n/alice

# BrunnenG verifies Alice's identity via Alfis
# Checks capabilities from credentials
# Mounts according to namespace layout

ls /n/alice/
# blog/  code/  data/  secrets/
```

### Namespace Composition

```bash
# Create composite namespace from multiple identities
bind /n/alice/blog /workspace/blog
bind /n/bob/code /workspace/code
bind /n/charlie/data /workspace/data

# All verified via BrunnenG + Alfis
# Each identity's access controlled by credentials
# Economic defense applied per-identity
```

## Cryptographic Groups (P256)

### Group Creation

```bash
# Admin creates group
echo "create admins.corp.ygg" > $id/groups/ctl

# Generates P256 group key (sealed in TPM)
# Publishes group record to Alfis
```

### Adding Members

```bash
# Admin adds Alice to group
echo "add alice.ygg admins.corp.ygg" > $id/groups/ctl

# Creates P256 membership proof
# Signs with group admin key
# Publishes to Alfis
```

### Verifying Membership

```bash
# Anyone can verify Alice is in admin group
echo "verify alice.ygg admins.corp.ygg" > $id/groups/verify
cat $id/groups/verify
# "valid" (verified via Alfis blockchain)
```

### Ring Signatures (Privacy)

```bash
# Alice proves group membership without revealing which member
echo "ring-sign admins.corp.ygg challenge123" > $id/groups/sign
sig=`{cat $id/groups/sign}

# Verifier knows:
# - Signature is from someone in admins.corp.ygg
# - Cannot determine which specific member
# - Cannot be forged by non-members
```

## Security Properties

### USB Drive Compromise

**Attacker steals USB:**
- ✓ Has: Certificate, merkle proof, old TPM public keys
- ✗ Doesn't have: TPM private key, ability to sign
- **Result**: Can verify Alice's identity, cannot impersonate

### TPM Compromise

**Attacker compromises one TPM:**
- ✓ Has: Private key for that machine
- ✗ Doesn't have: USB drive, other machines' TPMs
- **Response**: User revokes that TPM binding via Alfis
- **Result**: Compromise isolated to one machine

### Alfis Record Tampering

**Attacker tries to modify trust:alice.ygg:**
- All updates require signature from IDstable's private key
- Private key sealed in TPM
- **Result**: Blockchain record is tamper-proof

### Offline Verification

**Air-gapped network:**
- BrunnenG maintains Alfis cache
- Cache synced periodically via USB
- Identity verification works offline using cached records
- **Result**: No network dependency for authentication

## Active Directory Feature Parity

| Feature | Active Directory | BrunnenG-Lux9 |
|---------|------------------|---------------|
| Authentication | Kerberos (password) | TPM + touch (hardware) |
| Identity Store | LDAP servers | Alfis blockchain |
| Single Sign-On | Windows-only | Cross-platform DIDs |
| Group Policy | GPO objects | Namespace templates |
| DNS | Windows DNS | Alfis blockchain |
| Certificates | Windows CA | Self-sovereign |
| File Sharing | SMB/CIFS | 9P protocol |
| Replication | DC sync | Blockchain consensus |
| Offline Support | Cached credentials | Cached Alfis records |
| Audit Logging | Event Viewer | Blockchain + TPM |

## Economic Defense

### Cost Scaling Formula

```
fee = (base_fee + data_size_kb × storage_rate) × 2^(attempts_in_window)
```

**Examples:**
- 1st write: 0.001 EMC
- 10th write: 0.512 EMC
- 20th write: 524.288 EMC
- **Result**: Spam becomes economically impossible

### Applied to Operations

```bash
# First IPC: cheap
echo "send msg" > /n/brunnen/economic/send
# Cost: 0.001 EMC

# 20th IPC in 1 minute: expensive
echo "send spam" > /n/brunnen/economic/send
# Cost: 524.288 EMC
# Result: Rate limited by economics, not arbitrary rules
```

## Implementation Phases

### Phase 1: MVP (USB + TPM + Local)
- [x] TPM 2.0 integration (`/dev/tpm`)
- [ ] USB identity bundles
- [ ] Local authentication
- [ ] Basic 9P server

### Phase 2: Network (Alfis + Yggdrasil)
- [ ] Alfis integration (`/dev/alfis`)
- [ ] DID resolution
- [ ] Remote verification
- [ ] Mesh networking

### Phase 3: Credentials (Groups + Capabilities)
- [ ] Verifiable credentials
- [ ] P256 groups
- [ ] Namespace templates
- [ ] Pebble integration

### Phase 4: Production (Enterprise Features)
- [ ] AD bridge (migration path)
- [ ] Keycloak integration
- [ ] PAM module
- [ ] Audit logging

## Open Questions

1. **USB Drive Security**: Software-only encryption vs hardware tokens?
   - **Solution**: Support both, start with USB ($5), offer hardware option ($10)

2. **Alfis Scalability**: How many identities can blockchain support?
   - **Research needed**: Benchmark Alfis with 10K+ identity records

3. **TPM Portability**: How to use identity on non-TPM machines?
   - **Fallback**: Encrypted USB mode with PIN (less secure, but works)

4. **Ring Signature Performance**: P256 ring signatures at scale?
   - **Research needed**: Performance testing with 100+ member groups

## References

- W3C DID Specification: https://www.w3.org/TR/did-core/
- W3C Verifiable Credentials: https://www.w3.org/TR/vc-data-model/
- Alfis Project: https://github.com/Revertron/Alfis
- TPM 2.0 Specification: https://trustedcomputinggroup.org/
- 9P Protocol: http://9p.io/sys/man/5/INDEX.html

## Conclusion

BrunnenG transforms Lux9 into a distributed operating system with:
- **Hardware-rooted identity** (TPM + USB)
- **Blockchain verification** (Alfis)
- **Standards compliance** (W3C DIDs)
- **Universal access** (9P protocol)
- **Economic security** (BCRA built-in)

Everything exposed as files. Everything verified by math. Everything distributed by default.

**This is what Active Directory should have been.**
