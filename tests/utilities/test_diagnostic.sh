#!/bin/bash

# Test at a specific position
POSITION="position fen r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 5"

echo "Testing move counts at depth 1:"
echo ""
echo "WITHOUT RankedMovePicker:"
echo -e "setoption name UseRankedMovePicker value false\n$POSITION\ngo depth 1" | ./bin/seajay 2>&1 | grep -E "nodes|bestmove"

echo ""
echo "WITH RankedMovePicker:"
echo -e "setoption name UseRankedMovePicker value true\n$POSITION\ngo depth 1" | ./bin/seajay 2>&1 | grep -E "nodes|bestmove"
