#!/bin/bash
# Exact GUI analysis workflow test - simulates what ChessBase/Arena/etc. does
# Tests the specific issue: engine doesn't respond after position changes during analysis

ENGINE="./bin/seajay"
LOG="gui_test.log"

echo "=== GUI Analysis Simulation Test ===" | tee $LOG
echo "Testing the exact workflow that fails in GUI applications" | tee -a $LOG
echo "" | tee -a $LOG

# Function to send UCI commands and capture output
send_commands() {
    local desc=$1
    shift
    echo "[$desc]" | tee -a $LOG
    {
        for cmd in "$@"; do
            echo ">>> $cmd" >&2
            echo "$cmd"
            if [[ "$cmd" == "go infinite" ]]; then
                sleep 2  # Let it analyze for 2 seconds
            elif [[ "$cmd" == "stop" ]]; then
                sleep 0.5  # Give time for stop to process
            else
                sleep 0.1  # Small delay between other commands
            fi
        done
    } | $ENGINE 2>&1 | while IFS= read -r line; do
        echo "<<< $line" | tee -a $LOG
    done
    echo "" | tee -a $LOG
}

# Test 1: Basic stop/go cycle (this usually works)
echo "TEST 1: Basic stop/go cycle" | tee -a $LOG
send_commands "Basic stop/go" \
    "uci" \
    "isready" \
    "position startpos" \
    "go infinite" \
    "stop" \
    "isready" \
    "quit"

# Test 2: The problematic GUI workflow - position change during analysis
echo "TEST 2: Position change during analysis (GUI game review)" | tee -a $LOG
(
    echo "uci"
    sleep 0.1
    echo "isready"
    sleep 0.1
    
    # Start analyzing initial position
    echo "position startpos moves e2e4"
    echo "go infinite"
    sleep 2
    
    # GUI moves to next position - this is where it fails
    echo "stop"
    sleep 0.5
    echo "position startpos moves e2e4 e7e5"
    echo "go infinite"  # <-- This often doesn't work properly
    sleep 2
    
    # Try to move to another position
    echo "stop"
    sleep 0.5
    echo "position startpos moves e2e4 e7e5 g1f3"
    echo "go infinite"  # <-- This usually never starts
    sleep 2
    
    echo "stop"
    echo "quit"
) | $ENGINE 2>&1 | tee -a $LOG | grep -E "(uci|ready|position|go |stop|bestmove|info depth [1-3] |Failed|Error)"

echo "" | tee -a $LOG

# Test 3: Rapid position switches (like clicking through a game quickly)
echo "TEST 3: Rapid position switches" | tee -a $LOG
(
    echo "uci"
    sleep 0.1
    echo "isready"
    sleep 0.1
    
    # Rapidly switch positions like a user clicking through moves
    for move_seq in "" "e2e4" "e2e4 e7e5" "e2e4 e7e5 g1f3" "e2e4 e7e5 g1f3 b8c6"; do
        echo "stop"  # Stop any ongoing search
        echo "position startpos moves $move_seq"
        echo "go infinite"
        sleep 0.5  # Very brief analysis
    done
    
    echo "stop"
    echo "quit"
) | $ENGINE 2>&1 | tee -a $LOG | grep -E "(position|go |stop|bestmove|Failed|Error)"

echo "" | tee -a $LOG

# Test 4: Stop without waiting for bestmove (common GUI behavior)
echo "TEST 4: Stop without waiting for bestmove" | tee -a $LOG
(
    echo "uci"
    sleep 0.1
    
    # Start and immediately change position (no wait for bestmove)
    echo "position startpos"
    echo "go infinite"
    sleep 0.3  # Very short time
    echo "stop"
    # Immediately set new position without waiting
    echo "position startpos moves e2e4"
    echo "go infinite"
    sleep 0.3
    echo "stop"
    # And again
    echo "position startpos moves e2e4 e7e5"
    echo "go infinite"
    sleep 0.3
    echo "stop"
    echo "quit"
) | $ENGINE 2>&1 | tee -a $LOG | grep -E "(position|go |stop|bestmove|info depth)"

echo "" | tee -a $LOG
echo "=== Test Summary ===" | tee -a $LOG
echo "Check the log file '$LOG' for detailed output" | tee -a $LOG
echo "Look for:" | tee -a $LOG
echo "  1. Missing 'bestmove' responses after 'stop'" | tee -a $LOG
echo "  2. No 'info' output after position changes" | tee -a $LOG
echo "  3. Engine not responding to 'go infinite' after stop" | tee -a $LOG