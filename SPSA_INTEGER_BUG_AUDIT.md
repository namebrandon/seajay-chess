# SPSA Integer Parameter Bug Audit

## Status: CRITICAL BUG FOUND

### Summary
All integer UCI parameters (except PST parameters) have the float truncation bug. This means SPSA tuning cannot properly explore parameter space for ANY of these options.

## Affected Parameters (Need Fix)

### High Priority (Marked for SPSA tuning in uci.md)
1. **MaxCheckPly** (line 738) - ⭐ High Impact
2. **AspirationWindow** (line 1067) - ⭐
3. **AspirationMaxAttempts** (line unknown) - Needs investigation
4. **LMRMinDepth** (line 874) - ⭐
5. **LMRMinMoveNumber** (line 885) - ⭐
6. **LMRBaseReduction** (line 896)
7. **LMRDepthFactor** (line 907)
8. **NullMoveStaticMargin** (line 955) - ⭐
9. **CountermoveBonus** (line 986) - ⭐

### Move Count Pruning Suite (lines 1029-1061)
10. **MoveCountLimit3** (line 1029) - ⭐
11. **MoveCountLimit4** (line 1033) - ⭐
12. **MoveCountLimit5** (line 1037) - ⭐
13. **MoveCountLimit6** (line 1041) - ⭐
14. **MoveCountLimit7** (line 1045) - ⭐
15. **MoveCountLimit8** (line 1049) - ⭐
16. **MoveCountHistoryThreshold** (line 1053) - ⭐
17. **MoveCountHistoryBonus** (line 1057)
18. **MoveCountImprovingRatio** (line 1061)

### Stability Parameters (Need investigation)
19. **StabilityThreshold**
20. **OpeningStability**
21. **MiddlegameStability**
22. **EndgameStability**

### Low Priority (Not intended for SPSA)
- Hash (line 764) - Hardware dependent
- Threads (line 788) - Hardware dependent
- QSearchNodeLimit - Debug only

## Already Fixed
✅ All PST parameters (pawn_eg_*, knight_eg_*, etc.) - Uses std::stod + std::round

## Code Pattern to Fix

### Current (BROKEN)
```cpp
int value = std::stoi(value);  // Truncates 90.6 to 90
```

### Fixed
```cpp
int paramValue = 0;
try {
    double dv = std::stod(value);
    paramValue = static_cast<int>(std::round(dv));
} catch (...) {
    paramValue = std::stoi(value);  // Fallback for integer strings
}
```

## Impact Assessment

### Severity: CRITICAL
- **19+ parameters** cannot be properly tuned with SPSA
- All previous SPSA runs on these parameters were compromised
- Parameters can only decrease or stay same, never increase past initial value

### Example Impact
- LMRMinMoveNumber starting at 6:
  - SPSA proposes 6.6 → truncates to 6 (no change)
  - SPSA proposes 5.4 → truncates to 5 (explores down)
  - Can NEVER reach 7 unless proposal is exactly ≥7.0

## Recommended Action Plan

### Phase 1: Critical Parameters (Immediate)
Fix these first as they're marked for SPSA tuning:
1. MaxCheckPly
2. LMRMinDepth
3. LMRMinMoveNumber
4. NullMoveStaticMargin
5. CountermoveBonus
6. AspirationWindow

### Phase 2: Move Count Suite
Fix all MoveCountLimit parameters together for consistency

### Phase 3: Remaining Parameters
Fix stability and other tunable parameters

### Phase 4: Verification
1. Create comprehensive test for all parameters
2. Verify rounding behavior (x.5 → x+1)
3. Re-run SPSA tests with fixed code

## Test Command for Each Parameter

```bash
# Example for MaxCheckPly
echo -e "uci\nsetoption name MaxCheckPly value 6.6\nquit" | ./bin/seajay 2>&1 | grep "set to"
# Should show: "set to 7" not "set to 6"
```

## Conclusion

This is a **systemic bug** affecting virtually all SPSA-tunable integer parameters. The PST fix was correct but needs to be applied everywhere. Without these fixes, SPSA tuning is fundamentally broken for integer parameters.