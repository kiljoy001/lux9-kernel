#!/bin/bash
# Formal Verification Script for Lux9 Reduced Scheduler
set -euo pipefail

INPUT="kernel/9front-port/sched_verified.c"
PREPROCESSED="/tmp/sched_verified_pre.i"
SCRIPTS_DIR="scripts"

echo "Step 1: Preprocessing $INPUT..."
$SCRIPTS_DIR/framac_plan9_v2.sh "$INPUT" "$PREPROCESSED"

echo "Step 2: Running Frama-C WP..."
frama-c -wp \
    -wp-model "Typed+ref" \
    -wp-prover "alt-ergo,z3" \
    -wp-timeout 30 \
    -wp-log "a:wp_log.txt" \
    -machdep gcc_x86_64 \
    "$PREPROCESSED"

echo "Verification complete. Check wp_log.txt for details."
