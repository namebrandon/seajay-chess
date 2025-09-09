#!/bin/bash

echo "Testing move generation differences:"
echo ""
echo "WITHOUT RankedMovePicker (depth 8):"
echo -e "setoption name UseRankedMovePicker value false\nposition startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6\ngo depth 8" | ./bin/seajay 2>&1 | grep -E "score cp|bestmove" | tail -2

echo ""
echo "WITH RankedMovePicker (depth 8):"
echo -e "setoption name UseRankedMovePicker value true\nposition startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6\ngo depth 8" | ./bin/seajay 2>&1 | grep -E "score cp|bestmove" | tail -2
