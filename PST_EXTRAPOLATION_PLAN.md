# PST Endgame Value Extrapolation Plan

## Based on SPSA Discoveries from 140k Games

### Discovered Patterns

#### 1. Pawn File Gradient (Endgame)
SPSA found inner center > outer center consistently:
- **Inner center (D&E)**: 100% base value
- **Outer center (C&F)**: ~87% of inner value
- **Extrapolated**:
  - B&G files: ~75% of inner value
  - A&H files: ~65% of inner value (edge pawns weak in endgames)

#### 2. Pawn Rank Values (Endgame)
Original values appear too high. Suggested adjustments:
```
Rank 2: Keep minimal (0-5)
Rank 3: 8-10 (was 10)
Rank 4: 20-25 (was 25)
Rank 5: 25-35 (was 40) - SPSA showed ~30
Rank 6: 45-55 (was 60) - SPSA showed ~53
Rank 7: 70-80 (was 90) - SPSA showed ~74
```

### Proposed Full Pawn Endgame PST

Based on SPSA findings, here's a complete pawn endgame table:

```cpp
// Pawn Endgame PST (incorporating SPSA discoveries)
// Format: mg/eg (we're updating eg values)

Rank 8: [All zeros - promoted]
Rank 7: 50/52  50/52  50/72  50/78  50/78  50/72  50/52  50/52
Rank 6: 40/36  40/36  30/44  40/52  40/52  30/44  40/36  40/36
Rank 5: 30/20  30/20  20/26  30/30  30/30  20/26  30/20  30/20
Rank 4: 20/16  20/16  10/20  20/24  20/24  10/20  20/16  20/16
Rank 3:  5/6    5/6    5/8   10/10  10/10   5/8    5/6    5/6
Rank 2:  0/0    0/0    0/0   -5/0   -5/0    0/0    0/0    0/0
Rank 1: [All zeros - no pawns]
        A      B      C      D      E      F      G      H
```

### Key Principles Applied:

1. **File Gradient**: 
   - D&E (inner): 100% 
   - C&F (outer): 87%
   - B&G: 75%
   - A&H: 65%

2. **Rank Progression**:
   - Smoothly increasing toward promotion
   - Lower absolute values than original
   - Reflects SPSA findings

3. **Symmetry**: 
   - Left-right mirror symmetry maintained
   - Center files most valuable

### Other Pieces (Minor Adjustments)

#### Rook Endgame
- **7th rank**: Reduce to 21 (was 25)
- **Active ranks (4-6)**: Keep at 10-11
- **Back rank**: Keep low (5)

#### Knight Endgame
- **Center**: 14-15 (was 15)
- **Extended center**: 9-10 (was 10)
- **Edges**: Keep penalties
- **Corners**: Keep strong penalties

#### Bishop Endgame
- **Long diagonals**: 18-19 (was 20)
- **Center**: 14-15 (was 15)
- **Edges**: Keep small penalties

#### Queen Endgame
- **Active squares**: 7-8 (was 10)
- **Center**: 9-10 (was 10)

## Implementation Options:

### Option 1: Direct Application
Apply these extrapolated values directly to the full PST tables.

### Option 2: Extended SPSA Run
Run SPSA on ALL squares (64 parameters per piece type) using these as starting values.

### Option 3: Zoned SPSA (Recommended)
Create zones based on patterns:
```
pawn_eg_inner_center_r5  // D&E files
pawn_eg_outer_center_r5  // C&F files  
pawn_eg_wing_r5          // B&G files
pawn_eg_edge_r5          // A&H files
```

This reduces parameters while capturing the discovered patterns.

### Option 4: Mathematical Formula
Implement PST as a formula based on discovered patterns:
```cpp
int pawnEndgameValue(Square sq) {
    int rank = rankOf(sq);
    int file = fileOf(sq);
    
    // Base value by rank (from SPSA)
    int baseValue[] = {0, 0, 8, 20, 28, 50, 75, 0};
    
    // File multiplier (from SPSA pattern)
    float fileMultiplier = 1.0f;
    if (file == 0 || file == 7) fileMultiplier = 0.65f;  // A&H
    else if (file == 1 || file == 6) fileMultiplier = 0.75f;  // B&G
    else if (file == 2 || file == 5) fileMultiplier = 0.87f;  // C&F
    // D&E remain at 1.0
    
    return baseValue[rank] * fileMultiplier;
}
```

## Validation Approach:

1. **Implement extrapolated values**
2. **Test against original PST** (should gain ELO)
3. **Run verification SPSA** with fewer parameters to fine-tune
4. **Compare to hand-tuned values**

## Expected Impact:

- **Immediate**: 5-10 ELO from better pawn endgame evaluation
- **After full tuning**: 10-20 ELO possible
- **Main benefit**: More accurate endgame transitions

## Next Steps:

1. Implement Option 3 (Zoned SPSA) for verification
2. Test extrapolated values in matches
3. Consider formula-based approach for production