#!/bin/bash
# Diagnose why SEE isn't showing expected improvement

echo "============================================"
echo "SEE Performance Diagnosis"
echo "============================================"
echo ""

STAGE15="/workspace/binaries/seajay_stage15_sprt_candidate1"
STAGE14="/workspace/binaries/seajay-stage14-final"

# Test 1: Verify SEE is actually being used in search
echo "=== Test 1: SEE Usage in Search ==="
echo "Searching a tactical position for 5 seconds..."
echo ""

POSITION="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"

echo "Stage 15 with SEE:"
(echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value aggressive\nposition fen $POSITION\ngo movetime 5000\nquit") | timeout 7 $STAGE15 2>&1 | grep -E "SEE pruning|nodes|nps|depth [8-9]|depth 1[0-9]" | tail -5
echo ""

# Test 2: Check if SEE thresholds might be too aggressive
echo "=== Test 2: SEE Pruning Thresholds ==="
echo ""

echo "Conservative pruning (threshold -100):"
(echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value conservative\nposition startpos\ngo movetime 2000\nquit") | timeout 3 $STAGE15 2>&1 | grep "SEE pruning" | tail -1
echo ""

echo "Aggressive pruning (threshold -50):"
(echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value aggressive\nposition startpos\ngo movetime 2000\nquit") | timeout 3 $STAGE15 2>&1 | grep "SEE pruning" | tail -1
echo ""

# Test 3: Move ordering quality check
echo "=== Test 3: Move Ordering Quality ==="
echo "Position with obvious capture (Bxf7+):"
echo ""

CAPTURE_POS="r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -"

echo "Stage 14 first moves at depth 1:"
(echo -e "position fen $CAPTURE_POS\ngo depth 1\nquit") | $STAGE14 2>/dev/null | grep "info depth 1" | head -1
echo ""

echo "Stage 15 (SEE ON) first moves at depth 1:"
(echo -e "uci\nsetoption name SEEMode value production\nposition fen $CAPTURE_POS\ngo depth 1\nquit") | $STAGE15 2>/dev/null | grep "info depth 1" | head -1
echo ""

# Test 4: Performance overhead
echo "=== Test 4: Performance Overhead ==="
echo "Fixed depth search from startpos..."
echo ""

echo "Stage 14 NPS (depth 7):"
NPS14=$(echo -e "position startpos\ngo depth 7\nquit" | timeout 10 $STAGE14 2>/dev/null | grep "info depth 7" | tail -1 | grep -o "nps [0-9]*" | awk '{print $2}')
echo "NPS: $NPS14"
echo ""

echo "Stage 15 SEE OFF NPS (depth 7):"
NPS15_OFF=$(echo -e "uci\nsetoption name SEEMode value off\nposition startpos\ngo depth 7\nquit" | timeout 10 $STAGE15 2>/dev/null | grep "info depth 7" | tail -1 | grep -o "nps [0-9]*" | awk '{print $2}')
echo "NPS: $NPS15_OFF"
echo ""

echo "Stage 15 SEE ON NPS (depth 7):"
NPS15_ON=$(echo -e "uci\nsetoption name SEEMode value production\nsetoption name SEEPruning value aggressive\nposition startpos\ngo depth 7\nquit" | timeout 10 $STAGE15 2>/dev/null | grep "info depth 7" | tail -1 | grep -o "nps [0-9]*" | awk '{print $2}')
echo "NPS: $NPS15_ON"
echo ""

# Test 5: Specific problematic positions
echo "=== Test 5: Known Problem Positions ==="
echo ""

# Position where SEE might misevaluate
echo "Testing position with pinned piece:"
PINNED="r3k2r/p1ppqpb1/Bn2pnp1/3PN3/1p2P3/2N2Q2/PPPB1PPP/R3K2R b KQkq -"
echo "Position: Black bishop on a6 is pinned"
echo ""

echo "Stage 15 SEE evaluation (should not take bishop):"
(echo -e "uci\nsetoption name SEEMode value production\nposition fen $PINNED\ngo depth 6\nquit") | timeout 5 $STAGE15 2>/dev/null | grep -E "bestmove|info depth 6" | tail -2
echo ""

# Test 6: Check for bugs in special moves
echo "=== Test 6: Special Moves (En Passant, Castling) ==="
echo ""

# En passant position
EP_POS="rnbqkbnr/pppp1p1p/8/3Pp3/8/8/PPP1PPPP/RNBQKBNR w KQkq e6"
echo "En passant position - d5xe6 should be considered:"
(echo -e "uci\nsetoption name SEEMode value production\nposition fen $EP_POS\ngo depth 6\nquit") | timeout 3 $STAGE15 2>/dev/null | grep "bestmove"
echo ""

echo "============================================"
echo "Diagnosis Complete"
echo "============================================"
echo ""
echo "Things to check:"
echo "1. Is SEE pruning too aggressive? (>50% pruned might be too much)"
echo "2. Is there a performance regression? (NPS should be similar)"
echo "3. Are special moves handled correctly?"
echo "4. Is move ordering actually improved?"
echo ""