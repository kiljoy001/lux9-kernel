#!/usr/bin/env bash
# Legacy wrapper for the canonical initrd build.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Legacy wrapper: delegating initrd build to $ROOT_DIR/GNUmakefile"
exec make -C "$ROOT_DIR" -f GNUmakefile userspace-all
