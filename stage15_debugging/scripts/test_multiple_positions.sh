#!/bin/bash
# Test multiple positions to understand the evaluation bug

echo "Testing Multiple Positions - Stage 14 vs Stage 15"
echo "=================================================="
echo ""

test_position() {
    local moves="$1"
    local desc="$2"
    
    echo "$desc"
    echo "Moves: $moves"
    
    echo -n "Stage 14: "
    echo -e "uci\nposition startpos moves $moves\ngo depth 1\nquit" | /workspace/binaries/seajay-stage14-final 2>&1 | grep "score cp" | awk '{for(i=1;i<=NF;i++) if($i=="cp") print $(i+1)}'
    
    echo -n "Stage 15: "
    echo -e "uci\nposition startpos moves $moves\ngo depth 1\nquit" | /workspace/binaries/seajay_stage15_tuned_fixed 2>&1 | grep "score cp" | awk '{for(i=1;i<=NF;i++) if($i=="cp") print $(i+1)}'
    
    echo "---"
}

# Test various positions
test_position "" "Starting position"
test_position "e2e4" "After 1.e4"
test_position "e2e4 c7c5" "After 1.e4 c5 (Sicilian)"
test_position "e2e4 e7e5" "After 1.e4 e5"
test_position "d2d4" "After 1.d4"
test_position "d2d4 d7d5" "After 1.d4 d5"
test_position "g1f3" "After 1.Nf3"

echo ""
echo "=================================================="
echo "Analysis:"
echo "Stage 14 should show small positive values (White's advantage)"
echo "If Stage 15 shows large negative values, SEE is broken"