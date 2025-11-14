
# Crypto Standards Search Examples

Base URL: http://localhost:8983/solr/crypto_standards/select

## Algorithm-Specific Searches

### AES Standards
?q=algorithms_ss:AES

### Post-Quantum Algorithms  
?q=algorithms_ss:KYBER OR algorithms_ss:DILITHIUM OR algorithms_ss:SPHINCS

### Hash Functions
?q=algorithms_ss:SHA* OR algorithms_ss:BLAKE2* OR algorithms_ss:KECCAK

### Stream Ciphers
?q=algorithms_ss:CHACHA20 OR algorithms_ss:SALSA20

## Document Type Searches

### NIST FIPS Standards
?q=category_s:nist_fips_standard

### IETF RFCs
?q=category_s:ietf_rfc

### Research Papers
?q=category_s:research_paper

## Compliance Searches

### FIPS 140-3 Compliant Algorithms
?q=compliance_standards_ss:FIPS-140-3

### CNSA 2.0 Algorithms
?q=compliance_standards_ss:CNSA-2.0

### Post-Quantum Ready
?q=compliance_standards_ss:Post-Quantum

## Content Searches

### Implementation Guidance
?q=content:"implementation" AND content:"algorithm"

### Security Analysis
?q=content:"security" AND content:"analysis"

### Test Vectors
?q=content:"test vector" OR content:"test case"

## Faceted Searches

### Algorithm Categories with Counts
?q=*:*&facet=true&facet.field=algorithms_ss

### Document Types
?q=*:*&facet=true&facet.field=category_s

### Compliance Standards
?q=*:*&facet=true&facet.field=compliance_standards_ss

## Advanced Queries

### Large Documents (Implementation Guides)
?q=page_count_i:[100 TO *]

### Recent NIST Publications
?q=file_name_s:FIPS* AND category_s:nist_fips_standard

### RFC with Specific Numbers
?q=rfc_number_i:[7000 TO 8500]

### Multi-Algorithm Documents
?q=algorithms_ss:(AES AND SHA AND HMAC)
