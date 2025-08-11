#!/bin/bash

# Test draw detection performance optimization

echo "=== Testing Draw Detection Performance Optimization ==="
echo ""

# Test positions
STARTPOS="position startpos"
ENDGAME="position fen r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"
REPETITION="position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 g8f6 f3g1 f6g8 g1f3 g8f6"

# Function to run test
run_test() {
    local binary=$1
    local desc=$2
    local pos=$3
    local depth=$4
    
    echo "$desc - Depth $depth:"
    result=$(echo -e "$pos\ngo depth $depth\nquit" | timeout 5 $binary 2>&1 | grep "info depth $depth" | tail -1)
    
    # Extract NPS
    nps=$(echo "$result" | grep -oP 'nps \K[0-9]+')
    nodes=$(echo "$result" | grep -oP 'nodes \K[0-9]+')
    time=$(echo "$result" | grep -oP 'time \K[0-9]+')
    
    if [ -n "$nps" ]; then
        echo "  NPS: $nps, Nodes: $nodes, Time: ${time}ms"
    else
        echo "  Failed to get stats"
    fi
    echo ""
}

# Compare original vs optimized at depth 6
echo "=== Starting Position - Depth 6 ==="
if [ -f "../bin/seajay_stage9b_draws" ]; then
    run_test "../bin/seajay_stage9b_draws" "Original" "$STARTPOS" 6
fi
run_test "../bin/seajay_stage9b_draws_optimized" "Optimized" "$STARTPOS" 6

echo "=== Middle Game Position - Depth 5 ==="
if [ -f "../bin/seajay_stage9b_draws" ]; then
    run_test "../bin/seajay_stage9b_draws" "Original" "$ENDGAME" 5
fi
run_test "../bin/seajay_stage9b_draws_optimized" "Optimized" "$ENDGAME" 5

echo "=== Repetition Test Position - Depth 4 ==="
if [ -f "../bin/seajay_stage9b_draws" ]; then
    run_test "../bin/seajay_stage9b_draws" "Original" "$REPETITION" 4
fi
run_test "../bin/seajay_stage9b_draws_optimized" "Optimized" "$REPETITION" 4

echo "=== Draw Detection Tests ==="
echo "Testing insufficient material:"
echo -e "position fen 8/8/8/4k3/8/3K4/8/8 w - - 0 1\ngo depth 1\nquit" | ../bin/seajay_stage9b_draws_optimized 2>/dev/null | grep -q "Draw by insufficient material" && echo "  ✓ Insufficient material detected" || echo "  ✗ Failed"

echo "Testing 50-move rule:"
echo -e "position fen 8/8/8/4k3/8/3K4/8/8 w - - 99 1\ngo depth 1\nquit" | ../bin/seajay_stage9b_draws_optimized 2>/dev/null | grep -q "50-move" && echo "  ✓ 50-move rule detected" || echo "  ✗ Failed (or not yet at 100 halfmoves)"

echo ""
echo "=== Summary ==="
echo "Optimizations implemented:"
echo "1. Strategic draw checking (not every node)"
echo "2. Cached insufficient material detection"
echo "3. Early exit in repetition detection"
echo "4. Limited lookback based on 50-move rule"