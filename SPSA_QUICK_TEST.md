# SPSA Quick Validation Test - Endgame Book Version

## Test Configuration for OpenBench

### SPSA Parameters (4 high-impact parameters)
```
pawn_eg_r6_d, int, 60, 30, 100, 5, 0.002
pawn_eg_r6_e, int, 60, 30, 100, 5, 0.002
pawn_eg_r7_center, int, 90, 50, 150, 6, 0.002
rook_eg_7th, int, 25, 15, 40, 3, 0.002
```

### OpenBench Settings - FAST VALIDATION (30-45 minutes)
- **Test Type**: SPSA
- **Base Branch**: main  
- **Test Branch**: feature/20250831-pst-interpolation
- **Book**: endgames.epd (157k endgame positions!)
- **Time Control**: 5+0.05 (fast for endgames)
- **Iterations**: 1000 (should be enough with endgame book)
- **Pairs Per**: 16
- **Alpha**: 0.602
- **Gamma**: 0.101  
- **A-Ratio**: 0.1
- **Reporting**: Batched
- **Distribution**: Single

### Expected Timeline
- **Total Games**: ~32,000 (1000 iterations × 16 pairs × 2)
- **Time**: 30-45 minutes with 5+0.05
- **Clear Signal**: Should see parameter movement within first 200-300 iterations

### What Success Looks Like

Within first 300 iterations you should see:
- `pawn_eg_r7_center` moving UP from 90 (likely toward 110-130)
- `pawn_eg_r6_d/e` showing consistent movement (probably up)
- `rook_eg_7th` adjusting (likely up from 25 toward 30-35)

### If Test Succeeds → Full Test

Once validated, run FULL 20-parameter suite:
- Same endgame book
- Increase iterations to 50,000-100,000
- Time control: 10+0.1 (for stability)
- Expected time: 5-10 hours
- Expected gain: Significant in endgames

## Alternative: ULTRA-QUICK Test (10-15 minutes)

If you want even faster validation:
```
# Just 2 parameters
pawn_eg_r7_center, int, 90, 50, 150, 6, 0.002
rook_eg_7th, int, 25, 15, 40, 3, 0.002
```
- Iterations: 500
- Pairs Per: 8  
- Time: 3+0.03
- Should see movement within 10-15 minutes

## Notes

The endgame book is PERFECT because:
1. Every position uses our PST endgame values
2. No noise from opening/middlegame positions
3. Much faster convergence
4. Clearer validation signal

With 157k positions, you have excellent variety for testing!