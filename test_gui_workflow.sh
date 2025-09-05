#!/bin/bash
# Test GUI analysis workflow

ENGINE="./bin/seajay"

echo "Testing GUI analysis workflow..."

# Test 1: Start analysis, then stop
echo "Test 1: Start infinite analysis and stop"
(
    echo "uci"
    echo "position startpos moves e2e4"
    echo "go infinite"
    sleep 2
    echo "stop"
    sleep 0.5
    echo "quit"
) | $ENGINE 2>&1 | grep -E "(bestmove|info depth|stopped)" | head -20

echo ""
echo "Test 2: Rapid position changes (GUI game review)"
(
    echo "uci"
    echo "position startpos"
    echo "go infinite"
    sleep 1
    echo "stop"
    echo "position startpos moves e2e4"  
    echo "go infinite"
    sleep 1
    echo "stop"
    echo "position startpos moves e2e4 e7e5"
    echo "go infinite"
    sleep 1
    echo "stop"
    echo "quit"
) | $ENGINE 2>&1 | grep -E "(bestmove|position|stop)" | tail -15

echo ""
echo "Test 3: Stop and restart"
(
    echo "uci"
    echo "position startpos"
    echo "go infinite"
    sleep 1
    echo "stop"
    sleep 0.5
    echo "go infinite"
    sleep 1
    echo "stop"
    echo "quit"
) | $ENGINE 2>&1 | grep -E "(bestmove|info depth [1-3] )" | head -10
