#!/usr/bin/env bash
# Preprocess Plan 9 code for Frama-C, dropping Plan 9-specific pragmas.
set -euo pipefail
inp=$1
out=$2
gcc -D__FRAMAC__ -E -C -I kernel/include -I kernel/9front-pc64 -I kernel/9front-port -I . "$inp" \
  | sed -e 's/µs/us/g' -e '/µs/d' -e '/#pragma varargck/d' > "$out"
