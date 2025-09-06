#!/bin/bash

# Test Option A: Revised evasion move ordering
BINARY="./bin/seajay"
PROBLEM_FEN="r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17"

echo "================================================"
echo "Option A Test: Captures > Blocks > King Moves"
echo "================================================"
echo ""

# Test at key depths
echo "Testing key depths with Option A fix:"
echo "--------------------------------------"
for depth in 1 2 3 4 5 6 7 8 9 10; do
    result=$(echo -e "position fen $PROBLEM_FEN\ngo depth $depth" | $BINARY 2>&1)
    
    # Extract metrics
    bestmove=$(echo "$result" | grep "bestmove" | tail -1 | awk '{print $2}')
    info_line=$(echo "$result" | grep "info depth $depth " | tail -1)
    score=$(echo "$info_line" | grep -o "score cp [0-9-]*" | awk '{print $3}')
    nodes=$(echo "$info_line" | grep -o "nodes [0-9]*" | awk '{print $2}')
    seldepth=$(echo "$info_line" | grep -o "seldepth [0-9]*" | awk '{print $2}')
    
    printf "Depth %2d: move=%-6s score=%4s cp  nodes=%6s  seldepth=%2s\n" \
           "$depth" "$bestmove" "$score" "$nodes" "$seldepth"
done

echo ""
echo "================================================"
echo "Comparison with Baseline (Before Fix)"
echo "================================================"
echo ""
echo "Baseline at depth 4-6:"
echo "  Depth 4: move=e8f7   score=134 cp  nodes=4568"
echo "  Depth 6: move=e8f7   score=158 cp  nodes=8214"
echo ""
echo "Key Questions:"
echo "1. Does e8f7 still appear at depth 4+?"
echo "2. Are node counts reduced?"
echo "3. Is the score better (higher)?"
echo ""

# Test time-limited search
echo "================================================"
echo "Time-Limited Search Test (100ms)"
echo "================================================"
result=$(echo -e "position fen $PROBLEM_FEN\ngo movetime 100" | $BINARY 2>&1)
final_depth=$(echo "$result" | grep "info depth" | tail -1 | grep -o "depth [0-9]*" | awk '{print $2}')
bestmove=$(echo "$result" | grep "bestmove" | awk '{print $2}')
final_score=$(echo "$result" | grep "info depth" | tail -1 | grep -o "score cp [0-9-]*" | awk '{print $3}')
total_nodes=$(echo "$result" | grep "info depth" | tail -1 | grep -o "nodes [0-9]*" | awk '{print $2}')

echo "  Final depth: $final_depth"
echo "  Best move: $bestmove"
echo "  Final score: $final_score cp"
echo "  Total nodes: $total_nodes"
echo ""
echo "(Baseline was: depth 10, move e8f7, 140 cp, 74789 nodes)"