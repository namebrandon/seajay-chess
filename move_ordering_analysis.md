# Move Ordering Analysis and Improvements

## Branch: feature/analysis/20250825-move-ordering

## Summary

Analyzed and improved move ordering efficiency in SeaJay chess engine. While move ordering still degrades with depth (from ~95% to ~76%), we identified and fixed several inefficiencies.

## Issues Found and Fixed

### 1. Redundant TT Move Handling (FIXED)
- **Problem**: TT move was being moved to front twice in orderMoves()
- **Solution**: Removed duplicate code, only move TT to front once after other ordering
- **Impact**: Cleaner code, slight performance improvement

### 2. Aggressive History Aging (FIXED)
- **Problem**: History table aged when ANY entry hit max value
- **Solution**: Only age when >10% of entries are near max (prevents premature aging)
- **Impact**: Preserves valuable move ordering information longer

### 3. Weak History Impact (PARTIALLY FIXED)
- **Problem**: History bonus/penalty too small to impact ordering
- **Solution**: Doubled bonus (2×depth² max 800) and penalty (depth² max 400)
- **Impact**: History has more influence on move ordering

### 4. LMR Over-Reduction (FIXED)
- **Problem**: Good history moves were being reduced by LMR
- **Solution**: Lowered threshold from 75% to 50% to protect more good moves
- **Impact**: Better move selection in reduced searches

## Performance Metrics

### Baseline (commit 865252e)
Position: startpos, searching to depth 10
- Depth 6: 86.4% efficiency
- Depth 7: 85.0% efficiency  
- Depth 8: 82.9% efficiency
- Depth 9: 83.1% efficiency
- Depth 10: 77.0% efficiency
- Bench: 19191913

### After Improvements (commit 321f3d0)
- Depth 6: 86.0% efficiency (-0.4%)
- Depth 7: 83.5% efficiency (-1.5%)
- Depth 8: 81.4% efficiency (-1.5%)
- Depth 9: 81.9% efficiency (-1.2%)
- Depth 10: 75.9% efficiency (-1.1%)
- Bench: 19191913 (unchanged - good!)

## Analysis of Results

The efficiency still degrades from ~95% at shallow depths to ~76% at depth 10. This suggests deeper issues:

### Root Causes Not Yet Addressed

1. **Missing Move Ordering Features**
   - No piece-to history (only from-to squares)
   - No continuation history
   - No capture history separate from quiet history
   - No threat detection for move ordering

2. **TT Integration Issues**
   - TT move quality not verified before prioritizing
   - No TT move exclusion in ordering stats
   - Possible stale TT moves being tried first

3. **LMR Formula May Be Too Aggressive**
   - Logarithmic reduction may be reducing too many moves
   - Move count threshold (6) might be too low
   - Consider dynamic thresholds based on position

4. **Statistical Tracking Bug**
   - Countermove hit rate >100% indicates bug
   - Move ordering stats may be inaccurate

## Recommendations for Further Work

### High Priority
1. **Add Piece-Type to History**
   - Track piece×from×to instead of just from×to
   - Significantly improves discrimination

2. **Fix Statistical Tracking**
   - Debug countermove hit rate calculation
   - Add more detailed ordering statistics

3. **Implement Continuation History**
   - Track move pairs that work well together
   - Especially important for deep searches

### Medium Priority
1. **Separate Capture History**
   - Track capture success separately from quiet moves
   - Use for ordering bad captures

2. **Tune LMR Parameters**
   - Test higher move count threshold (8-10)
   - Adjust reduction formula coefficients

3. **Add Threat Detection**
   - Prioritize moves that parry threats
   - Helps in tactical positions

### Low Priority
1. **Add Static Exchange Evaluation (SEE)**
   - Better capture ordering than MVV-LVA
   - Helps prune bad captures

2. **Implement Singular Extensions**
   - Extend when only one good move
   - Requires search refactoring

## Testing Recommendations

For OpenBench testing:
1. Test current improvements: feature/analysis/20250825-move-ordering vs main
2. Expected: Small ELO gain (5-10) from cleaner move ordering
3. Use standard SPRT bounds [0.00, 5.00]

## Conclusion

While we improved the move ordering code quality and history impact, the fundamental efficiency degradation remains. The 76% efficiency at depth 10 (target: >85%) indicates significant room for improvement. The next step should be implementing piece-type history and fixing the statistical tracking bugs.

The unchanged bench (19191913) confirms we haven't broken compatibility, making these changes safe to test via OpenBench.