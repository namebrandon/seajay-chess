#!/bin/bash
# Quick test of properly fixed Stage 15

echo "============================================"
echo "Testing Properly Fixed Stage 15"
echo "============================================"
echo ""

STAGE15_PROPER="/workspace/binaries/seajay-stage15-properly-fixed"
STAGE14="/workspace/binaries/seajay-stage14-final"
STAGE15_ORIG="/workspace/binaries/seajay-stage15-original"

echo "Binary checksums:"
echo "Stage 15 Properly Fixed: $(md5sum $STAGE15_PROPER | cut -d' ' -f1)"
echo "Stage 15 Original:       $(md5sum $STAGE15_ORIG | cut -d' ' -f1)"
echo "Stage 14 Final:          $(md5sum $STAGE14 | cut -d' ' -f1)"
echo ""

echo "Evaluation comparison (depth 5):"
echo "================================="
echo ""

for pos in "startpos" "startpos moves e2e4" "startpos moves e2e4 c7c5"; do
    echo "Position: $pos"
    echo -n "  Stage 14:          "
    echo -e "uci\nposition $pos\ngo depth 5\nquit" | $STAGE14 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'
    echo -n "  Stage 15 Original: "
    echo -e "uci\nposition $pos\ngo depth 5\nquit" | $STAGE15_ORIG 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'
    echo -n "  Stage 15 Fixed:    "
    echo -e "uci\nposition $pos\ngo depth 5\nquit" | $STAGE15_PROPER 2>&1 | grep "score cp" | tail -1 | sed 's/.*score cp \([-0-9]*\).*/\1 cp/'
    echo ""
done

echo "Quick match test (20 games at 1+0.01):"
echo "======================================="
echo ""

FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_DIR="/workspace/stage15_debugging/quick_test_proper"
mkdir -p $OUTPUT_DIR

echo "Stage 15 Properly Fixed vs Stage 14:"
$FASTCHESS \
    -engine cmd="$STAGE15_PROPER" name=Stage15-ProperFix \
    -engine cmd="$STAGE14" name=Stage14-Final \
    -each tc=1+0.01 \
    -games 2 -rounds 10 -repeat \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/test.log" level=error \
    -recover 2>&1 | grep -E "Score:|Elo:" | tail -2

echo ""
echo "============================================"
echo "If evaluations match and Elo is positive,"
echo "the fix is successful!"
echo "============================================"