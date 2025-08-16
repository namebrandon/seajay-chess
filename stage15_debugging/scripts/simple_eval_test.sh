#!/bin/bash
# Simple evaluation test using go perft with eval output

echo "Simple Evaluation Test - Stage 14 vs Stage 15"
echo "=============================================="
echo ""

# Test simple position
echo "Testing after 1.e4 c5:"
echo ""

echo "Stage 14 Final:"
echo -e "uci\nposition startpos moves e2e4 c7c5\ngo depth 1\nquit" | /workspace/binaries/seajay-stage14-final 2>&1 | grep -E "score cp|bestmove"

echo ""
echo "Stage 15 Tuned:"  
echo -e "uci\nposition startpos moves e2e4 c7c5\ngo depth 1\nquit" | /workspace/binaries/seajay_stage15_tuned_fixed 2>&1 | grep -E "score cp|bestmove"

echo ""
echo "=============================================="
echo "Look for score differences (cp = centipawns)"
echo "Normal should be around +20 to +30 cp"
echo "Bug would show +290 cp or similar"