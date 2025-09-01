#!/bin/bash

echo "=== SPSA Readiness Verification ==="
echo

# 1. Check that parameters are exposed
echo "1. Checking UCI parameters are exposed..."
PARAM_COUNT=$(echo -e "uci\nquit" | ./bin/seajay 2>/dev/null | grep -c "eg_")
echo "   Found $PARAM_COUNT SPSA parameters"

# 2. Check that parameters update PST tables
echo -e "\n2. Checking parameters update PST tables..."
DEFAULT=$(echo -e "dumpPST\nquit" | ./bin/seajay 2>&1 | grep "7 |" | head -1)
MODIFIED=$(echo -e "setoption name pawn_eg_r7_center value 120\ndumpPST\nquit" | ./bin/seajay 2>&1 | grep "7 |" | head -1)

if [ "$DEFAULT" != "$MODIFIED" ]; then
    echo "   ✓ PST tables update correctly"
else
    echo "   ✗ PST tables NOT updating!"
fi

# 3. Check bench stability
echo -e "\n3. Checking bench remains stable..."
BENCH1=$(echo "bench" | ./bin/seajay 2>/dev/null | grep "Benchmark complete" | awk '{print $4}')
BENCH2=$(echo -e "setoption name pawn_eg_r7_center value 120\nbench" | ./bin/seajay 2>/dev/null | grep "Benchmark complete" | awk '{print $4}')

if [ "$BENCH1" == "19191913" ] && [ "$BENCH2" == "19191913" ]; then
    echo "   ✓ Bench stable at 19191913"
else
    echo "   ✗ Bench changed! $BENCH1 -> $BENCH2"
fi

# 4. Final readiness check
echo -e "\n=== SPSA Quick Test Recommendation ==="
echo
echo "For a 1-2 hour validation test, use these 4 parameters:"
echo
echo "pawn_eg_r6_d, int, 60, 30, 100, 5, 0.002"
echo "pawn_eg_r6_e, int, 60, 30, 100, 5, 0.002"  
echo "pawn_eg_r7_center, int, 90, 50, 150, 6, 0.002"
echo "rook_eg_7th, int, 25, 15, 40, 3, 0.002"
echo
echo "Settings for OpenBench:"
echo "- Test Type: SPSA"
echo "- Time Control: 10+0.1"
echo "- Iterations: 5000 (for quick test)"
echo "- Games per iteration: 32"
echo
echo "This will:"
echo "1. Test ~160k games in 1-2 hours"
echo "2. Show parameter movement if working"
echo "3. Validate the infrastructure"
echo
echo "After validation, run full 20-parameter suite with 100k+ iterations."