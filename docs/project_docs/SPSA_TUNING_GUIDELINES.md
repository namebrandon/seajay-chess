# SPSA Tuning Guidelines for UCI Parameters

## Critical Issue: Float Values in Integer Parameters

### The Problem
OpenBench's SPSA implementation often sends **fractional values** for integer parameters during perturbation. For example:
- Parameter with initial value 90 might receive: 89.4, 90.6, 91.2, etc.
- If not handled correctly, these get truncated (not rounded), creating a **downward bias**
- This prevents SPSA from exploring values above the initial setting

### Example of the Bug
```cpp
// WRONG - Truncates 90.6 to 90
int value = std::stoi("90.6");  // Throws exception or truncates

// CORRECT - Rounds 90.6 to 91
double dv = std::stod("90.6");
int value = static_cast<int>(std::round(dv));
```

### Impact
- **Without proper rounding**: Parameter starting at 90 can never reach 91
- **With proper rounding**: 90.5+ rounds to 91, allowing upward exploration
- This completely breaks SPSA's ability to optimize parameters

## Implementation Pattern

### Required Headers
```cpp
#include <cmath>  // For std::round
#include <string> // For std::stod
```

### Correct Implementation for Integer Parameters
```cpp
// Pattern for handling SPSA integer parameters
else if (optionName == "MyIntParameter") {
    try {
        // Primary path: Handle float values from SPSA
        int paramValue = 0;
        try {
            double dv = std::stod(value);
            paramValue = static_cast<int>(std::round(dv));
        } catch (...) {
            // Fallback: Direct integer parsing for manual UCI commands
            paramValue = std::stoi(value);
        }
        
        // Apply bounds checking
        paramValue = std::clamp(paramValue, minValue, maxValue);
        
        // Update the parameter
        updateParameter(paramValue);
        
        // Log for debugging
        std::cerr << "info string " << optionName << " set to " << paramValue << std::endl;
    } catch (...) {
        std::cerr << "info string Invalid value for " << optionName << ": " << value << std::endl;
    }
}
```

### Implementation for Float Parameters
```cpp
// Pattern for actual float parameters
else if (optionName == "MyFloatParameter") {
    try {
        float paramValue = std::stof(value);
        
        // Apply bounds checking
        paramValue = std::clamp(paramValue, minValue, maxValue);
        
        // Update the parameter
        updateParameter(paramValue);
        
        std::cerr << "info string " << optionName << " set to " << paramValue << std::endl;
    } catch (...) {
        std::cerr << "info string Invalid value for " << optionName << ": " << value << std::endl;
    }
}
```

## Testing Your Implementation

### 1. Manual Test
```bash
# Test rounding behavior
echo -e "uci\nsetoption name YourParam value 50.4\nquit" | ./bin/seajay 2>&1 | grep "set to"
# Should show: "set to 50"

echo -e "uci\nsetoption name YourParam value 50.5\nquit" | ./bin/seajay 2>&1 | grep "set to"
# Should show: "set to 51"

echo -e "uci\nsetoption name YourParam value 50.6\nquit" | ./bin/seajay 2>&1 | grep "set to"
# Should show: "set to 51"
```

### 2. Verification Script
Create a test script `test_spsa_rounding.sh`:
```bash
#!/bin/bash
PARAM=$1
ENGINE="./bin/seajay"

echo "Testing SPSA rounding for $PARAM"
for val in 89.4 89.5 89.6 90.4 90.5 90.6; do
    result=$(echo -e "uci\nsetoption name $PARAM value $val\nquit" | $ENGINE 2>&1 | grep "set to" | awk '{print $NF}')
    echo "$val -> $result"
done
```

### 3. Expected Behavior
- x.0 to x.4 → rounds down to x
- x.5 to x.9 → rounds up to x+1
- This matches standard mathematical rounding

## Common Pitfalls

### 1. Missing cmath Include
```cpp
// Symptom: Compilation error "std::round not found"
// Fix: Add #include <cmath>
```

### 2. Using stoi Directly
```cpp
// WRONG - Throws exception on "90.6"
int value = std::stoi(optionValue);

// CORRECT - Handles fractional values
double dv = std::stod(optionValue);
int value = static_cast<int>(std::round(dv));
```

### 3. Not Rebuilding After Fix
```bash
# Always rebuild after fixing UCI handling
rm -rf build/
./build.sh
```

### 4. Forgetting Parameter Recalculation
Some parameters affect incremental data structures:
```cpp
// After updating PST parameters
m_board.recalculatePSTScore();

// After updating evaluation weights
m_evaluator.recalculate();
```

## SPSA Configuration Considerations

### Step Size (C_end) Selection
When SPSA uses very small step sizes, rounding can cause "stuck" parameters:
- If C_end < 0.5, integer parameters may never change
- Solution: Use appropriate C_end values (typically 1.0 or higher for integers)

### Example SPSA Configuration
```
# Good: C_end = 2.0 ensures meaningful exploration
MyIntParam, int, 50, 20, 80, 2.0, 0.002

# Bad: C_end = 0.3 might get stuck due to rounding
MyIntParam, int, 50, 20, 80, 0.3, 0.002
```

## Affected Parameters in SeaJay

### Currently Fixed
- All PST endgame parameters (`pawn_eg_*`, `knight_eg_*`, etc.)

### Need Review/Fix
All integer spin options that could be SPSA tuned:
- MaxCheckPly
- AspirationWindow
- AspirationMaxAttempts
- LMRMinDepth
- LMRMinMoveNumber
- LMRBaseReduction
- LMRDepthFactor
- NullMoveStaticMargin
- CountermoveBonus
- MoveCountLimit3-8
- MoveCountHistoryThreshold
- MoveCountHistoryBonus
- MoveCountImprovingRatio
- StabilityThreshold
- OpeningStability
- MiddlegameStability
- EndgameStability

### Float Parameters (if implemented)
- CounterMoveHistoryWeight (mentioned in docs but not implemented)

## Verification Checklist

- [ ] Include `<cmath>` for std::round
- [ ] Use std::stod + std::round for integer parameters
- [ ] Test with fractional values (x.4, x.5, x.6)
- [ ] Verify rounding works correctly (x.5 rounds up)
- [ ] Rebuild engine after changes
- [ ] Test that parameter changes affect evaluation/search
- [ ] Document C_end requirements for SPSA

## Summary

**The Bug**: Integer parameters receiving float values from SPSA get truncated instead of rounded, creating downward bias and preventing upward exploration.

**The Fix**: Parse as double, round to nearest integer.

**The Test**: Verify that x.5 rounds up to x+1.

**The Impact**: Without this fix, SPSA cannot properly optimize parameters.

This is a **critical bug** that affects all SPSA tuning of integer parameters. Any engine using OpenBench SPSA must implement proper float handling or tuning will be severely compromised.