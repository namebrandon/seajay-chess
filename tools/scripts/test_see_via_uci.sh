#!/bin/bash
# Test Stage 15 SEE functionality via UCI interface
# This script verifies that SEE is active and working correctly

echo "=============================================="
echo "Stage 15 SEE Functionality Test via UCI"
echo "=============================================="
echo ""

BINARY="/workspace/binaries/seajay_stage15_sprt_candidate1"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found: $BINARY"
    exit 1
fi

echo "Testing binary: $BINARY"
echo ""

# Test 1: Basic UCI and version check
echo "=== Test 1: Version and UCI Options ==="
echo -e "uci\nquit" | $BINARY 2>/dev/null | grep -E "id name|option name SEE"
echo ""

# Test 2: Set SEE options and verify acceptance
echo "=== Test 2: Setting SEE Options ==="
{
    echo "uci"
    echo "setoption name SEEMode value production"
    echo "setoption name SEEPruning value aggressive"
    echo "isready"
    echo "quit"
} | $BINARY 2>&1 | grep -E "SEE|readyok|info string"
echo ""

# Test 3: Test position with obvious capture sequence
echo "=== Test 3: Position with Clear Capture Sequence ==="
echo "Position: r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - (Italian Game)"
echo "Expected: SEE should evaluate Bxf7+ as winning material"
echo ""
{
    echo "uci"
    echo "setoption name SEEMode value production"
    echo "setoption name SEEPruning value aggressive"
    echo "isready"
    echo "position fen r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -"
    echo "go depth 8"
    sleep 2
    echo "quit"
} | $BINARY 2>/dev/null | grep -E "info depth|bestmove|SEE"
echo ""

# Test 4: Compare move ordering with SEE off vs on
echo "=== Test 4: Move Ordering Comparison ==="
echo "Testing position with multiple captures..."
echo ""

echo "--- With SEE Mode OFF ---"
{
    echo "uci"
    echo "setoption name SEEMode value off"
    echo "isready"
    echo "position fen rnbqkb1r/pp1ppppp/5n2/2p5/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -"
    echo "go depth 6"
    sleep 1
    echo "quit"
} | $BINARY 2>/dev/null | grep "info depth 6" | head -1
echo ""

echo "--- With SEE Mode PRODUCTION ---"
{
    echo "uci"
    echo "setoption name SEEMode value production"
    echo "isready"
    echo "position fen rnbqkb1r/pp1ppppp/5n2/2p5/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -"
    echo "go depth 6"
    sleep 1
    echo "quit"
} | $BINARY 2>/dev/null | grep "info depth 6" | head -1
echo ""

# Test 5: Test position with bad capture that SEE should prune
echo "=== Test 5: Bad Capture Detection ==="
echo "Position where queen takes defended pawn (should be pruned by SEE)"
echo "FEN: rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPPQPPP/RNB1KBNR b KQkq -"
echo "Black queen can take e4 but it's defended - SEE should score this negatively"
echo ""
{
    echo "uci"
    echo "setoption name SEEMode value production"
    echo "setoption name SEEPruning value aggressive"
    echo "isready"
    echo "position fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPPQPPP/RNB1KBNR b KQkq -"
    echo "go depth 8"
    sleep 2
    echo "quit"
} | $BINARY 2>/dev/null | grep -E "info depth [678]|bestmove"
echo ""

# Test 6: Performance metrics with SEE
echo "=== Test 6: SEE Performance Metrics ==="
echo "Running longer search to see SEE statistics..."
{
    echo "uci"
    echo "setoption name SEEMode value production"
    echo "setoption name SEEPruning value aggressive"
    echo "isready"
    echo "position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 g8f6 d2d3 f8c5"
    echo "go depth 10"
    sleep 3
    echo "quit"
} | $BINARY 2>/dev/null | grep -E "info depth|nps|nodes|bestmove"
echo ""

# Test 7: Specific SEE test positions
echo "=== Test 7: Known SEE Test Positions ==="
echo ""

echo "Test 7a: Simple pawn takes pawn (equal exchange)"
echo "Position: rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -"
{
    echo "uci"
    echo "setoption name SEEMode value production"
    echo "isready"
    echo "position fen rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -"
    echo "go depth 6"
    sleep 1
    echo "quit"
} | $BINARY 2>/dev/null | grep -E "bestmove|info depth 6"
echo ""

echo "Test 7b: Complex tactical position"
echo "Position: r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq -"
{
    echo "uci"
    echo "setoption name SEEMode value production"
    echo "setoption name SEEPruning value aggressive"
    echo "isready"
    echo "position fen r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq -"
    echo "go depth 8"
    sleep 2
    echo "quit"
} | $BINARY 2>/dev/null | grep -E "bestmove|info depth [678]"
echo ""

# Test 8: Verify SEE modes work
echo "=== Test 8: All SEE Modes ==="
for mode in off testing shadow production; do
    echo "--- Testing SEEMode=$mode ---"
    {
        echo "uci"
        echo "setoption name SEEMode value $mode"
        echo "isready"
        echo "position startpos"
        echo "go depth 5"
        sleep 0.5
        echo "quit"
    } | $BINARY 2>/dev/null | grep "info depth 5" | head -1
done
echo ""

# Test 9: Verify SEE pruning modes
echo "=== Test 9: SEE Pruning Modes ==="
for pruning in off conservative aggressive; do
    echo "--- Testing SEEPruning=$pruning ---"
    {
        echo "uci"
        echo "setoption name SEEMode value production"
        echo "setoption name SEEPruning value $pruning"
        echo "isready"
        echo "position startpos moves e2e4 e7e5 g1f3 b8c6"
        echo "go depth 6"
        sleep 0.5
        echo "quit"
    } | $BINARY 2>/dev/null | grep "info depth 6" | head -1
done
echo ""

echo "=============================================="
echo "SEE Functionality Test Complete"
echo "=============================================="
echo ""
echo "What to look for:"
echo "1. SEE options should be visible in UCI output"
echo "2. Different SEE modes should produce slightly different search results"
echo "3. Aggressive pruning should reduce node counts"
echo "4. Move ordering should improve with SEE enabled"
echo "5. No crashes or errors when using SEE options"
echo ""
echo "To compare with Stage 14 (no SEE):"
echo "  Replace $BINARY with /workspace/binaries/seajay-stage14-final"
echo "  Stage 14 should not have SEE options and may search more nodes"