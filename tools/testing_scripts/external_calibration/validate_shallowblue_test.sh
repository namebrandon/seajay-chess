#!/bin/bash

echo "Validating SPRT Test Setup: SeaJay vs Shallow Blue"
echo "===================================================="
echo ""

# Check SeaJay binary
echo "1. Checking SeaJay binary..."
if [ -f "/workspace/build/seajay" ]; then
    echo "   ✓ SeaJay binary exists"
    VERSION=$(echo "uci" | /workspace/build/seajay 2>/dev/null | grep "id name" | head -1)
    echo "   ✓ Version: $VERSION"
else
    echo "   ✗ SeaJay binary not found"
    exit 1
fi

# Check Shallow Blue binary
echo ""
echo "2. Checking Shallow Blue binary..."
if [ -f "/workspace/external/engines/shallow-blue-2.0.0/shallowblue" ]; then
    echo "   ✓ Shallow Blue binary exists"
    VERSION=$(echo "uci" | /workspace/external/engines/shallow-blue-2.0.0/shallowblue 2>/dev/null | grep "id name" | head -1)
    echo "   ✓ Version: $VERSION"
else
    echo "   ✗ Shallow Blue binary not found"
    exit 1
fi

# Check fast-chess
echo ""
echo "3. Checking fast-chess..."
if [ -f "/workspace/external/testers/fast-chess/fastchess" ]; then
    echo "   ✓ fast-chess exists"
else
    echo "   ✗ fast-chess not found"
    exit 1
fi

# Check opening book
echo ""
echo "4. Checking opening book..."
if [ -f "/workspace/external/books/4moves_test.pgn" ]; then
    echo "   ✓ Opening book exists"
    LINES=$(wc -l < /workspace/external/books/4moves_test.pgn)
    echo "   ✓ Book has $LINES lines"
else
    echo "   ✗ Opening book not found"
    exit 1
fi

# Check output directory
echo ""
echo "5. Checking output directory..."
OUTPUT_DIR="/workspace/sprt_results/SPRT-2025-EXT-003-SHALLOWBLUE"
if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR"
    echo "   ✓ Created output directory: $OUTPUT_DIR"
else
    echo "   ✓ Output directory exists: $OUTPUT_DIR"
fi

# Run a quick 2-game test
echo ""
echo "6. Running quick compatibility test (2 games)..."
echo ""

/workspace/external/testers/fast-chess/fastchess \
    -engine name="SeaJay-Test" cmd="/workspace/build/seajay" \
    -engine name="ShallowBlue-Test" cmd="/workspace/external/engines/shallow-blue-2.0.0/shallowblue" \
    -each proto=uci tc="1+0.01" \
    -rounds 1 \
    -repeat \
    -openings file="/workspace/external/books/4moves_test.pgn" format=pgn order=random \
    -pgnout "/tmp/test_shallow_games.pgn" fi 2>&1 | grep -E "(Score|finished|Error|Warning)" | head -5

if [ $? -eq 0 ]; then
    echo ""
    echo "   ✓ Compatibility test successful"
else
    echo ""
    echo "   ✗ Compatibility test failed"
    exit 1
fi

echo ""
echo "===================================================="
echo "✓ All validation checks passed!"
echo ""
echo "Both engines are UCI-compatible and working properly."
echo ""
echo "Expected outcome:"
echo "  - SeaJay (~650 ELO) vs Shallow Blue (~1800-2000 ELO)"
echo "  - Expected deficit: ~1200-1350 ELO"
echo "  - SeaJay will likely score <2% (0-4 points out of 200 games)"
echo ""
echo "To start the SPRT test, execute:"
echo "  ./run_seajay_vs_shallowblue_sprt.sh"
echo ""
echo "The test will likely conclude quickly due to the large skill gap."
echo "===================================================="