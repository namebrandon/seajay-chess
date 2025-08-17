# Stage 11 Remediation Audit - MVV-LVA Move Ordering

## Original Requirements

From Stage 11 implementation summary:
- Implement Most Valuable Victim - Least Valuable Attacker (MVV-LVA) move ordering
- Improve alpha-beta pruning efficiency by ordering captures
- Formula: `score = VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker]`
- Expected 15-30% node reduction, +50-100 Elo improvement
- Feature flag for A/B testing

## Known Issues Going In

1. **Compile-time flag `ENABLE_MVV_LVA`**:
   - Used in src/search/negamax.cpp (lines 6, 82)
   - Used in src/search/quiescence_optimized.cpp (lines 87, 300)
   - Defined in CMakeLists.txt (line 54)
   - Violates "NO COMPILE-TIME FEATURE FLAGS" principle

## Additional Issues Found (Chess Engine Expert Analysis)

### CRITICAL BUG: Inconsistent Scoring Systems
1. **Dual Table System Conflict**:
   - Header file (`move_ordering.h`) defines simple arrays with formula
   - Implementation (`move_ordering.cpp`) uses different precomputed 2D table
   - Values don't match! Header suggests PxQ=899, implementation returns 505
   - Tests expect header values but code uses table values
   - This is a fundamental correctness issue

2. **Table Index Confusion**:
   - Table indexed as `[victim_type][attacker_type]`
   - First column (index 0) is NO_PIECE_TYPE, not PAWN
   - Error-prone indexing scheme

### Architectural Issues

3. **Stage Mixing - SEE Integration**:
   - Stage 11 (MVV-LVA) is intertwined with Stage 15 (SEE)
   - The `ENABLE_MVV_LVA` flag actually enables SEE-aware ordering
   - `g_seeMoveOrdering` is used even though this is Stage 11
   - Violates single-stage focus principle

4. **Sort Stability Problem**:
   - Uses `std::sort` instead of `std::stable_sort`
   - Equal MVV-LVA scores get arbitrary ordering
   - Can cause search instability and non-deterministic behavior
   - No explicit tiebreaking implementation

### Code Quality Issues

5. **Massive Code Duplication**:
   - Inline scoring in sort lambda (lines 126-185) duplicates `scoreMove()` logic
   - Makes maintenance error-prone
   - Statistics only updated in one path

6. **Performance Inefficiency**:
   - Duplicated scoring logic instead of calling `scoreMove()`
   - Modern compilers would inline anyway

## Implementation Deviations

1. **Not Pure MVV-LVA**: Implementation includes SEE integration from Stage 15
2. **Wrong Values**: Using different scoring table than documented
3. **No Tiebreaking**: Missing from-square tiebreaker mentioned in docs

## Missing Features

- No UCI option for MVV-LVA control (though expert recommends always-on)
- No explicit tiebreaking strategy
- Statistics not collected in optimized path

## Incorrect Implementations

1. **Scoring Values**: Mismatch between header definitions and implementation
2. **Sort Algorithm**: Should use stable_sort for deterministic behavior
3. **Stage Separation**: MVV-LVA mixed with SEE

## Testing Gaps

- Tests expect wrong values (header values vs implementation values)
- No test for sort stability
- No test for statistics collection in optimized path
- No verification that MVV-LVA works without SEE

## Utilities Related to This Stage

- `tests/test_mvv_lva.cpp` - Unit tests (may have wrong expected values)
- `tests/test_mvv_lva_phase7.cpp` - Performance validation

## Recommended Remediation Actions

### Priority 1 - Critical Fixes
1. **Fix Scoring System**:
   - Remove dual table system
   - Use simple formula: `VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker]`
   - Ensure tests match implementation

2. **Remove Compile Flag**:
   - Make MVV-LVA always active (no UCI option needed per expert)
   - Remove `ENABLE_MVV_LVA` from all files
   - Remove from CMakeLists.txt

3. **Separate from SEE**:
   - Stage 11 should be pure MVV-LVA
   - Remove SEE integration code
   - Use simple `MvvLvaOrdering` directly, not `g_seeMoveOrdering`

### Priority 2 - Correctness Fixes
4. **Fix Sort Stability**:
   - Change to `std::stable_sort` or add explicit tiebreaker
   - Ensure deterministic move ordering

5. **Remove Code Duplication**:
   - Call `scoreMove()` from sort lambda
   - Let compiler handle inlining

### Priority 3 - Clean Up
6. **Update Documentation**:
   - Document that MVV-LVA is always on
   - Clarify actual scoring values used
   - Remove references to feature flags

## Expert Recommendation Summary

Per chess-engine-expert analysis:
- MVV-LVA should **always be enabled** - it's fundamental to alpha-beta efficiency
- No valid reason to disable it in production
- The only future choice should be between MVV-LVA and SEE (Stage 15)
- Current implementation has correct special case handling (promotions, en passant)
- Main issues are architectural (mixing stages) and correctness (dual scoring systems)

## Estimated Remediation Effort

- **Critical Fixes**: 2-3 hours (scoring system, compile flag removal)
- **Stage Separation**: 1-2 hours (remove SEE integration)
- **Testing**: 1 hour (fix tests, verify correctness)
- **Total**: 4-6 hours

## Risk Assessment

- **High Risk**: Scoring system bug could affect move ordering quality
- **Medium Risk**: SEE integration removal needs careful testing
- **Low Risk**: Sort stability fix is straightforward

This audit reveals more serious issues than just compile flags - there's a fundamental correctness problem with the scoring system that needs immediate attention.