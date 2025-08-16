#!/bin/bash
# Stage 15 Day 8.1: Test different SEE pruning margins

BINARY="/workspace/binaries/seajay_stage15_bias_bugfix2"
OUTPUT_DIR="/workspace/project_docs/stage_implementations/stage_15_see/tuning_results"

mkdir -p "$OUTPUT_DIR"

echo "=== Stage 15 SEE Margin Tuning Test ==="
echo "Testing different pruning margin values..."
echo ""

# Function to run a quick test with specific margins
test_margins() {
    local name=$1
    local conservative=$2
    local aggressive=$3
    local endgame=$4
    
    echo "Testing $name: Conservative=$conservative, Aggressive=$aggressive, Endgame=$endgame"
    
    # Run a quick self-play test with these settings
    # Note: We need to modify the source to test different values
    # For now, we'll document what values to test
    
    echo "  - Would test with these margin values"
    echo "  - Need to recompile with modified thresholds"
    echo ""
}

# Current baseline values
echo "BASELINE VALUES (Current Implementation):"
echo "  Conservative: -100 (only clearly bad captures)"
echo "  Aggressive: -50 (more pruning)"
echo "  Endgame: -25 (even more aggressive)"
echo ""

echo "RECOMMENDED TEST CONFIGURATIONS:"
echo ""

echo "1. More Conservative Set:"
test_margins "Conservative-150" -150 -75 -50

echo "2. Baseline (Current):"
test_margins "Baseline" -100 -50 -25

echo "3. More Aggressive Set:"
test_margins "Aggressive-75" -75 -25 0

echo "4. Very Aggressive Set:"
test_margins "VeryAggressive-50" -50 0 25

echo "5. Stockfish-like Values:"
test_margins "Stockfish" -90 -40 -20

echo ""
echo "To test each configuration:"
echo "1. Edit /workspace/src/search/quiescence.h with new threshold values"
echo "2. Rebuild: cd /workspace/build && cmake .. && make -j"
echo "3. Run self-play test with cutechess-cli"
echo ""

# Let's at least test the current implementation's statistics
echo "=== Testing Current Implementation Statistics ==="
echo ""

# Create a test position
cat > /tmp/see_test.epd << EOF
r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3
r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 4 5
r3k2r/ppp2ppp/2n2n2/2bpp3/2B1P3/3P1N2/PPP2PPP/R3K2R w KQkq - 0 9
r2qk2r/ppp2ppp/2n2n2/2bpp3/2B1P3/3P1N2/PPP2PPP/R2QK2R w KQkq - 0 9
r1bq1rk1/ppp2ppp/2n2n2/2bpp3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 0 10
EOF

# Run engine with different SEE modes
for mode in off conservative aggressive; do
    echo "Testing with SEEPruning=$mode:"
    echo "position startpos
setoption name SEEPruning value $mode
go movetime 5000
quit" | $BINARY 2>/dev/null | grep -E "info string SEE|bestmove" | head -5
    echo ""
done

echo "=== Margin Tuning Recommendations ==="
echo ""
echo "Based on the Stage 15 implementation and common practices:"
echo ""
echo "1. Conservative Margin (-100 current):"
echo "   - Test range: -150, -125, -100, -75, -50"
echo "   - Expected: -100 to -125 optimal for safety"
echo ""
echo "2. Aggressive Margin (-50 current):"
echo "   - Test range: -75, -50, -25, 0"
echo "   - Expected: -50 to -75 optimal balance"
echo ""
echo "3. Endgame Margin (-25 current):"
echo "   - Test range: -50, -25, 0, 25"
echo "   - Expected: -25 to 0 optimal for endgames"
echo ""
echo "Next Steps:"
echo "1. Create modified binaries with each margin set"
echo "2. Run 50-100 game matches between versions"
echo "3. Measure win rate and pruning statistics"
echo "4. Select optimal values based on Elo gain"