#!/bin/bash
# Quick SEE comparison test - Stage 15 vs Stage 14

echo "=========================================="
echo "Quick SEE Comparison Test"
echo "=========================================="
echo ""

STAGE15="/workspace/binaries/seajay_stage15_sprt_candidate1"
STAGE14="/workspace/binaries/seajay-stage14-final"

# Test 1: Tactical position - should show SEE impact
TACTICAL_FEN="r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -"

echo "=== Tactical Position Test ==="
echo "FEN: $TACTICAL_FEN"
echo ""

echo "Stage 14 (No SEE):"
echo -e "position fen $TACTICAL_FEN\ngo depth 10\nquit" | $STAGE14 2>/dev/null | grep -E "info depth 10|bestmove" | head -2
echo ""

echo "Stage 15 (SEE OFF):"
echo -e "uci\nsetoption name SEEMode value off\nposition fen $TACTICAL_FEN\ngo depth 10\nquit" | $STAGE15 2>/dev/null | grep -E "info depth 10|bestmove" | head -2
echo ""

echo "Stage 15 (SEE PRODUCTION + Aggressive Pruning):"
echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value aggressive\nposition fen $TACTICAL_FEN\ngo depth 10\nquit" | $STAGE15 2>/dev/null | grep -E "info depth 10|bestmove|SEE pruning" | head -3
echo ""

# Test 2: Position with bad captures
BAD_CAPTURE_FEN="rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -"

echo "=== Bad Capture Position Test ==="
echo "FEN: $BAD_CAPTURE_FEN"
echo "(Black bishop on c5 - taking it with Bxc5 loses material)"
echo ""

echo "Stage 14 analysis (depth 8):"
echo -e "position fen $BAD_CAPTURE_FEN\ngo depth 8\nquit" | $STAGE14 2>/dev/null | grep "info depth 8" | head -1
echo ""

echo "Stage 15 with SEE (depth 8):"
echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value aggressive\nposition fen $BAD_CAPTURE_FEN\ngo depth 8\nquit" | $STAGE15 2>/dev/null | grep "info depth 8" | head -1
echo ""

# Test 3: Node count comparison
echo "=== Node Count Comparison (Starting Position, Depth 8) ==="
echo ""

echo "Stage 14 nodes:"
NODES_14=$(echo -e "position startpos\ngo depth 8\nquit" | $STAGE14 2>/dev/null | grep "info depth 8" | tail -1 | grep -o "nodes [0-9]*" | awk '{print $2}')
echo "Nodes: $NODES_14"
echo ""

echo "Stage 15 (SEE OFF) nodes:"
NODES_15_OFF=$(echo -e "uci\nsetoption name SEEMode value off\nposition startpos\ngo depth 8\nquit" | $STAGE15 2>/dev/null | grep "info depth 8" | tail -1 | grep -o "nodes [0-9]*" | awk '{print $2}')
echo "Nodes: $NODES_15_OFF"
echo ""

echo "Stage 15 (SEE PRODUCTION + Aggressive) nodes:"
NODES_15_SEE=$(echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value aggressive\nposition startpos\ngo depth 8\nquit" | $STAGE15 2>/dev/null | grep "info depth 8" | tail -1 | grep -o "nodes [0-9]*" | awk '{print $2}')
echo "Nodes: $NODES_15_SEE"
echo ""

# Calculate difference
if [ -n "$NODES_14" ] && [ -n "$NODES_15_OFF" ] && [ -n "$NODES_15_SEE" ]; then
    echo "Analysis:"
    echo "- Stage 15 with SEE OFF should be similar to Stage 14: $NODES_15_OFF vs $NODES_14"
    
    if [ "$NODES_15_SEE" -lt "$NODES_14" ]; then
        REDUCTION=$(echo "scale=2; (1 - $NODES_15_SEE / $NODES_14) * 100" | bc)
        echo "- Stage 15 with SEE reduces nodes by: ${REDUCTION}%"
    fi
fi

echo ""
echo "=========================================="
echo "Checking Binary Integrity"
echo "=========================================="
echo ""

echo "Stage 14 binary:"
ls -la $STAGE14
md5sum $STAGE14
echo ""

echo "Stage 15 binary:"
ls -la $STAGE15
md5sum $STAGE15
echo ""

# Verify SEE is compiled in
echo "Checking Stage 15 has SEE symbols:"
nm $STAGE15 2>/dev/null | grep -i "see" | head -5
if [ $? -eq 0 ]; then
    echo "✓ SEE symbols found in binary"
else
    echo "✗ WARNING: No SEE symbols found!"
fi