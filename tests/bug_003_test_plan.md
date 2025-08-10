# Bug #003 Test Plan: Promotion Move Handling
## Critical Issue: Blocked Pawn Promotion Generation

### Executive Summary
Position `r3k3/P7/8/8/8/8/8/4K3 w - - 0 1` has a white pawn on a7 blocked by a black rook on a8.
- **Expected**: 5 moves (king moves only, NO promotion moves)
- **Suspected Bug**: SeaJay may be generating illegal promotion moves for the blocked pawn

### Root Cause Analysis Areas

#### 1. Pawn Quiet Move Generation (`move_generation.cpp:219-255`)
**Critical Code Section (lines 231-237):**
```cpp
// Single pawn push
if (toInt >= 0 && toInt < 64) {
    Square to = static_cast<Square>(toInt);
    if (!(occupied & squareBB(to))) {  // ← CHECK: Is this correctly detecting blocked squares?
        // Check for promotion (pawn is moving from 7th rank to 8th rank)
        if ((us == WHITE && rankOf(from) == 6) || (us == BLACK && rankOf(from) == 1)) {
            moves.addPromotionMoves(from, to);  // ← BUG: This adds 4 moves if square is "empty"
```

**Potential Issue**: The code checks if the destination square is occupied, but for the position in question:
- Pawn is on a7 (from square)
- Destination would be a8 (to square)
- The condition `!(occupied & squareBB(to))` should FAIL if a8 has the black rook
- If this check passes incorrectly, 4 illegal promotion moves would be generated

#### 2. Board Occupancy Tracking
**Files to Check:**
- `/workspace/src/core/board.cpp` - How is `occupied()` maintained?
- `/workspace/src/core/bitboard.h` - Are bitboard operations correct?

### Test Methodology

#### Phase 1: Immediate Verification
1. Build SeaJay with debug output
2. Test the exact failing position
3. Add debug output to show:
   - Value of `occupied` bitboard
   - Value of `squareBB(a8)`
   - Result of the AND operation
   - Whether promotion moves are being added

#### Phase 2: Systematic Position Testing
Test all positions below and compare with Stockfish 17.1

### Test Cases

#### Category 1: Blocked Promotion Positions (Should Generate NO Promotions)

| # | FEN | Description | Expected Moves |
|---|-----|-------------|----------------|
| 1 | `r3k3/P7/8/8/8/8/8/4K3 w - - 0 1` | Original bug position - White pawn a7 blocked by black rook a8 | 5 (King: e1-d1,d2,e2,f1,f2) |
| 2 | `rnbqkbnr/P7/8/8/8/8/8/4K3 w kq - 0 1` | White pawn a7 blocked by full black back rank | 5 (King moves only) |
| 3 | `4k3/8/8/8/8/8/p7/R3K3 b - - 0 1` | Black pawn a2 blocked by white rook a1 | 5 (King: e8-d8,d7,e7,f8,f7) |
| 4 | `n3k3/P7/8/8/8/8/8/4K3 w - - 0 1` | White pawn a7 blocked by black knight a8 | 5 (King moves only) |
| 5 | `b3k3/1P6/8/8/8/8/8/4K3 w - - 0 1` | White pawn b7 blocked by black bishop b8 | 5 (King moves only) |
| 6 | `q3k3/3P4/8/8/8/8/8/4K3 w - - 0 1` | White pawn d7 blocked by black queen d8 | 5 (King moves only) |
| 7 | `4k3/7P/8/8/8/8/8/4K2R w - - 0 1` | White pawn h7 blocked by black king h8 | 6 (King: 5, Rook: h1-g1) |

#### Category 2: Valid Promotion Positions (Should Generate Promotions)

| # | FEN | Description | Expected Moves |
|---|-----|-------------|----------------|
| 8 | `4k3/P7/8/8/8/8/8/4K3 w - - 0 1` | White pawn a7, a8 empty | 9 (King: 5, Promotions: a7-a8=Q/R/B/N) |
| 9 | `4k3/1P6/8/8/8/8/8/4K3 w - - 0 1` | White pawn b7, b8 empty | 9 (King: 5, Promotions: b7-b8=Q/R/B/N) |
| 10 | `4k3/4P3/8/8/8/8/8/4K3 w - - 0 1` | White pawn e7, e8 empty | 9 (King: 5, Promotions: e7-e8=Q/R/B/N) |
| 11 | `4k3/8/8/8/8/8/p7/4K3 b - - 0 1` | Black pawn a2, a1 empty | 9 (King: 5, Promotions: a2-a1=q/r/b/n) |

#### Category 3: Promotion with Capture Positions

| # | FEN | Description | Expected Moves |
|---|-----|-------------|----------------|
| 12 | `rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1` | White pawn a7 can capture rook a8 OR knight b8 | 13 (King: 5, Captures: a7xb8=Q/R/B/N, a7xa8=Q/R/B/N) |
| 13 | `1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1` | White pawn a7 can only capture rook b8 | 9 (King: 5, Captures: a7xb8=Q/R/B/N) |
| 14 | `n3k3/1P6/8/8/8/8/8/4K3 w - - 0 1` | White pawn b7 can capture knight a8, c8 empty | 13 (King: 5, Quiet: b7-c8=Q/R/B/N, Capture: b7xa8=Q/R/B/N) |

#### Category 4: Edge Cases

| # | FEN | Description | Expected Moves |
|---|-----|-------------|----------------|
| 15 | `4k3/PPP5/8/8/8/8/8/4K3 w - - 0 1` | Three white pawns on 7th rank, a8 blocked by king | 13 (King: 5, b7-b8=Q/R/B/N, c7-c8=Q/R/B/N) |
| 16 | `r2nkb1r/PPPPPPPP/8/8/8/8/8/4K3 w - - 0 1` | All white pawns on 7th, various blocks | Count individually |

### Validation Script

```cpp
// test_promotion_bug.cpp
#include <iostream>
#include <vector>
#include <string>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

struct TestCase {
    std::string fen;
    std::string description;
    int expectedMoveCount;
    bool shouldHavePromotions;
};

int main() {
    std::vector<TestCase> tests = {
        // Blocked promotions - NO promotion moves should be generated
        {"r3k3/P7/8/8/8/8/8/4K3 w - - 0 1", "Bug #003: Blocked pawn a7", 5, false},
        {"rnbqkbnr/P7/8/8/8/8/8/4K3 w kq - 0 1", "Blocked by full rank", 5, false},
        {"4k3/8/8/8/8/8/p7/R3K3 b - - 0 1", "Black blocked pawn", 5, false},
        
        // Valid promotions
        {"4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "Clear promotion path", 9, true},
        {"4k3/1P6/8/8/8/8/8/4K3 w - - 0 1", "b7 pawn clear", 9, true},
        
        // Capture promotions
        {"rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "Can capture two pieces", 13, true},
    };
    
    for (const auto& test : tests) {
        Board board;
        board.setFromFEN(test.fen);
        
        MoveList moves;
        MoveGenerator::generateAllMoves(board, moves);
        
        // Count promotion moves
        int promotionCount = 0;
        for (size_t i = 0; i < moves.size(); i++) {
            Move move = moves[i];
            if (moveFlags(move) & PROMOTION) {
                promotionCount++;
                
                // Debug output for promotions
                std::cout << "  Promotion found: " 
                          << squareToString(moveFrom(move)) 
                          << "-" << squareToString(moveTo(move))
                          << " (flags: " << std::hex << moveFlags(move) << std::dec << ")"
                          << std::endl;
            }
        }
        
        bool hasPromotions = (promotionCount > 0);
        bool testPassed = (moves.size() == test.expectedMoveCount) && 
                         (hasPromotions == test.shouldHavePromotions);
        
        std::cout << (testPassed ? "[PASS]" : "[FAIL]") 
                  << " " << test.description << std::endl;
        std::cout << "  FEN: " << test.fen << std::endl;
        std::cout << "  Expected: " << test.expectedMoveCount 
                  << " moves, " << (test.shouldHavePromotions ? "WITH" : "NO") 
                  << " promotions" << std::endl;
        std::cout << "  Got: " << moves.size() 
                  << " moves, " << promotionCount << " promotions" << std::endl;
        
        if (!testPassed) {
            // List all moves for debugging
            std::cout << "  Generated moves:" << std::endl;
            for (size_t i = 0; i < moves.size(); i++) {
                Move move = moves[i];
                std::cout << "    " << moveToString(move) 
                          << " (flags: " << std::hex << moveFlags(move) << std::dec << ")"
                          << std::endl;
            }
        }
        std::cout << std::endl;
    }
    
    return 0;
}
```

### Stockfish Validation Commands

For each test position, validate with:
```bash
echo -e "position fen [FEN]\ngo perft 1\nquit" | ./external/engines/stockfish/stockfish 2>/dev/null | grep "Nodes searched"
```

For move breakdown:
```bash
echo -e "position fen [FEN]\nd\nquit" | ./external/engines/stockfish/stockfish
```

### Expected Results Table

| Position | SeaJay Should Generate | Stockfish Generates | Match? |
|----------|------------------------|---------------------|--------|
| Bug #003 FEN | 5 moves (NO promotions) | 5 moves | Must match |
| Clear a7 pawn | 9 moves (4 promotions) | 9 moves | Must match |
| Blocked variations | King moves only | King moves only | Must match |

### Debug Instrumentation

Add to `move_generation.cpp` line 233:
```cpp
// Debug output for Bug #003
if (rankOf(from) == 6 && us == WHITE) {  // 7th rank white pawn
    std::cerr << "DEBUG: Pawn on " << squareToString(from) 
              << " checking " << squareToString(to) << std::endl;
    std::cerr << "  Occupied bitboard: " << std::hex << occupied << std::dec << std::endl;
    std::cerr << "  Square " << squareToString(to) << " bit: " 
              << std::hex << squareBB(to) << std::dec << std::endl;
    std::cerr << "  Is blocked? " << (occupied & squareBB(to) ? "YES" : "NO") << std::endl;
}
```

### Fix Verification Process

1. **Run test suite** with debug output enabled
2. **Identify** if promotion moves are incorrectly generated
3. **Locate** the exact condition that's failing
4. **Fix** the bug (likely in the occupancy check)
5. **Retest** all positions to ensure no regression
6. **Validate** with Stockfish for all test cases
7. **Run full perft suite** to ensure no other moves broken

### Success Criteria

- Bug #003 position generates exactly 5 moves (no promotions)
- All blocked pawn positions generate NO promotion moves
- All clear pawn positions generate correct promotion moves
- All test cases match Stockfish 17.1 move counts
- No regression in existing perft tests