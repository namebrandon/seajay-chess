# Knight Outposts Feature Implementation Plan

## Feature Overview
A knight outpost is a square where a knight:
1. Cannot be attacked by enemy pawns
2. Is protected by a friendly pawn
3. Is in enemy territory or center (ranks 4-6 for white, ranks 3-5 for black)
4. Optionally: Cannot be easily exchanged by enemy knights/bishops

**Expected ELO Gain**: +10-15 ELO (based on other engines)

## Key Implementation Principles

### 1. REUSE EXISTING CALCULATIONS
- `whitePawnAttacks` and `blackPawnAttacks` are already computed for mobility
- Don't recalculate these - reuse them!
- This ensures ZERO additional overhead for pawn attack computation

### 2. BITBOARD BATCH OPERATIONS
- Process ALL knights at once, not individually
- Use bitwise AND operations to find outposts
- No loops over individual pieces

### 3. CONSERVATIVE VALUES
- Start with 15-20 cp for a basic outpost
- Add small bonuses for quality factors (5-10 cp)
- Remember: SeaJay needs smaller values than other engines

## Implementation Strategy

### Phase KO1: Basic Outpost Detection (Infrastructure)
**Goal**: Detect outpost squares efficiently
**Expected ELO**: 0 (infrastructure only)

```cpp
// Define outpost ranks for each side
constexpr Bitboard WHITE_OUTPOST_RANKS = RANK_4_BB | RANK_5_BB | RANK_6_BB;
constexpr Bitboard BLACK_OUTPOST_RANKS = RANK_3_BB | RANK_4_BB | RANK_5_BB;

// In evaluate.cpp, after pawn attacks are computed:
// Find potential outpost squares (safe from enemy pawns)
Bitboard whiteOutpostSquares = WHITE_OUTPOST_RANKS & ~blackPawnAttacks;
Bitboard blackOutpostSquares = BLACK_OUTPOST_RANKS & ~whitePawnAttacks;

// Find protected outpost squares
Bitboard whiteProtectedOutposts = whiteOutpostSquares & whitePawnAttacks;
Bitboard blackProtectedOutposts = blackOutpostSquares & blackPawnAttacks;

// Find actual knight outposts
Bitboard whiteKnightOutposts = whiteKnights & whiteProtectedOutposts;
Bitboard blackKnightOutposts = blackKnights & blackProtectedOutposts;

// Phase KO1: Return 0 for now
int outpostBonus = 0;
```

### Phase KO2: Basic Outpost Bonus
**Goal**: Apply simple bonus for outposts
**Expected ELO**: +8-12

```cpp
// Conservative starting values
constexpr int KNIGHT_OUTPOST_BONUS = 18;  // Start conservative

// Count outposts and apply bonus
int whiteOutposts = popCount(whiteKnightOutposts);
int blackOutposts = popCount(blackKnightOutposts);

int outpostValue = (whiteOutposts - blackOutposts) * KNIGHT_OUTPOST_BONUS;
Score outpostScore(outpostValue);
```

### Phase KO3: Outpost Quality Refinements (Optional)
**Goal**: Add bonuses for higher quality outposts
**Expected ELO**: +3-5 additional

Quality factors to consider:
1. **Central outposts** (files c-f) worth more than wing outposts
2. **Advanced outposts** (rank 6 for white) worth more
3. **Outposts that can't be challenged** by enemy minor pieces

```cpp
// Example refinement - central files bonus
constexpr Bitboard CENTRAL_FILES = FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB;

Bitboard whiteCentralOutposts = whiteKnightOutposts & CENTRAL_FILES;
Bitboard blackCentralOutposts = blackKnightOutposts & CENTRAL_FILES;

int centralBonus = (popCount(whiteCentralOutposts) - popCount(blackCentralOutposts)) * 8;
outpostValue += centralBonus;
```

## Critical Success Factors

### 1. Location in Code
Place the outpost evaluation **AFTER** pawn attack calculation but **BEFORE** the main piece loops:
```cpp
// Calculate pawn attacks for each side
Bitboard wp = whitePawns;
while (wp) {
    Square sq = popLsb(wp);
    whitePawnAttacks |= pawnAttacks(WHITE, sq);
}
// ... same for black ...

// *** ADD KNIGHT OUTPOST EVALUATION HERE ***
// This is the perfect spot - pawn attacks are ready, pieces haven't been consumed

// Then continue with mobility evaluation...
```

### 2. Testing Strategy
- **Phase KO1**: Verify detection works, bench should stay same with bonus=0
- **Phase KO2**: Test with conservative bonus (15-20 cp)
- **Phase KO3**: Only if KO2 shows positive results

### 3. Avoid These Pitfalls
- ❌ Don't loop through knights individually
- ❌ Don't recalculate pawn attacks
- ❌ Don't start with high values (>25 cp)
- ❌ Don't add complex logic in first phases

## Specific Implementation Details

### File Modifications
1. **src/evaluation/evaluate.cpp** - Main implementation
2. No new files needed - keep it simple!

### Exact Location (Line ~597)
```cpp
// After this existing code:
Bitboard bp = blackPawns;
while (bp) {
    Square sq = popLsb(bp);
    blackPawnAttacks |= pawnAttacks(BLACK, sq);
}

// *** INSERT KNIGHT OUTPOST EVALUATION HERE ***

// Before this existing code:
// Phase 2: Conservative mobility bonuses per move count
```

### Values to Try
```cpp
// Phase KO2 - Start conservative
KNIGHT_OUTPOST_BONUS = 18;  // Try 15, 18, 20

// Phase KO3 - Quality bonuses (if KO2 succeeds)
CENTRAL_OUTPOST_BONUS = 8;   // Additional for central files
ADVANCED_OUTPOST_BONUS = 10; // Additional for rank 6/3
```

## Example Complete Implementation (Phase KO2)

```cpp
// Knight outpost evaluation (Phase KO2)
// An outpost is a square where a knight:
// 1. Cannot be attacked by enemy pawns
// 2. Is protected by friendly pawns  
// 3. Is in enemy territory (ranks 4-6 for white)

constexpr Bitboard WHITE_OUTPOST_RANKS = RANK_4_BB | RANK_5_BB | RANK_6_BB;
constexpr Bitboard BLACK_OUTPOST_RANKS = RANK_3_BB | RANK_4_BB | RANK_5_BB;
constexpr int KNIGHT_OUTPOST_BONUS = 18;

// Find outpost squares (safe from enemy pawns and protected by friendly pawns)
Bitboard whiteOutpostSquares = WHITE_OUTPOST_RANKS & ~blackPawnAttacks & whitePawnAttacks;
Bitboard blackOutpostSquares = BLACK_OUTPOST_RANKS & ~whitePawnAttacks & blackPawnAttacks;

// Find knights on outpost squares
Bitboard whiteKnightOutposts = whiteKnights & whiteOutpostSquares;
Bitboard blackKnightOutposts = blackKnights & blackOutpostSquares;

// Apply bonus
int knightOutpostValue = (popCount(whiteKnightOutposts) - popCount(blackKnightOutposts)) * KNIGHT_OUTPOST_BONUS;
Score knightOutpostScore(knightOutpostValue);

// Add to total evaluation
totalWhite += knightOutpostScore;
```

## Why This Will Succeed

1. **Zero Additional Loops** - Uses existing pawn attack calculations
2. **Bitboard Efficiency** - All operations are bitwise AND/popcount
3. **Conservative Values** - Starting at 18 cp based on SeaJay's sensitivity
4. **Clear Phases** - Each phase is independently testable
5. **Proven Concept** - Knight outposts are well-understood and valuable

## Common Knight Outpost Squares

For reference, these are typical strong outpost squares:
- **White**: e5, d5, c5, f5 (central), b5, g5 (wing)
- **Black**: e4, d4, c4, f4 (central), b4, g4 (wing)

The best outposts are usually on e5/e4 and d5/d4 as they control critical central squares.

## Expected Timeline

1. **Phase KO1** (30 min): Infrastructure, no ELO change expected
2. **Phase KO2** (30 min): Basic bonus, test with OpenBench
3. **Phase KO3** (optional): Quality refinements if KO2 succeeds

Total implementation time: ~1 hour for core feature