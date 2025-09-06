# e8f7 Node Explosion Fix - Phase 2 Results Summary

## Problem Statement
Engine repeatedly chooses e8f7 (king move) causing node explosion in position:
`r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17`

## Phase 2 Implementation Results

### Phase 2.1: In-Check Evasion Ordering (FAILED)
**Branch:** bugfix/nodexp/20250906-e8f7-v2  
**Commit:** 2caec0c (REVERTED)  
**UCI Option:** InCheckEvasionOrdering  

**SPRT Test 459 Results:**
- **Elo:** -11.68 ± 8.09 (95% CI)
- **LLR:** -2.97 (failed, crossed lower bound -2.94)
- **Games:** 3958 (W: 1152, L: 1285, D: 1521)
- **Verdict:** FAILED - Significant strength loss

**Analysis:** Deprioritizing king moves in check positions hurt tactical play. The feature reduced nodes but at too high a cost in playing strength.

### Phase 2.2: Root Quiet Re-ranking (NEUTRAL)
**Branch:** bugfix/nodexp/20250906-e8f7-v2  
**Commits:** edfc210, 1f9f7ae  
**UCI Option:** RootKingPenalty (default: 0, range: 0-1000)  

**Implementation:**
- Made existing hardcoded -200 penalty toggleable via UCI
- Fixed bug: now excludes castling from penalty
- Only penalizes non-capturing, non-castling king moves at root

**SPRT Test Results:**

| Test | Penalty | Elo | LLR | Games | Verdict |
|------|---------|-----|-----|-------|---------|
| 460 | 0 | +0.97 ± 7.61 | -0.21 | 4298 | NEUTRAL |
| 462 | 100 | -2.80 ± 7.86 | -1.11 | 4336 | SLIGHTLY NEGATIVE |

**Local Testing (Problem FEN at depth 8):**
- Penalty=0: Chooses b7b6, 24,759 nodes
- Penalty=200: Chooses e8f7, 30,805 nodes  
- Penalty=400: Still chooses e8f7

**Conclusion:** Root move ordering has minimal impact. Default set to 0 (no penalty) as optimal.

## Key Learnings

1. **Root-level fixes insufficient:** The e8f7 problem persists despite move ordering changes
2. **Position analysis:** The position is NOT in check, limiting effectiveness of check-based solutions
3. **Move quality:** e8f7 may genuinely be a good move that overcomes penalties
4. **Search depth:** The issue appears deeper in the search tree, not at root level

## Why Phase 2 Approaches Failed

1. **Phase 2.1 (In-check evasion):** 
   - Too aggressive in limiting king escapes
   - Missed critical tactical moves
   - Position wasn't in check anyway

2. **Phase 2.2 (Root penalty):**
   - Root ordering has limited impact on deep searches
   - TT and deep search override root ordering
   - e8f7's evaluation advantage overcomes penalties

## Recommendations for Phase 3

1. **TT Interaction Analysis:**
   - Add logging to understand why e8f7 gets locked in
   - Check if shallow TT entries dominate
   - Verify replacement policy in tactical positions

2. **Main Search Modifications:**
   - Consider check extension caps (Phase 2.3)
   - Limit consecutive extensions
   - Add depth-based tapering

3. **Alternative Approaches:**
   - Focus on quiescence search improvements
   - Investigate why nodes explode specifically with this position
   - Consider position-specific heuristics

## Current Branch Status
- **Branch:** bugfix/nodexp/20250906-e8f7-v2
- **Latest commit:** 1f9f7ae
- **Bench:** 19191913
- **UCI Default:** RootKingPenalty=0

## Next Steps
Awaiting decision on:
1. Proceed to Phase 2.3 (check extension caps)
2. Jump to Phase 3 (TT diagnostics)
3. Alternative approach focusing on deeper search layers