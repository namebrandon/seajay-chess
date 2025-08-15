#!/bin/bash
# Validate Stage 15 binaries before SPRT testing

echo "=========================================="
echo "Stage 15 Binary Validation"
echo "=========================================="
echo ""

# Test Stage 15 candidate
echo "=== Stage 15 SEE Candidate ==="
echo "Binary: /workspace/binaries/seajay_stage15_sprt_candidate1"
echo -e "position startpos\ngo depth 5\nquit" | /workspace/binaries/seajay_stage15_sprt_candidate1 2>/dev/null | grep -E "bestmove|info depth 5"
echo ""

# Test Stage 14 baseline
echo "=== Stage 14 Baseline ==="
echo "Binary: /workspace/binaries/seajay-stage14-final"
echo -e "position startpos\ngo depth 5\nquit" | /workspace/binaries/seajay-stage14-final 2>/dev/null | grep -E "bestmove|info depth 5"
echo ""

# Compare versions
echo "=== Version Information ==="
echo "Stage 15 version:"
echo "uci" | /workspace/binaries/seajay_stage15_sprt_candidate1 2>/dev/null | grep "id name"
echo ""
echo "Stage 14 version:"
echo "uci" | /workspace/binaries/seajay-stage14-final 2>/dev/null | grep "id name"
echo ""

# Test SEE options in Stage 15
echo "=== SEE Options Test (Stage 15 only) ==="
echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value aggressive\nquit" | /workspace/binaries/seajay_stage15_sprt_candidate1 2>&1 | grep -E "SEE|info string"
echo ""

echo "=== Validation Complete ==="
echo "If both engines produced bestmove outputs, they are ready for SPRT testing."