# SPRT Test: SeaJay Stage 14 vs Stockfish (1200 ELO)

## Test Information
- **Test ID:** SPRT-2025-EXT-008-SF1200-STAGE14
- **Date:** Fri Aug 15 10:40:51 AM CDT 2025
- **Purpose:** Evaluate SeaJay Stage 14 against Stockfish limited to ~1200 ELO

## Engines
- **SeaJay:** Stage 14 C10-CONSERVATIVE - Estimated ~2250 ELO
- **Stockfish:** Limited to ~1200 ELO using Skill Level settings + 1 CPU core

## SeaJay Stage 14 Achievements
- +300 ELO improvement over Stage 13
- Basic quiescence search with captures and check evasions
- Conservative delta pruning (900cp/600cp margins)
- MVV-LVA move ordering in quiescence
- Transposition table integration
- Final candidate after extensive debugging (C1-C10 candidates)

## Stockfish Settings
Using Skill Level + single CPU core for fair comparison:
- **Skill Level:** 5 (consistent benchmark level)
- **Threads:** 1 (single-threaded - CRITICAL for fairness with SeaJay)
- **Hash:** 16 MB (minimal)

Note: SeaJay is not multi-threaded, so Stockfish must also use 1 CPU core.

Skill Level strength is approximate:
- Level 0: Weakest (makes obvious blunders)
- Level 5: Our benchmark level (estimated 1000-1400 ELO range)
- Level 10: Intermediate 
- Level 15: Strong club level
- Level 20: Full strength

## SPRT Configuration
- **Hypothesis:** Testing if SeaJay Stage 14 is 300-500 ELO stronger than 1200-rated opponent
- **Elo bounds:** [300, 500]
- **Significance:** α = 0.05, β = 0.05
- **Time control:** 10+0.1
- **Opening book:** 4moves_test.pgn
- **Max rounds:** 200
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
With Stage 14's massive improvements:
- Expected score: 80-90% (SeaJay should dominate)
- SPRT should terminate quickly with H1 acceptance
- This tests our estimated ~2250 ELO strength against 1200 benchmark
- Demonstrates progress from previous stages

## Historical Context
- Stage 12: ~1800 ELO (TT implementation)  
- Stage 13: ~1950 ELO (iterative deepening)
- Stage 14: ~2250 ELO (quiescence search +300 Elo)

## Fairness Considerations
- Single CPU core for Stockfish (critical for fair comparison)
- Same time controls for both engines
- Same opening book positions
- Sequential games to avoid resource contention

## Notes
This test validates Stage 14's substantial improvement against our external benchmark.
The ~1000 ELO advantage should result in dominant performance.
Primary goal is to confirm Stage 14's strength estimation is realistic.
