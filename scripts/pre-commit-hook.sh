#!/bin/bash
# Pre-commit hook for lux9-kernel
# Delegates to the Python verification manager for SQLite-backed integrity checks.

REPO_ROOT=$(git rev-parse --show-toplevel)
PYTHON_SCRIPT="$REPO_ROOT/scripts/verification_manager.py"

if [ -f "$PYTHON_SCRIPT" ]; then
    python3 "$PYTHON_SCRIPT"
    EXIT_CODE=$?
    exit $EXIT_CODE
else
    echo "Error: Verification manager not found at $PYTHON_SCRIPT"
    exit 1
fi
