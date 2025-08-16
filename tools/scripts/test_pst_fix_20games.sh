#!/bin/bash
# Test PST Fix - 20 games from startpos at 10+0.1 time control
# Self-play with fixed binary to test symmetry

echo "================================================"
echo "PST Fix Symmetry Test - 20 games at 10+0.1"
echo "================================================"
echo ""
echo "Binary: /workspace/build/seajay (freshly built)"
echo "Time Control: 10+0.1"
echo "Starting position: startpos"
echo ""

OUTPUT_DIR="/workspace/pst_fix_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

echo "Output directory: $OUTPUT_DIR"
echo ""

# Run the match
/workspace/external/testers/fast-chess/fastchess \
    -engine cmd="/workspace/build/seajay" name=SeaJay-Fixed-1 \
    -engine cmd="/workspace/build/seajay" name=SeaJay-Fixed-2 \
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

# Calculate percentages
if [ $total -gt 0 ]; then
    white_pct=$(echo "scale=1; $wins_white * 100 / $total" | bc)
    black_pct=$(echo "scale=1; $wins_black * 100 / $total" | bc)
    draw_pct=$(echo "scale=1; $draws * 100 / $total" | bc)
    
    echo "Percentages:"
    echo "  White: ${white_pct}%"
    echo "  Black: ${black_pct}%"
    echo "  Draws: ${draw_pct}%"
    echo ""
fi

# Check for color bias
echo "Color Bias Analysis:"
if [ $wins_black -eq 0 ]; then
    echo "  WARNING: No Black wins observed!"
    echo "  This suggests potential residual color bias"
elif [ $wins_white -gt $((wins_black * 2)) ]; then
    echo "  White wins significantly more than Black"
    echo "  Some first-move advantage is expected"
else
    echo "  Results appear reasonably balanced"
fi

echo ""
echo "PGN file: $OUTPUT_DIR/games.pgn"
echo "Log file: $OUTPUT_DIR/match.log"
echo ""

# Show a sample game to check move quality
echo "Sample opening moves from first game:"
grep -A 5 "^1\." "$OUTPUT_DIR/games.pgn" | head -10

echo ""
echo "================================================"
echo "Test Complete"
echo "================================================"