#!/bin/bash
# Test WAC.001 position in UCI mode

FEN="2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - -"

echo "Testing WAC.001: Expected Qg6"
echo ""

echo "Legacy ordering:"
(echo "uci"; echo "position fen $FEN"; echo "go depth 10") | ./bin/seajay 2>&1 | grep -E "bestmove|depth 10 "

echo ""
echo "RankedMovePicker:"
(echo "uci"; echo "setoption name UseRankedMovePicker value true"; echo "position fen $FEN"; echo "go depth 10") | ./bin/seajay 2>&1 | grep -E "bestmove|depth 10 "
