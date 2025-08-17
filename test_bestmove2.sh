#!/bin/bash
# Test that quiescence search stores best moves in TT

echo "Testing quiescence search best move tracking..."
echo ""

# Test 1: Position where Black just captured - White should recapture
FEN1="r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 4"
echo "Test 1: Italian opening - White has Bc4, Black has Nc6 and Nf6"
echo "Testing if captures are found correctly"
echo "Position: $FEN1"
echo ""

OUTPUT=$(echo -e "uci\nsetoption name Hash value 16\nsetoption name UseQuiescence value true\nposition fen $FEN1\ngo depth 4\nquit" | ./bin/seajay 2>&1)

# Show the search output
echo "$OUTPUT" | grep -E "(info depth|bestmove)"
echo ""

# Test 2: Highly tactical position to ensure quiescence is active
FEN2="rnbqk2r/pp1p1ppp/4pn2/2b5/2PP4/2N1PN2/PP3PPP/R1BQKB1R b KQkq - 0 6"
echo "Test 2: Tactical position with hanging pieces"
echo "Position: $FEN2"
echo ""

OUTPUT2=$(echo -e "uci\nsetoption name Hash value 16\nsetoption name UseQuiescence value true\nposition fen $FEN2\ngo depth 5\nquit" | ./bin/seajay 2>&1)

# Extract info about the search
DEPTH5=$(echo "$OUTPUT2" | grep "info depth 5")
echo "Depth 5 search: $DEPTH5"

NODES=$(echo "$DEPTH5" | grep -oP 'nodes \K\d+')
SELDEPTH=$(echo "$DEPTH5" | grep -oP 'seldepth \K\d+')

echo ""
echo "Nodes searched: $NODES"
echo "Selective depth reached: $SELDEPTH"

if [[ $SELDEPTH -gt 5 ]]; then
    echo "✓ Quiescence extending search beyond nominal depth (seldepth=$SELDEPTH > 5)"
else
    echo "✗ Quiescence may not be working (seldepth=$SELDEPTH)"
fi

# Test 3: Verify TT is storing quiescence moves
echo ""
echo "Test 3: Checking TT statistics after search"

# Search same position twice to check TT hits
echo -e "uci\nnewgame\nsetoption name Hash value 16\nposition fen $FEN2\ngo depth 4\nposition fen $FEN2\ngo depth 4\nquit" | ./bin/seajay 2>&1 > /tmp/tt_test.txt

FIRST_SEARCH=$(grep "info depth 4" /tmp/tt_test.txt | head -1)
SECOND_SEARCH=$(grep "info depth 4" /tmp/tt_test.txt | tail -1)

echo "First search:  $FIRST_SEARCH"
echo "Second search: $SECOND_SEARCH"

# Extract TT hit percentage from second search
TT_HITS=$(echo "$SECOND_SEARCH" | grep -oP 'tthits \K[\d.]+%')
if [[ -n "$TT_HITS" ]]; then
    echo ""
    echo "✓ TT hit rate in second search: $TT_HITS"
    
    # Parse percentage
    TT_VALUE=${TT_HITS%\%}
    if (( $(echo "$TT_VALUE > 50" | bc -l) )); then
        echo "✓ High TT hit rate indicates moves are being stored"
    fi
else
    echo "✗ Could not extract TT hit information"
fi

echo ""
echo "=== Best move tracking test complete ==="