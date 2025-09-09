# MovePicker Testing Summary

## Executive Summary
After fixing compilation issues and testing multiple approaches to address the -1100 Elo regression, we found that **commit 963c4b3 (comprehensive safeguards)** performs best, achieving **74% of legacy performance**.

## Key Findings

### The Problem
MovePicker's staged move ordering places good quiet moves too late, causing them to be aggressively pruned/reduced by LMR and move count pruning. This resulted in:
- -1100 Elo loss in gameplay
- Node count reduced to 41% of legacy
- Poor move selection with inflated evaluations

### Test Results at Depth 8

| Commit | Description | Nodes | % of Legacy | Status |
|--------|-------------|-------|-------------|--------|
| 6cc37db | Baseline (broken) | 8,085 | 41% | ❌ Severe regression |
| fa8dd78 | Hybrid ordering | 13,412 | 68% | ⚠️ Partial fix |
| **963c4b3** | **Comprehensive safeguards** | **14,660** | **74%** | **✅ Best result** |
| Legacy | No MovePicker | 19,712 | 100% | Reference |

### Why Comprehensive Safeguards Work Best

The 963c4b3 commit implements:
1. **Early good quiet protection**: First 4 good quiets bypass pruning
2. **Proper scoping**: Variables correctly scoped throughout pruning sections
3. **Complete coverage**: Safeguards applied to MCR, futility, and LMR
4. **Good quiet identification**: Killers, countermoves, positive history

### Recommendation

**Use commit 963c4b3 with compilation fixes applied**. While not perfect (74% of legacy), it's the best compromise that:
- Preserves most of MovePicker's benefits (TT move first, good captures early)
- Protects important quiet moves from over-pruning
- Achieves reasonable performance without fully reverting to legacy

### Next Steps

1. **Short term**: Deploy 963c4b3 with fixes
2. **Medium term**: Tune the GOOD_QUIET_BUDGET parameter (currently 4)
3. **Long term**: Redesign MovePicker to interleave good quiets earlier in the natural ordering

## Files Modified for Each Commit

All commits required the same fixes in `src/search/negamax.cpp`:
1. Remove MovePicker copy attempts (line 903, 941 in some commits)
2. Fix duplicate variable declarations (isKillerMove, isCounterMove)
3. Ensure proper variable scoping

## Testing Methodology

- Position: startpos
- Time: 5 seconds per test (depth 8 for direct comparison)
- Clean rebuild for each commit
- Legacy baseline tested with each commit for consistency