#!/bin/bash

# Perft validation script for SeaJay
# Tests standard positions to ensure move generation correctness

echo "==================================================="
echo "SeaJay Perft Validation Suite"
echo "==================================================="
echo ""

PERFT_TOOL="./bin/perft_tool"
FAILED=0
PASSED=0

# Test positions with known perft values
declare -a TESTS=(
    # Format: "FEN|depth|expected_nodes|description"
    "startpos|4|197281|Starting position depth 4"
    "startpos|5|4865609|Starting position depth 5"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -|3|97862|Kiwipete position depth 3"
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -|5|674624|Endgame position depth 5"
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -|4|422333|Complex position depth 4"
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -|4|2103487|Position with checks depth 4"
)

echo "Running perft tests..."
echo ""

for test in "${TESTS[@]}"; do
    IFS='|' read -r fen depth expected desc <<< "$test"
    
    echo -n "Testing: $desc... "
    
    # Run perft and capture result
    if [ "$fen" = "startpos" ]; then
        result=$($PERFT_TOOL startpos $depth 2>&1 | grep "Result:" | awk '{print $2}')
    else
        result=$($PERFT_TOOL "$fen" $depth 2>&1 | grep "Result:" | awk '{print $2}')
    fi
    
    if [ "$result" = "$expected" ]; then
        echo "✓ PASSED ($result nodes)"
        ((PASSED++))
    else
        echo "✗ FAILED (expected $expected, got $result)"
        ((FAILED++))
    fi
done

echo ""
echo "==================================================="
echo "Results: $PASSED passed, $FAILED failed"
echo "==================================================="

if [ $FAILED -eq 0 ]; then
    echo "All perft tests passed! Move generation is correct."
    exit 0
else
    echo "Some tests failed. Please check move generation."
    exit 1
fi