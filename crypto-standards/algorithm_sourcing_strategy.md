# Complete Algorithm Sourcing Strategy: 200+ Algorithms

## Official Sources by Category

### Government/Standards Bodies (Highest Priority)
*These provide official, legally compliant specifications*

#### NIST (US) - Primary Source
- **Location**: https://csrc.nist.gov/publications
- **Key Documents**:
  - FIPS 197 (AES specification)
  - FIPS 180-4 (SHA-2 family)
  - FIPS 202 (SHA-3/Keccak)
  - FIPS 203 (ML-KEM/Kyber) 
  - FIPS 204 (ML-DSA/Dilithium)
  - FIPS 205 (SLH-DSA/SPHINCS+)
  - SP 800-series (implementation guidance)

#### IETF - Internet Standards
- **Location**: https://tools.ietf.org/rfc/
- **Key RFCs**:
  - RFC 7539 (ChaCha20-Poly1305)
  - RFC 8439 (ChaCha20-Poly1305 for IETF Protocols)
  - RFC 2104 (HMAC)
  - RFC 2898 (PBKDF2)
  - RFC 8017 (PKCS #1 v2.2 - RSA)
  - RFC 8446 (TLS 1.3)

#### ISO/IEC - International Standards
- **Location**: https://www.iso.org/standard/
- **Key Standards**:
  - ISO/IEC 18033 (Encryption algorithms)
  - ISO/IEC 10118 (Hash functions)
  - ISO/IEC 9797 (Message Authentication Codes)
  - ISO/IEC 11770 (Key management)

### National Standards (International Coverage)

#### Japan - CRYPTREC
- **Location**: https://www.cryptrec.go.jp/en/
- **Algorithms**: Camellia, MISTY1, KCipher-2
- **Documents**: CRYPTREC Report, e-Government Recommended Ciphers List

#### China - State Cryptography Administration
- **Location**: http://www.oscca.gov.cn/ 
- **Algorithms**: SM2, SM3, SM4, SM9
- **Documents**: GM/T standards series

#### Russia - GOST Standards
- **Location**: https://tc26.ru/en/
- **Algorithms**: GOST 28147-89, GOST R 34.11, Streebog, Kuznyechik
- **Documents**: GOST R series, RFC 4357, RFC 5830

#### Europe - ECRYPT
- **Location**: https://www.ecrypt.eu.org/
- **Algorithms**: European stream cipher recommendations
- **Documents**: eSTREAM portfolio, ECRYPT yearly reports

### Academic Sources (Research & Proofs)

#### Cryptology ePrint Archive (IACR)
- **Location**: https://eprint.iacr.org/
- **Content**: Latest research papers with proofs
- **Search Strategy**: 
  ```bash
  # Search for specific algorithms
  curl "https://eprint.iacr.org/search?q=ChaCha20"
  curl "https://eprint.iacr.org/search?q=BLAKE2"
  curl "https://eprint.iacr.org/search?q=Argon2"
  ```

#### ACM Digital Library
- **Location**: https://dl.acm.org/
- **Content**: Peer-reviewed algorithm papers
- **Key Conferences**: CCS, CRYPTO, EUROCRYPT, ASIACRYPT

#### IEEE Xplore
- **Location**: https://ieeexplore.ieee.org/
- **Content**: Hardware implementations, side-channel analysis
- **Search Terms**: "cryptographic algorithm", "implementation"

#### Springer Cryptographic Literature
- **Location**: https://link.springer.com/
- **Content**: LNCS series with detailed proofs
- **Key Series**: Lecture Notes in Computer Science (LNCS)

### Algorithm Competition Winners

#### AES Competition (Historical)
- **Finalists**: Rijndael (winner), Serpent, Twofish, RC6, MARS
- **Source**: https://csrc.nist.gov/projects/cryptographic-standards-and-guidelines/archived-crypto-projects/aes-development

#### SHA-3 Competition
- **Finalists**: Keccak (winner), BLAKE, Grøstl, JH, Skein
- **Source**: https://csrc.nist.gov/projects/hash-functions

#### CAESAR Competition (AEAD)
- **Winners**: AEGIS, ASCON, COLM, Deoxys-II, OCB, ACORN
- **Source**: https://competitions.cr.yp.to/caesar.html

#### Password Hashing Competition
- **Winner**: Argon2
- **Finalists**: Catena, Lyra2, Makwa, yescrypt
- **Source**: https://password-hashing.net/

#### eSTREAM (Stream Ciphers)
- **Portfolio**: Trivium, MICKEY, Grain, HC-128, Rabbit, Salsa20, SOSEMANUK
- **Source**: https://www.ecrypt.eu.org/stream/

### Implementation References

#### Reference Implementations
- **SUPERCOP**: https://bench.cr.yp.to/supercop.html
  - Optimized implementations of many algorithms
  - Performance benchmarks
  - Multiple architectures

- **libsodium**: https://github.com/jedisct1/libsodium
  - Modern crypto library with clean implementations
  - ChaCha20, Poly1305, BLAKE2, Argon2, Ed25519

- **HACL***: https://github.com/hacl-star/hacl-star
  - Formally verified implementations in F*
  - Proven memory-safe C code generation

#### Test Vectors Sources
- **NIST CAVP**: https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program
- **RFC Test Vectors**: Embedded in RFC specifications
- **Academic Papers**: Usually include test vectors in appendices

## Systematic Download Strategy

### Phase 1: Official Government Standards
```bash
# Download all NIST publications
./scripts/download_nist_standards.sh

# Download IETF RFCs for crypto algorithms  
./scripts/download_ietf_crypto_rfcs.sh

# Download ISO/IEC crypto standards
./scripts/download_iso_crypto_standards.sh
```

### Phase 2: National Standards
```bash
# Download international crypto standards
./scripts/download_cryptrec_standards.sh    # Japan
./scripts/download_sm_algorithms.sh         # China  
./scripts/download_gost_standards.sh        # Russia
./scripts/download_ecrypt_portfolio.sh      # Europe
```

### Phase 3: Academic Research
```bash
# Download from IACR ePrint Archive
./scripts/download_iacr_papers.sh

# Download competition documentation
./scripts/download_crypto_competitions.sh

# Download reference implementations
./scripts/download_reference_implementations.sh
```

### Phase 4: Implementation Sources
```bash
# Download and analyze existing implementations
./scripts/analyze_supercop_implementations.sh
./scripts/analyze_libsodium_source.sh
./scripts/analyze_hacl_star_proofs.sh
```

## Algorithm Source Matrix

### Tier 1: Official Standards (Government/IETF)
| Algorithm | Official Source | Implementation Guide | Test Vectors |
|-----------|----------------|---------------------|-------------|
| AES | NIST FIPS 197 | ✅ Complete spec | ✅ NIST CAVP |
| SHA-3 | NIST FIPS 202 | ✅ Complete spec | ✅ NIST CAVP |
| ChaCha20 | IETF RFC 7539 | ✅ Complete spec | ✅ RFC vectors |
| ML-KEM | NIST FIPS 203 | ✅ Complete spec | ✅ NIST CAVP |
| ML-DSA | NIST FIPS 204 | ✅ Complete spec | ✅ NIST CAVP |
| HMAC | IETF RFC 2104 | ✅ Complete spec | ✅ RFC vectors |
| PBKDF2 | IETF RFC 2898 | ✅ Complete spec | ✅ RFC vectors |

### Tier 2: Competition Winners
| Algorithm | Competition Source | Academic Papers | Reference Implementation |
|-----------|-------------------|-----------------|------------------------|
| Serpent | AES finalist | ✅ Available | ✅ SUPERCOP |
| Twofish | AES finalist | ✅ Available | ✅ SUPERCOP |
| BLAKE2 | SHA-3 finalist | ✅ Available | ✅ SUPERCOP |
| Skein | SHA-3 finalist | ✅ Available | ✅ SUPERCOP |
| Argon2 | PHC winner | ✅ Available | ✅ libsodium |
| Trivium | eSTREAM portfolio | ✅ Available | ✅ SUPERCOP |

### Tier 3: National/Regional Standards
| Algorithm | National Source | Documentation | Implementation Status |
|-----------|-----------------|---------------|---------------------|
| Camellia | CRYPTREC (Japan) | ✅ Available | ✅ Multiple sources |
| SM4 | China GM/T | ✅ Available | ⚠️ Limited |
| GOST | Russia GOST R | ✅ Available | ⚠️ Limited |
| Kuznyechik | Russia GOST R | ✅ Available | ⚠️ Limited |

## Acquisition Priority

### Immediate (Week 1-2)
1. **Download all NIST FIPS documents** - Complete official specs
2. **Download key IETF RFCs** - Internet standard algorithms
3. **Extract SUPERCOP implementations** - Reference code for 100+ algorithms

### Short-term (Week 3-4)
1. **Download competition documentation** - AES, SHA-3, CAESAR, PHC, eSTREAM
2. **Academic papers for proof techniques** - IACR ePrint targeted searches
3. **National standards** - CRYPTREC, GM/T, GOST series

### Medium-term (Month 2)
1. **Specialized algorithm families** - Format-preserving encryption, threshold crypto
2. **Hardware-oriented algorithms** - Lightweight crypto, IoT-specific
3. **Emerging algorithms** - Latest research, post-quantum candidates

## Implementation Database Structure

```json
{
  "algorithm_id": "aes_256_gcm",
  "name": "AES-256-GCM",
  "category": "aead",
  "sources": {
    "official_spec": "NIST FIPS 197 + SP 800-38D",
    "ietf_rfc": "RFC 5288",
    "academic_proof": "iacr_2005_061.pdf",
    "reference_implementation": "supercop/crypto_aead/aes256gcmv1",
    "test_vectors": "nist_cavp_gcm_vectors.txt"
  },
  "verification_status": {
    "mathematical_proof": true,
    "reference_implementation": true,
    "test_vectors": true,
    "formal_verification": "pending"
  },
  "implementation_priority": 1
}
```

## Success Criteria

**Complete Algorithm Coverage:**
- ✅ 40+ Tier 1 algorithms with official specs
- ✅ 60+ Tier 2 algorithms with competition documentation  
- ✅ 100+ Tier 3 algorithms with academic sources
- ✅ Reference implementations for all 200+ algorithms
- ✅ Test vectors for validation
- ✅ Mathematical proofs where available

This comprehensive sourcing strategy ensures we have **legitimate, verifiable sources** for every algorithm in our OpenSSL replacement, with mathematical rigor and legal compliance.