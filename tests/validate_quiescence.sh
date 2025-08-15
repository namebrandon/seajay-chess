#!/bin/bash

# Quick validation of quiescence search functionality
# Tests a few key positions to verify quiescence is finding tactics

ENGINE="/workspace/bin/seajay"

if [ ! -f "$ENGINE" ]; then
    echo "Building engine first..."
    ./build_testing.sh
fi

echo "========================================="
echo "Quiescence Search Validation Test"
echo "========================================="
echo ""

# Test 1: Simple hanging piece (should find immediately with quiescence)
echo "Test 1: Hanging Piece Detection"
echo "Position: White bishop can capture hanging knight on f6"
echo "Expected: Bxf6"
FEN1="rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 4"
echo "Testing..."
RESULT1=$(echo -e "position fen $FEN1\ngo depth 1\nquit" | $ENGINE 2>/dev/null | grep "bestmove" | awk '{print $2}')
if [[ "$RESULT1" == *"f6"* ]]; then
    echo "✓ PASSED: Found capture $RESULT1"
else
    echo "✗ FAILED: Expected Bxf6, got $RESULT1"
fi
echo ""

# Test 2: Fork position (requires quiescence to see)
echo "Test 2: Knight Fork Detection"
echo "Position: Knight can fork king and rook"
echo "Expected: Nf7+ (forking king and rook)"
FEN2="r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 5"
echo "Testing at depth 3..."
RESULT2=$(echo -e "position fen $FEN2\ngo depth 3\nquit" | $ENGINE 2>/dev/null | grep "bestmove" | awk '{print $2}')
echo "Found move: $RESULT2"
echo ""

# Test 3: Back rank mate in 1
echo "Test 3: Back Rank Mate Detection"  
echo "Position: Back rank mate with Rd8#"
echo "Expected: Rd8# or Rd8+"
FEN3="6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1"
echo "Testing..."
RESULT3=$(echo -e "position fen $FEN3\ngo depth 1\nquit" | $ENGINE 2>/dev/null | grep "bestmove" | awk '{print $2}')
if [[ "$RESULT3" == *"d8"* ]]; then
    echo "✓ PASSED: Found mate $RESULT3"
else
    echo "✗ FAILED: Expected Rd8#, got $RESULT3"
fi
echo ""

# Get quiescence statistics
echo "========================================="
echo "Quiescence Search Statistics"
echo "========================================="
echo ""
echo "Analyzing a tactical position at depth 5..."
FEN_TACTICAL="r2qkb1r/pp1n1ppp/2p1pn2/3p4/2PP4/2N1PN2/PP3PPP/R1BQKB1R w KQkq - 0 7"

OUTPUT=$(echo -e "position fen $FEN_TACTICAL\ngo depth 5\nquit" | $ENGINE 2>/dev/null)

# Extract statistics
DEPTH=$(echo "$OUTPUT" | grep -oP "depth \K\d+" | tail -1)
SELDEPTH=$(echo "$OUTPUT" | grep -oP "seldepth \K\d+" | tail -1)
NODES=$(echo "$OUTPUT" | grep -oP "nodes \K\d+" | tail -1)
NPS=$(echo "$OUTPUT" | grep -oP "nps \K\d+" | tail -1)
TIME=$(echo "$OUTPUT" | grep -oP "time \K\d+" | tail -1)

echo "Search Statistics:"
echo "  Depth reached: $DEPTH"
echo "  Selective depth: $SELDEPTH"
echo "  Nodes searched: $NODES"
echo "  Nodes per second: $NPS"
echo "  Time: ${TIME}ms"
echo ""

# Check if quiescence is extending search
if [ -n "$SELDEPTH" ] && [ -n "$DEPTH" ] && [ "$SELDEPTH" -gt "$DEPTH" ]; then
    EXTENSION=$((SELDEPTH - DEPTH))
    echo "✓ Quiescence is working: Extended search by $EXTENSION ply"
else
    echo "⚠ Warning: Quiescence may not be extending search properly"
fi

echo ""
echo "========================================="
echo "Validation Complete"
echo "========================================="

# Show current build mode
echo ""
echo "Current Build Mode:"
$ENGINE 2>/dev/null | head -3 | grep -E "Quiescence|Stage"

exit 0