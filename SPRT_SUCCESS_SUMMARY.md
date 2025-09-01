# SPRT Success: +10.38 ELO from SPSA-Tuned PST

## Test Results
- **Test ID**: 363
- **URL**: https://openbench.seajay-chess.dev/test/363/
- **ELO Gain**: +10.38 ± 6.38 (95% confidence)
- **LLR**: 2.96 (passed [-2.00, 2.00])
- **Games**: 6,496 (W: 2356, L: 2162, D: 1978)
- **Time Control**: 10+0.1s

## What We Changed

### SPSA Discovered Patterns (from 140k games):
1. **Inner center pawns (D&E files) > Outer center pawns (C&F files)**
   - Rank 5: 29 vs 27
   - Rank 6: 51 vs 48
   
2. **All endgame values reduced by ~25-30%**
   - Rank 5: 40 → 29 (D&E), 40 → 27 (C&F)
   - Rank 6: 60 → 51 (D&E), 60 → 48 (C&F)
   - Rank 7: 90 → 75

3. **Piece values slightly reduced**
   - Rook 7th: 25 → 20
   - Queen active: 10 → 7

## Critical Bug Fixes That Enabled This

### 1. Float Rounding Bug
- **Problem**: UCI parameters were truncating floats (90.6 → 90) instead of rounding
- **Impact**: SPSA couldn't explore values above starting point
- **Fix**: Added std::round for proper handling

### 2. Parameter Conflict Bug  
- **Problem**: pawn_eg_r5_d and pawn_eg_r5_e were overwriting each other
- **Impact**: Parameters were fighting for control of same squares
- **Fix**: Remapped so _d controls D&E files, _e controls C&F files

## Final UCI Parameters Used

```
pawn_eg_r3_d=8 pawn_eg_r3_e=7 pawn_eg_r4_d=18 pawn_eg_r4_e=16 pawn_eg_r5_d=29 pawn_eg_r5_e=27 pawn_eg_r6_d=51 pawn_eg_r6_e=48 pawn_eg_r7_center=75 rook_eg_7th=20 rook_eg_active=12 rook_eg_passive=5 knight_eg_center=15 knight_eg_extended=10 knight_eg_edge=-25 knight_eg_corner=-40 bishop_eg_long_diag=19 bishop_eg_center=14 bishop_eg_edge=-5 queen_eg_center=9 queen_eg_active=7 queen_eg_back=5
```

## Lessons Learned

1. **SPSA works** when implementation bugs are fixed
2. **Endgame books** provide cleaner signal for endgame parameters
3. **Non-obvious patterns** (inner > outer center) can be discovered by SPSA
4. **Extrapolation is valid** - patterns found in subset can extend to full table
5. **Original hand-tuned values** were systematically too high

## Next Steps

1. ✅ Merge these PST values to main branch
2. Consider SPSA tuning of remaining parameters (with float rounding fix)
3. Run longer SPSA with more parameters now that infrastructure is proven
4. Fix float rounding bug in all other UCI integer parameters

## Total ELO Gain Path
- Phase 1 (interpolation): +5-15 ELO
- Phase 2a (manual tuning): +30-40 ELO (but had regression issues)
- SPSA tuning: +10.38 ELO (clean, validated gain)

The SPSA infrastructure is now proven and can be used for future parameter optimization.