#!/bin/bash
# Test if the testing methodology itself could cause White bias

echo "=========================================="
echo "Testing Methodology Validation"
echo "=========================================="
echo ""

BINARY="/workspace/binaries/seajay-stage14-final"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_DIR="/workspace/stage15_debugging/methodology_test"
mkdir -p "$OUTPUT_DIR"

echo "Test 1: Self-play with explicit White/Black names (original method)"
echo "===================================================================="
$FASTCHESS \
    -engine cmd="$BINARY" name=White \
    -engine cmd="$BINARY" name=Black \
    -each tc=1+0.01 \
    -games 2 -rounds 5 -repeat \
    -pgnout file="$OUTPUT_DIR/test1.pgn" notation=san \
    -log file="$OUTPUT_DIR/test1.log" level=error \
    -recover 2>&1 | grep -E "Score|White|Black|Started|Finished" | head -20

echo ""
echo "Test 2: Self-play with neutral names"
echo "====================================="
$FASTCHESS \
    -engine cmd="$BINARY" name=Engine1 \
    -engine cmd="$BINARY" name=Engine2 \
    -each tc=1+0.01 \
    -games 2 -rounds 5 -repeat \
    -pgnout file="$OUTPUT_DIR/test2.pgn" notation=san \
    -log file="$OUTPUT_DIR/test2.log" level=error \
    -recover 2>&1 | grep -E "Score|Engine|Started|Finished" | head -20

echo ""
echo "Test 3: Reverse order (Black first, White second)"
echo "=================================================="
$FASTCHESS \
    -engine cmd="$BINARY" name=Black \
    -engine cmd="$BINARY" name=White \
    -each tc=1+0.01 \
    -games 2 -rounds 5 -repeat \
    -pgnout file="$OUTPUT_DIR/test3.pgn" notation=san \
    -log file="$OUTPUT_DIR/test3.log" level=error \
    -recover 2>&1 | grep -E "Score|White|Black|Started|Finished" | head -20

echo ""
echo "Comparing results..."
echo "===================="
for test in test1 test2 test3; do
    echo "$test results:"
    if [ -f "$OUTPUT_DIR/$test.pgn" ]; then
        WHITE_WINS=$(grep -c "\[Result \"1-0\"\]" "$OUTPUT_DIR/$test.pgn" 2>/dev/null || echo "0")
        BLACK_WINS=$(grep -c "\[Result \"0-1\"\]" "$OUTPUT_DIR/$test.pgn" 2>/dev/null || echo "0")
        DRAWS=$(grep -c "\[Result \"1/2-1/2\"\]" "$OUTPUT_DIR/$test.pgn" 2>/dev/null || echo "0")
        echo "  White wins: $WHITE_WINS, Black wins: $BLACK_WINS, Draws: $DRAWS"
    fi
done

echo ""
echo "If all three tests show similar White bias, it's the engine."
echo "If results differ significantly, it could be a testing artifact."