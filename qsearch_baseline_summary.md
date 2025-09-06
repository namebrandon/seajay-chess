# QSearch Baseline Test Summary

## Test Configuration
- **Date**: 2025-09-06
- **Branch**: bugfix/20250906-qsearch-improvements
- **Commit**: a9911a7
- **Bench**: 19191913
- **Problem Position**: r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17

## Key Findings

### 1. **Confirmed: e8f7 Problem Exists**
With quiescence enabled, the engine switches to e8f7 at depth 4 and sticks with it:
- Depth 1-2: d5d4 (correct)
- Depth 3: b7b6 (different)
- **Depth 4-10: e8f7 (problematic)**

### 2. **Confirmed: Quiescence is the Cause**
Without quiescence, the engine never chooses e8f7:
- Chooses reasonable moves: c8d7, d5d4, g4a4, e8d7
- Much more varied move selection
- No fixation on king moves

### 3. **Node Count Comparison**

| Depth | With Quiescence | Without Quiescence | Ratio |
|-------|----------------|-------------------|-------|
| 4     | 4,568          | 585               | 7.8x  |
| 6     | 8,214          | 3,487             | 2.4x  |
| 8     | 29,789         | 15,532            | 1.9x  |
| 10    | 71,141         | 59,038            | 1.2x  |

**Observation**: Quiescence causes significant node explosion, especially at shallow depths where the e8f7 decision is made.

### 4. **maxCheckPly Testing Issue**
The maxCheckPly parameter is not currently exposed as a UCI option, so we couldn't test different values directly. This needs to be implemented as part of Phase 1.

### 5. **Move Characteristics at Depth 4**
- **With Quiescence**: e8f7 (134 cp, 4568 nodes, seldepth 10)
- **Without Quiescence**: d5d4 (162 cp, 585 nodes, seldepth 6)

The quiescence search is:
- Finding a worse move (134 cp vs 162 cp)
- Using 7.8x more nodes
- Searching 4 plies deeper in selective depth

## Baseline Metrics for Improvement

### Target Improvements:
1. **Move Quality**: Should choose d5d4 or c8d7 at depth 4+, not e8f7
2. **Node Reduction**: Reduce node count at depth 4 from 4,568 to <1,000
3. **Score Stability**: Maintain score around 160-175 cp (not drop to 134)
4. **PV Stability**: No move flip at depth 3-4

### Success Criteria for Phase 1:
- [ ] No e8f7 selection at depths 4-10
- [ ] Node count at depth 4 < 2,000
- [ ] Score remains > 150 cp at all depths
- [ ] Tactical suite performance not degraded

## Next Steps
1. Implement maxCheckPly as UCI option
2. Test with maxCheckPly = 3
3. Verify fix on problem position
4. Run broader tactical suite
5. Prepare for OpenBench testing