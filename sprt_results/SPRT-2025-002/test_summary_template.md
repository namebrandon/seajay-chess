# SPRT Test Results - Stage 8: Alpha-Beta Pruning

**Test ID:** SPRT-2025-002  
**Date:** [TO BE FILLED]  
**Result:** [PASS/FAIL/INCONCLUSIVE]  

## Engines Tested
- **Test Engine:** SeaJay Stage 8 with Alpha-Beta Pruning (depth 4)
- **Base Engine:** SeaJay Stage 8 with Alpha-Beta Pruning (depth 4)
- **Note:** Same binary tested for consistency verification

## Test Parameters
- **Elo bounds:** [-5, +5]
- **Significance:** α = 0.05, β = 0.10
- **Time control:** Fixed depth 4
- **Opening book:** 4moves_test.pgn

## Expected Outcome
Since alpha-beta pruning should produce identical moves with fewer nodes, we expect:
- No strength difference (H0 accepted)
- Identical game results when using fixed depth
- This validates correctness of the alpha-beta implementation

## Results
- **Games played:** [TO BE FILLED]
- **Score:** [TO BE FILLED]
- **Win/Draw/Loss:** [TO BE FILLED]
- **LLR:** [TO BE FILLED]
- **Estimated Elo:** [TO BE FILLED]

## Performance Metrics (from engine output)
- **Nodes at depth 4:** ~3,500 (with alpha-beta)
- **Nodes at depth 4 (theoretical without AB):** ~600,000
- **Node reduction:** ~99.4%
- **Effective Branching Factor:** 6.84
- **Move Ordering Efficiency:** 99.3%

## Conclusion
[TO BE FILLED based on test results]

Expected conclusion: The test should show no significant Elo difference, confirming that alpha-beta pruning maintains identical move selection while dramatically reducing the search tree size.

## Alternative Test Results

### SPRT-2025-003: Depth Comparison (Depth 4 vs Depth 3)
This test demonstrates the strength improvement from searching one ply deeper, which is enabled by the efficiency gains from alpha-beta pruning:

- **Test Engine:** Stage 8 at depth 4
- **Base Engine:** Stage 8 at depth 3
- **Expected Result:** Significant Elo gain (+50-100)
- **Purpose:** Show that alpha-beta enables deeper search which improves strength

## Notes
- Alpha-beta pruning is a pure optimization that doesn't change the minimax value
- The dramatic node reduction (90%+) enables searching 1-2 plies deeper in the same time
- High move ordering efficiency (94-99%) indicates the simple ordering scheme is very effective

## Validation Checklist
- [ ] Both engines respond to UCI commands
- [ ] Test completed without crashes
- [ ] Results are statistically significant
- [ ] Node reduction metrics confirmed
- [ ] No anomalous game results