#!/bin/bash
# Test for White bias in Stage 15 properly fixed version

echo "=========================================="
echo "White Bias Test - Stage 15 Properly Fixed"
echo "=========================================="
echo ""

STAGE15_PROPER="/workspace/binaries/seajay-stage15-properly-fixed"
STAGE15_ORIG="/workspace/binaries/seajay-stage15-original"
STAGE14="/workspace/binaries/seajay-stage14-final"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"

echo "Testing for White bias in self-play from starting position"
echo "Running 100 games for each version at 5+0.05 time control"
echo ""

# Test Stage 14 baseline
echo "1. Stage 14 Self-Play (Baseline):"
echo "=================================="
OUTPUT_DIR="/workspace/stage15_debugging/white_bias_test/stage14"
mkdir -p $OUTPUT_DIR

$FASTCHESS \
    -engine cmd="$STAGE14" name=Stage14-White \
    -engine cmd="$STAGE14" name=Stage14-Black \
    -each tc=5+0.05 \
    -games 2 -rounds 50 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/test.log" level=error \
    -recover 2>&1 | grep -A3 "Score of"

echo ""
echo "2. Stage 15 Original Self-Play:"
echo "================================"
OUTPUT_DIR="/workspace/stage15_debugging/white_bias_test/stage15_original"
mkdir -p $OUTPUT_DIR

$FASTCHESS \
    -engine cmd="$STAGE15_ORIG" name=Stage15Orig-White \
    -engine cmd="$STAGE15_ORIG" name=Stage15Orig-Black \
    -each tc=5+0.05 \
    -games 2 -rounds 50 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/test.log" level=error \
    -recover 2>&1 | grep -A3 "Score of"

echo ""
echo "3. Stage 15 Properly Fixed Self-Play:"
echo "======================================"
OUTPUT_DIR="/workspace/stage15_debugging/white_bias_test/stage15_proper"
mkdir -p $OUTPUT_DIR

$FASTCHESS \
    -engine cmd="$STAGE15_PROPER" name=Stage15Fixed-White \
    -engine cmd="$STAGE15_PROPER" name=Stage15Fixed-Black \
    -each tc=5+0.05 \
    -games 2 -rounds 50 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/test.log" level=error \
    -recover 2>&1 | grep -A3 "Score of"

echo ""
echo "=========================================="
echo "Analysis:"
echo "=========================================="
echo "In self-play, we expect roughly 50-50 win rates"
echo "If White wins >65% of games, there's likely a bias"
echo ""

# Count actual game results from PGN files
echo "Detailed Results:"
echo ""

for version in "stage14" "stage15_original" "stage15_proper"; do
    PGN="/workspace/stage15_debugging/white_bias_test/$version/games.pgn"
    if [ -f "$PGN" ]; then
        echo "$version:"
        echo -n "  White wins: "
        grep -c "1-0" "$PGN" 2>/dev/null || echo "0"
        echo -n "  Black wins: "
        grep -c "0-1" "$PGN" 2>/dev/null || echo "0"
        echo -n "  Draws:      "
        grep -c "1/2-1/2" "$PGN" 2>/dev/null || echo "0"
        echo ""
    fi
done

echo "=========================================="
echo "Note: The original White bias showed ~90% White wins"
echo "If we still see that pattern, the bias is NOT fixed"
echo "=========================================="