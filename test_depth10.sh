#!/bin/bash
echo "Depth 10 comparison:"
echo ""
echo "WITHOUT RankedMovePicker:"
echo -e "setoption name UseRankedMovePicker value false\nposition startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6\ngo depth 10" | ./bin/seajay 2>&1 | grep "bestmove"

echo ""
echo "WITH RankedMovePicker:"
echo -e "setoption name UseRankedMovePicker value true\nposition startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6\ngo depth 10" | ./bin/seajay 2>&1 | grep "bestmove"
