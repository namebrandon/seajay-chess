#!/bin/bash

# MovePicker Performance Test Script
# Run consistent tests across different commits

echo "MovePicker Performance Test"
echo "==========================="
echo "Commit: $(git rev-parse --short HEAD)"
echo "Message: $(git log --oneline -1)"
echo ""

# Clean rebuild is MANDATORY
echo "Performing clean rebuild..."
rm -rf build openbench-build
make clean > /dev/null 2>&1
if ! make > /dev/null 2>&1; then
    echo "BUILD FAILED - Cannot test this commit"
    exit 1
fi

echo "Build successful, starting tests..."
echo ""

# Test 1: Legacy baseline (MovePicker disabled)
echo "Test 1: Legacy Ordering (baseline)"
echo "-----------------------------------"
echo "position startpos
setoption name UseStagedMovePicker value false
go movetime 5000
quit" | ./seajay 2>/dev/null | grep -E "depth [0-9]+ |bestmove" | tail -2

echo ""

# Test 2: MovePicker Minimal mode
echo "Test 2: MovePicker Minimal Mode"
echo "--------------------------------"
echo "position startpos
setoption name UseStagedMovePicker value true
setoption name MovePickerMode value min
go movetime 5000
quit" | ./seajay 2>/dev/null | grep -E "depth [0-9]+ |bestmove" | tail -2

echo ""

# Test 3: MovePicker Standard mode
echo "Test 3: MovePicker Standard Mode"
echo "---------------------------------"
echo "position startpos
setoption name UseStagedMovePicker value true
setoption name MovePickerMode value standard
go movetime 5000
quit" | ./seajay 2>/dev/null | grep -E "depth [0-9]+ |bestmove" | tail -2

echo ""

# Test 4: Quick depth 8 test for node comparison
echo "Test 4: Depth 8 Node Count Comparison"
echo "--------------------------------------"
echo "Legacy at depth 8:"
echo "position startpos
setoption name UseStagedMovePicker value false
go depth 8
quit" | ./seajay 2>/dev/null | grep "depth 8 " | tail -1

echo ""
echo "MovePicker standard at depth 8:"
echo "position startpos
setoption name UseStagedMovePicker value true
setoption name MovePickerMode value standard
go depth 8
quit" | ./seajay 2>/dev/null | grep "depth 8 " | tail -1

echo ""
echo "Test complete for commit $(git rev-parse --short HEAD)"