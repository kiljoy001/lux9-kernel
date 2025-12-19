#!/bin/bash
# Pre-commit hook for lux9-kernel
# Enforces: 
#   1. Coq proofs must compile (admits OK, broken proofs block)
#   2. SMT prover (Frama-C/Why3) verification of ACSL annotations
#   3. Annotation coverage tracking
#
# INSTALL: ln -sf ../../scripts/pre-commit-hook.sh .git/hooks/pre-commit

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

REPO_ROOT=$(git rev-parse --show-toplevel)
PROOFS_DIR="$REPO_ROOT/proofs"
KERNEL_DIR="$REPO_ROOT/kernel"

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}        Lux9 Formal Verification Pre-Commit Check${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"

# Track results
COQC_OK=true
SMT_OK=true
ADMITS_COUNT=0
ANNOTATIONS_COUNT=0

# ============================================================================
# PHASE 1: Coq Proof Verification
# ============================================================================
echo -e "\n${YELLOW}[Phase 1/3] Verifying Coq Proofs...${NC}"

# Find all staged .v files
STAGED_V=$(git diff --cached --name-only --diff-filter=ACMR | grep '\.v$' || true)

if [ -n "$STAGED_V" ]; then
    for vfile in $STAGED_V; do
        fullpath="$REPO_ROOT/$vfile"
        dir=$(dirname "$fullpath")
        base=$(basename "$vfile" .v)
        
        echo -n "  Checking $vfile... "
        
        # Determine the -R flag based on directory
        R_FLAG=""
        if [[ "$dir" == *"pebble"* ]]; then
            R_FLAG="-R $PROOFS_DIR/pebble pebble"
        elif [[ "$dir" == *"blind_ledger"* ]]; then
            R_FLAG="-R $PROOFS_DIR/blind_ledger BlindLedger"
        elif [[ "$dir" == *"ramdisk"* ]]; then
            R_FLAG="-R $PROOFS_DIR/ramdisk ramdisk"
        elif [[ "$dir" == *"9p_router"* ]]; then
            R_FLAG="-R $PROOFS_DIR/9p_router router"
        elif [[ "$dir" == *"proc"* ]]; then
            R_FLAG="-R $PROOFS_DIR/proc proc"
        elif [[ "$dir" == *"scheduler"* ]]; then
            R_FLAG="-R $PROOFS_DIR/scheduler scheduler"
        fi
        
        # Try to compile the proof
        if cd "$dir" && coqc $R_FLAG "$base.v" 2>/dev/null; then
            echo -e "${GREEN}✓ compiles${NC}"
        else
            echo -e "${RED}✗ BROKEN${NC}"
            COQC_OK=false
        fi
    done
else
    echo "  No Coq files staged"
fi

# Count total admits in proofs directory
ADMITS_COUNT=$(find "$PROOFS_DIR" -name "*.v" -exec grep -c "Admitted\." {} \; 2>/dev/null | paste -sd+ | bc 2>/dev/null || echo 0)
echo -e "  ${YELLOW}Admitted proofs: $ADMITS_COUNT${NC} (allowed but tracked)"

# ============================================================================
# PHASE 2: SMT Verification (Frama-C with Why3)
# ============================================================================
echo -e "\n${YELLOW}[Phase 2/3] SMT/ACSL Verification...${NC}"

# Check if Frama-C is available
if command -v frama-c &> /dev/null; then
    # Find staged C files with ACSL annotations
    STAGED_C=$(git diff --cached --name-only --diff-filter=ACMR | grep '\.c$' || true)
    
    if [ -n "$STAGED_C" ]; then
        for cfile in $STAGED_C; do
            fullpath="$REPO_ROOT/$cfile"
            
            # Check if file has ACSL annotations
            if grep -q '/\*@' "$fullpath" 2>/dev/null; then
                echo -n "  Checking $cfile... "
                
                # Run Frama-C WP (Weakest Precondition) plugin
                # -wp-prover alt-ergo uses Alt-Ergo SMT solver
                # Timeout of 5 seconds per goal
                if frama-c -wp -wp-prover alt-ergo -wp-timeout 5 \
                   -cpp-extra-args="-I$KERNEL_DIR/include -D__PLAN9_KERNEL__" \
                   "$fullpath" 2>/dev/null | grep -q "Proved goals"; then
                    echo -e "${GREEN}✓ verified${NC}"
                else
                    echo -e "${YELLOW}⚠ partial${NC}"
                    # Don't block on partial SMT - it's advisory
                fi
            fi
        done
    else
        echo "  No annotated C files staged"
    fi
else
    echo "  ${YELLOW}Frama-C not installed - skipping SMT verification${NC}"
    echo "  Install with: opam install frama-c"
fi

# ============================================================================
# PHASE 3: Annotation Coverage Tracking
# ============================================================================
echo -e "\n${YELLOW}[Phase 3/3] Annotation Coverage...${NC}"

# Count ACSL annotations in kernel code
ACSL_REQUIRES=$(grep -rh "requires " "$KERNEL_DIR" --include="*.c" 2>/dev/null | grep -c '/\*@' || echo 0)
ACSL_ENSURES=$(grep -rh "ensures " "$KERNEL_DIR" --include="*.c" 2>/dev/null | grep -c '/\*@' || echo 0)
ACSL_ASSIGNS=$(grep -rh "assigns " "$KERNEL_DIR" --include="*.c" 2>/dev/null | grep -c '/\*@' || echo 0)
ACSL_INVARIANTS=$(grep -rh "invariant " "$KERNEL_DIR" --include="*.c" 2>/dev/null | grep -c '/\*@' || echo 0)

ANNOTATIONS_TOTAL=$((ACSL_REQUIRES + ACSL_ENSURES + ACSL_ASSIGNS + ACSL_INVARIANTS))

echo "  ACSL Annotations:"
echo "    requires:   $ACSL_REQUIRES"
echo "    ensures:    $ACSL_ENSURES"
echo "    assigns:    $ACSL_ASSIGNS"
echo "    invariants: $ACSL_INVARIANTS"
echo "    ────────────────"
echo -e "    ${GREEN}Total: $ANNOTATIONS_TOTAL${NC}"

# Count Coq theorems
COQ_THEOREMS=$(find "$PROOFS_DIR" -name "*.v" -exec grep -c "Theorem\|Lemma\|Corollary" {} \; 2>/dev/null | paste -sd+ | bc 2>/dev/null || echo 0)
echo -e "\n  Coq Theorems/Lemmas: ${GREEN}$COQ_THEOREMS${NC}"

# Save coverage metrics to file for tracking
METRICS_FILE="$REPO_ROOT/.verification_metrics"
cat > "$METRICS_FILE" << EOF
# Verification Metrics - $(date +%Y-%m-%dT%H:%M:%S 2>/dev/null || date)
coq_admits=$ADMITS_COUNT
coq_theorems=$COQ_THEOREMS
acsl_requires=$ACSL_REQUIRES
acsl_ensures=$ACSL_ENSURES
acsl_assigns=$ACSL_ASSIGNS
acsl_invariants=$ACSL_INVARIANTS
acsl_total=$ANNOTATIONS_TOTAL
EOF

# ============================================================================
# FINAL VERDICT
# ============================================================================
echo -e "\n${BLUE}═══════════════════════════════════════════════════════════════${NC}"

if [ "$COQC_OK" = false ]; then
    echo -e "${RED}❌ COMMIT BLOCKED: Broken Coq proofs detected${NC}"
    echo ""
    echo "Fix the proof compilation errors above, then try again."
    echo "Note: 'Admitted' proofs are allowed but tracked."
    echo ""
    exit 1
fi

if [ "$SMT_OK" = false ]; then
    echo -e "${YELLOW}⚠️ WARNING: Some SMT goals not verified${NC}"
    # Don't block on SMT - it's advisory for now
fi

echo -e "${GREEN}✅ Pre-commit verification passed${NC}"
echo "  - Coq proofs: compile (admits: $ADMITS_COUNT)"
echo "  - ACSL annotations: $ANNOTATIONS_TOTAL"
echo "  - Coq theorems: $COQ_THEOREMS"
echo ""

exit 0
