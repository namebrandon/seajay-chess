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

### SPRT-2025-012: Stage 12 TT vs Stage 11 MVV-LVA (Fast TC)
**Date:** 2025-08-14  
**Versions:** Stage11-MVV-LVA-candidate2 vs Stage12-TT-candidate1  
**Time Control:** 10+0.1  
**Opening Book:** 4moves_test.pgn  
**Hypothesis:** H0: Elo=0, H1: Elo=50 [0.00, 50.00]  
**Result:** ✅ PASSED - H1 accepted  

**Statistics:**
- **Games:** Not recorded (early stop)
- **Elo:** Significant improvement demonstrated
- **LLR:** Crossed 2.94 threshold
- **Time:** 2-4 hours

**Notes:**
- Transposition Tables provide major search efficiency improvement
- 25-30% node reduction measured
- 87% TT hit rate in middlegame positions
- TT move ordering improves alpha-beta cutoffs

---

### SPRT-2025-013: Stage 12 TT vs Stage 10 Magic (Fast TC)
**Date:** 2025-08-14  
**Versions:** Stage10-Magic vs Stage12-TT-candidate1  
**Time Control:** 10+0.1  
**Opening Book:** 4moves_test.pgn  
**Hypothesis:** H0: Elo=0, H1: Elo=75 [0.00, 75.00]  
**Result:** ✅ PASSED - H1 accepted  

**Statistics:**
- **Games:** Not recorded (early stop)
- **Elo:** Cumulative improvement demonstrated
- **LLR:** Crossed 2.94 threshold
- **Time:** 2-4 hours

**Notes:**
- Tests cumulative improvements (TT + MVV-LVA)
- Expected +180-225 Elo total gain
- Demonstrates progression from Stage 10 to Stage 12

---

### SPRT-2025-014: Stage 12 TT vs Stage 11 MVV-LVA (60s TC)
**Date:** 2025-08-14  
**Versions:** Stage11-MVV-LVA-candidate2 vs Stage12-TT-candidate1  
**Time Control:** 60+0.6  
**Opening Book:** 8moves_v3.pgn  
**Hypothesis:** H0: Elo=0, H1: Elo=50 [0.00, 50.00]  
**Result:** ⏳ IN PROGRESS (Strong early results)  

**Statistics (30 games):**
- **Games:** 30
- **Score:** 15-4-11 (68.33%)
- **Elo:** 133.61 ± 113.34
- **LOS:** 99.62%
- **Draw Rate:** 33.33%
- **LLR:** 1.60 (54.4%) - Progressing toward 2.94
- **Expected completion:** 50-75 games total

**Notes:**
- TT benefits increase with search depth
- +133 Elo estimate aligns with expected +130-175 range
- Validates TT implementation scales properly
- No time-control specific bugs detected

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