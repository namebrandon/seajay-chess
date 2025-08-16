#!/bin/bash
# Test if White bias exists in Stage 14

echo "Testing White Bias in Stage 14"
echo "==============================="
echo ""

STAGE14="/workspace/binaries/seajay-stage14-final"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"

echo "Stage 14 Self-Play from STARTPOS (20 games):"
OUTPUT_DIR="/workspace/stage15_debugging/stage14_bias"
mkdir -p $OUTPUT_DIR

$FASTCHESS \
    -engine cmd="$STAGE14" name=White \
    -engine cmd="$STAGE14" name=Black \
    -each tc=1+0.01 \
    -games 2 -rounds 10 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" notation=san \
    -log file="$OUTPUT_DIR/test.log" level=warn \
    -recover

echo ""
echo "Results:"
echo "========"
echo -n "White wins (1-0): "
grep -c "\[Result \"1-0\"\]" "$OUTPUT_DIR/games.pgn"
echo -n "Black wins (0-1): "
grep -c "\[Result \"0-1\"\]" "$OUTPUT_DIR/games.pgn"
echo -n "Draws (1/2-1/2):  "
grep -c "\[Result \"1/2-1/2\"\]" "$OUTPUT_DIR/games.pgn"

# Calculate percentage
WHITE_WINS=$(grep -c "\[Result \"1-0\"\]" "$OUTPUT_DIR/games.pgn")
TOTAL_GAMES=$(grep -c "\[Result" "$OUTPUT_DIR/games.pgn")
if [ $TOTAL_GAMES -gt 0 ]; then
    WHITE_PCT=$((WHITE_WINS * 100 / TOTAL_GAMES))
    echo ""
    echo "White win rate: ${WHITE_PCT}%"
fi

echo ""
echo "=================================="
echo "If White wins >70%, Stage 14 also has the bias"
echo "This would mean it's an older bug, not Stage 15 specific"
echo "=================================="