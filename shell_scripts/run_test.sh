#!/bin/bash
set -euo pipefail

REPO=/home/scott/Repo/lux9-kernel
LOG=${REPO}/qemu.log

echo "[run_test] building kernel"
cd "${REPO}"
make -s >/dev/null

echo "[run_test] launching qemu (pebble selftest)"
rm -f "${LOG}"
timeout 45s make run >/dev/null

if ! grep -q "PEBBLE: selftest PASS" "${LOG}"; then
    echo "[run_test] pebble selftest FAILED"
    tail -n 200 "${LOG}" || true
    exit 1
fi

if ! grep -q "PEBBLE: /dev/sip/issue test PASS" "${LOG}"; then
    echo "[run_test] pebble sip issue test FAILED"
    tail -n 200 "${LOG}" || true
    exit 1
fi

echo "[run_test] pebble tests passed"
