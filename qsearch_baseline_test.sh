#!/bin/bash

# QSearch Baseline Test Script
# Tests the problem position to establish baseline behavior before improvements

BINARY="./bin/seajay"
PROBLEM_FEN="r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17"
OUTPUT_FILE="qsearch_baseline_results.txt"

echo "========================================" | tee $OUTPUT_FILE
echo "QSearch Baseline Test Results" | tee -a $OUTPUT_FILE
echo "Date: $(date)" | tee -a $OUTPUT_FILE
echo "Branch: $(git branch --show-current)" | tee -a $OUTPUT_FILE
echo "Commit: $(git rev-parse --short HEAD)" | tee -a $OUTPUT_FILE
echo "========================================" | tee -a $OUTPUT_FILE
echo "" | tee -a $OUTPUT_FILE

# Get bench count for reference
echo "Getting bench count..." | tee -a $OUTPUT_FILE
BENCH_COUNT=$(echo "bench" | $BINARY 2>&1 | grep "Benchmark complete" | awk '{print $4}')
echo "Bench: $BENCH_COUNT" | tee -a $OUTPUT_FILE
echo "" | tee -a $OUTPUT_FILE

# Test 1: Default behavior with quiescence (depths 1-10)
echo "TEST 1: Default Behavior (Quiescence Enabled)" | tee -a $OUTPUT_FILE
echo "===============================================" | tee -a $OUTPUT_FILE
for depth in {1..10}; do
    echo "Depth $depth:" | tee -a $OUTPUT_FILE
    result=$(echo -e "position fen $PROBLEM_FEN\ngo depth $depth" | $BINARY 2>&1 | grep "bestmove" | tail -1)
    pv_line=$(echo -e "position fen $PROBLEM_FEN\ngo depth $depth" | $BINARY 2>&1 | grep "info depth $depth " | tail -1)
    
    # Extract key metrics
    bestmove=$(echo "$result" | awk '{print $2}')
    score=$(echo "$pv_line" | grep -o "score cp [0-9-]*" | awk '{print $3}')
    nodes=$(echo "$pv_line" | grep -o "nodes [0-9]*" | awk '{print $2}')
    seldepth=$(echo "$pv_line" | grep -o "seldepth [0-9]*" | awk '{print $2}')
    
    echo "  Best move: $bestmove | Score: $score cp | Nodes: $nodes | Seldepth: $seldepth" | tee -a $OUTPUT_FILE
done
echo "" | tee -a $OUTPUT_FILE

# Test 2: Without quiescence
echo "TEST 2: Quiescence Disabled" | tee -a $OUTPUT_FILE
echo "============================" | tee -a $OUTPUT_FILE
for depth in {1..10}; do
    echo "Depth $depth:" | tee -a $OUTPUT_FILE
    result=$(echo -e "setoption name UseQuiescence value false\nposition fen $PROBLEM_FEN\ngo depth $depth" | $BINARY 2>&1 | grep "bestmove" | tail -1)
    pv_line=$(echo -e "setoption name UseQuiescence value false\nposition fen $PROBLEM_FEN\ngo depth $depth" | $BINARY 2>&1 | grep "info depth $depth " | tail -1)
    
    bestmove=$(echo "$result" | awk '{print $2}')
    score=$(echo "$pv_line" | grep -o "score cp [0-9-]*" | awk '{print $3}')
    nodes=$(echo "$pv_line" | grep -o "nodes [0-9]*" | awk '{print $2}')
    seldepth=$(echo "$pv_line" | grep -o "seldepth [0-9]*" | awk '{print $2}')
    
    echo "  Best move: $bestmove | Score: $score cp | Nodes: $nodes | Seldepth: $seldepth" | tee -a $OUTPUT_FILE
done
echo "" | tee -a $OUTPUT_FILE

# Test 3: Different maxCheckPly values (4, 5, 6)
echo "TEST 3: Different maxCheckPly Values" | tee -a $OUTPUT_FILE
echo "=====================================" | tee -a $OUTPUT_FILE
for checkply in 3 4 5 6; do
    echo "maxCheckPly = $checkply (depth 6):" | tee -a $OUTPUT_FILE
    # Note: maxCheckPly might not be a UCI option yet, so this might need adjustment
    result=$(echo -e "position fen $PROBLEM_FEN\ngo depth 6" | $BINARY 2>&1 | grep "bestmove" | tail -1)
    pv_line=$(echo -e "position fen $PROBLEM_FEN\ngo depth 6" | $BINARY 2>&1 | grep "info depth 6 " | tail -1)
    
    bestmove=$(echo "$result" | awk '{print $2}')
    score=$(echo "$pv_line" | grep -o "score cp [0-9-]*" | awk '{print $3}')
    nodes=$(echo "$pv_line" | grep -o "nodes [0-9]*" | awk '{print $2}')
    
    echo "  Best move: $bestmove | Score: $score cp | Nodes: $nodes" | tee -a $OUTPUT_FILE
done
echo "" | tee -a $OUTPUT_FILE

# Test 4: Static evaluation comparison
echo "TEST 4: Static Evaluation of Key Moves" | tee -a $OUTPUT_FILE
echo "=======================================" | tee -a $OUTPUT_FILE

# Original position
echo "Original position:" | tee -a $OUTPUT_FILE
eval_orig=$(echo -e "position fen $PROBLEM_FEN\ndebug eval" | $BINARY 2>&1 | grep "Total Evaluation" | tail -1 | grep -o "[0-9-]*" | tail -1)
echo "  Static eval: $eval_orig cp" | tee -a $OUTPUT_FILE

# After e8f7
echo "After e8f7:" | tee -a $OUTPUT_FILE
eval_e8f7=$(echo -e "position fen $PROBLEM_FEN\nmoves e8f7\ndebug eval" | $BINARY 2>&1 | grep "Total Evaluation" | tail -1 | grep -o "[0-9-]*" | tail -1)
echo "  Static eval: $eval_e8f7 cp" | tee -a $OUTPUT_FILE

# After c8d7
echo "After c8d7:" | tee -a $OUTPUT_FILE
eval_c8d7=$(echo -e "position fen $PROBLEM_FEN\nmoves c8d7\ndebug eval" | $BINARY 2>&1 | grep "Total Evaluation" | tail -1 | grep -o "[0-9-]*" | tail -1)
echo "  Static eval: $eval_c8d7 cp" | tee -a $OUTPUT_FILE

# After d5d4
echo "After d5d4:" | tee -a $OUTPUT_FILE
eval_d5d4=$(echo -e "position fen $PROBLEM_FEN\nmoves d5d4\ndebug eval" | $BINARY 2>&1 | grep "Total Evaluation" | tail -1 | grep -o "[0-9-]*" | tail -1)
echo "  Static eval: $eval_d5d4 cp" | tee -a $OUTPUT_FILE

echo "" | tee -a $OUTPUT_FILE

# Test 5: Time-limited search (to see behavior under time pressure)
echo "TEST 5: Time-Limited Search (100ms)" | tee -a $OUTPUT_FILE
echo "====================================" | tee -a $OUTPUT_FILE
result=$(echo -e "position fen $PROBLEM_FEN\ngo movetime 100" | $BINARY 2>&1)
final_depth=$(echo "$result" | grep "info depth" | tail -1 | grep -o "depth [0-9]*" | awk '{print $2}')
bestmove=$(echo "$result" | grep "bestmove" | awk '{print $2}')
final_score=$(echo "$result" | grep "info depth" | tail -1 | grep -o "score cp [0-9-]*" | awk '{print $3}')
total_nodes=$(echo "$result" | grep "info depth" | tail -1 | grep -o "nodes [0-9]*" | awk '{print $2}')

echo "  Final depth: $final_depth" | tee -a $OUTPUT_FILE
echo "  Best move: $bestmove" | tee -a $OUTPUT_FILE
echo "  Final score: $final_score cp" | tee -a $OUTPUT_FILE
echo "  Total nodes: $total_nodes" | tee -a $OUTPUT_FILE
echo "" | tee -a $OUTPUT_FILE

# Summary
echo "========================================" | tee -a $OUTPUT_FILE
echo "SUMMARY" | tee -a $OUTPUT_FILE
echo "========================================" | tee -a $OUTPUT_FILE
echo "Key observations:" | tee -a $OUTPUT_FILE
echo "1. Does e8f7 appear at depth 4+? Check TEST 1" | tee -a $OUTPUT_FILE
echo "2. Does disabling quiescence fix it? Check TEST 2" | tee -a $OUTPUT_FILE
echo "3. Node count explosion? Compare TEST 1 vs TEST 2" | tee -a $OUTPUT_FILE
echo "4. Static eval correctly ranks moves? Check TEST 4" | tee -a $OUTPUT_FILE
echo "" | tee -a $OUTPUT_FILE

echo "Test complete. Results saved to $OUTPUT_FILE"