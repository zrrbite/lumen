#!/bin/bash
# Integration test for STM32F429-DISCO.
# Flashes the test firmware, then reads back variables by symbol name.
#
# Usage:
#   ./tests/verify_integration.sh

set -e
cd "$(dirname "$0")/.."

source "$HOME/.cargo/env" 2>/dev/null || true

CHIP="STM32F429ZITx"
BUILD_DIR="build-arm"
BINARY="$BUILD_DIR/integration_f429"

echo "=== Building ==="
cmake --build "$BUILD_DIR" --target integration_f429 2>&1 | tail -3

# Helper: resolve symbol address from ELF
sym_addr() {
    arm-none-eabi-nm "$BINARY" | grep "$1" | grep -v "__cxa\|__gnu" | head -1 | awk '{print $1}'
}

# Helper: read word(s) at symbol
read_sym() {
    local addr=$(sym_addr "$1")
    local count="${2:-1}"
    if [ -z "$addr" ]; then
        echo "ERR"
        return
    fi
    probe-rs read b32 "0x$addr" "$count" --chip "$CHIP" 2>/dev/null
}

# Helper: read single u32, return as decimal
read_u32() {
    local raw=$(read_sym "$1" 1)
    if [ "$raw" = "ERR" ]; then
        echo "-1"
        return
    fi
    echo $((16#$raw))
}

echo ""
echo "=== Flashing ==="
killall -q probe-rs 2>/dev/null || true
sleep 1

# Flash and run — the firmware completes all 9 steps then enters WFI
# After WFI, we can read memory freely
probe-rs run --chip "$CHIP" "$BINARY" 2>/dev/null &
PROBE_PID=$!
sleep 5

# Firmware should be done by now (9 steps, no real delays)
# Kill probe-rs to free the USB device
kill $PROBE_PID 2>/dev/null || true
wait $PROBE_PID 2>/dev/null || true
sleep 1

echo ""
echo "=== Checking results ==="

# Read the control block magic
MAGIC=$(read_sym "ctrl" 1)
echo "ctrl.magic = $MAGIC"

if [ "$MAGIC" != "BEEFCAFE" ]; then
    echo "FAIL: Firmware did not complete (magic=$MAGIC)"
    echo ""
    echo "Trying longer wait + re-read..."
    sleep 3
    MAGIC=$(read_sym "ctrl" 1)
    echo "ctrl.magic = $MAGIC (retry)"
    if [ "$MAGIC" != "BEEFCAFE" ]; then
        echo "FAIL: Still not ready. The MCU may have halted when probe-rs was killed."
        exit 1
    fi
fi

# Read final state
TOUCH=$(read_u32 "touch_count")
BAR=$(read_u32 "bar_value")

echo "touch_count = $TOUCH"
echo "bar_value   = $BAR"

# After 3 button clicks: touch_count=3, bar_value=75
PASS=0
FAIL=0

check() {
    local name="$1" got="$2" expected="$3"
    if [ "$got" = "$expected" ]; then
        printf "  PASS  %-20s %s == %s\n" "$name" "$got" "$expected"
        PASS=$((PASS + 1))
    else
        printf "  FAIL  %-20s %s != %s\n" "$name" "$got" "$expected"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo "=== Verifying ==="
check "touch_count" "$TOUCH" "3"
check "bar_value" "$BAR" "75"

# Read step results array to verify intermediate states
RESULTS_ADDR=$(sym_addr "results")
if [ -n "$RESULTS_ADDR" ]; then
    # Each StepResult: crc(4) + vars[8](32) + var_count(1) + padding(3) = 40 bytes = 10 words
    # 9 steps = 90 words
    RAW=$(read_sym "results" 90)
    R=($RAW)

    STEP_NAMES=("initial" "press1" "release1" "after1" "press2" "release2" "after2" "press3" "release3")
    # Expected [touch_count, bar_value] at each step
    EXPECTED_V0=(0 0 1 1 1 2 2 2 3)
    EXPECTED_V1=(0 0 25 25 25 50 50 50 75)

    for i in $(seq 0 8); do
        BASE=$((i * 10))
        V0=$((16#${R[$((BASE + 1))]}))
        V1=$((16#${R[$((BASE + 2))]}))
        check "${STEP_NAMES[$i]}_count" "$V0" "${EXPECTED_V0[$i]}"
        check "${STEP_NAMES[$i]}_bar" "$V1" "${EXPECTED_V1[$i]}"
    done
fi

echo ""
echo "=== Results ==="
echo "$PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
