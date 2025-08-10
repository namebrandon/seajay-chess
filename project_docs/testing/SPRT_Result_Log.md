# SeaJay Chess Engine - SPRT Result Log

This document tracks all SPRT (Sequential Probability Ratio Test) results for the SeaJay chess engine development.

## SPRT Test Format

Each test entry includes:
- **Test ID**: Unique identifier (e.g., SPRT-2025-001)
- **Date**: Test completion date
- **Versions**: Old vs New engine versions tested
- **Time Control**: Time control used for games
- **Opening Book**: Opening positions used
- **Hypothesis**: H0 and H1 Elo bounds
- **Result**: PASS/FAIL with final statistics
- **Games**: Number of games played
- **Elo Gain**: Measured Elo improvement
- **Notes**: Additional observations

---

## Test Results

### SPRT-2025-007-FIXED: Stage 9 PST Implementation
**Date:** 2025-08-10  
**Versions:** Stage8-AlphaBeta vs Stage9-PST-Fixed  
**Time Control:** 10+0.1  
**Opening Book:** 4moves_test.pgn  
**Hypothesis:** H0: Elo=0, H1: Elo=50 [0.00, 50.00]  
**Result:** ✅ PASSED - H1 accepted  

**Statistics:**
- **Games:** 44
- **Score:** 25-0-19 (78.41%)
- **Elo:** 224.04 ± 91.29
- **LOS:** 100.00%
- **Draw Rate:** 27.27%
- **LLR:** 2.95 (100.2%) - Crossed 2.94 threshold
- **Time:** 17 minutes 27 seconds

**Notes:**
- Massive Elo gain of ~224 from PST implementation
- Zero losses in 44 games (perfect win record)
- All draws were by 3-fold repetition (expected without repetition detection)
- PST provides strong positional guidance leading to dominant performance
- Fast SPRT convergence indicates clear superiority

---

### Previous SPRT Tests

*(Earlier SPRT tests from Phase 2 will be documented here as they are discovered in the sprt_results directory)*

---

## Summary Statistics

**Total SPRT Tests Completed:** 1  
**Tests Passed:** 1  
**Tests Failed:** 0  
**Average Elo Gain (Passed Tests):** 224.04  

## Testing Infrastructure

**Tool:** fast-chess  
**Location:** `/workspace/external/testers/fast-chess/`  
**Script:** `/workspace/tools/scripts/run-sprt.sh`  
**Results Directory:** `/workspace/sprt_results/`  

## Notes

- Stage 9 shows exceptional improvement from PST evaluation
- Repetition detection (Stage 10) should reduce draw rate
- Future tests will include longer time controls for more accurate Elo measurements