#!/bin/bash
# Test Stage 15 Original SPRT Candidate 1 vs Stage 14
# This will help us understand if the original Stage 15 was working

echo "=========================================="
echo "Testing Original Stage 15 Candidate 1"
echo "=========================================="
echo ""

STAGE15_ORIG="/workspace/binaries/seajay-stage15-original"
STAGE14="/workspace/binaries/seajay-stage14-final"
STAGE15_FIXED="/workspace/binaries/seajay-stage15-fixed"

echo "Binary checksums:"
echo "Stage 15 Original: $(md5sum $STAGE15_ORIG | cut -d' ' -f1)"
echo "Stage 14 Final:    $(md5sum $STAGE14 | cut -d' ' -f1)"
echo "Stage 15 Fixed:    $(md5sum $STAGE15_FIXED | cut -d' ' -f1)"
echo ""

# Test evaluations at key positions
echo "Evaluation comparison at starting position:"
echo "==========================================="

echo -n "Stage 14 Final:    "
echo -e "uci\nposition startpos\ngo depth 5\nquit" | $STAGE14 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo -n "Stage 15 Original: "
echo -e "uci\nposition startpos\ngo depth 5\nquit" | $STAGE15_ORIG 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo -n "Stage 15 Fixed:    "
echo -e "uci\nposition startpos\ngo depth 5\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo ""
echo "After 1.e4:"
echo "==========="

echo -n "Stage 14 Final:    "
echo -e "uci\nposition startpos moves e2e4\ngo depth 5\nquit" | $STAGE14 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo -n "Stage 15 Original: "
echo -e "uci\nposition startpos moves e2e4\ngo depth 5\nquit" | $STAGE15_ORIG 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo -n "Stage 15 Fixed:    "
echo -e "uci\nposition startpos moves e2e4\ngo depth 5\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo ""
echo "After 1.e4 c5:"
echo "=============="

echo -n "Stage 14 Final:    "
echo -e "uci\nposition startpos moves e2e4 c7c5\ngo depth 5\nquit" | $STAGE14 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo -n "Stage 15 Original: "
echo -e "uci\nposition startpos moves e2e4 c7c5\ngo depth 5\nquit" | $STAGE15_ORIG 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo -n "Stage 15 Fixed:    "
echo -e "uci\nposition startpos moves e2e4 c7c5\ngo depth 5\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'

echo ""
echo "Quick match test (10 games at 1+0.01):"
echo "======================================="
echo ""

# Quick test: Stage 15 Original vs Stage 14
echo "Running Stage 15 Original vs Stage 14 (10 games)..."
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_DIR="/workspace/stage15_debugging/quick_test_original"
mkdir -p $OUTPUT_DIR

$FASTCHESS \
    -engine cmd="$STAGE15_ORIG" name=Stage15-Original \
    -engine cmd="$STAGE14" name=Stage14-Final \
    -each tc=1+0.01 \
    -games 2 -rounds 5 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/test.log" level=error \
    -recover 2>&1 | grep -E "Score:|Elo:"

echo ""
echo "Running Stage 15 Fixed vs Stage 14 (10 games)..."
OUTPUT_DIR="/workspace/stage15_debugging/quick_test_fixed"
mkdir -p $OUTPUT_DIR

$FASTCHESS \
    -engine cmd="$STAGE15_FIXED" name=Stage15-Fixed \
    -engine cmd="$STAGE14" name=Stage14-Final \
    -each tc=1+0.01 \
    -games 2 -rounds 5 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/test.log" level=error \
    -recover 2>&1 | grep -E "Score:|Elo:"