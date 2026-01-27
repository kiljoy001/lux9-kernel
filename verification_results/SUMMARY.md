# Frama-C Verification Summary

**Date:** Tue Dec 23 01:26:23 AM PST 2025
**File:** kernel/9front-port/devram.c
**Frama-C Version:** 31.0 (Gallium)

## Analyses Performed

1. **Value Analysis**
   - Status: ❌ Failed
   - Alarms: 

2. **Runtime Error Detection (RTE)**
   - Status: ❌ Failed
   - Assertions Generated: 

3. **Weakest Precondition (WP)**
   - Status: ❌ Failed

4. **Dependencies Analysis**
   - Status: ✅ Complete

5. **Eva Analysis**
   - Status: ✅ Complete

6. **Security Checks**
   - Nonce references: 0
   - Unlocked accesses: 21
   - State changes: 2
   - Non-CT comparisons: 4

## Results

All analysis logs are available in: `verification_results/`

### Key Findings



### Recommendations

- Review all alarms in value_analysis.log
- Verify all WP proof obligations
- Check dependency analysis for information leaks
- Fix any non-constant-time operations in security-critical paths

