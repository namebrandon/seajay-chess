#!/bin/bash
# Debug what's happening with RankedMovePicker

FEN="2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - -"
echo "Position: WAC.001"
echo "FEN: $FEN"
echo "Best move should be: Qg6"
echo ""

# First verify the position is correct
echo "Checking if Qg6 is a legal move in this position:"
(echo "uci"; echo "position fen $FEN"; echo "go perft 1"; echo "quit") | ./bin/seajay 2>&1 | grep "g3g6"

echo ""
echo "Testing at depth 10 with legacy ordering:"
(echo "uci"; echo "position fen $FEN"; echo "go depth 10") | ./bin/seajay 2>&1 | grep -E "depth 10|bestmove" | tail -2

echo ""
echo "Testing at depth 10 with RankedMovePicker:"
(echo "uci"; echo "setoption name UseRankedMovePicker value true"; echo "position fen $FEN"; echo "go depth 10") | ./bin/seajay 2>&1 | grep -E "depth 10|bestmove" | tail -2
