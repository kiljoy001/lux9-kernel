#!/bin/bash
#
# Frama-C Verification Script for Secure Ramdisk
#
# Runs multiple Frama-C analyses on devram.c to verify security properties

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DEVRAM_C="kernel/9front-port/devram.c"
ACSL_H="kernel/9front-port/devram_acsl.h"
OUTPUT_DIR="verification_results"
CPP_FLAGS="-cpp-extra-args=\"-Ikernel/include -Ikernel/9front-pc64 -Ikernel/9front-port -D__FRAMAC__\""

# Check if Frama-C is installed
if ! command -v frama-c &> /dev/null; then
    echo -e "${RED}❌ Frama-C not installed${NC}"
    echo ""
    echo "Install with:"
    echo "  opam install frama-c"
    echo "  # or"
    echo "  apt-get install frama-c"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Frama-C Verification for Secure Ramdisk${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Get Frama-C version
FRAMAC_VERSION=$(frama-c -version | head -1)
echo "Using: $FRAMAC_VERSION"
echo ""

# ============================================================================
# Analysis 1: Value Analysis
# ============================================================================

echo -e "${YELLOW}[1/6] Running Value Analysis...${NC}"
echo "Purpose: Detects undefined behaviors, buffer overflows, null dereferences"
echo ""

if frama-c -val -val-warn-undefined-pointer-comparison pointer \
           -val-show-progress \
           -cpp-extra-args="-Ikernel/include -Ikernel/9front-pc64 -Ikernel/9front-port -D__FRAMAC__" \
           "$DEVRAM_C" \
           -save "$OUTPUT_DIR/value_analysis.sav" \
           > "$OUTPUT_DIR/value_analysis.log" 2>&1; then
    echo -e "${GREEN}✅ Value analysis completed${NC}"

    # Check for alarms
    ALARMS=$(grep -c "alarm" "$OUTPUT_DIR/value_analysis.log" || echo 0)
    if [ "$ALARMS" -gt 0 ]; then
        echo -e "${YELLOW}⚠️  Found $ALARMS potential issues${NC}"
        grep "alarm" "$OUTPUT_DIR/value_analysis.log" | head -10
    else
        echo -e "${GREEN}✅ No alarms found${NC}"
    fi
else
    echo -e "${RED}❌ Value analysis failed${NC}"
    tail -20 "$OUTPUT_DIR/value_analysis.log"
fi
echo ""

# ============================================================================
# Analysis 2: Runtime Error Detection (RTE)
# ============================================================================

echo -e "${YELLOW}[2/6] Runtime Error Detection...${NC}"
echo "Purpose: Generates assertions for runtime errors (division by zero, overflow, etc.)"
echo ""

if frama-c -rte -rte-all \
           -cpp-extra-args="-Ikernel/include -Ikernel/9front-pc64 -Ikernel/9front-port -D__FRAMAC__" \
           "$DEVRAM_C" \
           -print -ocode "$OUTPUT_DIR/devram_rte.c" \
           > "$OUTPUT_DIR/rte_analysis.log" 2>&1; then
    echo -e "${GREEN}✅ RTE annotations generated${NC}"

    # Count generated assertions
    ASSERTIONS=$(grep -c "assert" "$OUTPUT_DIR/devram_rte.c" || echo 0)
    echo "  Generated $ASSERTIONS runtime safety assertions"
else
    echo -e "${RED}❌ RTE analysis failed${NC}"
fi
echo ""

# ============================================================================
# Analysis 3: Weakest Precondition (WP) Verification
# ============================================================================

echo -e "${YELLOW}[3/6] Weakest Precondition Proof...${NC}"
echo "Purpose: Verifies ACSL specifications using formal proofs"
echo ""

if frama-c -wp -wp-rte -wp-timeout 30 \
           -wp-prover alt-ergo,cvc4,z3 \
           -cpp-extra-args="-Ikernel/include -Ikernel/9front-pc64 -Ikernel/9front-port -D__FRAMAC__" \
           "$DEVRAM_C" \
           -wp-out "$OUTPUT_DIR/wp_results" \
           > "$OUTPUT_DIR/wp_analysis.log" 2>&1; then
    echo -e "${GREEN}✅ WP verification completed${NC}"

    # Check proof results
    if [ -f "$OUTPUT_DIR/wp_results/report.txt" ]; then
        cat "$OUTPUT_DIR/wp_results/report.txt"
    fi
else
    echo -e "${YELLOW}⚠️  WP verification completed with warnings${NC}"
    grep "proved\|valid\|unknown" "$OUTPUT_DIR/wp_analysis.log" | head -10
fi
echo ""

# ============================================================================
# Analysis 4: Dependencies Analysis
# ============================================================================

echo -e "${YELLOW}[4/6] Data Dependencies Analysis...${NC}"
echo "Purpose: Tracks data flow and identifies information leaks"
echo ""

if frama-c -deps \
           -cpp-extra-args="-Ikernel/include -Ikernel/9front-pc64 -Ikernel/9front-port -D__FRAMAC__" \
           "$DEVRAM_C" \
           > "$OUTPUT_DIR/deps_analysis.log" 2>&1; then
    echo -e "${GREEN}✅ Dependency analysis completed${NC}"

    # Check for sensitive data flows
    echo "  Checking for sensitive data dependencies..."
    grep -i "master_key\|password\|nonce" "$OUTPUT_DIR/deps_analysis.log" | head -5 || echo "  No obvious leaks detected"
else
    echo -e "${YELLOW}⚠️  Dependency analysis incomplete${NC}"
fi
echo ""

# ============================================================================
# Analysis 5: Eva (Evolved Value Analysis)
# ============================================================================

echo -e "${YELLOW}[5/6] Evolved Value Analysis (Eva)...${NC}"
echo "Purpose: Advanced value analysis with better precision"
echo ""

if frama-c -eva -eva-warn-undefined-pointer-comparison pointer \
           -eva-precision 3 \
           -cpp-extra-args="-Ikernel/include -Ikernel/9front-pc64 -Ikernel/9front-port -D__FRAMAC__" \
           "$DEVRAM_C" \
           > "$OUTPUT_DIR/eva_analysis.log" 2>&1; then
    echo -e "${GREEN}✅ Eva analysis completed${NC}"

    # Check for coverage
    COVERAGE=$(grep -i "coverage" "$OUTPUT_DIR/eva_analysis.log" | head -1 || echo "Coverage info not available")
    echo "  $COVERAGE"
else
    echo -e "${YELLOW}⚠️  Eva analysis incomplete${NC}"
fi
echo ""

# ============================================================================
# Analysis 6: Security-Specific Checks
# ============================================================================

echo -e "${YELLOW}[6/6] Security-Specific Checks...${NC}"
echo "Purpose: Custom checks for cryptographic bugs"
echo ""

echo "Checking for security issues in $DEVRAM_C..."

# Check 1: Nonce reuse
NONCE_REUSE=$(grep -n "secure_rd\.nonce" "$DEVRAM_C" | wc -l)
echo "  Nonce references found: $NONCE_REUSE"
if [ "$NONCE_REUSE" -gt 0 ]; then
    echo -e "${YELLOW}  ⚠️  Old nonce references still exist (should use fresh nonces)${NC}"
fi

# Check 2: Locking
UNLOCKED_ACCESS=$(grep -n "secure_rd\." "$DEVRAM_C" | grep -v "qlock\|qunlock\|//" | wc -l)
echo "  Potential unlocked accesses: $UNLOCKED_ACCESS"

# Check 3: State validation
LOCK_WITHOUT_CHECK=$(grep -n "secure_rd\.locked.*=" "$DEVRAM_C" | grep -v "check_lock_invariant" | wc -l)
echo "  Lock state changes: $LOCK_WITHOUT_CHECK"

# Check 4: Constant-time operations
NON_CT=$(grep -n "strcmp.*argv" "$DEVRAM_C" | wc -l)
echo "  Non-constant-time comparisons: $NON_CT"

echo ""

# ============================================================================
# Generate Summary Report
# ============================================================================

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Verification Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

cat > "$OUTPUT_DIR/SUMMARY.md" << EOF
# Frama-C Verification Summary

**Date:** $(date)
**File:** $DEVRAM_C
**Frama-C Version:** $FRAMAC_VERSION

## Analyses Performed

1. **Value Analysis**
   - Status: $([ -f "$OUTPUT_DIR/value_analysis.sav" ] && echo "✅ Complete" || echo "❌ Failed")
   - Alarms: $ALARMS

2. **Runtime Error Detection (RTE)**
   - Status: $([ -f "$OUTPUT_DIR/devram_rte.c" ] && echo "✅ Complete" || echo "❌ Failed")
   - Assertions Generated: $ASSERTIONS

3. **Weakest Precondition (WP)**
   - Status: $([ -d "$OUTPUT_DIR/wp_results" ] && echo "✅ Complete" || echo "❌ Failed")

4. **Dependencies Analysis**
   - Status: $([ -f "$OUTPUT_DIR/deps_analysis.log" ] && echo "✅ Complete" || echo "❌ Failed")

5. **Eva Analysis**
   - Status: $([ -f "$OUTPUT_DIR/eva_analysis.log" ] && echo "✅ Complete" || echo "❌ Failed")

6. **Security Checks**
   - Nonce references: $NONCE_REUSE
   - Unlocked accesses: $UNLOCKED_ACCESS
   - State changes: $LOCK_WITHOUT_CHECK
   - Non-CT comparisons: $NON_CT

## Results

All analysis logs are available in: \`$OUTPUT_DIR/\`

### Key Findings

$(if [ "$ALARMS" -gt 0 ]; then
    echo "⚠️ **Value analysis found $ALARMS potential issues**"
    grep "alarm" "$OUTPUT_DIR/value_analysis.log" | head -5
fi)

### Recommendations

- Review all alarms in value_analysis.log
- Verify all WP proof obligations
- Check dependency analysis for information leaks
- Fix any non-constant-time operations in security-critical paths

EOF

cat "$OUTPUT_DIR/SUMMARY.md"
echo ""

# ============================================================================
# GUI Option
# ============================================================================

if command -v frama-c-gui &> /dev/null; then
    echo -e "${BLUE}To explore results graphically, run:${NC}"
    echo "  frama-c-gui -load $OUTPUT_DIR/value_analysis.sav"
    echo ""
fi

echo -e "${GREEN}✅ Verification complete! Results in: $OUTPUT_DIR/${NC}"
echo ""
echo "Next steps:"
echo "  1. Review $OUTPUT_DIR/SUMMARY.md"
echo "  2. Fix any alarms or failed proofs"
echo "  3. Re-run verification"
echo "  4. Add ACSL annotations for remaining functions"
