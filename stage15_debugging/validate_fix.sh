#!/bin/bash

# Validation script for Stage 15 PST double-negation bug fix
# Tests all positions documented in README.md

echo "====================================================="
echo "Stage 15 Bug Fix Validation"
echo "Testing positions from README.md documentation"
echo "====================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test binaries
STAGE14="/workspace/binaries/seajay-stage14-final"
STAGE15_BUGGY="/workspace/binaries/seajay_stage15_tuned_fixed"  # The buggy one from commits
STAGE15_FIXED="/workspace/binaries/seajay_stage15_fixed"       # Our fixed version

# Function to test a position
test_position() {
    local description="$1"
    local position="$2"
    local expected_14="$3"
    local expected_15_buggy="$4"
    
    echo "Testing: $description"
    echo "Position: $position"
    
    # Get Stage 14 evaluation
    eval_14=$(echo -e "uci\nposition $position\ngo depth 1\nquit" | $STAGE14 2>&1 | grep "score cp" | head -1 | sed -n 's/.*score cp \(-*[0-9]*\).*/\1/p')
    
    # Get Stage 15 buggy evaluation (if binary exists)
    if [ -f "$STAGE15_BUGGY" ]; then
        eval_15_buggy=$(echo -e "uci\nposition $position\ngo depth 1\nquit" | $STAGE15_BUGGY 2>&1 | grep "score cp" | head -1 | sed -n 's/.*score cp \(-*[0-9]*\).*/\1/p')
    else
        eval_15_buggy="N/A"
    fi
    
    # Get Stage 15 fixed evaluation
    eval_15_fixed=$(echo -e "uci\nposition $position\ngo depth 1\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | head -1 | sed -n 's/.*score cp \(-*[0-9]*\).*/\1/p')
    
    # Display results
    printf "  Stage 14 (baseline):     %6s cp (expected: ~%s cp)\n" "$eval_14" "$expected_14"
    printf "  Stage 15 buggy:          %6s cp (expected: ~%s cp)\n" "$eval_15_buggy" "$expected_15_buggy"
    printf "  Stage 15 FIXED:          %6s cp " "$eval_15_fixed"
    
    # Check if fixed version matches Stage 14 (within 40 cp tolerance)
    if [ -n "$eval_14" ] && [ -n "$eval_15_fixed" ]; then
        diff=$((eval_15_fixed - eval_14))
        abs_diff=${diff#-}  # Remove negative sign if present
        
        if [ $abs_diff -le 40 ]; then
            printf "${GREEN}✓ PASS${NC} (diff: %d cp)\n" "$diff"
        else
            printf "${RED}✗ FAIL${NC} (diff: %d cp)\n" "$diff"
        fi
    else
        printf "${YELLOW}? UNKNOWN${NC}\n"
    fi
    
    echo "---"
}

# Run all test positions from README
echo "Test Positions from README.md:"
echo "=============================="
echo ""

test_position "Starting position" \
              "startpos" \
              "50" "-240"

test_position "After 1.e4" \
              "startpos moves e2e4" \
              "25" "315"

test_position "After 1.e4 c5 (Sicilian)" \
              "startpos moves e2e4 c7c5" \
              "65" "-225"

test_position "After 1.e4 e5" \
              "startpos moves e2e4 e7e5" \
              "50" "-240"

test_position "After 1.d4" \
              "startpos moves d2d4" \
              "25" "315"

test_position "After 1.d4 d5" \
              "startpos moves d2d4 d7d5" \
              "50" "-240"

test_position "After 1.Nf3" \
              "startpos moves g1f3" \
              "0" "290"

echo ""
echo "====================================================="
echo "Summary:"
echo "====================================================="

# Check for the characteristic 290 cp error pattern
echo ""
echo "Checking for 290 cp error pattern:"
echo -e "uci\nposition startpos\ngo depth 1\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | head -1
echo -e "uci\nposition startpos moves e2e4\ngo depth 1\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | head -1

# Calculate if we still have the bug
startpos_eval=$(echo -e "uci\nposition startpos\ngo depth 1\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | head -1 | sed -n 's/.*score cp \(-*[0-9]*\).*/\1/p')
e4_eval=$(echo -e "uci\nposition startpos moves e2e4\ngo depth 1\nquit" | $STAGE15_FIXED 2>&1 | grep "score cp" | head -1 | sed -n 's/.*score cp \(-*[0-9]*\).*/\1/p')

if [ -n "$startpos_eval" ] && [ -n "$e4_eval" ]; then
    # The bug would show: startpos = -240, after e4 = +315 (difference of 555!)
    # Fixed should show: startpos = +50, after e4 = -25 (difference of 75)
    diff=$((e4_eval - startpos_eval))
    abs_diff=${diff#-}
    
    echo ""
    if [ $abs_diff -gt 500 ]; then
        echo -e "${RED}✗ BUG STILL PRESENT${NC} - Evaluation swing of $abs_diff cp detected!"
    elif [ $abs_diff -lt 150 ]; then
        echo -e "${GREEN}✓ BUG FIXED!${NC} - Normal evaluation swing of $abs_diff cp"
    else
        echo -e "${YELLOW}⚠ PARTIAL FIX${NC} - Evaluation swing of $abs_diff cp (expected ~75)"
    fi
fi

echo ""
echo "====================================================="
echo "Validation complete!"
echo "====================================================="