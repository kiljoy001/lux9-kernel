# NIST Cryptographic Standards Index

## FIPS Standards (Mandatory for Federal Use)

### FIPS 197 - Advanced Encryption Standard (AES)
- **File**: FIPS-197-AES.pdf
- **Algorithms**: AES-128, AES-192, AES-256
- **Status**: Current standard
- **Implementation Priority**: Tier 1

### FIPS 180-4 - Secure Hash Standard
- **File**: FIPS-180-4-SHA2.pdf  
- **Algorithms**: SHA-224, SHA-256, SHA-384, SHA-512, SHA-512/224, SHA-512/256
- **Status**: Current standard
- **Implementation Priority**: Tier 1

### FIPS 202 - SHA-3 Standard
- **File**: FIPS-202-SHA3.pdf
- **Algorithms**: SHA3-224, SHA3-256, SHA3-384, SHA3-512, SHAKE128, SHAKE256
- **Status**: Current standard
- **Implementation Priority**: Tier 1

### FIPS 203 - Module-Lattice-Based Key-Encapsulation Mechanism
- **File**: FIPS-203-ML-KEM.pdf
- **Algorithms**: ML-KEM-512, ML-KEM-768, ML-KEM-1024
- **Status**: Current standard (2024)
- **Implementation Priority**: Tier 1 (Post-quantum)

### FIPS 204 - Module-Lattice-Based Digital Signature Standard
- **File**: FIPS-204-ML-DSA.pdf
- **Algorithms**: ML-DSA-44, ML-DSA-65, ML-DSA-87
- **Status**: Current standard (2024)
- **Implementation Priority**: Tier 1 (Post-quantum)

### FIPS 205 - Stateless Hash-Based Digital Signature Standard
- **File**: FIPS-205-SLH-DSA.pdf
- **Algorithms**: SLH-DSA variants
- **Status**: Current standard (2024)
- **Implementation Priority**: Tier 1 (Post-quantum)

## SP 800 Series (Implementation Guidance)

### Modes of Operation
- **SP 800-38A**: ECB, CBC, CFB, OFB, CTR modes
- **SP 800-38B**: CMAC authentication
- **SP 800-38C**: CCM authenticated encryption
- **SP 800-38D**: GCM authenticated encryption
- **SP 800-38F**: Key wrapping modes

### Key Management
- **SP 800-56A**: ECDH key agreement
- **SP 800-108**: KDF specifications
- **SP 800-90A**: DRBG random number generation

## Implementation Requirements

All FIPS algorithms must be implemented with:
1. **Exact specification compliance**
2. **CAVP test vector validation**
3. **FIPS 140-3 module requirements**
4. **Formal verification where possible**

## Test Vectors

NIST CAVP (Cryptographic Algorithm Validation Program) provides test vectors:
- https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program

## Usage in OpenSSL Replacement

- **Tier 1 Priority**: All FIPS algorithms are mandatory
- **Government Compliance**: Required for federal contracts
- **Mathematical Foundation**: Use extracted proofs for formal verification
- **Performance**: Kernel-space implementation for maximum speed
