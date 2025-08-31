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

## Implementation Status

### Phase 1: COMPLETED ✅
**Implementation Date**: August 31, 2025  
**Branch**: `feature/20250831-pst-interpolation`

#### SPRT Test Results
- **ELO Gain**: 46.50 ± 18.21 (95% CI)
- **SPRT**: 10.0+0.10s Threads=1 Hash=8MB
- **LLR**: 3.26 (-2.94, 2.94) [0.00, 8.00] - **PASSED** ✅
- **Games**: N: 932 W: 384 L: 260 D: 288
- **Penta**: [31, 83, 155, 125, 72]
- **Test URL**: https://openbench.seajay-chess.dev/test/351/
- **Bench**: 19191913 (unchanged) ✅

#### What Was Implemented
1. ✅ Phase calculation function (0-256 scale based on material)
2. ✅ PST interpolation using fixed-point arithmetic
3. ✅ UCI option `UsePSTInterpolation` (default: true)
4. ✅ Updated endgame PST values for all piece types
5. ✅ Integration with existing Board::pstScore() accumulator
6. ✅ Documentation updates (UCI.md, deferred items tracker)

#### Performance Impact
- **NPS**: No measurable impact (<1%)
- **ELO**: +46.50 ELO (significantly exceeding expectations)
- **Stability**: Rock-solid with 932 games played

### Next Steps & Strategic Refinements

Based on the exceptional Phase 1 results (46.50 ELO gain), here's the refined roadmap:

#### Lock In Strategy
1. **Default on**: `UsePSTInterpolation` enabled by default (already done)
2. **Retire toggle**: After one confirmatory SPRT, remove the UCI option
3. **Single phase API**: Centralize `phase0to256(board)` computed once per eval, reused everywhere ✅

#### Phase 2: Stage 2 PST Deltas (Ready to Start)
**Priority Order (based on impact)**:

##### 2a. Pawn/Rook First (Highest Impact)
- **Pawns**: 
  - Add modest EG-heavier deltas for centralization
  - Enhance passer posture bonuses
  - Tapered blending already in place
- **Rooks**: 
  - Further enhance 7th rank activity (partially done)
  - Increase open file bonuses in endgame
- **Critical**: Guard double scaling - audit to ensure no term is both PST-modified AND separately phase-scaled

##### 2b. Knights/Bishops/Queen (Conservative)
- **Knights**: Slightly worse in EG (mobility restricted)
- **Bishops**: Conservative EG differentiation for activity
- **Queens**: Slightly more activity bonuses in EG
- **Approach**: Avoid large swings that could destabilize play

#### Phase 3: Phase-Aware Non-PST Terms

##### Current State Analysis
- **King Safety**: Currently stepwise (OPENING+MIDDLEGAME → MG, ENDGAME → EG)
  - Location: `src/evaluation/king_safety.cpp`
  - Optional tidy-up: Convert to 0-256 continuous blend for consistency
  - Not required for correctness but improves smoothness

##### Implementation Priority
1. **Passed Pawns**: Taper UP toward EG (critical for conversion)
   - Add rank-aware exponential growth
   - Currently static - high impact opportunity

2. **King Safety**: Taper DOWN toward EG
   - Keep MG pressure strong
   - Convert from stepwise to continuous (optional)

3. **Mobility/Open Files**:
   - Slight MG emphasis for general mobility
   - Increase EG emphasis for rook on open/semi-open files
   - Rook on 7th rank scaling

4. **Bishop Pair** (future):
   - Not yet implemented in SeaJay
   - Slightly stronger in MG than EG
   - Add after basics work

#### Phase 4: Optional AB Tests (Behind Flags)

##### Pawn-Aware Phase
- Trial `phase += k*(pawnCount-8)` with small k
- Validate no tempo artifacts
- UCI flag for safe testing

##### Non-Linear Taper
- Sigmoid clamp for king safety only
- Keep everything else linear
- Test on tactical positions

#### Phase 5: Parameter Plumbing (Prep for SPSA)

##### Infrastructure Requirements
1. **Central table**: Move all MG/EG PST values and phase-scaled weights to single header/struct
2. **Dump/load**: UCI command to dump/load weights (compile-time initially)
3. **Naming discipline**: Consistent `MgEgScore(valMg, valEg)` for SPSA-friendliness

#### Phase 6: Instrumentation

##### Development Tools
1. **EvalTrace** (dev-only): Dump per-term MG/EG and final blended scores
2. **Invariants**: Unit tests for phase edges (0 and 256 must match pure EG/MG)

#### Testing Cadence
1. **Micro**: Bench and NPS check (<1-2% delta)
2. **SPRT**: Short runs (10k-20k) per cluster of PST deltas
   - Bounds: [0.0, 8.0] or [1.0, 10.0] depending on expected gain
3. **Confirmatory**: One longer SPRT after bundling best deltas

#### Risk/Performance Guards
- **O(1) blend only**: Always use incremental `board.pstScore()` ✅
- **No per-piece loops**: Already achieved ✅
- **Hashing**: TT doesn't depend on blended eval ✅
- **SMP**: All new state read-only ✅

#### Deferred for SPSA Tuning
- Phase weight constants (KNIGHT=1, BISHOP=1, ROOK=2, QUEEN=4)
- Expected additional 5-10 ELO from optimization
- Documented in `/workspace/docs/project_docs/deferred_items_tracker.md`

## Conclusion

Phase 1 implementation was a resounding success:
- **46.50 ELO gain** far exceeds the projected 5-15 ELO
- Infrastructure proved robust and efficient
- No performance regression
- Clean, maintainable implementation

The phased approach allowed for conservative initial implementation with excellent results. Phase 2 can now proceed with confidence.
