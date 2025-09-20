#!/bin/bash
# Test WAC.001 position correctly

FEN="2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - -"

echo "Testing WAC.001: $FEN"
echo "Expected best move: Qg6 (g3g6)"
echo ""

echo "With legacy ordering (depth 8):"
(echo "position fen $FEN"; echo "go depth 8") | ./bin/seajay 2>&1 | grep -E "bestmove|info depth 8"

echo ""
echo "With RankedMovePicker (depth 8):"
(echo "setoption name UseRankedMovePicker value true"; echo "position fen $FEN"; echo "go depth 8") | ./bin/seajay 2>&1 | grep -E "bestmove|info depth 8"
