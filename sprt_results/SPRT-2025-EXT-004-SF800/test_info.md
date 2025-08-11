# SPRT Test: SeaJay vs Stockfish (800 ELO)

## Test Information
- **Test ID:** SPRT-2025-EXT-004-SF800
- **Date:** Mon Aug 11 10:53:56 AM CDT 2025
- **Purpose:** Evaluate SeaJay against Stockfish limited to ~800 ELO

## Engines
- **SeaJay:** Version 2.9.1-draw-detection (Phase 2 complete, ~650 ELO estimated)
- **Stockfish:** Limited to ~800 ELO using Skill Level 0-1

## Stockfish Settings
- **Skill Level:** 0 (weakest setting, approximately 800 ELO)
- **UCI_LimitStrength:** Not used (minimum is 1320 ELO)
- **Threads:** 1 (single-threaded for consistency)
- **Hash:** 16 MB (minimal)

## SPRT Configuration
- **Hypothesis:** Testing if SeaJay is within 50-250 ELO of 800-rated opponent
- **Elo bounds:** [-250, -50]
- **Significance:** α = 0.05, β = 0.05
- **Time control:** 10+0.1
- **Opening book:** 4moves_test.pgn
- **Max rounds:** 500
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
Given the ~150 ELO expected difference:
- SeaJay expected to score 20-30% (100-150 points out of 500 games)
- This would demonstrate SeaJay is approaching beginner human level
- Test will show if SeaJay can win some games against 800-rated play

## Notes
Stockfish at Skill Level 0 plays at approximately 800 ELO.
This represents a reasonable challenge for SeaJay's current development.
Success here would validate Phase 2 achievements.
