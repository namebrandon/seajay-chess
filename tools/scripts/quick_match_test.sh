#!/bin/bash
# Quick match test - 10 games at fast time control

echo "=========================================="
echo "Quick Match Test: Stage 15 vs Stage 14"
echo "10 games at 5+0.05"
echo "=========================================="
echo ""

FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_DIR="/workspace/sprt_results/quick_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

echo "Running 10-game match..."
echo ""

$FASTCHESS \
    -engine cmd="/workspace/binaries/seajay_stage15_sprt_candidate1" name=Stage15-SEE \
        option.SEEMode=production option.SEEPruning=aggressive \
    -engine cmd="/workspace/binaries/seajay-stage14-final" name=Stage14-Base \
    -each tc=5+0.05 \
    -games 2 -rounds 5 -repeat -concurrency 1 \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/match.log" level=warn \
    -recover

echo ""
echo "Results:"
tail -20 "$OUTPUT_DIR/match.log" | grep -E "Score|Elo|Stage"
echo ""
echo "Full log: $OUTPUT_DIR/match.log"