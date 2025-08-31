# PST Phase Interpolation Implementation Plan

## Executive Summary
SeaJay already maintains an incremental Mg/Eg PST accumulator in Board (`m_pstScore: MgEgScore`) and exposes it via `Board::pstScore()`. We should leverage this to implement tapering with near-zero overhead: compute a single 0–256 phase weight per evaluation and linearly interpolate the already-summed mg/eg totals. Then, gradually introduce differentiated endgame PST values by piece. This approach maximizes impact while preserving NPS.

## Current State Analysis

### What Already Exists
1. **MgEgScore + PST Tables** (`src/evaluation/pst.h`)
   - PST values are defined as MgEgScore per piece/square.
   - King already has distinct mg/eg values; other pieces mostly identical mg==eg.

2. **Incremental PST Accumulator in Board** (`src/core/board.h/.cpp`)
   - Board maintains `m_pstScore: MgEgScore`, updated incrementally in all make/unmake paths.
   - This is from White’s perspective and already correct for both colors.
   - Critical enabler: interpolate totals without per-piece loops.

3. **Game Phase Utilities** (`src/search/game_phase.h`)
   - Categorical phase detection exists and is cached in evaluate().
   - We will add a light-weight continuous phase value (0–256) alongside it.

### What's Missing
1. **Interpolation Usage in Eval**
   - `evaluate()` currently uses `pstScore.mg` only; it needs to blend mg/eg using a continuous phase.
2. **Continuous Phase Value**
   - Add a fast 0–256 phase weight based on non-pawn material (NPM), with optional pawn adjustment.
3. **Differentiated EG PST for Non-king Pieces**
   - Introduce modest eg deviations for P/N/B/R/Q to realize tapering benefits without destabilizing play.

## Implementation Approach

### Phase 1: Define Endgame PST Values

#### 1.1 Pawn Endgame PST
```cpp
// Current: All values identical for mg/eg
MgEgScore(20, 20)  // Rank 6

// Proposed: Stronger advancement bonus in endgame
MgEgScore(20, 35)  // Rank 6 - passed pawns more valuable
MgEgScore(50, 90)  // Rank 7 - near promotion critical
```

**Rationale**: Passed pawns become exponentially more valuable in endgames

#### 1.2 Knight Endgame PST
```cpp
// Current: Knights same in both phases
MgEgScore(20, 20)  // Center squares

// Proposed: Slightly less central preference in endgame
MgEgScore(20, 15)  // Center squares
MgEgScore(-50, -40) // Edge squares (less penalty)
```

**Rationale**: Knights are slightly weaker in endgames but edge penalties should be reduced

#### 1.3 Bishop Endgame PST
```cpp
// Current: Bishops same in both phases
MgEgScore(15, 15)  // Long diagonals

// Proposed: Increased long diagonal bonus
MgEgScore(15, 20)  // Long diagonals
MgEgScore(-20, -10) // Corners (less penalty)
```

**Rationale**: Bishops become relatively stronger in open endgame positions

#### 1.4 Rook Endgame PST
```cpp
// Current: Rooks same in both phases
MgEgScore(10, 10)  // 7th rank

// Proposed: Much stronger 7th rank and activity
MgEgScore(10, 25)  // 7th rank
MgEgScore(0, 10)   // Active placement
```

**Rationale**: Rooks dominate endgames, especially on 7th rank

#### 1.5 Queen Endgame PST
```cpp
// Current: Queens same in both phases
MgEgScore(5, 5)  // Center control

// Proposed: More aggressive positioning
MgEgScore(5, 10)  // Center control
MgEgScore(-20, -5) // Back rank (less penalty)
```

**Rationale**: Queens need more activity in endgames

### Phase 2: Implement Phase Calculation (Continuous, Fast)

#### 2.1 Material-Based Phase Value
```cpp
// src/evaluation/phase.h (or alongside evaluate.cpp)
constexpr int PHASE_WEIGHT[6] = { 0, 1, 1, 2, 4, 0 }; // P,N,B,R,Q,K
constexpr int TOTAL_PHASE = 24; // 2N+2B+2R+Q per side (=12/side *2)

inline int phase0to256(const Board& board) noexcept {
    int phase = 0;
    phase += popCount(board.pieces(KNIGHT)) * PHASE_WEIGHT[KNIGHT];
    phase += popCount(board.pieces(BISHOP)) * PHASE_WEIGHT[BISHOP];
    phase += popCount(board.pieces(ROOK))   * PHASE_WEIGHT[ROOK];
    phase += popCount(board.pieces(QUEEN))  * PHASE_WEIGHT[QUEEN];
    // Scale to [0,256] with rounding; 256=full MG, 0=EG
    return std::clamp((phase * 256 + TOTAL_PHASE/2) / TOTAL_PHASE, 0, 256);
}
```

#### 2.2 Optional: Pawn-aware Adjustment
```cpp
inline int phase0to256PawnAware(const Board& board) noexcept {
    int phase = phase0to256(board);
    int pawnCount = popCount(board.pieces(PAWN));
    int pawnAdj = (pawnCount - 8) * 3; // ±3 per pawn from baseline
    return std::clamp(phase + pawnAdj, 0, 256);
}
```

### Phase 3: Implement Interpolation

#### 3.1 Use Board’s Incremental Mg/Eg Totals (No Per-Piece Loop)
```cpp
// In src/evaluation/evaluate.cpp
const MgEgScore& pst = board.pstScore();
int phase = phase0to256(board);      // or phase0to256PawnAware(board)
int inv   = 256 - phase;
// Fixed-point blend with rounding, avoid division costs (div by 256 => >>8)
int blended = (pst.mg.value() * phase + pst.eg.value() * inv + 128) >> 8;
Score pstScore(blended);
```

This reuses the maintained `m_pstScore` (Mg/Eg sums) for O(1) blending cost per evaluation.

#### 3.2 Incremental State
Already implemented as `Board::m_pstScore` (MgEgScore). No new fields or loops are required.

### Phase 4: Testing Strategy

#### 4.1 Correctness Testing
1. **Perft validation** - Ensure move generation unchanged
2. **Bench validation** - Must remain 19191913 nodes
3. **Interpolation unit tests** - Verify smooth transitions
4. **Edge cases** - Test with extreme material imbalances

#### 4.2 Performance Testing
1. **NPS measurement** - Should have minimal impact (<2%)
2. **Profile interpolation overhead** - Ensure division by 256 is optimized
3. **Cache behavior** - Monitor for increased cache misses

#### 4.3 Strength Testing
1. **Self-play testing** - Old vs new at various time controls
2. **SPRT bounds** - Suggest [0.00, 8.00] for expected gain
3. **Position suites** - Test on endgame positions specifically

## Implementation Phases

### Stage 1: Minimal Implementation (Conservative)
- Add interpolation using `board.pstScore()` (no PST value changes yet).
- Wire a UCI toggle `UsePSTInterpolation` (default: on).
- Expected: neutral to small positive (king PST already benefits), near-zero NPS cost.

### Stage 2: Gradual PST EG Differentiation
- Introduce modest eg deltas for P and R first (highest EG impact).
- Then N/B/Q with conservative adjustments.
- Keep linear interpolation.
- Expected: +10–15 ELO combined; tune via SPRT.

### Stage 3: Advanced Features (Optional)
- Pawn-aware phase adjustment (guarded flag).
- Piece/pattern-aware phase (defer until stable).
- Expected: up to +15–20 ELO total with tuning.
- Risk: Medium (interactions with other phase-scaled terms).

## Performance Considerations

### Memory Impact
- No additional memory if using immediate calculation
- +32 bytes per board if caching mg/eg scores
- Negligible impact on cache usage

### CPU Impact
- One fixed-point blend per evaluation (shift-based), no per-piece loop.
- Estimated overhead: ~0% (within noise of current evaluation time)

### Thread Safety
- All calculations are read-only on shared data
- Phase calculation is deterministic
- Ready for LazySMP parallelization

## Risk Analysis

### Low Risk Elements
- Infrastructure already exists and tested
- King already uses this system successfully
- Can be disabled via UCI option if needed
- No impact on move generation

### Medium Risk Elements
- Choosing optimal endgame values requires tuning
- Phase calculation formula affects transition smoothness
- May interact with other evaluation terms

### Mitigation Strategies
1. Start with interpolation only (no new PST deltas) to validate plumbing.
2. Add UCI option to disable interpolation.
3. Roll out PST eg deltas piece-by-piece with SPRT gating.
4. Use SPSA to tune eg deltas and phase weights if needed.

## Expected Outcomes

### Immediate Benefits (Stage 1)
- Better pawn endgame evaluation
- Improved rook activity in endgames
- 5-8 ELO improvement
- No NPS regression

### Full Implementation Benefits (Stage 2)
- Smooth evaluation transitions
- Better positional play in endgames
- 10-15 ELO improvement
- Foundation for further tuning

### Long-term Benefits
- Enables piece-specific endgame knowledge
- Allows SPSA tuning of both mg and eg values
- Better scaling with time control
- More human-like position evaluation

## Recommended Implementation Order

1. **Week 1**: Implement Phase Interpolation (no table changes)
   - Add `phase0to256()` (and optional pawn-aware version behind flag).
   - Blend `board.pstScore()` in evaluate() via fixed-point arithmetic.
   - Add UCI option `UsePSTInterpolation` on/off.

2. **Week 1-2**: Update PST EG Values
   - Start with pawns (most impactful)
   - Then rooks (second most impactful)
   - Finally knights, bishops, queens

3. **Week 2**: Testing
   - Bench/perft, NPS checks; SPRT on small deltas.

4. **Week 3**: Testing and Tuning
   - Run SPRT tests
   - Tune phase weights if needed
   - Document performance impact

## Success Metrics

### Must Have
- ✅ No perft regression (bench = 19191913)
- ✅ NPS impact ~0%
- ✅ Positive ELO gain (>0)
- ✅ Thread-safe implementation

### Should Have
- ✅ 10+ ELO improvement
- ✅ UCI configurable
- ✅ Improved endgame play
- ✅ Clean, maintainable code

### Nice to Have
- ✅ 15+ ELO improvement
- ✅ Incremental updates
- ✅ SPSA tuned values
- ✅ <1% NPS impact

## Conclusion

This implementation is a natural next step for SeaJay:
- Infrastructure is already in place
- Low risk with high reward potential
- Proven technique used by all strong engines
- Improves positional understanding significantly

The phased approach allows for conservative initial implementation with room for optimization based on testing results.
