#!/bin/bash
# Hardware-in-the-loop visual test for STM32F429-DISCO.
#
# Flashes test firmware, reads framebuffer after each test scenario,
# saves as PPM, compares against reference screenshots.
#
# Usage:
#   ./tests/verify_hw.sh              # Run tests, compare against refs
#   ./tests/verify_hw.sh --update     # Update reference screenshots
#
# Prerequisites: probe-rs, arm build configured

set -e
cd "$(dirname "$0")/.."

source "$HOME/.cargo/env" 2>/dev/null || true

CHIP="STM32F429ZITx"
BUILD_DIR="build-arm"
BINARY="$BUILD_DIR/hw_test_f429"
REF_DIR="tests/hw_screenshots"
OUT_DIR="tests/hw_screenshots/actual"

TESTS=("blank" "label" "button_normal" "button_pressed" "progress_50" "multi_widget")

mkdir -p "$REF_DIR" "$OUT_DIR"

echo "=== Building hw_test_f429 ==="
cmake --build "$BUILD_DIR" --target hw_test_f429 2>&1 | tail -3

echo ""
echo "=== Flashing ==="
probe-rs download --chip "$CHIP" "$BINARY" 2>&1 | tail -2
probe-rs reset --chip "$CHIP" 2>&1 || true
echo "Waiting for firmware to complete..."
sleep 3

echo ""
echo "=== Reading test results ==="

# Find the ctrl struct — we need its address
# The ctrl struct is in BSS. Read the symbol table to find it.
# For now, use a known approach: read fb_addr from the ctrl struct.

# Read first 8 words from the ctrl struct location
# ctrl is in BSS which starts after .data in SRAM
# We'll try to read from where the linker placed it
# Actually, probe-rs read needs the address. Let's use nm to find it:
CTRL_ADDR=$(arm-none-eabi-nm "$BINARY" | grep "ctrl" | grep -v "crtc\|__" | head -1 | awk '{print $1}')

if [ -z "$CTRL_ADDR" ]; then
    echo "ERROR: Could not find ctrl struct address in binary"
    
    exit 1
fi

echo "ctrl struct at 0x$CTRL_ADDR"

# Read control struct (8 words)
CTRL=$(probe-rs read b32 "0x$CTRL_ADDR" 8 --chip "$CHIP" 2>&1)
WORDS=($CTRL)
MAGIC="${WORDS[0]}"
CURRENT="${WORDS[1]}"
TOTAL="${WORDS[2]}"
FB_ADDR="${WORDS[3]}"
FB_W="${WORDS[4]}"
FB_H="${WORDS[5]}"
FB_BPP="${WORDS[6]}"

echo "magic=$MAGIC total=$TOTAL fb=0x$FB_ADDR ${FB_W}x${FB_H} bpp=$FB_BPP"

if [ "$MAGIC" != "BEEFCAFE" ]; then
    echo "ERROR: Firmware not ready (magic=$MAGIC)"
    
    exit 1
fi

W=$((16#$FB_W))
H=$((16#$FB_H))
FB_BYTES=$((W * H * 2))
FB_WORDS=$((FB_BYTES / 4))

echo "Framebuffer: ${W}x${H}, $FB_BYTES bytes ($FB_WORDS words)"

PASS=0
FAIL=0
NEW=0

for i in "${!TESTS[@]}"; do
    NAME="${TESTS[$i]}"
    echo ""
    echo "--- Test $i: $NAME ---"

    # Read framebuffer
    probe-rs read b32 "0x$FB_ADDR" "$FB_WORDS" --chip "$CHIP" --output "$OUT_DIR/${NAME}.bin" 2>/dev/null

    # Convert RGB565 binary to PPM
    python3 -c "
import struct, sys
w, h = $W, $H
with open('$OUT_DIR/${NAME}.bin', 'rb') as f:
    data = f.read()
with open('$OUT_DIR/${NAME}.ppm', 'wb') as f:
    f.write(f'P6\n{w} {h}\n255\n'.encode())
    for i in range(0, len(data), 2):
        if i+1 < len(data):
            px = struct.unpack('<H', data[i:i+2])[0]
            r = ((px >> 11) & 0x1F) << 3
            g = ((px >> 5) & 0x3F) << 2
            b = (px & 0x1F) << 3
            f.write(bytes([r, g, b]))
" 2>/dev/null

    REF="$REF_DIR/${NAME}.ppm"
    ACTUAL="$OUT_DIR/${NAME}.ppm"

    if [ "$1" = "--update" ]; then
        cp "$ACTUAL" "$REF"
        echo "  UPDATED $NAME"
    elif [ -f "$REF" ]; then
        if cmp -s "$REF" "$ACTUAL"; then
            echo "  PASS $NAME"
            PASS=$((PASS + 1))
            rm -f "$ACTUAL" "$OUT_DIR/${NAME}.bin"
        else
            echo "  FAIL $NAME (differs from reference)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo "  NEW $NAME (no reference — run --update)"
        NEW=$((NEW + 1))
    fi

    # Advance to next test by writing next_cmd
    NEXT=$((i + 1))
    probe-rs write b32 "0x$CTRL_ADDR" --value $((NEXT + 0x1C)) --chip "$CHIP" 2>/dev/null || true
    # Actually we need to write to ctrl.next_cmd which is at ctrl_addr + 7*4 = +28 = +0x1C
    # probe-rs write writes to the address, not the struct offset. Let me compute:
    CMD_ADDR=$(printf "%X" $((16#$CTRL_ADDR + 28)))
    probe-rs write b32 "0x$CMD_ADDR" --value $NEXT --chip "$CHIP" 2>/dev/null || true
    sleep 1
done

# Clean up



echo ""
echo "=== Results ==="
TOTAL_TESTS=${#TESTS[@]}
if [ "$1" = "--update" ]; then
    echo "Updated $TOTAL_TESTS reference screenshots."
else
    echo "$PASS passed, $FAIL failed, $NEW new (of $TOTAL_TESTS)"
    [ $FAIL -eq 0 ] && [ $NEW -eq 0 ] && exit 0 || exit 1
fi
