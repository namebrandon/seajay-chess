#!/bin/bash
# Run all white bias self-play tests sequentially
# This will test Stage 7, 8, and 9 to identify when the bias was introduced

echo "=============================================="
echo "WHITE BIAS INVESTIGATION - COMPLETE TEST SUITE"
echo "=============================================="
echo ""
echo "This will run self-play tests for:"
echo "  - Stage 7: Material-only evaluation (expect no bias)"
echo "  - Stage 8: Material tracking (expect no bias)"
echo "  - Stage 9: PST implementation (expect WHITE BIAS)"
echo ""
echo "Time control: 10+0.1 for all tests"
echo "Games per stage: 100"
echo ""

# Make scripts executable
chmod +x /workspace/tools/scripts/sprt_tests/white_bias_investigation/*.sh

# Stage 7 Test
echo "Starting Stage 7 test..."
echo "------------------------"
./stage7_selfplay.sh
echo ""
echo "Stage 7 complete. Press Enter to continue to Stage 8..."
read

# Stage 8 Test
echo "Starting Stage 8 test..."
echo "------------------------"
./stage8_selfplay.sh
echo ""
echo "Stage 8 complete. Press Enter to continue to Stage 9..."
read

# Stage 9 Test
echo "Starting Stage 9 test..."
echo "------------------------"
./stage9_selfplay.sh
echo ""

# Summary
echo "=============================================="
echo "WHITE BIAS INVESTIGATION - SUMMARY"
echo "=============================================="
echo ""

# Read results from log files
if [ -f "stage7_selfplay_results.log" ]; then
    echo "Stage 7 Results (Engine A vs Engine B):"
    grep -A2 "Percentages:" stage7_selfplay_results.log | tail -3
    echo ""
fi

if [ -f "stage8_selfplay_results.log" ]; then
    echo "Stage 8 Results (Engine A vs Engine B):"
    grep -A2 "Percentages:" stage8_selfplay_results.log | tail -3
    echo ""
fi

if [ -f "stage9_selfplay_results.log" ]; then
    echo "Stage 9 Results (Engine A vs Engine B):"
    grep -A2 "Percentages:" stage9_selfplay_results.log | tail -3
    echo ""
fi

echo "=============================================="
echo "CONCLUSION:"
echo ""
echo "Since engines A and B each play both colors equally:"
echo "  - Balanced A/B scores = No color bias"
echo "  - Imbalanced A/B scores = Color bias exists"
echo ""
echo "If our hypothesis is correct:"
echo "  - Stage 7 & 8: Should show balanced A/B results (~50% each)"
echo "  - Stage 9: Should show significant imbalance (indicating color bias)"
echo ""
echo "This would confirm that the PST implementation in Stage 9"
echo "introduced the color bias bug that has affected all subsequent stages."
echo ""
echo "All logs saved in: /workspace/tools/scripts/sprt_tests/white_bias_investigation/"