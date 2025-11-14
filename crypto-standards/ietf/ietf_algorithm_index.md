# IETF Cryptographic Standards Index

## Core Authentication (HMAC Family)
- **RFC 2104**: HMAC base specification
- **RFC 4231**: HMAC-SHA-224/256/384/512 test vectors
- **RFC 6234**: SHA implementations with HMAC

## Password-Based Cryptography
- **RFC 2898**: PBKDF2 specification  
- **RFC 8018**: PKCS #5 v2.1 (updated PBKDF2)
- **RFC 7914**: scrypt key derivation function

## Modern Symmetric Cryptography
- **RFC 7539**: ChaCha20-Poly1305 specification
- **RFC 8439**: ChaCha20-Poly1305 AEAD standard
- **RFC 7693**: BLAKE2 hash function
- **RFC 3394**: AES Key Wrap algorithm

## Key Derivation Functions
- **RFC 5869**: HKDF (HMAC-based KDF)
- **RFC 6234**: SHA-based key derivation
- **RFC 8018**: Password-based key derivation

## Elliptic Curve Cryptography
- **RFC 7748**: Curve25519 and Curve448
- **RFC 8032**: EdDSA (Ed25519/Ed448 signatures)
- **RFC 5639**: Brainpool elliptic curves
- **RFC 6979**: Deterministic ECDSA signatures

## Authenticated Encryption
- **RFC 5116**: AEAD interface specification
- **RFC 4106**: AES-GCM for IPsec
- **RFC 8439**: ChaCha20-Poly1305 AEAD
- **RFC 7539**: ChaCha20-Poly1305 for IETF protocols

## Public Key Cryptography
- **RFC 3447**: RSA PKCS #1 v2.1
- **RFC 8017**: RSA PKCS #1 v2.2 (current)
- **RFC 6979**: Deterministic DSA/ECDSA

## TLS/SSL Implementation
- **RFC 8446**: TLS 1.3 (current standard)
- **RFC 5246**: TLS 1.2 (legacy support)
- **RFC 8447**: ECDHE-PSK cipher suites

## Implementation Priority for OpenSSL Replacement

### Tier 1 (Internet Core Standards)
1. **ChaCha20-Poly1305** (RFC 7539, 8439) - Modern AEAD
2. **HMAC** (RFC 2104) - Message authentication
3. **HKDF** (RFC 5869) - Key derivation
4. **Ed25519** (RFC 8032) - Modern signatures
5. **BLAKE2** (RFC 7693) - Fast hash function

### Tier 2 (Established Standards)
1. **AES-GCM** (RFC 4106) - Authenticated encryption
2. **PBKDF2** (RFC 2898, 8018) - Password-based KDF
3. **RSA** (RFC 8017) - Legacy public key
4. **Curve25519** (RFC 7748) - Key agreement

### Tier 3 (Compatibility)
1. **scrypt** (RFC 7914) - Alternative password KDF
2. **Brainpool curves** (RFC 5639) - European ECC
3. **TLS 1.2** (RFC 5246) - Legacy TLS support

## Test Vectors and Validation

All RFCs include comprehensive test vectors for implementation validation.
These are essential for ensuring compatibility with existing internet infrastructure.

## Usage in OpenSSL Replacement

- **Internet Standards Compliance**: All Tier 1 algorithms mandatory
- **TLS Implementation**: RFC 8446 (TLS 1.3) as primary target
- **API Design**: RFC 5116 AEAD interface as model
- **Performance**: ChaCha20-Poly1305 preferred over AES-GCM for software
