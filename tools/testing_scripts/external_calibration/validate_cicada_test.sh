#!/bin/bash

echo "Validating SPRT Test Setup for SeaJay vs Cicada"
echo "================================================"
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

# Check Cicada binary
echo ""
echo "2. Checking Cicada binary..."
if [ -f "/workspace/external/engines/cicada-linux-v0.1/cicada" ]; then
    echo "   ✓ Cicada binary exists"
    VERSION=$((echo "uci"; sleep 0.5; echo "quit") | timeout 2 /workspace/external/engines/cicada-linux-v0.1/cicada 2>/dev/null | grep "id name" | head -1)
    echo "   ✓ Version: $VERSION"
else
    echo "   ✗ Cicada binary not found"
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
OUTPUT_DIR="/workspace/sprt_results/SPRT-2025-EXT-001-CICADA"
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
    -engine name="Cicada-Test" cmd="/workspace/external/engines/cicada-linux-v0.1/cicada" \
    -each proto=uci tc="1+0.01" \
    -rounds 1 \
    -repeat \
    -openings file="/workspace/external/books/4moves_test.pgn" format=pgn order=random \
    -pgnout "/tmp/test_games.pgn" fi 2>&1 | grep -E "(Score|finished|Error|Warning)"

if [ $? -eq 0 ]; then
    echo ""
    echo "   ✓ Compatibility test successful"
else
    echo ""
    echo "   ✗ Compatibility test failed"
    exit 1
fi

echo ""
echo "================================================"
echo "✓ All validation checks passed!"
echo ""
echo "The SPRT test is ready to run."
echo "To start the test, execute:"
echo "  ./run_seajay_vs_cicada_sprt.sh"
echo ""
echo "Expected test duration: 1-4 hours depending on results"
echo "The test will automatically stop when statistical significance is reached."
echo "================================================"