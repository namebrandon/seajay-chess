# King Evasion Bug Fix Plan - OpenBench Phased Approach

## Overview
This document outlines the phased approach to fix the King evasion bug that causes illegal moves when the King attempts to escape check. The bug affects 100% of illegal moves identified in game analysis.

## Bug Summary
- **Issue:** King generates illegal moves when in check
- **Root Cause:** King blocks sliding piece ray attacks when checking escape squares
- **Location:** `/workspace/src/core/move_generation.cpp:1002` in `generateKingEvasions()`
- **Impact:** 8 confirmed illegal moves in tournament games

## Critical Requirements
- **MANDATORY:** OpenBench test after EVERY phase
- **MANDATORY:** Every commit must include "bench <node-count>"
- **MANDATORY:** Wait for human confirmation before proceeding to next phase
- **Current Branch:** bugfix/20250821-illegal-move-generation
- **Stockfish Reference:** Available at `/workspace/external/stockfish/stockfish` for validation

---

## PHASE 1: Test Infrastructure (No ELO change expected)

### Purpose
Establish comprehensive test suite without changing engine behavior

### Phase 1A: Create Test Infrastructure
- Create `/workspace/tests/king_evasion_tests.cpp`
- Add framework for comparing SeaJay vs Stockfish moves
- No engine changes - infrastructure only

### Phase 1B: Add Test Positions
- Add all 8 illegal move positions from games:
  1. After 14. Nxd8+ (f7g8 illegal)
  2. After 62. Nd6+ (b7b8 illegal)
  3. After 23. Nxg7+ (e6d7 illegal)
  4. After 48. e8=Q+ (d7c6 illegal)
  5. After 50. Ne5+ (f7g7 illegal)
  6. After 36. Kd2 c1=Q+ (d2e3 illegal)
  7. After 31. Rb7+ (b6a5 illegal)
  8. After 28. Rxe5+ (e4e3 illegal)
- Add expert-recommended positions:
  - Rook check on file: "4k3/8/8/8/8/8/8/4R3 b - - 0 1"
  - Bishop check on diagonal: "8/8/8/8/8/2k5/8/B7 w - - 0 1"
  - Queen check: "3q4/8/8/8/8/8/8/4K3 w - - 0 1"
- Create validation script showing current failures
- Use Stockfish at `/workspace/external/stockfish/stockfish` for reference moves

### Phase 1C: Commit and OpenBench Test
```bash
# Get bench count
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'

# Commit with bench
git add -A
git commit -m "test: Add King evasion test infrastructure (Phase 1) - bench [NODE_COUNT]"
git push

# 🛑 STOP - AWAIT OPENBENCH TEST
# Expected: 0 ELO change (tests only, no functional changes)
# OpenBench: bugfix/20250821-illegal-move-generation vs main
```

**Human Action Required:** Run OpenBench test and confirm no regression before proceeding to Phase 2

---

## PHASE 2: Ray Attack Infrastructure (0 ELO expected)

### Purpose
Build helper functions without integrating them

### Phase 2A: Implement Ray Helpers
Add to `/workspace/src/core/bitboard.cpp`:
```cpp
// Get bishop attacks with custom occupancy (for King safety checks)
Bitboard getBishopAttacksNoKing(Square sq, Bitboard occupancyNoKing) {
    // Use existing magic bitboard infrastructure with modified occupancy
    return getBishopAttacks(sq, occupancyNoKing);
}

// Get rook attacks with custom occupancy (for King safety checks)
Bitboard getRookAttacksNoKing(Square sq, Bitboard occupancyNoKing) {
    // Use existing magic bitboard infrastructure with modified occupancy
    return getRookAttacks(sq, occupancyNoKing);
}
```

### Phase 2B: Unit Test Ray Functions
- Test ray detection with modified occupancy
- Verify correctness in isolation
- Still no integration with move generation

### Phase 2C: Commit and OpenBench Test
```bash
# Get bench count
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'

# Commit with bench
git add -A
git commit -m "feat: Add ray attack helpers for King safety (Phase 2) - bench [NODE_COUNT]"
git push

# 🛑 STOP - AWAIT OPENBENCH TEST
# Expected: 0 ELO change (helpers not integrated)
# OpenBench: bugfix/20250821-illegal-move-generation vs main
```

**Human Action Required:** Run OpenBench test and confirm no regression before proceeding to Phase 3

---

## PHASE 3: King Safety Function (0-5 ELO expected)

### Purpose
Implement AND integrate the actual fix

### Phase 3A: Add isKingMoveSafe()
Add to `/workspace/src/core/move_generation.cpp`:
```cpp
bool MoveGenerator::isKingMoveSafe(const Board& board, Square from, Square to, Color enemyColor) {
    // Remove king from occupancy for slider attack detection
    Bitboard occupancyNoKing = board.occupied() ^ squareBB(from);
    
    // Check non-slider attacks normally (King still blocks these)
    if (board.pieces(enemyColor, PAWN) & getPawnAttacks(to, ~enemyColor)) return false;
    if (board.pieces(enemyColor, KNIGHT) & getKnightAttacks(to)) return false;
    if (board.pieces(enemyColor, KING) & getKingAttacks(to)) return false;
    
    // Check slider attacks with modified occupancy (King removed)
    Bitboard bishopsQueens = board.pieces(enemyColor, BISHOP) | board.pieces(enemyColor, QUEEN);
    if (bishopsQueens & getBishopAttacks(to, occupancyNoKing)) return false;
    
    Bitboard rooksQueens = board.pieces(enemyColor, ROOK) | board.pieces(enemyColor, QUEEN);
    if (rooksQueens & getRookAttacks(to, occupancyNoKing)) return false;
    
    return true;
}
```

### Phase 3B: Integrate into Engine
- Replace `isSquareAttacked()` in `generateKingEvasions()` at line 1002
- Change from:
  ```cpp
  if (!isSquareAttacked(board, to, them)) {
  ```
- To:
  ```cpp
  if (isKingMoveSafe(board, kingSquare, to, them)) {
  ```
- Fix is now active in the engine
- Should eliminate all illegal moves

### Phase 3C: Commit and OpenBench Test
```bash
# Get bench count  
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'

# Commit with bench
git add -A
git commit -m "fix: Integrate King safety check in evasion generation (Phase 3) - bench [NODE_COUNT]"
git push

# 🛑 STOP - AWAIT OPENBENCH TEST
# Expected: 0-5 ELO gain (avoiding illegal moves)
# OpenBench: bugfix/20250821-illegal-move-generation vs main
```

**Human Action Required:** Run OpenBench test and verify fix doesn't lose ELO before proceeding to Phase 4

---

## PHASE 4: Validation Suite (0 ELO expected)

### Purpose
Comprehensive testing to ensure fix is complete

### Phase 4A: Perft Validation
- Run full perft suite to depth 6
- Compare all results with Stockfish at `/workspace/external/stockfish/stockfish`
- Validate using: `echo -e "position fen [FEN]\ngo perft [depth]\nquit" | /workspace/external/stockfish/stockfish`
- Document any discrepancies
- Focus on positions with checks

### Phase 4B: Test Illegal Positions
- Verify all 8 game positions now generate only legal moves
- Run 100 fast games (1+0.01) checking for illegal moves
- Update test documentation with results
- Create permanent regression test

### Phase 4C: Commit and OpenBench Test
```bash
# Get bench count
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'

# Commit with bench
git add -A
git commit -m "test: Complete validation of King evasion fix (Phase 4) - bench [NODE_COUNT]"
git push

# 🛑 STOP - AWAIT OPENBENCH TEST
# Expected: 0 ELO change (validation only)
# OpenBench: bugfix/20250821-illegal-move-generation vs main
```

**Human Action Required:** Run final OpenBench test before merging to main

---

## Critical Reminders

### Before EVERY Commit
```bash
# ALWAYS get fresh bench count
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'
```

### Stockfish Validation Commands
```bash
# Test a specific position with Stockfish
echo -e "position fen [FEN]\ngo perft [depth]\nquit" | /workspace/external/stockfish/stockfish

# Get legal moves from a position
echo -e "position fen [FEN]\nd\nquit" | /workspace/external/stockfish/stockfish

# Compare perft results
echo -e "position fen [FEN]\ngo perft [depth]\nquit" | /workspace/external/stockfish/stockfish 2>/dev/null | grep "Nodes searched"
```

### Expected OpenBench Results
- **Phase 1:** 0 ELO (test infrastructure only)
- **Phase 2:** 0 ELO (unused helpers)
- **Phase 3:** 0-5 ELO gain (actual fix)
- **Phase 4:** 0 ELO (validation only)

### If Any Phase Fails
1. Stop immediately
2. Debug the specific phase
3. Do NOT proceed to next phase
4. May need to create sub-phases (3A.1, 3A.2, etc.)
5. Document failure in this plan

### Merge Strategy
Only after ALL 4 phases pass OpenBench:
```bash
git checkout main
git merge bugfix/20250821-illegal-move-generation
git push origin main
git push origin --delete bugfix/20250821-illegal-move-generation
```

---

## Risk Mitigation

- **Backup current binary** before changes
- **Test incrementally** - Don't batch changes
- **Profile performance** after Phase 3 to ensure no regression
- **Keep detailed notes** of any unexpected behavior

## Performance Considerations

### Expected Impact
- Phase 3: One XOR operation per King move in check
- Overall: <1% performance impact expected
- Memory: No additional memory requirements

### Alternative Approaches (if needed)
If performance regression >1%:
- Consider caching King safety calculations
- Pre-compute escape squares when entering check
- Use make/unmake only for edge cases

## Test Position Reference

### Positions from Illegal Games
```
// Game 1: After 14. Nxd8+
"position after Nxd8+ with Kf7"

// Game 2: After 62. Nd6+
"position after Nd6+ with Kb7"

// Game 3: After 23. Nxg7+
"position after Nxg7+ with Ke6"

// Game 4: After 48. e8=Q+
"position after e8=Q+ with Kd7"

// Game 5: After 50. Ne5+
"position after Ne5+ with Kf7"

// Game 6: After 36. Kd2 c1=Q+
"position after c1=Q+ with Kd2"

// Game 7: After 31. Rb7+
"position after Rb7+ with Kb6"

// Game 8: After 28. Rxe5+
"position after Rxe5+ with Ke4"
```

### Expert Test Positions
```
// Rook check on file (black to move)
"4k3/8/8/8/8/8/8/4R3 b - - 0 1"

// Bishop check on diagonal (white to move)
"8/8/8/8/8/2k5/8/B7 w - - 0 1"

// Queen check (white to move)
"3q4/8/8/8/8/8/8/4K3 w - - 0 1"
```

## Success Criteria

### Bug is Fixed When:
1. All 8 illegal game positions generate only legal moves
2. Perft matches Stockfish for check evasion positions
3. No strength regression in OpenBench (may show small gain)
4. Performance impact <1%
5. No new bugs introduced in pin detection or castling

## Historical Context
This bug pattern has appeared in many engines:
- Crafty (fixed in v12.x)
- Fruit 2.0 (fixed in 2.1)
- Multiple TCEC tournament entries

The fix approach (removing King from occupancy for slider attacks) is the industry-standard solution used by Stockfish, Ethereal, and other top engines.