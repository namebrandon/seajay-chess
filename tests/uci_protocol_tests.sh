#!/bin/bash
# Comprehensive UCI Protocol Test Suite for SeaJay Chess Engine
# Based on QA Expert recommendations

set -e  # Exit on any error

SEAJAY_BIN="/workspace/bin/seajay"
TEST_RESULTS=""
TESTS_PASSED=0
TESTS_FAILED=0

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test result tracking
add_result() {
    local test_name="$1"
    local status="$2"
    local details="$3"
    
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}✓${NC} $test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}❌${NC} $test_name"
        if [ -n "$details" ]; then
            echo "   Details: $details"
        fi
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    
    TEST_RESULTS="${TEST_RESULTS}\n$status: $test_name"
}

# Test helper: Run UCI command and check response
test_uci_command() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"
    local timeout_seconds="${4:-2}"
    
    echo "Testing: $test_name"
    
    # Create temporary files
    local input_file=$(mktemp)
    local output_file=$(mktemp)
    
    # Write input to file
    echo -e "$input" > "$input_file"
    
    # Run SeaJay with timeout
    if timeout "$timeout_seconds" "$SEAJAY_BIN" < "$input_file" > "$output_file" 2>&1; then
        local output=$(cat "$output_file")
        
        if echo "$output" | grep -q "$expected_pattern"; then
            add_result "$test_name" "PASS"
        else
            add_result "$test_name" "FAIL" "Expected pattern '$expected_pattern' not found in output: $output"
        fi
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            add_result "$test_name" "FAIL" "Command timed out after $timeout_seconds seconds"
        else
            add_result "$test_name" "FAIL" "Command failed with exit code $exit_code"
        fi
    fi
    
    # Cleanup
    rm -f "$input_file" "$output_file"
}

# Test helper: Check response time
test_response_time() {
    local test_name="$1"
    local input="$2" 
    local max_time_ms="$3"
    
    echo "Testing response time: $test_name"
    
    local input_file=$(mktemp)
    echo -e "$input" > "$input_file"
    
    local start_time=$(date +%s%3N)
    "$SEAJAY_BIN" < "$input_file" > /dev/null 2>&1
    local end_time=$(date +%s%3N)
    
    local elapsed_ms=$((end_time - start_time))
    
    if [ $elapsed_ms -le $max_time_ms ]; then
        add_result "$test_name (${elapsed_ms}ms)" "PASS"
    else
        add_result "$test_name" "FAIL" "Response took ${elapsed_ms}ms, expected <${max_time_ms}ms"
    fi
    
    rm -f "$input_file"
}

echo "=== SeaJay Chess Engine - UCI Protocol Test Suite ==="
echo "Engine: $SEAJAY_BIN"
echo "Date: $(date)"
echo

# Check if engine exists
if [ ! -f "$SEAJAY_BIN" ]; then
    echo -e "${RED}ERROR:${NC} SeaJay binary not found at $SEAJAY_BIN"
    echo "Please build the engine first: cd /workspace/build && make -j"
    exit 1
fi

echo "=== Basic UCI Protocol Tests ==="

# Test 1: Basic UCI handshake
test_uci_command "UCI handshake" "uci\nquit" "id name SeaJay"

# Test 2: Engine identification
test_uci_command "Engine identification" "uci\nquit" "id author Brandon Harris"

# Test 3: UCI OK response
test_uci_command "UCI OK response" "uci\nquit" "uciok"

# Test 4: Ready check
test_uci_command "Ready check" "uci\nisready\nquit" "readyok"

# Test 5: Clean quit
test_uci_command "Clean quit" "uci\nquit" "uciok"

echo
echo "=== Position Setup Tests ==="

# Test 6: Starting position setup
test_uci_command "Starting position setup" "uci\nposition startpos\ngo movetime 100\nquit" "bestmove"

# Test 7: Position with moves
test_uci_command "Position with moves" "uci\nposition startpos moves e2e4 e7e5\ngo movetime 100\nquit" "bestmove"

# Test 8: FEN position setup
test_uci_command "FEN position setup" "uci\nposition fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\ngo movetime 100\nquit" "bestmove"

# Test 9: Complex FEN position
test_uci_command "Complex FEN position" "uci\nposition fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -\ngo movetime 100\nquit" "bestmove"

echo
echo "=== Time Control Tests ==="

# Test 10: Fixed movetime
test_uci_command "Fixed movetime" "uci\nposition startpos\ngo movetime 500\nquit" "bestmove"

# Test 11: Time and increment
test_uci_command "Time and increment" "uci\nposition startpos\ngo wtime 60000 btime 60000 winc 1000 binc 1000\nquit" "bestmove"

# Test 12: Fixed depth
test_uci_command "Fixed depth" "uci\nposition startpos\ngo depth 1\nquit" "bestmove"

echo
echo "=== Move Format Tests ==="

# Test 13: Normal move application
test_uci_command "Normal move application" "uci\nposition startpos moves e2e4\ngo movetime 100\nquit" "bestmove"

# Test 14: Capture move application  
test_uci_command "Capture move application" "uci\nposition startpos moves e2e4 d7d5 e4d5\ngo movetime 100\nquit" "bestmove"

# Test 15: Castling move application
test_uci_command "Castling move application" "uci\nposition startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 e1g1\ngo movetime 100\nquit" "bestmove"

echo
echo "=== Error Handling Tests ==="

# Test 16: Invalid command handling
test_uci_command "Invalid command handling" "invalid_command\nuci\nquit" "uciok"

# Test 17: Malformed position handling
test_uci_command "Malformed position handling" "uci\nposition invalid_fen\nposition startpos\ngo movetime 100\nquit" "bestmove"

# Test 18: Invalid move handling
test_uci_command "Invalid move handling" "uci\nposition startpos moves e2e9\nposition startpos\ngo movetime 100\nquit" "bestmove"

echo
echo "=== Performance Tests ==="

# Test 19: UCI command response time
test_response_time "UCI command response" "uci\nquit" 100

# Test 20: Ready check response time
test_response_time "Ready check response" "uci\nisready\nquit" 50

# Test 21: Quick move generation
test_response_time "Quick move generation" "uci\nposition startpos\ngo movetime 10\nquit" 500

echo
echo "=== Advanced Protocol Tests ==="

# Test 22: Multiple game simulation
test_uci_command "Multiple position changes" "uci\nposition startpos\ngo movetime 50\nposition startpos moves e2e4\ngo movetime 50\nposition startpos moves d2d4\ngo movetime 50\nquit" "bestmove"

# Test 23: Search info output
test_uci_command "Search info output" "uci\nposition startpos\ngo movetime 200\nquit" "info depth"

# Test 24: Principal variation output
test_uci_command "Principal variation output" "uci\nposition startpos\ngo movetime 200\nquit" "info.*pv"

# Test 25: Node count reporting
test_uci_command "Node count reporting" "uci\nposition startpos\ngo movetime 200\nquit" "info.*nodes"

echo
echo "=== Endgame Position Tests ==="

# Test 26: Checkmate position (no legal moves) - Fool's Mate position
test_uci_command "Checkmate position" "uci\nposition fen rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 0 1\ngo movetime 100\nquit" "bestmove 0000\\|bestmove (none)"

# Test 27: Stalemate position - Classic stalemate with king trapped
test_uci_command "Stalemate position" "uci\nposition fen 7k/5Q2/5K2/8/8/8/8/8 b - - 0 1\ngo movetime 100\nquit" "bestmove 0000\\|bestmove (none)"

echo
echo "=== Final Results ==="
echo "==============================================="
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo "Total Tests: $((TESTS_PASSED + TESTS_FAILED))"
echo "==============================================="

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL UCI PROTOCOL TESTS PASSED!${NC}"
    echo "SeaJay is ready for GUI integration."
    exit 0
else
    echo -e "${RED}❌ Some tests failed. Please review and fix issues.${NC}"
    exit 1
fi