# Known Bugs - SeaJay Chess Engine

This document tracks identified but unresolved bugs in the SeaJay chess engine, providing detailed analysis and debugging information for future resolution.

## Bug #008: Stage 13 Time Management Critical Failure

**Status:** RESOLVED  
**Priority:** Critical (resolved)  
**Discovery Date:** 2025-08-14  
**Resolution Date:** 2025-08-14  
**Impact:** Caused Stage 13 to only search depth 1, 0% win rate in SPRT tests

### Summary

Stage 13 iterative deepening implementation had critical time management bugs that caused the engine to only search to depth 1 in all positions, resulting in catastrophic performance (0% win rate against Stage 12).

### Root Cause Analysis

Two critical bugs were identified:
1. **Cached Time Bug:** The `shouldStopSearching()` function was using cached `timeRemaining` instead of actual elapsed time
2. **Fallback EBF Bug:** The fallback effective branching factor was set to 30, causing immediate time exhaustion

### Resolution Details

**Fixes Applied:**
1. Changed `shouldStopSearching()` to use actual elapsed time: `elapsedTime = Time::now() - m_searchStartTime`
2. Changed fallback EBF from 30 to reasonable values (2.0 for early depths, 5.0 for later)
3. Added minimum depth 4 before allowing time-based stopping

**File Modified:** `/workspace/src/search/time_management.cpp`

### Test Results After Fix

**SPRT Testing Results:**
- Stage 13 vs Stage 11: **+372 Elo** (H1 accepted)
- Stage 13 vs Stage 12: **+143 Elo** (H1 accepted)
- Win rates improved from 0% to expected ranges

### Verification

All SPRT tests now pass with expected Elo gains. The bug was discovered through SPRT testing which revealed the catastrophic failure immediately.

## Bug #001: Position 3 Systematic Perft Deficit at Depth 6

**Status:** RESOLVED - En Passant Check Evasion Fixed  
**Priority:** Low (resolved)  
**Discovery Date:** 2025-08-09  
**Resolution Date:** 2025-08-09  
**Impact:** Was 0.026% accuracy deficit (2,871 missing nodes out of 11,030,083)

### Summary

Position 3 showed a systematic perft deficit at depth 6. The engine was generating 11,027,212 nodes instead of the expected 11,030,083 nodes (verified with Stockfish 17.1).

**ROOT CAUSE:** SeaJay failed to generate en passant captures when the king was in check. The bug occurred because the `generateCheckEvasions` function didn't consider en passant captures as valid check evasion moves.

**RESOLUTION:** Fixed by adding en passant handling to the `generateCheckEvasions` function in `/workspace/src/core/move_generation.cpp` (lines 908-945). The fix properly handles:
1. En passant captures that block sliding piece checks
2. En passant captures of the checking piece (if it's the pawn that just moved)
3. Validation that the en passant move doesn't leave the king in check

### Position Details

**FEN:** `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1`

**Expected vs Actual Results:**
- Depth 1: 14 moves (✅ matches)
- Depth 2: 191 nodes (✅ matches) 
- Depth 3: 2,812 nodes (✅ matches)
- Depth 4: 43,238 nodes (✅ matches)
- Depth 5: 674,624 expected vs 674,543 actual (-81 nodes)
- Depth 6: 11,030,083 expected vs 11,027,212 actual (-2,871 nodes)

### Systematic Analysis

#### Root Move Breakdown (Depth 5)

**Perfect Moves (4/14):**
- `a5a4`: 52,943 nodes ✅
- `a5a6`: 59,028 nodes ✅  
- `e2e3`: 45,326 nodes ✅
- `e2e4`: 36,889 nodes ✅
- `g2g3`: 14,747 nodes ✅
- `g2g4`: 53,895 nodes ✅

**Deficit Moves (8/14) - ALL ROOK MOVES:**
- `b4a4`: 45,580 vs 45,591 (-11 nodes)
- `b4b1`: 69,653 vs 69,665 (-12 nodes)  
- `b4b2`: 48,486 vs 48,498 (-12 nodes)
- `b4b3`: 59,708 vs 59,719 (-11 nodes)
- `b4c4`: 63,770 vs 63,781 (-11 nodes)
- `b4d4`: 59,563 vs 59,574 (-11 nodes)
- `b4e4`: 54,181 vs 54,192 (-11 nodes)
- `b4f4`: 10,774 vs 10,776 (-2 nodes)

#### Key Patterns Identified

1. **Move Type Selectivity:** Only rook moves show deficits; king and pawn moves are perfect
2. **Consistent Deficit:** Most rook moves lose exactly 11-12 nodes (except f4 which gives check)
3. **Depth Dependency:** Issue only manifests at depth 5+; depth 4 is perfect
4. **Systematic Distribution:** ALL rook moves affected, not random subset
5. **Destination Correctness:** Positions after problematic moves generate correct counts

### Debugging Infrastructure Created

**Tool:** `/workspace/tools/perft_debug.cpp`

**Capabilities:**
- Comparative analysis with Stockfish
- Move-by-move breakdown and comparison
- Automated divergence detection
- Drill-down analysis into specific move sequences

**Usage Examples:**
```bash
# Compare with Stockfish at specific depth
./tools/perft_debug compare "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" 5

# Drill down into specific move
./tools/perft_debug drill "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" "b4b1" 4

# Automated divergence analysis
./tools/perft_debug find "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" 3
```

### Technical Analysis

#### Excluded Causes

**✅ En Passant Issues:** Ruled out - no en passant square in position  
**✅ Castling Issues:** Ruled out - no castling rights in position  
**✅ Major Algorithmic Errors:** Ruled out - 99.974% accuracy demonstrates sound implementation  
**✅ Move Generation Completeness:** Ruled out - destination positions generate correct counts  
**✅ Promotion Handling:** Ruled out - pawns can't promote immediately from this position

#### Potential Causes

**🔍 Legal Move Filtering:** `leavesKingInCheck()` function may be over-filtering in complex tactical positions  
**🔍 Pin Detection:** Subtle bugs in pin calculation affecting rook moves in deeper positions  
**🔍 Position State Corruption:** Make/unmake might cause subtle state issues affecting deeper searches  
**🔍 Discovered Check Handling:** Missing moves that create or prevent discovered checks  
**🔍 Complex Tactical Positions:** Bug manifests only in specific tactical scenarios at depth 5+

#### Debugging Strategy

**Phase 1:** Use comparative divide to isolate positions with discrepancies  
**Phase 2:** Drill down recursively until exact divergent move found  
**Phase 3:** Analyze specific move generation logic for that scenario  
**Phase 4:** Fix underlying bug in move generator or legal move filter

### Code Areas of Interest

**Primary Suspects:**
- `/workspace/src/core/move_generation.cpp` - Lines 664-690 (`leavesKingInCheck`)
- `/workspace/src/core/move_generation.cpp` - Lines 693-750 (`getPinnedPieces`) 
- `/workspace/src/core/move_generation.cpp` - Lines 859-918 (`generateCheckEvasions`)
- `/workspace/src/core/board.cpp` - Make/unmake implementation for position state

**Secondary Areas:**
- Attack generation for sliding pieces (rook-specific issue)
- Complex pin/discovery interaction logic
- Check detection in tactical positions

### Impact Assessment

**Functional Impact:** None - engine plays correctly  
**Validation Impact:** Minor - affects only advanced perft validation  
**Performance Impact:** None detected  
**Accuracy Impact:** 0.026% deviation from perfect move generation

### Resolution Timeline

**Immediate:** Documented and deferred (Phase 1 completion prioritized)  
**Future Phase:** Detailed debugging using systematic comparative analysis  
**Estimated Effort:** 2-4 hours of focused debugging with comparative tools

### Verification Strategy

Once fixed, validate with:
1. **Position 3 depth 6:** Must achieve exactly 11,030,083 nodes
2. **Regression testing:** Ensure all other 24 perft tests still pass
3. **Tactical test positions:** Verify fix doesn't break other complex positions
4. **Performance validation:** Ensure fix doesn't impact search speed

### Specific Problem Positions Identified

Through systematic debugging with the perft_debug tool, the following positions exhibit the en passant bug:

#### Position 1: Missing b5xc6
**FEN:** `8/8/8/1Ppp3r/1K3p1k/8/4P1P1/1R6 w - c6 0 1`
- **Expected moves:** 7 (Stockfish)
- **SeaJay generates:** 6 moves
- **Missing move:** b5xc6 (en passant capture)

#### Position 2: Missing b5xc6 and d5xc6
**FEN:** `8/8/8/1PpP3r/1K3p1k/8/6P1/1R6 w - c6 0 1`
- **Expected moves:** 9 (Stockfish)
- **SeaJay generates:** 7 moves  
- **Missing moves:** b5xc6, d5xc6 (both en passant captures)

#### Position 3: Black en passant working correctly
**FEN:** `8/8/3k4/8/1pPp4/8/1K6/8 b - c3 0 1`
- **Expected moves:** 11 (Stockfish)
- **SeaJay generates:** 11 moves ✓
- **Note:** Black's en passant capture b4xc3 is correctly generated

### Root Cause Analysis

The bug is specifically in WHITE pawn en passant capture generation. Black's en passant captures work correctly. The issue manifests when:
1. Black makes a double pawn push (e.g., c7-c5)
2. This creates an en passant square (e.g., c6)
3. White has a pawn on the 5th rank that could capture
4. SeaJay fails to generate the white pawn's en passant capture

### Resolution Details

**Fix Applied:** The bug was fixed by the cpp-pro agent after analysis by the chess-engine-expert. The solution added en passant move generation to the check evasion logic.

**Test Results After Fix:**
- Position 1: Now generates 7 moves correctly ✓
- Position 2: Now generates 9 moves correctly ✓  
- Position 3: Continues to work correctly (11 moves) ✓
- Position 3 depth 6: Now generates exactly 11,030,083 nodes ✓

**Test Coverage Created:**
- `/workspace/tests/test_en_passant_check_evasion.cpp` - Comprehensive test suite
- Regression tests added to prevent reintroduction
- Performance benchmarks established

**Verification:** All perft tests now pass with 100% accuracy. The systematic debugging approach using the `perft_debug` tool was instrumental in identifying this subtle bug.

---

## Bug #002: Zobrist Key Initialization Inconsistency

**Status:** RESOLVED  
**Priority:** Low (cosmetic issue during initialization)  
**Discovery Date:** 2025-08-09  
**Resolution Date:** 2025-08-10  
**Impact:** Zobrist key was showing as 0x0 instead of computed value after clear()

### Summary

The Zobrist key was not properly initialized when calling `clear()`, showing 0x0 instead of the correct computed hash. The validation system detected a mismatch between the actual key (0x0) and the reconstructed key (e.g., 0x341 for an empty board with WHITE to move). This didn't affect gameplay but triggered validation warnings.

### Reproduction Steps

```cpp
Board board;
board.setStartingPosition();
// At this point, board.zobristKey() returns 0x0
// But board.validateZobrist() fails with:
//   Actual: 0x0
//   Reconstructed: 0x341 (or similar)
```

### Example Output

```
Zobrist key mismatch!
  Actual:        0x0
  Reconstructed: 0x341
Invalid board state before operation: makeMove
```

### Root Cause Analysis

The `clear()` method was setting `m_zobristKey = 0` directly instead of computing the proper zobrist value for an empty board. Since the zobrist hash includes components for side-to-move and castling rights (even when 0), an empty board with WHITE to move should have a non-zero zobrist key.

### Code Location

**File:** `/workspace/src/core/board.cpp`  
**Method:** `Board::clear()`  
**Issue:** Was setting `m_zobristKey = 0` instead of calling `rebuildZobristKey()`

### Impact Assessment

**Functional Impact:** None - zobrist updates work correctly after first move  
**Validation Impact:** Triggers false positive warnings in debug mode  
**Game Play Impact:** None - doesn't affect move generation or game play  
**Search Impact:** Would only matter if using zobrist for hash tables before first move

### Workaround

Call `board.rebuildZobristKey()` explicitly after `setStartingPosition()`:

```cpp
Board board;
board.setStartingPosition();
board.rebuildZobristKey();  // Workaround
```

### Fix Applied

Modified `clear()` to call `rebuildZobristKey()` at the end:

```cpp
void Board::clear() {
    m_mailbox.fill(NO_PIECE);
    m_pieceBB.fill(0);
    m_pieceTypeBB.fill(0);
    m_colorBB.fill(0);
    m_occupied = 0;
    
    m_sideToMove = WHITE;
    m_castlingRights = NO_CASTLING;
    m_enPassantSquare = NO_SQUARE;
    m_halfmoveClock = 0;
    m_fullmoveNumber = 1;
    
    // Clear material tracking
    m_material.clear();
    m_evalCacheValid = false;
    
    // Bug #002 fix: Properly initialize zobrist key even for empty board
    // Must be done after setting all state variables
    rebuildZobristKey();  // FIXED: Now properly computes zobrist
}
```

### Verification Results

After the fix, all test cases pass:
```cpp
// Empty board now has correct zobrist
Board board;
assert(board.zobristKey() == 0x341);  // Correct value for empty board, WHITE to move
assert(board.validateZobrist());       // ✓ PASS

// Starting position works correctly
board.setStartingPosition();
assert(board.zobristKey() == 0x27c);  // Correct value for starting position
assert(board.validateZobrist());       // ✓ PASS

// Incremental updates work correctly
Move move = makeMove(E2, E4);
UndoInfo undo;
board.makeMove(move, undo);
assert(board.validateZobrist());       // ✓ PASS
```

### Resolution Details

**Fix Applied:** The `clear()` method now properly calls `rebuildZobristKey()` after initializing board state, ensuring the zobrist key is always consistent with the board position.

**Test Coverage:** Comprehensive tests verified:
- Empty board zobrist initialization
- Starting position zobrist initialization  
- Manual piece placement with incremental zobrist updates
- Make/unmake move zobrist consistency
- All validation checks pass

**Impact:** This fix ensures zobrist keys are always valid, eliminating false validation warnings and ensuring hash table lookups work correctly from the first position.

---

## Bug #003: Promotion Move Handling Edge Cases

**Status:** RESOLVED - Test Issue, Not Engine Bug  
**Priority:** Low (resolved)  
**Discovery Date:** 2025-08-09  
**Resolution Date:** 2025-08-10  
**Impact:** None - engine works correctly

### Summary

**RESOLUTION (2025-08-10):** After comprehensive investigation by chess-engine-expert, debugger, and cpp-pro agents, determined that SeaJay's promotion handling is 100% CORRECT. The suspected "bug" was actually incorrect test expectations that violated chess rules.

**Key Finding:** Test positions incorrectly assumed pawns could capture pieces directly in front of them. Chess rules state pawns move forward but capture diagonally only.

**Test Position Analysis:**
- FEN: `r3k3/P7/8/8/8/8/8/4K3 w - - 0 1`
- White pawn on a7 blocked by black rook on a8
- SeaJay correctly generates 5 moves (king moves only) ✅
- No illegal promotion moves generated ✅

### Investigation Results

#### All Issues RESOLVED - No Engine Bugs Found

**Issue 1: Promotion Check Detection** ✅ WORKING CORRECTLY
- Tests confirmed promotion moves properly detect check
- Attack maps update immediately after promotion
- No lag in check detection observed

**Issue 2: Under-promotion Edge Cases** ✅ WORKING CORRECTLY  
- All promotion types (Q, R, B, N) generate correctly
- Under-promotions work in all tactical positions
- Move generation includes all legal promotion options

**Issue 3: Promotion State Validation** ✅ WORKING CORRECTLY
- Zobrist keys remain consistent through 100+ make/unmake cycles
- No state corruption detected
- All validation tests pass

### Test Results Summary

**Comprehensive Testing Performed:**
- Created 10+ test positions covering all promotion scenarios
- Validated against Stockfish 17.1
- All tests pass with corrected expectations

**Example Verified Position:**
```
FEN: r3k3/P7/8/8/8/8/8/4K3 w - - 0 1
```
- Pawn on a7 blocked by rook on a8
- SeaJay correctly generates 5 king moves only
- No illegal promotion moves generated

### Code Analysis Results

**Code Verified Correct:**
- `/workspace/src/core/move_generation.cpp` - Lines 231-237 (pawn promotion generation) ✅
- `/workspace/src/core/board.cpp` - Lines 1167-1197 (promotion handling in makeMove) ✅
- `/workspace/src/core/board.cpp` - Lines 1306-1320 (promotion handling in unmakeMove) ✅

### Technical Verification

The promotion handling correctly:
- ✅ Blocks pawn forward moves when square is occupied
- ✅ Only allows diagonal captures for pawns
- ✅ Removes the pawn from origin
- ✅ Places promoted piece at destination
- ✅ Handles captures during promotion
- ✅ Updates all bitboards correctly
- ✅ Maintains zobrist key consistency
- ✅ Updates attack maps immediately
- ✅ Preserves state through make/unmake cycles
- ✅ Generates all promotion types (Q, R, B, N)

### Resolution Details

**Root Cause:** Test expectations incorrectly assumed pawns could capture pieces directly in front of them, violating fundamental chess rules.

**Investigation Process:**
1. Chess-engine-expert created comprehensive test plan
2. Debugger agent traced move generation logic
3. Cpp-pro agent ran tests and identified test expectation errors
4. Corrected test expectations to follow chess rules
5. All tests now pass with proper expectations

**Test Artifacts Created:**
- `/workspace/tests/test_promotion_bug.cpp` - Corrected test suite
- `/workspace/tests/debug_promotion.cpp` - Debug tool
- `/workspace/tests/test_promotion_final.cpp` - Final validation
- `/workspace/tests/BUG_003_RESOLUTION.md` - Complete documentation

### Lessons Learned

Always validate test expectations against chess rules and reference engines (Stockfish) before assuming engine bugs. In this case, the engine was working perfectly - the test expectations were wrong.

---

## Bug #004: UCI Protocol - Checkmate Position Move Generation

**Status:** RESOLVED - Fixed test cases  
**Priority:** Low (UCI compliance issue)  
**Discovery Date:** 2025-08-09  
**Resolution Date:** 2025-08-09  
**Impact:** Engine generates moves in checkmate positions (2 UCI tests failing)

### Summary

The UCI protocol tests were failing due to incorrect test positions, not an engine bug. The engine correctly:
1. Detects checkmate positions and returns 0 legal moves
2. Returns "bestmove 0000" when there are no legal moves
3. Properly differentiates between checkmate and stalemate

The issue was that the test suite had invalid FEN positions for the checkmate and stalemate tests.

### Resolution Details

**Test 26:** The FEN string in the test was malformed (missing move counters) and had wrong position:
```bash
# Original (wrong): "rnb1kbnr/pppp1ppp/4p3/8/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq -"
# Fixed: "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 0 1"
```

**Test 27:** The stalemate position was not actually stalemate:
```bash
# Original (wrong): "k7/8/1K6/8/8/8/8/1Q6 b - -"  # Black can move Ka8-b8
# Fixed: "7k/5Q2/5K2/8/8/8/8/8 b - - 0 1"  # Proper stalemate
```

### Root Cause (INCORRECT - Bug did not exist)

The engine was working correctly all along. The test positions were invalid.

### Verification

After fixing the test positions, all UCI protocol tests now pass:
```bash
Tests Passed: 27
Tests Failed: 0
Total Tests: 27
🎉 ALL UCI PROTOCOL TESTS PASSED!
```

The engine correctly:
- Returns "bestmove 0000" for checkmate positions
- Returns "bestmove 0000" for stalemate positions  
- Detects when there are no legal moves
- Differentiates between checkmate (king in check, no moves) and stalemate (king not in check, no moves)


---

## Bug #005: Edwards Position - Missing Legal Moves

**Status:** RESOLVED - Test expectation was incorrect  
**Priority:** Medium (incorrect move generation in complex positions)  
**Discovery Date:** 2025-08-09  
**Resolution Date:** 2025-08-09  
**Impact:** Generates 35 moves instead of 43 at depth 1

### Summary

The test expectation was incorrect. Both SeaJay and Stockfish 17.1 generate exactly **35 legal moves** for the Edwards position, not 43. Our engine is 100% correct for this position.

### Position Details

**FEN:** `r4rk1/2p2ppp/p7/q2Pp3/1n2P1n1/4QP2/PPP3PP/R1B1K2R w KQ - 0 1`

**Position Characteristics:**
- White king on e1 with castling rights KQ
- White rook on h1 (kingside castling available)
- Enemy queen on a5 creating threats
- Enemy knights on b4 and g4 creating pressure

### Verification Results

```
Expected moves (from test): 43
Stockfish 17.1:             35  ✓
SeaJay:                     35  ✓
```

### Resolution

Validated with Stockfish 17.1 which confirms exactly 35 legal moves. The test expectation of 43 moves was incorrect. Our engine correctly:
- Generates all legal pawn, piece, and king moves
- Properly evaluates castling is illegal (king would pass through check from queen on a5)
- Correctly filters moves based on pins and checks

The identical move lists between SeaJay and Stockfish confirm our move generation is accurate.

---

## Bug #006: Empty Board FEN Parsing Rejection

**Status:** RESOLVED - Correct behavior, not a bug  
**Priority:** Low (edge case handling)  
**Discovery Date:** 2025-08-09  
**Resolution Date:** 2025-08-09  
**Impact:** Cannot parse empty board FEN

### Summary

This is **correct behavior**, not a bug. Both SeaJay and Stockfish reject empty board positions as invalid.

### Position Details

**FEN:** `8/8/8/8/8/8/8/8 w - - 0 1`

### Verification Results

```
Stockfish 17.1: REJECTS (exits immediately)
SeaJay:         REJECTS (validation fails)
Chess GUIs:     REJECT as invalid
```

### Technical Details

SeaJay's `validateKings()` function correctly requires exactly one king per side:
```cpp
if (whiteKings != 1 || blackKings != 1) {
    return false;
}
```

This is correct per chess rules - a legal position must have both kings.

### Resolution

The behavior is correct and matches other chess engines. An empty board is not a legal chess position and should be rejected. No changes needed.

---

## Bug #007: King-Only Endgame Move Generation

**Status:** RESOLVED - Incorrect test expectations  
**Priority:** Low (no actual bug)  
**Discovery Date:** 2025-08-09  
**Resolution Date:** 2025-08-09  
**Impact:** None - engine is working correctly

### Summary

The test expectations were incorrect. Both SeaJay and Stockfish 17.1 generate the exact same number of legal moves for these positions. Our engine is 100% correct.

### Test Case 1: Two Kings

**FEN:** `8/8/8/4k3/8/8/8/4K3 w - - 0 1`

**Verification Results:**
```
Test expectation: 8 moves (INCORRECT)
Stockfish 17.1:   5 moves ✓
SeaJay:           5 moves ✓
```

**Correct moves generated:**
- e1d1, e1f1, e1d2, e1e2, e1f2

The white king correctly cannot move to squares adjacent to the black king (opposition rule).

### Test Case 2: Kings with Pawns

**FEN:** `8/2p5/8/KP6/8/8/8/k7 w - - 0 1`

**Verification Results:**
```
Test expectation: 5 moves (INCORRECT)
Stockfish 17.1:   4 moves ✓
SeaJay:           4 moves ✓
```

**Correct moves generated:**
- b5b6 (pawn advance), a5a4, a5b4, a5a6

### Resolution

Validated with Stockfish 17.1 which confirms our move generation is correct. The test expectations were wrong. Our engine correctly:
- Generates all legal king moves
- Properly enforces the opposition rule (kings cannot be adjacent)
- Handles pawn moves correctly

This is another case (like bugs #004, #005, #006) where the test data was incorrect, not the engine.

### Lesson Learned

Always validate test expectations with Stockfish before debugging. This prevents wasting time "fixing" correct behavior.

---

## Bug #008: Position 5 Perft Discrepancy at Depth 5

**Status:** Identified, very minor deviation  
**Priority:** Low (0.00001% error)  
**Discovery Date:** 2025-08-09  
**Impact:** 12 extra nodes at depth 5 (89,941,206 vs 89,941,194)

### Summary

Position 5 (middle game position) shows a tiny discrepancy of +12 nodes at depth 5, representing essentially perfect accuracy (100.000% when rounded to 3 decimal places).

### Position Details

**FEN:** `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8`

**Results:**
```
Depth 1-4: ✅ Perfect match
Depth 5: 89,941,206 (expected: 89,941,194)
Difference: +12 nodes (0.00001% error)
```

### Debugging Strategy

**CRITICAL:** Validate expected value with Stockfish:
```bash
echo "position fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" | \
./external/engines/stockfish/stockfish | grep "perft 5"
```

This discrepancy is so small it could be:
1. A test data error (wrong expected value)
2. An extremely subtle move generation difference
3. Different interpretation of a complex rule

### Priority Assessment

With 99.99999% accuracy, this is not a practical issue. It should be investigated only after all other bugs are resolved.

---

## Bug #009: Position 6 Test Data Inconsistency

**Status:** Identified - Test data error, not engine bug  
**Priority:** Low (test data issue)  
**Discovery Date:** 2025-08-09  
**Impact:** False positive test failures

### Summary

The perft test suite contains incorrect expected values for a test labeled "Position 6". Investigation reveals that the correct Position 6 from the Master Project Plan works perfectly with SeaJay.

### Investigation Results

**Correct Position 6 (from Master Project Plan):**
- **FEN:** `r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10`
- **Depth 1:** 46 moves ✅ (SeaJay correct)
- **Depth 2:** 2,079 nodes ✅ (SeaJay correct) 
- **Depth 3:** 89,890 nodes ✅ (SeaJay correct)
- **Depth 4:** 3,894,594 nodes ✅ (SeaJay correct)

**Test Output Shows Different Position:**
The test output shows a position with:
- 31 moves at depth 1 (instead of 46)
- 824 nodes at depth 2 (instead of 2,079)
- Move list starting with "d4e5" (not present in actual Position 6)

This indicates either:
1. The test file has been corrupted or modified
2. There's a duplicate/incorrect test overriding Position 6
3. The test is using a different FEN than specified

### Verification with perft_debug Tool

```bash
./tools/debugging/perft_debug compare "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10" 2
# Result: ✅ All moves match perfectly with Stockfish
```

### Resolution

1. The engine is **working correctly** for Position 6
2. The test file needs to be corrected or rebuilt
3. No engine bug exists for this position

---

## Bug #010: Stage 9b - setStartingPosition() Hang Issue

**Status:** RESOLVED - Fixed with bounds checking and assert protection  
**Priority:** CRITICAL (was blocking all testing)  
**Discovery Date:** 2025-08-10  
**Resolution Date:** 2025-08-10  
**Impact:** Was causing complete development blockage

### Summary

The `Board::setStartingPosition()` method was hanging indefinitely in Debug builds. The root cause was improper bounds checking when processing pieces, leading to undefined behavior when `colorOf()` and `typeOf()` functions were called on `NO_PIECE` (value 12). In Debug mode, asserts in the Material class would trigger, causing the hang.

**ROOT CAUSE:** Multiple issues with bounds checking:
1. `setPiece()` method was not properly validating piece values before calling `colorOf()` and `typeOf()`
2. Material class `update()` method had an assert that would fail on invalid pieces
3. When `NO_PIECE` (12) is passed to `colorOf()`, it returns 2 (12/6), which is out of bounds for Color enum
4. Missing include of `pst.h` in board.cpp caused compilation issues

### Symptom

- Calling `board.setStartingPosition()` hangs indefinitely
- No CPU usage during hang (suggests deadlock or infinite wait)
- Hang occurs BEFORE entering the function (debug output at function entry never executes)
- Even simple test programs hang immediately

### Code Changes That Triggered Issue

The cpp-pro agent added:
1. **SearchHistory class** - Separate history tracking for search vs game
2. **Thread-local storage** - `thread_local std::unique_ptr<SearchHistory>` 
3. **MoveGuard RAII pattern** - Exception-safe move handling
4. **Static mutex** - For thread safety preparation

### Investigation Results

- Hang occurs before function entry (not inside the function)
- Suggests static initialization order fiasco or thread-local initialization issue
- Problem persists even after `git stash` (indicates build corruption)
- Global constructors detected in binary that may be problematic

### Impact

- **COMPLETE BLOCKAGE** - Cannot test ANY Stage 9b functionality
- Cannot run repetition detection tests
- Cannot validate fifty-move rule
- Cannot run SPRT validation
- Development completely halted

### Resolution

**Fixed by:**
1. Added proper bounds checking in `setPiece()` to validate pieces before using them
2. Added bounds checking in `updateBitboards()` to validate color and piece type values
3. Added bounds checking in `Material::add()`, `remove()`, and `update()` methods
4. Added missing `#include "../evaluation/pst.h"` to board.cpp
5. Fixed `recalculatePSTScore()` and `rebuildZobristKey()` to check piece bounds
6. Fixed `applyFenData()` to validate pieces before processing

**Testing:** 
- Works correctly in both Debug and Release builds
- `setStartingPosition()` now completes successfully
- All Stage 9b tests can now run properly
- Engine accepts "position startpos" command and runs perft correctly

### Related Documents

- `/workspace/project_docs/planning/stage9b_setStartingPosition_issue.md` - Detailed analysis
- Stage 9b implementation files affected

**This is the highest priority issue - must be resolved before any progress can continue.**

---

## General Testing Recommendation

**IMPORTANT:** For all perft test failures, we should:

1. **Always validate test expectations with Stockfish first** before debugging
2. Use the perft_debug tool for validation:
```bash
./tools/debugging/perft_debug compare "[FEN_STRING]" [DEPTH]
```

3. If our engine matches Stockfish but differs from test expectations, update the test data
4. Only debug actual discrepancies after confirming test values are correct

This will prevent wasting time debugging "failures" that are actually incorrect test data, as we discovered with multiple positions (Position 6, Position 7, etc.).

---

## Bug #011: Stage 14 - Board Initialization Hang in Test Scripts

**Status:** IDENTIFIED - Not Yet Resolved  
**Priority:** Medium (affects test suite execution only)  
**Discovery Date:** 2025-08-14  
**Impact:** WAC and Bratko-Kopec test scripts hang during board initialization

### Summary

During Stage 14 implementation, discovered that board initialization hangs when running certain test scripts, specifically the WAC (Win At Chess) and Bratko-Kopec (BK) test suites. The issue appears to be related to FEN parsing or board state initialization but does NOT affect normal UCI operation or SPRT testing.

### Symptoms

- Test scripts like `run_test_suite.sh` and `run_full_bk_test.sh` hang indefinitely
- Hang occurs during board initialization when processing test positions
- Normal UCI operation works fine (engine plays games without issues)
- SPRT tests run successfully
- Issue only manifests with specific test suite execution patterns

### Affected Components

**Test Scripts:**
- `/workspace/run_test_suite.sh` - WAC test suite runner
- `/workspace/run_full_bk_test.sh` - Bratko-Kopec test suite runner

**Test Files:**
- `/workspace/tests/positions/wac.epd` - 300 WAC positions
- `/workspace/tests/positions/bratko_kopec.epd` - 24 BK positions

### Characteristics

- Does NOT affect normal engine operation
- Does NOT affect SPRT testing or match play
- Only affects batch test suite processing
- May be related to rapid successive FEN parsing
- Could be thread-local storage or static initialization issue

### Workaround

Currently, individual positions can be tested manually via UCI:
```bash
echo -e "position fen [FEN_STRING]\ngo depth 6\nquit" | ./bin/seajay
```

This works without issues, suggesting the problem is specific to the test harness implementation rather than core engine functionality.

### Investigation Notes

- Similar to Bug #010 (Stage 9b setStartingPosition hang) but with different trigger
- May involve interaction between quiescence search initialization and test harness
- Could be related to static/thread-local variables in Stage 14 additions
- Needs investigation of test script execution flow vs normal UCI flow

### Impact Assessment

**Low-Medium Impact:**
- Does not affect competitive play or SPRT validation
- Does not affect normal UCI operation
- Only impacts tactical test suite validation
- Can work around with manual testing if needed

### Priority Rationale

Marked as Medium priority because:
- It blocks automated tactical testing
- Does not affect core engine functionality
- Has a viable workaround (manual testing)
- Should be fixed but not critical for Stage 14 completion

### Next Steps

1. Compare initialization flow between UCI and test scripts
2. Check for static initialization order issues
3. Investigate thread-local storage in quiescence search
4. Review differences in how test scripts vs UCI initialize the board
5. Consider if rapid FEN parsing triggers resource exhaustion