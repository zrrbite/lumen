#!/bin/bash
# Integration test for STM32F429-DISCO.
# Runs scripted application interactions, reads back state + framebuffer.
#
# Usage:
#   ./tests/verify_integration.sh              # Run and compare
#   ./tests/verify_integration.sh --update     # Update references

set -e
cd "$(dirname "$0")/.."

source "$HOME/.cargo/env" 2>/dev/null || true

CHIP="STM32F429ZITx"
BUILD_DIR="build-arm"
BINARY="$BUILD_DIR/integration_f429"
REF_DIR="tests/integration_refs"
OUT_DIR="tests/integration_refs/actual"

mkdir -p "$REF_DIR" "$OUT_DIR"

echo "=== Building integration_f429 ==="
cmake --build "$BUILD_DIR" --target integration_f429 2>&1 | tail -3

echo ""
echo "=== Flashing ==="
killall -q probe-rs 2>/dev/null || true
sleep 1

# Flash, reset, then read. Use --connect-under-reset to ensure clean state.
probe-rs download --chip "$CHIP" --connect-under-reset "$BINARY" 2>&1 | tail -2
sleep 1

# Reset to start execution (release from debug halt)
probe-rs reset --chip "$CHIP" --connect-under-reset 2>&1 || true

echo "Waiting for firmware to complete..."
sleep 3

echo ""
echo "=== Reading control block ==="

CTRL_ADDR=$(arm-none-eabi-nm "$BINARY" | grep "ctrl" | grep -v "crtc\|__" | head -1 | awk '{print $1}')
if [ -z "$CTRL_ADDR" ]; then
    echo "ERROR: Cannot find ctrl address"
    true
    exit 1
fi

# Read control: magic, step_count, fb_addr, fb_w, fb_h, results_addr
CTRL_RAW=$(probe-rs read b32 "0x$CTRL_ADDR" 6 --chip "$CHIP" 2>&1)
WORDS=($CTRL_RAW)
MAGIC="${WORDS[0]}"
STEP_COUNT=$((16#${WORDS[1]}))
FB_ADDR="${WORDS[2]}"
FB_W=$((16#${WORDS[3]}))
FB_H=$((16#${WORDS[4]}))
RESULTS_ADDR="${WORDS[5]}"

echo "magic=$MAGIC steps=$STEP_COUNT fb=0x$FB_ADDR ${FB_W}x${FB_H} results=0x$RESULTS_ADDR"

if [ "$MAGIC" != "BEEFCAFE" ]; then
    echo "ERROR: Tests not complete (magic=$MAGIC)"
    true
    exit 1
fi

# Read all results: each StepResult = crc(4) + vars[8](32) + var_count(1) + padding(3) = 40 bytes = 10 words
RESULT_WORDS=$((STEP_COUNT * 10))
RESULTS_RAW=$(probe-rs read b32 "0x$RESULTS_ADDR" "$RESULT_WORDS" --chip "$CHIP" 2>&1)
R=($RESULTS_RAW)

STEP_NAMES=("initial" "press_btn" "release_btn" "after_click" "press_btn_2" "release_btn_2" "after_2_clicks" "press_btn_3" "release_btn_3")

# Expected state after each step:
# [touch_count, bar_value]
EXPECTED_VARS=(
    "0 0"       # initial
    "0 0"       # press (no click yet)
    "1 25"      # release (click fired)
    "1 25"      # after_click (same)
    "1 25"      # press 2
    "2 50"      # release 2
    "2 50"      # after 2 clicks
    "2 50"      # press 3
    "3 75"      # release 3
)

PASS=0
FAIL=0

echo ""
echo "=== Verifying steps ==="

for i in $(seq 0 $((STEP_COUNT - 1))); do
    NAME="${STEP_NAMES[$i]:-step_$i}"
    BASE=$((i * 10))
    CRC="${R[$BASE]}"
    VAR0=$((16#${R[$((BASE + 1))]}))
    VAR1=$((16#${R[$((BASE + 2))]}))

    EXPECTED=(${EXPECTED_VARS[$i]})
    EXP_V0="${EXPECTED[0]:-?}"
    EXP_V1="${EXPECTED[1]:-?}"

    # Check variables
    VAR_OK=true
    if [ "$EXP_V0" != "?" ] && [ "$VAR0" != "$EXP_V0" ]; then
        VAR_OK=false
    fi
    if [ "$EXP_V1" != "?" ] && [ "$VAR1" != "$EXP_V1" ]; then
        VAR_OK=false
    fi

    if $VAR_OK; then
        printf "  PASS  %-20s count=%d bar=%d crc=%s\n" "$NAME" "$VAR0" "$VAR1" "$CRC"
        PASS=$((PASS + 1))
    else
        printf "  FAIL  %-20s count=%d(exp %s) bar=%d(exp %s) crc=%s\n" "$NAME" "$VAR0" "$EXP_V0" "$VAR1" "$EXP_V1" "$CRC"
        FAIL=$((FAIL + 1))
    fi

    # Save CRC for reference comparison
    if [ "$1" = "--update" ]; then
        echo "$CRC" > "$REF_DIR/${NAME}.crc"
    elif [ -f "$REF_DIR/${NAME}.crc" ]; then
        REF_CRC=$(cat "$REF_DIR/${NAME}.crc")
        if [ "$CRC" != "$REF_CRC" ]; then
            printf "         ^ CRC mismatch: got %s, expected %s\n" "$CRC" "$REF_CRC"
        fi
    fi
done

# Also dump the final framebuffer as a screenshot
FB_BYTES=$((FB_W * FB_H * 2))
FB_WORDS=$((FB_BYTES / 4))
probe-rs read b32 "0x$FB_ADDR" "$FB_WORDS" --chip "$CHIP" --output "$OUT_DIR/final_fb.bin" 2>/dev/null || true

# Convert to PPM
python3 -c "
import struct
w, h = $FB_W, $FB_H
with open('$OUT_DIR/final_fb.bin', 'rb') as f:
    data = f.read()
with open('$OUT_DIR/final_fb.ppm', 'wb') as f:
    f.write(f'P6\n{w} {h}\n255\n'.encode())
    for i in range(0, min(len(data), w*h*2), 2):
        px = struct.unpack('<H', data[i:i+2])[0]
        r = ((px >> 11) & 0x1F) << 3
        g = ((px >> 5) & 0x3F) << 2
        b = (px & 0x1F) << 3
        f.write(bytes([r, g, b]))
print('Saved final framebuffer to $OUT_DIR/final_fb.ppm')
" 2>/dev/null || true

true

echo ""
echo "=== Results ==="
echo "$PASS passed, $FAIL failed (of $STEP_COUNT steps)"
[ $FAIL -eq 0 ]
