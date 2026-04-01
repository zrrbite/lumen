#!/bin/bash
# Read a variable from a running STM32 by symbol name.
# Usage: ./tests/probe_read.sh <elf_binary> <symbol_name> [word_count]
#
# Examples:
#   ./tests/probe_read.sh build-arm/stm32f429_hello touch_count
#   ./tests/probe_read.sh build-arm/integration_f429 bar_value
#   ./tests/probe_read.sh build-arm/integration_f429 test_fb 100  # read 100 words

set -e
source "$HOME/.cargo/env" 2>/dev/null || true

BINARY="$1"
SYMBOL="$2"
COUNT="${3:-1}"
CHIP="${CHIP:-STM32F429ZITx}"

if [ -z "$BINARY" ] || [ -z "$SYMBOL" ]; then
    echo "Usage: $0 <elf_binary> <symbol_name> [word_count]"
    exit 1
fi

# Find symbol address (handle C++ mangling)
ADDR=$(arm-none-eabi-nm "$BINARY" | grep "$SYMBOL" | grep -v "__cxa\|__gnu" | head -1 | awk '{print $1}')

if [ -z "$ADDR" ]; then
    echo "Symbol '$SYMBOL' not found in $BINARY"
    echo "Available symbols:"
    arm-none-eabi-nm "$BINARY" | grep -v "^$\|__" | tail -20
    exit 1
fi

echo "Reading $SYMBOL at 0x$ADDR ($COUNT words):"
probe-rs read b32 "0x$ADDR" "$COUNT" --chip "$CHIP"
