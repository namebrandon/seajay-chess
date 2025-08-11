#!/bin/bash

# Quick test to verify the search repetition fix is working

echo "=== Testing the repetition search fix ==="
echo "Binary timestamp: $(stat -c %y /workspace/build/seajay)"

# Run a quick 10-game match to see if the pattern changes
echo "Running quick 10-game test..."

cd /workspace

# Use fast time control for quick testing
timeout 60s /workspace/external/testers/fast-chess/fastchess \
  -engine cmd=/workspace/build/seajay name=Stage9b-SearchFix \
  -engine cmd=/workspace/build/seajay name=Stage9b-SearchFix-Copy \
  -each tc=0.1+0.01 \
  -rounds 5 \
  -repeat \
  -recover \
  -openings file=/workspace/external/books/varied_4moves.pgn format=pgn order=sequential \
  -pgnout games_quick_test.pgn \
  -concurrency 1

echo "Test completed!"
echo "Checking for repetition draws in the games:"
if grep -c "3-fold repetition" games_quick_test.pgn >/dev/null 2>&1; then
    echo "Found repetition draws:"
    grep "3-fold repetition" games_quick_test.pgn | head -5
else
    echo "No repetition draws found in quick test!"
fi