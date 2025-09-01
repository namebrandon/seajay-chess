# SPSA Endgame Tuning Configuration (4-5 Hour Run)

## Selected Parameters (10 High-Impact Endgame Parameters)

### Configuration for OpenBench
```
pawn_eg_r5_d, int, 40, 20, 70, 4, 0.002
pawn_eg_r5_e, int, 40, 20, 70, 4, 0.002
pawn_eg_r6_d, int, 60, 30, 100, 5, 0.002
pawn_eg_r6_e, int, 60, 30, 100, 5, 0.002
pawn_eg_r7_center, int, 90, 50, 150, 6, 0.002
rook_eg_7th, int, 25, 15, 40, 3, 0.002
rook_eg_active, int, 10, 5, 20, 2, 0.002
knight_eg_center, int, 15, 5, 25, 2, 0.002
bishop_eg_long_diag, int, 20, 10, 35, 3, 0.002
queen_eg_active, int, 10, 5, 20, 2, 0.002
```

## OpenBench Settings
- **Test Type**: SPSA
- **Base Branch**: main
- **Test Branch**: feature/20250831-pst-interpolation
- **Book**: endgames.epd (157k endgame positions)
- **Time Control**: 10+0.1
- **Iterations**: 25000-30000 (for 4-5 hours)
- **Pairs Per Iteration**: 16
- **Alpha**: 0.602
- **Gamma**: 0.101
- **A-Ratio**: 0.05 (more aggressive early exploration)
- **Reporting**: Batched
- **Distribution**: Single

## Why These Parameters?

### Critical Pawn Endgame Values (5 params)
- **pawn_eg_r5_d/e**: Rank 5 central pawns - crucial for breakthrough timing
- **pawn_eg_r6_d/e**: Rank 6 central pawns - passed pawn evaluation
- **pawn_eg_r7_center**: Rank 7 all center files - near-promotion bonus

### Rook Endgame Values (2 params)
- **rook_eg_7th**: Seventh rank dominance (classic endgame principle)
- **rook_eg_active**: Active rook positioning (ranks 4-6)

### Minor Piece Endgame (2 params)
- **knight_eg_center**: Knights need centralization in endgames
- **bishop_eg_long_diag**: Long diagonal control critical in endgames

### Queen Endgame (1 param)
- **queen_eg_active**: Queen activity vs passivity

## Expected Timeline

With 10 parameters at 10+0.1:
- **25,000 iterations** = 800,000 games
- **Estimated time**: 4-5 hours
- **Clear signal expected by**: 10,000 iterations (40%)
- **Final convergence**: 20,000+ iterations

## Success Metrics

Look for:
1. **Consistent movement** after 5,000 iterations (not just noise)
2. **Pawn values** likely to increase (passed pawns undervalued?)
3. **Rook 7th rank** might increase (Rook on 7th is powerful)
4. **Minor pieces** might decrease slightly (less important in endgames)

## Quick Validation Before Starting

Test that all parameters work:
```bash
for param in pawn_eg_r5_d pawn_eg_r6_d pawn_eg_r7_center rook_eg_7th rook_eg_active knight_eg_center bishop_eg_long_diag queen_eg_active; do
    echo "Testing $param with value 100.5..."
    echo -e "uci\nsetoption name $param value 100.5\nquit" | ./bin/seajay 2>&1 | grep "set to"
done
```

## Notes

- These 10 parameters cover the most important endgame piece-square relationships
- The endgame book ensures every game tests these values
- Reduced from 20 to 10 parameters for faster convergence in 4-5 hours
- Focus on pawns and rooks as they're most common in endgames
- C_end values chosen to allow meaningful exploration without instability

## After the Run

When you return, look for:
1. Parameters that moved significantly (>10% from starting value)
2. Parameters that barely moved (might be optimal already)
3. Any parameters hitting their bounds (might need wider range)

Save the final values and consider a follow-up run with:
- Tighter bounds around discovered optima
- Additional parameters if these show improvement
- Longer run (50k+ iterations) for final tuning

Good luck with the tuning run!