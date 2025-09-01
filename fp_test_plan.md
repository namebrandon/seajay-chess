# Futility Pruning Test Plan

## Current State Analysis

### Existing Implementation
- **Location**: `/workspace/src/search/negamax.cpp` lines 604-621
- **Current Depth Limit**: 4 (hardcoded)
- **Current Margin Formula**: `150 + 60 * depth` (hardcoded)
- **Conditions**: 
  - Not PV node
  - Depth <= 4 and > 0
  - Not in check
  - Move count > 1
  - Not capturing, not promotion, not giving check
  - Static eval <= alpha - margin

### Known Issues/Questions
1. Depth 4 limit seems conservative (Stockfish uses depth 8)
2. No UCI control over parameters (depth limit and margins)
3. Prior testing showed regressions at depths 5-6 (need to understand why)
4. Margins may not be monotonic or properly scaled

## Phase 1: Add UCI Control (Infrastructure)

### Objective
Add UCI options to control futility pruning parameters without changing behavior.

### Implementation Tasks
1. **Add UCI Options** (in `/workspace/src/uci/uci.cpp`):
   ```cpp
   // Futility Pruning options
   option name FutilityPruning type check default true
   option name FutilityMaxDepth type spin default 4 min 0 max 10
   option name FutilityBase type spin default 150 min 50 max 500
   option name FutilityScale type spin default 60 min 20 max 200
   ```

2. **Add Config Variables** (in `/workspace/src/core/engine_config.h`):
   ```cpp
   bool useFutilityPruning = true;
   int futilityMaxDepth = 4;
   int futilityBase = 150;
   int futilityScale = 60;
   ```

3. **Wire UCI to Config** (in `/workspace/src/uci/uci.cpp` handleSetOption)

4. **Update Futility Logic** (in `/workspace/src/search/negamax.cpp`):
   - Replace hardcoded `depth <= 4` with `depth <= g_config.futilityMaxDepth`
   - Replace hardcoded margin formula with `g_config.futilityBase + g_config.futilityScale * depth`
   - Add enable/disable check for `g_config.useFutilityPruning`

### Validation
- Compile and run bench - must still output 19191913
- Verify UCI options appear in GUI
- Test that default values match current hardcoded behavior

## Phase 2: Diagnostic Testing

### Objective
Understand why depths 5-6 cause regressions.

### Test Matrix

#### 2.1 Conservative Margin Testing
Test with very safe margins at higher depths:

| Depth | Current | Conservative | Ultra-Safe |
|-------|---------|--------------|------------|
| 1     | 210     | 210          | 250        |
| 2     | 270     | 270          | 350        |
| 3     | 330     | 330          | 450        |
| 4     | 390     | 390          | 550        |
| 5     | N/A     | 650          | 750        |
| 6     | N/A     | 900          | 1000       |
| 7     | N/A     | 1200         | 1400       |
| 8     | N/A     | 1600         | 1800       |

**Test Protocol**:
1. Set FutilityMaxDepth=5, test with conservative margins
2. If that works, test depth 6, etc.
3. If even ultra-safe margins fail, indicates implementation issue

#### 2.2 Tactical Awareness Testing
Check if we're missing tactical exceptions:

**Test Positions** (positions where futility might prune incorrectly):
```
# Position 1: Hanging piece that can be captured
position fen r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQ1RK1 b kq - 0 1

# Position 2: Promotion threat
position fen 8/1P6/8/8/8/8/1p6/8 w - - 0 1

# Position 3: Check evasion needed
position fen r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 b kq - 0 1

# Position 4: Zugzwang-like position
position fen 8/8/p1p5/1p5p/1P5P/8/PPP2K2/8 w - - 0 1
```

**Metrics to Track**:
- Best move found at each depth
- Node count
- Futility prunes counter
- Does increasing depth change the best move unexpectedly?

#### 2.3 Margin Monotonicity Check
Verify margins increase properly with depth:

```python
# Quick validation script (conceptual)
for depth in range(1, 9):
    margin = base + scale * depth
    print(f"Depth {depth}: {margin}")
    # Verify margin[depth] > margin[depth-1]
```

## Phase 3: Implementation Improvements

### Based on Diagnostic Results

#### 3.1 If Conservative Margins Work
- Implement array-based margins for fine control:
  ```cpp
  const int FutilityMargin[9] = {0, 210, 270, 330, 390, 500, 650, 850, 1100};
  ```
- Add UCI option for margin array or formula selection

#### 3.2 If Tactical Awareness Missing
- Add more conditions to skip futility:
  ```cpp
  bool isTactical = givesCheck || 
                    isPromotion(move) || 
                    SEE(move) > 0 ||
                    moveIsKiller(move) ||
                    board.isHanging(from);
  ```

#### 3.3 If Implementation Bug Found
- Fix the bug (e.g., score propagation, interaction with other pruning)
- Document the fix and retest

## Phase 4: Extended Futility (Move-Based)

### If Basic Futility Extended Successfully
Implement futility pruning after making a move:

```cpp
// After making the move, before recursion
if (depth <= ExtendedFutilityDepth && !givesCheck) {
    int futilityValue = staticEval + FutilityMargin[depth] + capturedValue;
    if (futilityValue <= alpha) {
        unmakeMove();
        continue;
    }
}
```

## Phase 5: SPRT Testing Protocol

### Test Sequence
1. **Baseline**: Current implementation (depth 4, formula 150+60*d)
2. **Phase 1**: UCI control with same defaults - SPRT [-3.00, 3.00]
3. **Phase 2a**: Depth 5 with conservative margins - SPRT [0.00, 5.00]
4. **Phase 2b**: Depth 6 if 5 succeeds - SPRT [0.00, 5.00]
5. **Phase 3**: Improved implementation - SPRT [0.00, 8.00]
6. **Phase 4**: Extended futility if applicable - SPRT [0.00, 5.00]

### Success Criteria
- No regression at current settings
- Positive ELO gain when extending depth with proper margins
- Reduced node count without losing tactical accuracy
- Clean perft validation (bench = 19191913)

## Phase 6: Documentation and Merge

### Final Deliverables
1. Optimized futility pruning with UCI control
2. Documentation of optimal depth and margins
3. Analysis of why certain depths work/don't work
4. Recommendations for future improvements

## Risk Mitigation

### Potential Issues and Solutions
1. **Regression in tactical positions**: Add tactical awareness
2. **Too aggressive at high depths**: Use conservative margins
3. **Interaction with other pruning**: Test with other features disabled
4. **Score propagation bugs**: Verify return values carefully

## Timeline Estimate
- Phase 1 (UCI Control): 1-2 hours
- Phase 2 (Diagnostics): 2-3 hours  
- Phase 3 (Improvements): 2-4 hours
- Phase 4 (Extended): 1-2 hours (if applicable)
- Phase 5 (SPRT): 24-48 hours (OpenBench time)
- Phase 6 (Documentation): 1 hour

Total: 2-3 days of development + testing time

## Notes
- Current implementation tested as optimal at depth 4 with +37.63 ELO
- Prior attempts at depth 6 caused regressions (need to understand why)
- Stockfish uses depth 8, but with sophisticated tactical awareness
- Key is finding the right balance for SeaJay's evaluation characteristics