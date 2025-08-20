# LMR Tuning Plan for SeaJay Chess Engine

**Date:** August 20, 2025  
**Current ELO:** ~2275 (with LMR +37 ELO gain)  
**Branch:** `integration/lmr-with-move-ordering`  

## Current LMR Performance

✅ **Successfully integrated LMR with move ordering:**
- Killer Moves: +31 ELO
- History Heuristic: +7 ELO
- LMR (conservative): +37 ELO
- **Total gain: ~75 ELO**

## Expert Analysis Summary

The chess-engine-expert identified that our `LMRMinMoveNumber=8` is **extremely conservative** for an engine with proper move ordering. We're missing significant reduction opportunities.

## Recommended Tuning Tests (In Priority Order)

### Test 1: Reduce LMRMinMoveNumber (HIGHEST PRIORITY)
**Expected gain: +15-25 ELO**

Current setting is way too conservative. With good move ordering, the first 4-5 moves are typically:
1. TT move (if exists)
2. Good captures (MVV-LVA)
3. Killer move 1
4. Killer move 2
5. History-ordered moves begin

**Test progression:**
```
Test 1A: LMRMinMoveNumber = 6  (Expected: +10-15 ELO)
Test 1B: LMRMinMoveNumber = 4  (Expected: +15-25 ELO)
Test 1C: LMRMinMoveNumber = 3  (Aggressive, worth testing)
```

### Test 2: Reduce Bad Captures
**Expected gain: +10-15 ELO**

Add SEE-based reduction for bad captures:
```cpp
// Reduce bad captures (SEE < -50)
if (isCapture(move) && SEE(move) < -50) {
    reduction += 1;  // Extra ply reduction
}
```

### Test 3: More Aggressive Depth Factor
**Expected gain: +5-10 ELO**

```
Current: LMRDepthFactor = 3  (depth 6 → reduction 2)
Test:    LMRDepthFactor = 2  (depth 6 → reduction 3)
```

### Test 4: Combined Aggressive Settings
After individual tests, try:
- LMRMinMoveNumber = 4
- LMRDepthFactor = 2
- Bad capture reductions enabled

## Re-search Rate Monitoring

Expected re-search rates for well-tuned LMR:
- **Good:** 5-15% of reduced nodes
- **Too aggressive:** >20% (losing tactics)
- **Too conservative:** <5% (not reducing enough)

Add statistics tracking:
```cpp
info string LMR re-search rate: 12.3%
```

## Testing Protocol

### OpenBench Configuration
```
Base: integration/lmr-with-move-ordering
Dev: integration/lmr-with-move-ordering
Dev Options: LMREnabled=true LMRMinMoveNumber=6

Time Control: 10+0.1
SPRT: [-5, 35] (standard bounds)
```

### Test Sequence
1. **Baseline:** Current settings (already +37 ELO)
2. **Test 1A:** LMRMinMoveNumber=6
3. **Test 1B:** LMRMinMoveNumber=4 (if 1A positive)
4. **Test 2:** Bad capture reductions (after best MinMoveNumber found)
5. **Test 3:** LMRDepthFactor=2 (optional)
6. **Final:** Best combination of all parameters

## Future Enhancements (After Basic Tuning)

### Phase 2 Improvements
1. **History-based adjustments:** Reduce less for moves with good history
2. **Killer move exception:** Don't reduce killer moves
3. **Improving moves:** Don't reduce when position is improving
4. **PV node consideration:** Less aggressive in PV nodes

### Phase 3: Advanced Formula
Implement logarithmic reduction formula:
```cpp
Reductions[depth][moveNum] = 0.75 + log(depth) * log(moveNum) / 2.25;
```

## Success Criteria

- [ ] Find optimal LMRMinMoveNumber (likely 4-6)
- [ ] Achieve additional +20-30 ELO from tuning
- [ ] Re-search rate between 5-15%
- [ ] Total LMR gain reaches +60-70 ELO
- [ ] No tactical blindness in test positions

## Summary

With proper tuning, we expect to gain an additional **+25-40 ELO** on top of the current +37 ELO, bringing total LMR contribution to **+60-75 ELO**. The most impactful change will be reducing LMRMinMoveNumber from 8 to 4-6.

Combined with existing improvements:
- Move ordering: +38 ELO
- LMR (tuned): +60-75 ELO
- **Total Stage 18-20 gain: ~100-115 ELO**

This would bring SeaJay to approximately **2300-2315 ELO**.