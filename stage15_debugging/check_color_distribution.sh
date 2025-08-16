#!/bin/bash
# Check who plays what color in Stage 14 vs Stage 15 games

echo "Checking color distribution in Stage 14 vs Stage 15 games"
echo "========================================================="

# Find the most recent Stage 14 vs Stage 15 games
LATEST_PGN=$(find /workspace/sprt_results -name "games.pgn" -mtime -1 2>/dev/null | head -1)

if [ -z "$LATEST_PGN" ]; then
    LATEST_PGN="/workspace/sprt_results/stage15_tuned_vs_stage14/games.pgn"
fi

if [ -f "$LATEST_PGN" ]; then
    echo "Analyzing: $LATEST_PGN"
    echo ""
    
    # Count games by who plays White
    echo "When Stage 14 plays White:"
    grep -B2 '\[Result' "$LATEST_PGN" | grep -A1 '\[White "Stage14' | grep Result | sort | uniq -c
    
    echo ""
    echo "When Stage 15 plays White:"
    grep -B2 '\[Result' "$LATEST_PGN" | grep -A1 '\[White "Stage15' | grep Result | sort | uniq -c
    
    echo ""
    echo "Overall results:"
    grep '\[Result' "$LATEST_PGN" | sort | uniq -c
else
    echo "No recent PGN files found"
fi

echo ""
echo "Key question: Is White winning regardless of which engine plays it?"
echo "Or is one engine winning regardless of color?"