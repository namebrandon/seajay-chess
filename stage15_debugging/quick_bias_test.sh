#!/bin/bash
# Quick test for White bias

echo "Quick White Bias Test (20 games each)"
echo "======================================"
echo ""

STAGE15_PROPER="/workspace/binaries/seajay-stage15-properly-fixed"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"

# Run self-play test
echo "Stage 15 Properly Fixed - Self Play from STARTPOS:"
OUTPUT_DIR="/workspace/stage15_debugging/bias_quick"
mkdir -p $OUTPUT_DIR

$FASTCHESS \
    -engine cmd="$STAGE15_PROPER" name=White \
    -engine cmd="$STAGE15_PROPER" name=Black \
    -each tc=1+0.01 \
    -games 2 -rounds 10 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" notation=san \
    -log file="$OUTPUT_DIR/test.log" level=warn \
    -recover

echo ""
echo "Game Results:"
echo "============="
echo -n "White wins (1-0): "
grep -c "\[Result \"1-0\"\]" "$OUTPUT_DIR/games.pgn"
echo -n "Black wins (0-1): "
grep -c "\[Result \"0-1\"\]" "$OUTPUT_DIR/games.pgn"
echo -n "Draws (1/2-1/2):  "
grep -c "\[Result \"1/2-1/2\"\]" "$OUTPUT_DIR/games.pgn"

echo ""
echo "Sample game endings:"
grep -B1 "Result" "$OUTPUT_DIR/games.pgn" | grep -E "1-0|0-1|1/2-1/2" | head -5

echo ""
echo "=================================="
echo "If White wins >70% we have a bias"
echo "=================================="