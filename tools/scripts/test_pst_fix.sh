#!/bin/bash
echo "Testing PST Fix - 10 games from startpos"
echo "========================================="

/workspace/external/testers/fast-chess/fastchess \
    -engine cmd="/workspace/binaries/seajay_stage15_pst_bugfix" name=Fixed-Stage15 \
    -engine cmd="/workspace/binaries/seajay_stage15_pst_bugfix" name=Fixed-Stage15-2 \
    -each tc=5+0.05 \
    -games 2 -rounds 5 -repeat -concurrency 1 \
    -pgnout file="/workspace/test_pst_fix.pgn" \
    -recover 2>/dev/null | grep -E "^(Started|Finished|Results)"

echo ""
echo "Checking results:"
wins_white=$(grep "Result \"1-0\"" /workspace/test_pst_fix.pgn | wc -l)
wins_black=$(grep "Result \"0-1\"" /workspace/test_pst_fix.pgn | wc -l)
draws=$(grep "Result \"1/2-1/2\"" /workspace/test_pst_fix.pgn | wc -l)
echo "White wins: $wins_white"
echo "Black wins: $wins_black"
echo "Draws: $draws"
