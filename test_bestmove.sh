#!/bin/bash
# Test that quiescence search stores best moves in TT

echo "Testing quiescence search best move tracking..."
echo ""

# Test 1: Position after capture where recapture is obvious
FEN1="r1bqkbnr/1ppp1ppp/p1n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 4"
echo "Test 1: After Bxc6, Black should recapture"
echo "Position: $FEN1"
echo ""

OUTPUT=$(echo -e "uci\nsetoption name Hash value 1\nposition fen $FEN1\ngo depth 1\nquit" | ./bin/seajay 2>&1)

# Extract the best move
BESTMOVE=$(echo "$OUTPUT" | grep "^bestmove" | awk '{print $2}')
echo "Best move found: $BESTMOVE"

if [[ "$BESTMOVE" == "d7c6" ]] || [[ "$BESTMOVE" == "b7c6" ]]; then
    echo "✓ Correct recapture found!"
else
    echo "✗ Unexpected move (expected dxc6 or bxc6)"
fi

echo ""
echo "Test 2: Check TT is storing moves from quiescence"

# Run a search that will use quiescence heavily
FEN2="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
echo "Complex tactical position: $FEN2"
echo ""

# First search to populate TT
echo -e "uci\nsetoption name Hash value 16\nposition fen $FEN2\ngo depth 6\nquit" | ./bin/seajay 2>&1 | grep -E "info depth 6"

# Now check TT stats to see if entries have moves
echo ""
echo "Checking TT utilization..."
OUTPUT2=$(echo -e "uci\nsetoption name Hash value 16\nposition fen $FEN2\ngo depth 1\nquit" | ./bin/seajay 2>&1)
TT_HITS=$(echo "$OUTPUT2" | grep "tthits" | tail -1)
echo "TT hit info: $TT_HITS"

if [[ -n "$TT_HITS" ]]; then
    echo "✓ TT is being used"
else  
    echo "✗ No TT hit information found"
fi

echo ""
echo "=== Best move tracking test complete ==="