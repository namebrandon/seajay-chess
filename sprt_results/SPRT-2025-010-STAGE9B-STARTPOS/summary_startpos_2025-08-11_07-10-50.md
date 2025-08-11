# SPRT Test Results - Stage 9b Optimizations (STARTPOS Variant)

**Test ID:** SPRT-2025-010-STAGE9B-STARTPOS  
**Date:** 2025-08-11_07-10-50  
**Variant:** STARTPOS (no opening book)
**Status:** See console output

## Test Configuration
- **Opening:** Starting position for all games
- **No book variety:** Pure engine vs engine from move 1

## Engines Tested
- **Test Engine:** Stage 9b with dual-mode optimization
- **Base Engine:** Stage 9 baseline (no draw detection)

## Test Parameters
- **Elo bounds:** [0, 30]
- **Significance:** α = 0.05, β = 0.05
- **Time control:** 10+0.1

## Optimization Details
- Eliminated vector operations from search hot path
- Zero heap allocations during search
- Stack-based history for repetition detection
- Dual-mode system (game vs search)

## Results
Check console_output_startpos_2025-08-11_07-10-50.txt for detailed results.

## Comparison with Book Test
This STARTPOS variant can be compared with the book-based test (SPRT-2025-009-STAGE9B) to see:
- Difference in draw rates
- Consistency of Elo improvement
- Impact of opening variety

## Files Generated
- Console output: console_output_startpos_2025-08-11_07-10-50.txt
- Games PGN: games_startpos_2025-08-11_07-10-50.pgn
- Log file: /workspace/sprt_results/SPRT-2025-010-STAGE9B-STARTPOS/sprt_startpos_2025-08-11_07-10-50.log

## Note on Parallel Testing
Both book-based and STARTPOS tests are available for comparison:
- Book test: /workspace/sprt_results/SPRT-2025-009-STAGE9B/
- STARTPOS test: /workspace/sprt_results/SPRT-2025-010-STAGE9B-STARTPOS/

Compare results to validate optimization consistency across different starting conditions.
