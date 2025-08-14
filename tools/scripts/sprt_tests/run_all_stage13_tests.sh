#!/bin/bash
# Master script to run all Stage 13 SPRT tests
# This script helps organize and run the various SPRT tests for Stage 13

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

echo "==================================================================="
echo "Stage 13 SPRT Test Suite"
echo "==================================================================="
echo ""
echo "Available tests:"
echo "  1) Stage 13 vs Stage 12 (startpos) - Basic test from starting position"
echo "  2) Stage 13 vs Stage 12 (4moves)   - Test with opening book variety"
echo "  3) Stage 13 vs Stage 11 (4moves)   - Test cumulative improvements"
echo "  4) Stage 13 vs Stage 12 (60+0.6)   - Long time control test"
echo "  5) Run tests 1-3 sequentially"
echo "  6) View existing results"
echo ""
echo "Which test would you like to run? (1-6, or 'q' to quit)"

read -r choice

case $choice in
    1)
        echo "Starting Stage 13 vs Stage 12 (startpos) test..."
        "$SCRIPT_DIR/run_stage13_vs_stage12_startpos.sh"
        ;;
    2)
        echo "Starting Stage 13 vs Stage 12 (4moves) test..."
        "$SCRIPT_DIR/run_stage13_vs_stage12_4moves.sh"
        ;;
    3)
        echo "Starting Stage 13 vs Stage 11 (4moves) test..."
        "$SCRIPT_DIR/run_stage13_vs_stage11_4moves.sh"
        ;;
    4)
        echo "Starting Stage 13 vs Stage 12 (60+0.6) test..."
        echo "WARNING: This test will take several hours!"
        "$SCRIPT_DIR/run_stage13_vs_stage12_60s.sh"
        ;;
    5)
        echo "Running tests 1-3 sequentially..."
        echo ""
        echo "Test 1/3: Stage 13 vs Stage 12 (startpos)"
        "$SCRIPT_DIR/run_stage13_vs_stage12_startpos.sh"
        echo ""
        echo "Test 2/3: Stage 13 vs Stage 12 (4moves)"
        "$SCRIPT_DIR/run_stage13_vs_stage12_4moves.sh"
        echo ""
        echo "Test 3/3: Stage 13 vs Stage 11 (4moves)"
        "$SCRIPT_DIR/run_stage13_vs_stage11_4moves.sh"
        echo ""
        echo "All tests complete!"
        ;;
    6)
        echo "Existing SPRT results:"
        echo ""
        ls -ltr "$PROJECT_ROOT/sprt_results/" | grep Stage13 || echo "No Stage 13 results found yet"
        echo ""
        echo "To view a specific result, use:"
        echo "  less <result_dir>/sprt_output.txt"
        ;;
    q|Q)
        echo "Exiting..."
        exit 0
        ;;
    *)
        echo "Invalid choice. Please run the script again and choose 1-6 or 'q'"
        exit 1
        ;;
esac

echo ""
echo "==================================================================="
echo "Test execution complete. Check the sprt_results directory for output."
echo "=================================================================="