#!/bin/bash
# Test PST Bias Fix #2 - 20 games from startpos at 10+0.1 time control
# Self-play with fixed binary to verify bias is resolved

echo "================================================"
echo "PST Bias Fix #2 Test - 20 games at 10+0.1"
echo "================================================"
echo ""
echo "Binary: /workspace/binaries/seajay_stage15_bias_bugfix2"
echo "Time Control: 10+0.1"
echo "Starting position: startpos"
echo ""

OUTPUT_DIR="/workspace/bias_fix2_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

echo "Output directory: $OUTPUT_DIR"
echo ""

# Run the match
/workspace/external/testers/fast-chess/fastchess \
    -engine cmd="/workspace/binaries/seajay_stage15_bias_bugfix2" name=SeaJay-BiasFix2-1 \
    -engine cmd="/workspace/binaries/seajay_stage15_bias_bugfix2" name=SeaJay-BiasFix2-2 \
    -each tc=10+0.1 \
    -games 2 -rounds 10 -repeat -concurrency 1 \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/match.log" level=info \
    -recover

echo ""
echo "================================================"
echo "Match Complete - Analyzing Results"
echo "================================================"
echo ""

# Analyze results
wins_white=$(grep "Result \"1-0\"" "$OUTPUT_DIR/games.pgn" | wc -l)
wins_black=$(grep "Result \"0-1\"" "$OUTPUT_DIR/games.pgn" | wc -l)
draws=$(grep "Result \"1/2-1/2\"" "$OUTPUT_DIR/games.pgn" | wc -l)
total=$((wins_white + wins_black + draws))

echo "Total games played: $total"
echo ""
echo "Results by outcome:"
echo "  White wins: $wins_white"
echo "  Black wins: $wins_black"
echo "  Draws: $draws"
echo ""

# Calculate percentages using awk
if [ $total -gt 0 ]; then
    white_pct=$(awk "BEGIN {printf \"%.1f\", $wins_white * 100 / $total}")
    black_pct=$(awk "BEGIN {printf \"%.1f\", $wins_black * 100 / $total}")
    draw_pct=$(awk "BEGIN {printf \"%.1f\", $draws * 100 / $total}")
    
    echo "Percentages:"
    echo "  White: ${white_pct}%"
    echo "  Black: ${black_pct}%"
    echo "  Draws: ${draw_pct}%"
    echo ""
fi

# Check for improvement
echo "Bias Analysis:"
if [ $wins_black -gt 0 ]; then
    echo "  ✓ Black wins observed! ($wins_black wins)"
    if [ $wins_white -gt $((wins_black * 3)) ]; then
        echo "  ⚠ White still wins significantly more"
    elif [ $wins_white -gt $((wins_black * 2)) ]; then
        echo "  ⚠ Some White advantage remains (expected with first move)"
    else
        echo "  ✓ Results appear reasonably balanced!"
    fi
else
    echo "  ✗ WARNING: No Black wins observed"
    echo "  Potential residual bias still present"
fi

echo ""
echo "Opening Moves Analysis:"
echo "First 5 moves from game 1 (check if Black plays normally now):"
grep -A 3 "^1\." "$OUTPUT_DIR/games.pgn" | head -6

echo ""
echo "PGN file: $OUTPUT_DIR/games.pgn"
echo "Log file: $OUTPUT_DIR/match.log"
echo ""
echo "================================================"
echo "Test Complete"
echo "================================================"