#!/bin/bash
# Test that RankedMovePicker generates all moves

# Test position: WAC.001
FEN="2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - -"

echo "Testing move generation for WAC.001 position..."
echo "Expected best move: Qg6"
echo ""

# Test with legacy ordering
echo "Legacy ordering:"
echo -e "position fen $FEN\ngo depth 8" | ./bin/seajay 2>&1 | grep "bestmove"

# Test with RankedMovePicker
echo ""
echo "RankedMovePicker:"
echo -e "setoption name UseRankedMovePicker value true\nposition fen $FEN\ngo depth 8" | ./bin/seajay 2>&1 | grep "bestmove"

# Let's also check what moves are considered at depth 1
echo ""
echo "Checking move generation at depth 1..."
echo -e "position fen $FEN\ngo depth 1" | ./bin/seajay 2>&1 | grep -E "info depth 1|bestmove"
