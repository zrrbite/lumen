#!/bin/bash
# Lumen full test suite.
# Run: ./tests/run_all.sh
# Exit 0 = all pass

set -e
cd "$(dirname "$0")/.."

PASS=0
FAIL=0
SKIP=0

run() {
    local name="$1"; shift
    printf "  %-38s" "$name"
    if "$@" > /tmp/lumen_test.log 2>&1; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL"
        cat /tmp/lumen_test.log | head -20
        FAIL=$((FAIL + 1))
    fi
}

skip() {
    printf "  %-38s%s\n" "$1" "SKIP ($2)"
    SKIP=$((SKIP + 1))
}

echo "========================================="
echo " Lumen Test Suite"
echo "========================================="

# --- Builds ---
echo ""
echo "--- Builds ---"
run "Desktop (visual_test)"     cmake --build build --target visual_test
run "Desktop (test_script)"     cmake --build build --target test_script
run "Desktop (test_gesture)"    cmake --build build --target test_gesture

[ -d build-arm ] && \
    run "ARM (F429)"            cmake --build build-arm --target stm32f429_hello || \
    skip "ARM (F429)" "not configured"

[ -d build-f769 ] && \
    run "ARM (F769)"            cmake --build build-f769 --target stm32f769_hello || \
    skip "ARM (F769)" "not configured"

[ -d build-rpi ] && \
    run "Linux (RPi)"           cmake --build build-rpi --target rpi_hello || \
    skip "Linux (RPi)" "not configured"

# --- Unit Tests ---
echo ""
echo "--- Unit Tests ---"
run "Script engine"             ./build/test_script
run "Gesture detector"          ./build/test_gesture

# --- Visual Tests ---
echo ""
echo "--- Visual Regression ---"
run "Visual tests (15 cases)"   ./build/visual_test

# --- Rust Tools ---
echo ""
echo "--- Tooling ---"
source "$HOME/.cargo/env" 2>/dev/null || true
if command -v cargo &>/dev/null; then
    run "lumen-tools build"     bash -c "cd tools && cargo build 2>&1"
else
    skip "lumen-tools build" "cargo not found"
fi

# --- Summary ---
echo ""
echo "========================================="
TOTAL=$((PASS + FAIL + SKIP))
echo " $PASS passed, $FAIL failed, $SKIP skipped ($TOTAL total)"
echo "========================================="

rm -f /tmp/lumen_test.log
[ $FAIL -eq 0 ]
