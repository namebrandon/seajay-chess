#!/bin/bash

# Test script for SPSA parameter float handling
# Tests that all parameters properly round float values instead of truncating

echo "============================================"
echo "SPSA Parameter Float Handling Test"
echo "============================================"
echo ""

# Test function that checks if a parameter rounds correctly
test_param() {
    local param=$1
    local test_value=$2
    local expected=$3
    local grep_pattern=$4
    
    # Use provided pattern or default to "set to"
    if [ -z "$grep_pattern" ]; then
        grep_pattern="set to"
    fi
    
    output=$(echo -e "uci\nsetoption name $param value $test_value\nquit" | ./bin/seajay 2>&1 | grep "$grep_pattern" | tail -1)
    
    if echo "$output" | grep -q "set to: $expected"; then
        echo "✓ $param: $test_value → $expected (PASS)"
        return 0
    else
        actual=$(echo "$output" | sed -n 's/.*set to: \([0-9]*\).*/\1/p')
        if [ -z "$actual" ]; then
            echo "✗ $param: $test_value → (no output) (FAIL)"
            echo "  Debug: $output"
        else
            echo "✗ $param: $test_value → $actual (expected $expected) (FAIL)"
        fi
        return 1
    fi
}

# Test function for parameters that display differently
test_param_special() {
    local param=$1
    local test_value=$2
    local expected=$3
    local display_pattern=$4
    
    output=$(echo -e "uci\nsetoption name $param value $test_value\nquit" | ./bin/seajay 2>&1 | grep "$display_pattern" | tail -1)
    
    if echo "$output" | grep -q "$expected"; then
        echo "✓ $param: $test_value → $expected (PASS)"
        return 0
    else
        echo "✗ $param: $test_value (FAIL - check output)"
        echo "  Output: $output"
        return 1
    fi
}

failures=0

echo "Testing Core Tactical Search Parameters:"
echo "-----------------------------------------"

# MaxCheckPly
test_param "MaxCheckPly" "7.4" "7" "Maximum check extension" || ((failures++))
test_param "MaxCheckPly" "7.6" "8" "Maximum check extension" || ((failures++))
test_param "MaxCheckPly" "7.5" "8" "Maximum check extension" || ((failures++))

# CountermoveBonus
test_param "CountermoveBonus" "9999.4" "9999" "CountermoveBonus" || ((failures++))
test_param "CountermoveBonus" "9999.6" "10000" "CountermoveBonus" || ((failures++))
test_param "CountermoveBonus" "9999.5" "10000" "CountermoveBonus" || ((failures++))

# MoveCountHistoryBonus
test_param "MoveCountHistoryBonus" "7.4" "7" "MoveCountHistoryBonus" || ((failures++))
test_param "MoveCountHistoryBonus" "7.6" "8" "MoveCountHistoryBonus" || ((failures++))
test_param "MoveCountHistoryBonus" "7.5" "8" "MoveCountHistoryBonus" || ((failures++))

# MoveCountHistoryThreshold
test_param "MoveCountHistoryThreshold" "1999.4" "1999" "MoveCountHistoryThreshold" || ((failures++))
test_param "MoveCountHistoryThreshold" "1999.6" "2000" "MoveCountHistoryThreshold" || ((failures++))
test_param "MoveCountHistoryThreshold" "1999.5" "2000" "MoveCountHistoryThreshold" || ((failures++))

echo ""
echo "Testing LMR Parameters:"
echo "-----------------------"

# LMRMinMoveNumber
test_param "LMRMinMoveNumber" "7.4" "7" "LMR min move number" || ((failures++))
test_param "LMRMinMoveNumber" "7.6" "8" "LMR min move number" || ((failures++))
test_param "LMRMinMoveNumber" "7.5" "8" "LMR min move number" || ((failures++))

# LMRHistoryThreshold (displayed with %)
test_param_special "LMRHistoryThreshold" "29.4" "29%" "LMR history threshold" || ((failures++))
test_param_special "LMRHistoryThreshold" "29.6" "30%" "LMR history threshold" || ((failures++))
test_param_special "LMRHistoryThreshold" "29.5" "30%" "LMR history threshold" || ((failures++))

# LMRBaseReduction (has special display with division by 100)
test_param_special "LMRBaseReduction" "29.4" "29" "LMR base reduction" || ((failures++))
test_param_special "LMRBaseReduction" "29.6" "30" "LMR base reduction" || ((failures++))
test_param_special "LMRBaseReduction" "29.5" "30" "LMR base reduction" || ((failures++))

# LMRDepthFactor (has special display with division by 100)
test_param_special "LMRDepthFactor" "199.4" "199" "LMR depth factor" || ((failures++))
test_param_special "LMRDepthFactor" "199.6" "200" "LMR depth factor" || ((failures++))
test_param_special "LMRDepthFactor" "199.5" "200" "LMR depth factor" || ((failures++))

echo ""
echo "============================================"
echo "TEST SUMMARY"
echo "============================================"
if [ $failures -eq 0 ]; then
    echo "✓ ALL TESTS PASSED!"
    echo "All tested parameters are SPSA-compatible."
    echo ""
    echo "Parameters ready for SPSA tuning:"
    echo "  - MaxCheckPly"
    echo "  - CountermoveBonus"
    echo "  - MoveCountHistoryBonus"
    echo "  - MoveCountHistoryThreshold"
    echo "  - LMRMinMoveNumber"
    echo "  - LMRHistoryThreshold"
    echo "  - LMRBaseReduction"
    echo "  - LMRDepthFactor"
else
    echo "✗ FAILURES DETECTED: $failures test(s) failed"
fi
echo "============================================"

exit $failures
