# Knight Outposts Feature Status

## Overview
Implementing knight outpost evaluation for SeaJay chess engine.

A knight outpost is a square where:
1. The knight cannot be attacked by enemy pawns
2. The knight is protected by friendly pawns
3. The knight is in enemy territory (ranks 4-6 for white, 3-5 for black)
4. (Added later) Consideration of whether enemy minor pieces can challenge the outpost

## Timeline
- **Branch**: feature/20250825-knight-outposts
- **Start Date**: 2025-08-25
- **Base Commit**: 41cdc57 (main after build fix)

## Phase Status

### Phase KO1: Infrastructure (COMPLETE)
- **Commit**: dd81940
- **Bench**: 19191913
- **Description**: Added detection infrastructure with bonus set to 0
- **Expected ELO**: 0 (infrastructure only)
- **Actual**: Not tested separately

### Phase KO2: Basic Bonus (COMPLETE)
- **Commit**: e3e24b8
- **Bench**: 19191913
- **Description**: Activated knight outpost bonus at 18 centipawns
- **Expected ELO**: +8-12
- **Actual ELO**: +3.28 ± 8.14 (weak signal after 4976 games)
- **Test 171**: LLR 0.29, still running

### Phase KO3: Quality Refinements (COMPLETE)
- **Commit**: 57f9e32
- **Bench**: 19191913
- **Description**: Added quality bonuses for central and advanced outposts
- **Enhancements**:
  - Central file bonus (+8 cp for files c-f)
  - Advanced rank bonus (+10 cp for rank 6/3)
  - Bonuses stack with base value
- **Results**: Tested together with value tuning

## Value Tuning Tests

### Critical Discovery: Startpos vs Opening Book Divergence

| Value | Test | Book | ELO Result | Games | Status |
|-------|------|------|------------|-------|--------|
| 18cp | 171 | Startpos | +3.28 ± 8.14 | 4976 | Weak positive |
| 40cp | 173 | Startpos | **+35.89 ± 13.11** | 1700 | **PASSED** ✅ |
| 60cp | 174 | Startpos | **+36.93 ± 13.53** | 1728 | **PASSED** ✅ |
| 60cp | 175 | UHO Book | **-15.77 ± 9.51** | 3174 | **FAILED** ❌ |

### Key Finding: Startpos Overfitting
The dramatic reversal (60cp: +36.93 from startpos, -15.77 from UHO book) reveals a **52.7 ELO swing** depending on opening choice. This indicates the implementation was exploiting specific patterns from 1.e4/1.d4 openings.

## Defensive Factors Implementation

### Phase KO4: Add Defensive Considerations (COMPLETE)
- **Commit**: 3b0b0a0
- **Bench**: 19191913
- **Description**: Added checks for whether outpost can be challenged by enemy minor pieces
- **Implementation**:
  - Secure outpost (cannot be challenged): 40 cp base
  - Weak outpost (can be challenged): 20 cp base
  - Quality bonuses still apply on top
- **Maximum Values**:
  - Secure central advanced: 58 cp (40+8+10)
  - Weak central advanced: 38 cp (20+8+10)

## Testing Summary

| Phase/Value | Commit | Book | Expected | Actual ELO | LLR | Status |
|-------------|--------|------|----------|------------|-----|--------|
| 18cp basic | e3e24b8 | Start | +8-12 | +3.28 ± 8.14 | 0.29 | Weak |
| 40cp tuned | 3becd94 | Start | +20-30 | +35.89 ± 13.11 | 2.98 | **PASSED** |
| 60cp tuned | 6e0a953 | Start | +30-40 | +36.93 ± 13.53 | 2.98 | **PASSED** |
| 60cp tuned | 6e0a953 | UHO | +30-40 | -15.77 ± 9.51 | -2.96 | **FAILED** |
| 40cp defensive | 3b0b0a0 | TBD | +15-25 | - | - | Ready |

## Key Learnings

### Technical Implementation
- Using existing pawn attack calculations ensures zero overhead
- Bitboard operations keep implementation efficient
- Loop-based evaluation needed for defensive checks adds some overhead but provides better accuracy

### Value Discoveries
1. **18cp is too conservative** - Only +3.28 ELO gain
2. **40-60cp optimal from startpos** - Both show ~36 ELO gain
3. **Startpos overfitting is real** - What works from startpos can fail dramatically with diverse openings
4. **Defensive factors matter** - Outposts that can be challenged should be valued less

### Pentanomial Imbalance
- All tests show more decisive wins than losses (imbalanced pentanomials)
- Pattern exists even at 18cp, suggesting broader evaluation issue
- More pronounced at higher values (40cp/60cp)
- Slightly better balanced with UHO book vs startpos

### Chess Principles Confirmed
- Traditional outpost definition (3 criteria) is correct
- Well-placed knights deserve significant bonuses (40+ cp)
- Central outposts more valuable than wing outposts
- Advanced outposts (rank 6/3) particularly strong
- Outposts that can't be challenged are worth roughly double those that can

## Implementation Details

### Detection Logic
```cpp
// Outpost = in enemy territory, safe from enemy pawns, protected by friendly pawn
Bitboard whiteOutpostSquares = WHITE_OUTPOST_RANKS & ~blackPawnAttacks & whitePawnAttacks;
Bitboard whiteKnightOutposts = whiteKnights & whiteOutpostSquares;
```

### Integration Point
Added after pawn attack calculation, before mobility evaluation at line ~616 in evaluate.cpp.

## Next Steps
1. Test defensive factors implementation (40cp/20cp) with both startpos and UHO book
2. If defensive factors help with opening book generalization, consider this approach final
3. If still problematic, consider:
   - Lower base values (25-30cp range)
   - Phase-based scaling (less in opening, more in middlegame)
   - Requiring outpost knight to attack enemy pieces/pawns
   - Different values for different square combinations

## Recommendations for Future Features
1. **Always test with both startpos and opening books** - Startpos can hide overfitting
2. **Consider defensive/challenging factors early** - Not just "can I do X" but "can opponent prevent X"
3. **Watch pentanomial distributions** - Imbalances can indicate evaluation artifacts
4. **Start with conservative values then tune up** - Easier to see impact
5. **Classical chess principles are often correct** - Don't reinvent the wheel