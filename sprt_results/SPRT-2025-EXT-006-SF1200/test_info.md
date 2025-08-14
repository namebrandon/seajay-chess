# SPRT Test: SeaJay Stage 12 vs Stockfish (1200 ELO)

## Test Information
- **Test ID:** SPRT-2025-EXT-006-SF1200
- **Date:** Thu Aug 14 08:46:00 AM CDT 2025
- **Purpose:** Evaluate SeaJay against Stockfish limited to ~1200 ELO

## Engines
- **SeaJay:** Stage 12 TT Final - Estimated ~1300 ELO
- **Stockfish:** Limited to ~1200 ELO using Skill Level settings

## Stockfish Settings
Since UCI_Elo minimum is 1320, we'll use Skill Level to approximate 1200 ELO:
- **Skill Level:** 5 (approximately 1200 ELO)
- **Threads:** 1 (single-threaded for consistency)
- **Hash:** 16 MB (minimal)

Note: Skill Level mapping (approximate):
- Level 0: ~800 ELO
- Level 5: ~1200 ELO
- Level 10: ~1800 ELO
- Level 15: ~2300 ELO
- Level 20: Full strength

## SPRT Configuration
- **Hypothesis:** Testing if SeaJay Stage 12 is 50-150 ELO stronger than 1200-rated opponent
- **Elo bounds:** [50, 150]
- **Significance:** α = 0.05, β = 0.05
- **Time control:** 10+0.1
- **Opening book:** 4moves_test.pgn
- **Max rounds:** 300
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
Given the ~100 ELO expected advantage for SeaJay:
- SeaJay expected to score 60-65% (180-195 points out of 300 games)
- This would confirm Stage 12 reaches beginner club player strength
- Test validates our estimate of ~1300 ELO for Stage 12

## Notes
Stockfish at Skill Level 5 approximates intermediate amateur play (~1200 ELO).
This test shows the gap between SeaJay and typical online players.
Phase 3-4 improvements should significantly close this gap.
