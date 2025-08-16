#!/bin/bash
# Test Stage 10 for White bias - first available version with PST

echo "=========================================="
echo "Stage 10 White Bias Test"
echo "First available stage with PST (introduced in Stage 9)"
echo "=========================================="
echo ""

BINARY="/workspace/binaries/seajay-stage10-x86-64"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_DIR="/workspace/stage15_debugging/stage10_bias_test"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Stage 10 binary not found at $BINARY"
    exit 1
fi

echo "Binary: $BINARY"
echo "MD5: $(md5sum $BINARY | cut -d' ' -f1)"
echo ""

mkdir -p "$OUTPUT_DIR"

echo "Running 20 games from startpos at 2+0.02 time control..."
echo ""

$FASTCHESS \
    -engine cmd="$BINARY" name=White \
    -engine cmd="$BINARY" name=Black \
    -each tc=2+0.02 \
    -games 2 -rounds 10 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" notation=san \
    -log file="$OUTPUT_DIR/test.log" level=warn \
    -recover

echo ""
echo "Results:"
echo "========"

# Count results
WHITE_WINS=$(grep -c "\[Result \"1-0\"\]" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
BLACK_WINS=$(grep -c "\[Result \"0-1\"\]" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
DRAWS=$(grep -c "\[Result \"1/2-1/2\"\]" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
TOTAL=$((WHITE_WINS + BLACK_WINS + DRAWS))

echo "White wins: $WHITE_WINS"
echo "Black wins: $BLACK_WINS"
echo "Draws:      $DRAWS"
echo "Total:      $TOTAL"
echo ""

if [ $TOTAL -gt 0 ]; then
    WHITE_PCT=$((WHITE_WINS * 100 / TOTAL))
    BLACK_PCT=$((BLACK_WINS * 100 / TOTAL))
    DRAW_PCT=$((DRAWS * 100 / TOTAL))
    
    echo "Percentages:"
    echo "White: ${WHITE_PCT}%"
    echo "Black: ${BLACK_PCT}%"
    echo "Draws: ${DRAW_PCT}%"
    echo ""
    
    if [ $WHITE_PCT -ge 80 ]; then
        echo "🔴 SEVERE WHITE BIAS DETECTED (80%+ White wins)"
        echo "The PST bug likely exists from Stage 10 (or Stage 9 when PST was introduced)"
    elif [ $WHITE_PCT -ge 70 ]; then
        echo "🟠 HIGH WHITE BIAS DETECTED (70-79% White wins)"
    elif [ $WHITE_PCT -ge 60 ]; then
        echo "🟡 MODERATE WHITE BIAS (60-69% White wins)"
    else
        echo "🟢 NORMAL - No significant bias detected"
    fi
fi

echo ""
echo "=========================================="
echo "Stage 10 was the first stage after PST introduction (Stage 9)"
echo "If bias exists here, it was introduced with PST"
echo "=========================================="