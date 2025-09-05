#!/bin/bash
# Test that bestmove is always sent after stop command

ENGINE="./bin/seajay"

echo "=== Testing bestmove on stop fix ==="
echo ""

echo "Test 1: Check that bestmove is sent after stop"
(
    echo "uci"
    sleep 0.1
    echo "position startpos"
    echo "go infinite"
    sleep 0.5
    echo "stop"
    sleep 0.5
    echo "quit"
) | $ENGINE 2>&1 | grep -E "(go infinite|stop|bestmove)" | head -5

echo ""
echo "Test 2: Multiple stop/go cycles"
(
    echo "uci"
    sleep 0.1
    echo "position startpos"
    echo "go infinite"
    sleep 0.3
    echo "stop"
    sleep 0.1
    echo "position startpos moves e2e4"
    echo "go infinite"
    sleep 0.3
    echo "stop"
    sleep 0.1
    echo "position startpos moves e2e4 e7e5"
    echo "go infinite"
    sleep 0.3
    echo "stop"
    sleep 0.1
    echo "quit"
) | $ENGINE 2>&1 | grep "bestmove" | head -5

echo ""
echo "Test 3: Verify position changes work after stop"
result=$(
    (
        echo "uci"
        echo "isready"
        sleep 0.1
        echo "position startpos"
        echo "go infinite"
        sleep 0.5
        echo "stop"
        sleep 0.2
        echo "position startpos moves e2e4"
        echo "go infinite"
        sleep 0.5
        echo "stop"
        sleep 0.2
        echo "quit"
    ) | $ENGINE 2>&1 | grep -c "bestmove"
)
echo "Number of bestmove responses: $result"
if [ "$result" -eq "2" ]; then
    echo "✓ PASS: Both go commands received bestmove responses"
else
    echo "✗ FAIL: Expected 2 bestmove responses, got $result"
fi