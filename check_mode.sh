#!/bin/bash
# Script to check which quiescence mode the engine was built with

BINARY="${1:-./bin/seajay}"

if [ ! -f "$BINARY" ]; then
    echo "Error: Binary not found at $BINARY"
    echo "Usage: $0 [path_to_seajay_binary]"
    exit 1
fi

echo "Checking build mode for: $BINARY"
echo "----------------------------------------"

# Check for mode strings in the binary
if strings "$BINARY" | grep -q "TESTING MODE - 10K"; then
    echo "Mode: TESTING (10K node limit)"
    echo "This build is for rapid development and testing."
elif strings "$BINARY" | grep -q "TUNING MODE - 100K"; then
    echo "Mode: TUNING (100K node limit)"
    echo "This build is for parameter tuning."
elif strings "$BINARY" | grep -q "PRODUCTION MODE"; then
    echo "Mode: PRODUCTION (no limits)"
    echo "This build is full strength for competitive play."
else
    echo "Mode: UNKNOWN"
    echo "Could not determine quiescence mode from binary."
fi

echo "----------------------------------------"
echo ""
echo "To see the mode at runtime, use:"
echo "  echo 'uci' | $BINARY"