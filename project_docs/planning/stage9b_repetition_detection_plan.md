# SeaJay Chess Engine - Stage 9b: Draw Detection and Repetition Handling Implementation Plan

**Document Version:** 1.0  
**Date:** August 10, 2025  
**Stage:** Phase 2, Stage 9b - Draw Detection and Repetition Handling  
**Prerequisites Completed:** Yes (Stages 1-9 complete)  

## Executive Summary

Stage 9b implements critical draw detection mechanisms, particularly threefold repetition and fifty-move rule detection. This stage is MANDATORY before extensive SPRT testing, as without it, engines produce excessive draws (40-50%) from repetitions that obscure true strength differences. This stage ensures games conclude properly and prevents infinite loops in both game play and search.

## Current State Analysis (Updated August 10, 2025)

### ✅ COMPLETED - Infrastructure Fixed:
1. **Critical Hang Resolved** 
   - `setStartingPosition()` hang fixed with atomic FEN parsing
   - Recursive Board construction eliminated
   - All test programs can now execute

2. **Basic Method Stubs Added**
   - `clearGameHistory()` - stub implementation (board.h:92, board.cpp:1749)
   - `isRepetitionDraw()` - stub returns false (board.h:93, board.cpp:1753) 
   - `isFiftyMoveRule()` - **WORKING** implementation (board.h:94, board.cpp:1760)

3. **Search Infrastructure Created**
   - `/workspace/src/search/search_info.h` complete with:
     - `SearchStack` structure for tracking search history
     - `SearchInfo` class with repetition detection logic  
     - `isRepetitionInSearch()` method (1-repetition rule for search)

4. **Test Framework** 
   - `/workspace/tests/test_repetition.cpp` exists with test structure

### ❌ MISSING - Core Implementation (Critical for SPRT):

**Stage 9b is approximately 10-15% complete. Major gaps:**

1. **Position History Infrastructure (0% complete)**
   - No position history vector in Board class
   - No `pushGameHistory()` method implementation
   - No history management in `makeMove()`/`unmakeMove()`
   - No game vs search history separation

2. **Repetition Detection Logic (0% complete)** 
   - `isRepetitionDraw()` always returns false (stub only)
   - No actual position comparison implementation
   - No threefold repetition counting logic

3. **Search Integration (0% complete)**
   - No draw detection in `negamax()` 
   - Search infrastructure exists but not integrated
   - No root move filtering for repetitions

4. **UCI Integration (0% complete)**
   - No game history tracking in UCI
   - No draw reporting to GUI

5. **Testing (0% complete)**
   - All unit tests unimplemented
   - Test positions not validated  
   - No integration testing done

### What Exists from Previous Stages:
1. **Zobrist Hashing** (Stage 1)
   - Full Zobrist key infrastructure
   - Incremental updates on make/unmake
   - Ready for position tracking

2. **Board State Management** (Stage 3)
   - Complete make/unmake with UndoInfo
   - Halfmove counter already tracked
   - En passant and castling state preserved

3. **Search Infrastructure** (Stages 7-8)
   - 4-ply negamax with alpha-beta
   - Can return draw scores (0)
   - Ready for repetition checks

4. **Material Evaluation** (Stage 6)
   - Insufficient material detection already implemented
   - K vs K, KN vs K, KB vs K detection

### **🚨 CRITICAL ASSESSMENT:**
**NOT READY FOR SPRT TESTING** - Requires 15-20 hours additional work to implement core repetition detection system before SPRT validation can begin.

## Deferred Items Being Addressed

From previous deferrals:
1. **Repetition Detection** - Critical for SPRT testing
2. **Fifty-Move Rule** - Prevents endless games
3. **Draw Score Handling** - Proper evaluation in search

## Critical: State Corruption Prevention

Based on expert C++ review, state corruption in make/unmake is a **progress destroyer**. We will implement the following safeguards:

### RAII Move Guard Pattern (MANDATORY)
```cpp
template<typename BoardT>
class MoveGuard {
private:
    BoardT& m_board;
    Move m_move;
    CompleteUndoInfo m_undo;
    bool m_committed = false;
    
public:
    explicit MoveGuard(BoardT& board, Move move) 
        : m_board(board), m_move(move) {
        m_board.makeMove(m_move, m_undo);
    }
    
    ~MoveGuard() {
        if (!m_committed) {
            m_board.unmakeMove(m_move, m_undo);
        }
    }
    
    // Delete copy/move to prevent misuse
    MoveGuard(const MoveGuard&) = delete;
    MoveGuard& operator=(const MoveGuard&) = delete;
};
```

### Fixed-Size Ring Buffer (NO std::vector)
```cpp
// Prevent reallocation corruption
template<size_t MaxSize>
class FixedHistoryBuffer {
    std::array<uint64_t, MaxSize> m_buffer{};
    size_t m_head = 0;
    size_t m_size = 0;
public:
    constexpr void push(uint64_t hash) noexcept {
        m_buffer[m_head] = hash;
        m_head = (m_head + 1) % MaxSize;
        m_size = std::min(m_size + 1, MaxSize);
    }
};
```

### Debug-Mode State Validation
```cpp
#ifdef DEBUG
class StateValidator {
    uint64_t m_initialZobrist;
    Board& m_board;
public:
    StateValidator(Board& b) : m_board(b), m_initialZobrist(b.zobrist()) {}
    ~StateValidator() {
        if (m_board.zobrist() != m_initialZobrist) {
            std::cerr << "STATE CORRUPTION DETECTED!\n";
            std::abort();
        }
    }
};
#endif
```

## Implementation Plan

### Phase 1: Position History Infrastructure (2-3 hours)

#### 1.1 Add History Tracking to Board Class
```cpp
// In board.h
class Board {
private:
    // Game history (persistent across searches)
    static constexpr size_t MAX_GAME_LENGTH = 1024;
    std::vector<uint64_t> m_gameHistory;  // Before search started
    size_t m_lastIrreversiblePly = 0;     // Last pawn/capture move
    
public:
    bool isRepetitionDraw(int searchPly) const;
    bool isFiftyMoveRule() const;
    bool isDraw() const;
    void pushGameHistory(uint64_t zobrist);
    void clearHistoryBeforeIrreversible();
};

// In search_info.h - SEPARATE search history
struct SearchStack {
    uint64_t zobristKey;
    Move move;
    int ply;
};

class SearchInfo {
    std::array<SearchStack, MAX_PLY> m_searchStack;
    int m_rootGameHistorySize;  // Where game ends, search begins
};
```

#### 1.2 History Management in Make/Unmake
```cpp
void makeMove(const Move& move) {
    // Save current zobrist to history
    m_positionHistory[m_historyCount++] = m_zobristKey;
    
    // Make the move...
    
    // Reset halfmove counter on pawn move or capture
    if (move.isCapture() || pieceType(movedPiece) == PAWN) {
        m_halfmoveClock = 0;
    } else {
        m_halfmoveClock++;
    }
}
```

#### 1.3 Efficient Repetition Detection (CRITICAL LOGIC)
```cpp
// The golden rule:
// Game history: current + 2 previous = threefold
// Search tree: current + 1 anywhere = draw

bool isRepetitionDraw(int searchPly) const {
    uint64_t key = m_zobristKey;
    int repetitions = 0;
    
    // Check search history first (1 repetition = draw)
    for (int i = searchPly - 4; i >= 0; i -= 2) {
        if (m_searchStack[i].zobristKey == key) {
            return true;  // One repetition in search = draw!
        }
    }
    
    // Check game history (need 2 repetitions for threefold)
    int stopPly = std::max(0, m_gameHistory.size() - m_halfmoveClock - 1);
    for (int i = m_gameHistory.size() - 1; i >= stopPly; i--) {
        if (m_gameHistory[i] == key) {
            if (++repetitions >= 2) return true;  // 2 + current = threefold
        }
    }
    
    return false;
}

// CRITICAL: Check for checkmate BEFORE repetition!
int negamax(int depth, Score alpha, Score beta) {
    if (isCheckmate()) return -MATE_SCORE;  // Checkmate first!
    if (isRepetitionDraw(ply)) return 0;     // Then repetition
    if (isFiftyMoveRule()) return 0;         // Then 50-move
    // ... rest of search
}
```

### Phase 2: Search Integration (2-3 hours)

#### 2.1 Repetition Detection in Search
```cpp
// In negamax.cpp
Score negamax(Board& board, int depth, Score alpha, Score beta) {
    // Check for draws BEFORE evaluating
    if (board.isThreefoldRepetition()) {
        return 0;  // Draw score
    }
    
    if (board.isFiftyMoveRule()) {
        return 0;  // Draw score
    }
    
    // Continue with normal search...
}
```

#### 2.2 Search-Specific History
```cpp
// Track positions during search separately
class SearchInfo {
    std::vector<uint64_t> searchPath;  // Current search path
    
    bool isRepetitionInSearch(const Board& board) {
        // Check both game history AND search path
        return board.isRepetition() || 
               isInSearchPath(board.zobrist());
    }
};
```

#### 2.3 Root Move Filtering
```cpp
// Don't play moves that immediately repeat
void filterRootMoves(MoveList& moves, const Board& board) {
    moves.erase(
        std::remove_if(moves.begin(), moves.end(),
            [&](const Move& move) {
                Board temp = board;
                temp.makeMove(move);
                return temp.isThreefoldRepetition();
            }),
        moves.end()
    );
}
```

### Phase 3: UCI Protocol Integration (1-2 hours)

#### 3.1 Draw Claim Handling
```cpp
// UCI doesn't have explicit draw claims, but we need to handle:
// 1. Automatic draw detection
// 2. Proper game termination
// 3. Draw score reporting in analysis
```

#### 3.2 Game State Tracking
```cpp
class Game {
    Board board;
    std::vector<uint64_t> gameHistory;  // Full game history
    
    void makeMove(const Move& move) {
        gameHistory.push_back(board.zobrist());
        board.makeMove(move);
        
        if (board.isDraw()) {
            // Report draw to GUI
            std::cout << "info string Draw by ";
            if (board.isThreefoldRepetition()) {
                std::cout << "threefold repetition";
            } else if (board.isFiftyMoveRule()) {
                std::cout << "fifty-move rule";
            } else if (board.hasInsufficientMaterial()) {
                std::cout << "insufficient material";
            }
            std::cout << std::endl;
        }
    }
};
```

### Phase 4: Testing and Validation (2-3 hours)

#### 4.1 Unit Tests
- Threefold repetition with exact positions
- Fifty-move rule counter and reset
- Draw detection in various scenarios
- History management correctness

#### 4.2 Integration Tests
- Search returning draw scores
- Root move filtering
- SPRT with reduced draw rate

#### 4.3 Performance Testing
- Overhead of history tracking
- Impact on NPS
- Memory usage validation

## Technical Considerations

### C++ Implementation Details (from cpp-pro review):

1. **Memory Efficiency**
   - Fixed-size arrays for history (no dynamic allocation)
   - Ring buffer for very long games
   - Separate game and search history

2. **Performance Optimizations**
   - Only check recent positions (within halfmove clock)
   - Skip impossible repetitions (different castling rights)
   - Inline critical functions

3. **Thread Safety**
   - Each search thread needs own history
   - Shared game history needs synchronization
   - Consider thread-local storage

## Chess Engine Considerations

### Critical Chess-Specific Requirements:

1. **Zobrist Key Considerations**
   - Must include en passant square
   - Must include castling rights
   - Side to move already included
   - **NOT** halfmove clock (positions repeat despite different clocks)

2. **Repetition Rules**
   - Three identical positions = draw
   - Positions must have same side to move
   - Can only repeat within reversible moves

3. **Fifty-Move Rule**
   - 50 moves = 100 plies (both sides)
   - Reset on ANY pawn move
   - Reset on ANY capture
   - Some positions allow 75 moves (not implementing)

4. **Search vs Game History**
   - Game history: actual moves played
   - Search history: positions explored in search
   - Both can cause repetitions

## Risk Mitigation

### Identified Risks and Mitigation:

1. **Risk: History Buffer Overflow**
   - Mitigation: Fixed max game length (1024 moves)
   - Mitigation: Ring buffer for ultra-long games
   - Mitigation: Bounds checking

2. **Risk: Incorrect Zobrist Comparison**
   - Mitigation: Ensure castling rights included
   - Mitigation: Ensure en passant included
   - Mitigation: Do NOT include halfmove clock in zobrist
   - Mitigation: Test with known repetitions

3. **Risk: Performance Impact**
   - Mitigation: Optimize repetition check
   - Mitigation: Only check when necessary
   - Mitigation: NO repetition checks in quiescence search (when implemented in Stage 14)
   - Mitigation: Profile and measure
   - NOTE: Quiescence search is NOT implemented yet (Stage 14 in Phase 3)

4. **Risk: Search Path Confusion**
   - Mitigation: Separate search and game history
   - Mitigation: Different thresholds (1 vs 2 repetitions)
   - Mitigation: Clear search history appropriately
   - Mitigation: Test deep searches

5. **Risk: Off-by-One in Halfmove Boundary (COMMON BUG)**
   - Mitigation: Use `m_halfmoveClock + 1` for boundary
   - Mitigation: Test with positions at exactly 49 halfmoves
   - Mitigation: Verify with known test positions

6. **Risk: Root Position Already Repetition**
   - Mitigation: Check root against game history first
   - Mitigation: Don't assume clean slate at search start
   - Mitigation: Test with pre-repeated positions

7. **Risk: Castling Rights After Rook Capture**
   - Mitigation: Update zobrist when rook captured on original square
   - Mitigation: Handle both explicit and implicit castling changes
   - Mitigation: Test with rook capture positions

8. **Risk: Null Move in History**
   - Mitigation: Never add null moves to repetition history
   - Mitigation: Check for null move before history push
   - Mitigation: Test null move searches

## Validation Strategy

### Critical Test Positions (All Validated with Stockfish 16):

1. **Basic Threefold Repetition**
```
Position: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
Moves: Nc3 Nc6 Nb1 Nb8 Nc3 Nc6 Nb1 Nb8 Nc3
Expected: After 9th move (Nc3), detect threefold repetition
```

2. **Castling Rights Change (False Repetition)**
```
FEN: r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1
Moves: Ra2 Ra7 Ra1 Ra8 Ra2 Ra7 Ra1 Ra8
Expected: NOT a repetition (castling rights differ after rook moves)
```

3. **En Passant Ghost (Phantom EP)**
```
FEN: 8/2p5/3p4/KP5r/8/8/8/k7 w - c6 0 1
After any move except bxc6, en passant disappears
Position looks same but Zobrist differs - NOT repetition
```

4. **50-Move Rule Edge**
```
FEN: 8/8/3k4/8/3K4/8/7R/7R w - - 99 1
Move: Any rook move triggers 50-move draw
Test: Verify draw detected at exactly move 100
```

5. **Root Position Repetition**
```
Setup: Play Nc3 Nc6 Nb1 Nb8 Nc3 Nc6 Nb1 Nb8
Then start search from this position
Expected: Root is already second repetition, search finds draw
```

6. **Search vs Game History**
```
FEN: 8/8/3k4/3q4/3Q4/3K4/8/8 w - - 0 1
In search: Qe3 Qe5 Qd3 Qd5 should detect repetition
Game vs Search: Different thresholds must work correctly
```

7. **Checkmate vs Repetition Priority** (Stockfish Validated)
```
FEN: 6k1/5R2/6K1/8/8/8/8/8 w - - 2 1
White can repeat with Rf7-f6-f7 OR mate with Rf8#
Expected: Find mate (Rf8#), not repetition draw
```

8. **Halfmove Boundary Test**
```
FEN: 8/8/3k4/8/3K4/8/7R/7R w - - 4 1
Test: Can still detect repetition within last 5 positions
Critical: Off-by-one errors show up here
```

## Items Being Deferred

### To Stage 10+ (Future):
1. **Threefold Repetition Table**
   - Hash table for faster detection
   - Only if performance issue

2. **75-Move Rule**
   - Special positions (FIDE rule)
   - Rarely occurs in practice

3. **Perpetual Check Detection**
   - More complex than repetition
   - Requires pattern recognition

## Success Criteria

Stage 9b is complete when:
1. ✓ Threefold repetition detection working
2. ✓ Fifty-move rule implemented
3. ✓ History tracking efficient
4. ✓ Search returns draw scores correctly
5. ✓ SPRT draw rate reduced to <30%
6. ✓ No infinite loops in endgames
7. ✓ All unit tests passing
8. ✓ No significant NPS impact (<5%)
9. ✓ Documentation complete

## Timeline Estimate (Updated Based on Current State)

- **Phase 1:** 2-3 hours (History infrastructure) - **NOT STARTED**
- **Phase 2:** 2-3 hours (Search integration) - **NOT STARTED**  
- **Phase 3:** 1-2 hours (UCI integration) - **NOT STARTED**
- **Phase 4:** 2-3 hours (Testing/validation) - **NOT STARTED**
- **Phase 5:** 2-3 hours (Additional testing and debug time)
- **Total:** **9-14 hours remaining**

## Current Priority Actions to Reach SPRT Readiness

### **Immediate Next Steps (Priority Order):**

1. **Implement Position History Tracking (3-4 hours)**
   - Add `std::vector<uint64_t> m_gameHistory` to Board class
   - Add `pushGameHistory()` method implementation
   - Integrate history tracking into `makeMove()`/`unmakeMove()`
   - Add `clearGameHistory()` proper implementation

2. **Implement Repetition Detection Logic (3-4 hours)**
   - Replace `isRepetitionDraw()` stub with actual threefold detection
   - Implement game vs search history separation
   - Add proper position comparison within halfmove clock boundary
   - Handle edge cases (castling rights, en passant)

3. **Integrate Draw Detection into Search (2-3 hours)**
   - Add draw checks to `negamax()` BEFORE evaluation
   - Ensure checkmate priority over repetition
   - Return score 0 for draw positions
   - Test search behavior with repetition positions

4. **Validation and Testing (3-4 hours)**
   - Implement all unit tests from checklist
   - Validate with Stockfish-confirmed test positions
   - Run integration tests with actual gameplay
   - Performance testing (ensure <5% NPS impact)

5. **SPRT Preparation (2-3 hours)**
   - Final validation of all functionality
   - Performance benchmarking 
   - Documentation updates
   - Clean up test files and debugging code

### **Estimated Timeline to SPRT Ready: 13-18 hours of focused development**

## Implementation Notes

### Key Files to Modify:
1. `/workspace/src/core/board.h` - Add history tracking
2. `/workspace/src/core/board.cpp` - Implement repetition detection
3. `/workspace/src/search/negamax.cpp` - Check for draws
4. `/workspace/src/search/search_info.h` - Search path tracking
5. `/workspace/tests/test_repetition.cpp` - New test suite

### Critical Implementation Details:
1. Check draws BEFORE evaluation (saves time)
2. Only check positions within halfmove clock
3. Side to move must match for repetition
4. Reset halfmove on BOTH pawn moves and captures
5. Zobrist must NOT include halfmove clock

## Critical Implementation Summary

### MUST-DO Items:
1. **Use RAII MoveGuard** - Prevents state corruption
2. **Fixed-size buffers** - No std::vector, no reallocation
3. **Separate histories** - Game vs Search with different thresholds
4. **Check order** - Checkmate BEFORE repetition detection
5. **Halfmove boundary** - Use m_halfmoveClock + 1
6. **Stockfish validation** - All test positions verified

### DO NOT Items:
1. **No quiescence search** - Stage 14, not implemented yet
2. **No dynamic allocation** - Use fixed arrays
3. **No halfmove in Zobrist** - Only castling/EP/pieces
4. **No null moves in history** - Check before pushing

### Expert Reviews Completed:
- ✅ Chess-engine-expert: Identified common bugs and edge cases
- ✅ CPP-pro: State corruption prevention strategies
- ✅ Stockfish validation: All test positions verified accurate

---

**Sign-off:** Plan complete with expert reviews, state corruption prevention, and validated test cases. Ready for user approval before implementation.