#!/bin/bash
# Test evaluation discrepancy between Stage 14 and Stage 15

echo "Testing Evaluation Bug - Stage 14 vs Stage 15"
echo "=============================================="
echo ""

# Test opening position after 1.e4 c5 (where bug appears)
TEST_FEN="rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2"

echo "Position: After 1.e4 c5 (Sicilian Defense)"
echo "FEN: $TEST_FEN"
echo ""

# Test Stage 14 (baseline)
echo "Stage 14 Final (baseline):"
echo -e "position fen $TEST_FEN\neval\nquit" | /workspace/binaries/seajay-stage14-final 2>/dev/null | grep -A5 "evaluation"
echo ""

# Test Stage 15 original (before PST fixes)
echo "Stage 15 Original (pre-PST fixes):"
if [ -f /workspace/binaries/seajay_stage15_sprt_candidate1 ]; then
    echo -e "position fen $TEST_FEN\neval\nquit" | /workspace/binaries/seajay_stage15_sprt_candidate1 2>/dev/null | grep -A5 "evaluation"
else
    echo "Binary not found"
fi
echo ""

# Test Stage 15 with PST fixes but no tuning
echo "Stage 15 PST Fixed (no tuning):"
if [ -f /workspace/binaries/seajay_stage15_bias_bugfix2 ]; then
    echo -e "position fen $TEST_FEN\neval\nquit" | /workspace/binaries/seajay_stage15_bias_bugfix2 2>/dev/null | grep -A5 "evaluation"
else
    echo "Binary not found"
fi
echo ""

# Test Stage 15 fully tuned and fixed
echo "Stage 15 Tuned & Fixed (current):"
echo -e "position fen $TEST_FEN\neval\nquit" | /workspace/binaries/seajay_stage15_tuned_fixed 2>/dev/null | grep -A5 "evaluation"
echo ""

echo "=============================================="
echo "Expected: All should show evaluation around +0.10 to +0.30"
echo "Bug: If any show +2.90 or -2.90, that's the problem"
echo ""

# Also test a deeper position where the expert saw the bug
echo "Testing position from actual game (deeper in opening):"
TEST_MOVES="e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3"
echo "Moves: 1.e4 c5 2.Nf3 d6 3.d4 cxd4 4.Nxd4 Nf6 5.Nc3"
echo ""

echo "Stage 14 Final:"
echo -e "position startpos moves $TEST_MOVES\neval\nquit" | /workspace/binaries/seajay-stage14-final 2>/dev/null | grep -A5 "evaluation"
echo ""

echo "Stage 15 Tuned & Fixed:"
echo -e "position startpos moves $TEST_MOVES\neval\nquit" | /workspace/binaries/seajay_stage15_tuned_fixed 2>/dev/null | grep -A5 "evaluation"
echo ""