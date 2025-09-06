#!/bin/bash

# Test different MaxCheckPly values on the problem position
BINARY="./bin/seajay"
PROBLEM_FEN="r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17"

echo "Testing MaxCheckPly Values on Problem Position"
echo "=============================================="
echo ""

# Test each MaxCheckPly value from 1 to 6
for checkply in 1 2 3 4 5 6; do
    echo "MaxCheckPly = $checkply"
    echo "------------------------"
    
    # Test at depths 4, 6, 8 (key depths for the problem)
    for depth in 4 6 8; do
        result=$(echo -e "setoption name MaxCheckPly value $checkply\nposition fen $PROBLEM_FEN\ngo depth $depth" | $BINARY 2>&1)
        
        # Extract metrics
        bestmove=$(echo "$result" | grep "bestmove" | tail -1 | awk '{print $2}')
        info_line=$(echo "$result" | grep "info depth $depth " | tail -1)
        score=$(echo "$info_line" | grep -o "score cp [0-9-]*" | awk '{print $3}')
        nodes=$(echo "$info_line" | grep -o "nodes [0-9]*" | awk '{print $2}')
        seldepth=$(echo "$info_line" | grep -o "seldepth [0-9]*" | awk '{print $2}')
        
        echo "  Depth $depth: move=$bestmove score=${score}cp nodes=$nodes seldepth=$seldepth"
    done
    echo ""
done

# Summary comparison at depth 6
echo "=============================================="
echo "SUMMARY: Depth 6 Comparison"
echo "=============================================="
echo "MaxCheckPly | Best Move | Score | Nodes"
echo "------------|-----------|-------|-------"

for checkply in 1 2 3 4 5 6; do
    result=$(echo -e "setoption name MaxCheckPly value $checkply\nposition fen $PROBLEM_FEN\ngo depth 6" | $BINARY 2>&1)
    bestmove=$(echo "$result" | grep "bestmove" | tail -1 | awk '{print $2}')
    info_line=$(echo "$result" | grep "info depth 6 " | tail -1)
    score=$(echo "$info_line" | grep -o "score cp [0-9-]*" | awk '{print $3}')
    nodes=$(echo "$info_line" | grep -o "nodes [0-9]*" | awk '{print $2}')
    
    printf "%-11d | %-9s | %-5s | %s\n" "$checkply" "$bestmove" "$score" "$nodes"
done