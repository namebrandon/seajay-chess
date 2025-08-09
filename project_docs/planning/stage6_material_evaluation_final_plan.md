# SeaJay Chess Engine - Stage 6: Material Evaluation Final Implementation Plan

**Document Version:** 2.0 (Final)  
**Date:** August 9, 2025  
**Stage:** Phase 2, Stage 6 - Material Evaluation  
**Prerequisites Completed:** Yes - Phase 1 Complete  
**Reviews Completed:** cpp-pro ✅, chess-engine-expert ✅  

## Executive Summary

Stage 6 implements material evaluation, marking the transition from random play to intelligent move selection based on piece values. This plan incorporates expert feedback on modern C++ practices and chess engine pitfalls to ensure a robust foundation for Phase 2.

## Current State Analysis

### Phase 1 Achievements:
- Complete legal move generation (99.974% perft accuracy)
- Full UCI protocol implementation
- Hybrid bitboard-mailbox architecture
- Comprehensive testing infrastructure
- SPRT framework ready

### Missing Components:
- No evaluation function
- No material tracking
- Random move selection only
- No score representation

## Deferred Items Review

### From Phase 1:
- **BUG #001**: Position 3 perft discrepancy (0.026% deficit) - Low priority, defer to later in Phase 2
- No items specifically deferred TO Stage 6

### Items We'll Defer FROM Stage 6:
- Piece-square tables → Stage 9
- Endgame-specific values → Future phases
- Material imbalance tables → Future phases
- Bishop pair bonus → Future phases

## Implementation Plan

### Phase 1: Core Data Structures (3 hours)

#### 1.1 Score Type with Strong Typing
```cpp
// src/evaluation/types.h
class Score {
    using value_type = int32_t;  // Internal: prevent overflow
    
    constexpr Score operator+(Score rhs) const noexcept {
        return Score(saturate_add(m_value, rhs.m_value));
    }
    
    static consteval Score zero() { return Score(0); }
    static consteval Score mate() { return Score(32000); }
    static consteval Score draw() { return Score(0); }
    
    int16_t to_cp() const { return std::clamp(m_value, -32000, 32000); }
};
```

#### 1.2 Material Tracking Structure
```cpp
// src/evaluation/material.h
class alignas(64) Material {  // Cache-line aligned
    std::array<int8_t, 12> m_counts{};
    std::array<Score, 2> m_values{};
    
    void update(Piece p, bool add);
    Score balance(Color stm) const;
    bool isInsufficientMaterial() const;
    bool isSameColoredBishops() const;  // Critical for draw detection
};
```

#### 1.3 Updated Piece Values
```cpp
constexpr std::array<Score, 6> PIECE_VALUES = {
    Score(100),   // PAWN
    Score(320),   // KNIGHT  
    Score(330),   // BISHOP (slightly > knight)
    Score(510),   // ROOK (updated from 500)
    Score(950),   // QUEEN (updated from 900)
    Score(0)      // KING (not counted in material)
};
```

### Phase 2: Evaluation Function (3 hours)

#### 2.1 Static Evaluation Entry Point
```cpp
// src/evaluation/evaluate.cpp
Result<Score> evaluate(const Board& board) {
    // Check insufficient material first
    if (board.material().isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Check same-colored bishops draw
    if (board.material().isSameColoredBishops()) {
        return Score::draw();
    }
    
    // Return material balance from side-to-move perspective
    return board.material().balance(board.sideToMove());
}
```

#### 2.2 Debug Verification
```cpp
#ifdef DEBUG
void verifyMaterialIncremental(const Board& board) {
    Material scratch = recountMaterialFromScratch(board);
    assert(scratch == board.material());
}
#endif
```

### Phase 3: Board Integration (4 hours)

#### 3.1 Material Updates in Board Class
```cpp
class Board {
    Material m_material;
    mutable Score m_evalCache{};
    mutable bool m_evalCacheValid{false};
    
    void setPiece(Square s, Piece p) {
        Piece old = m_mailbox[s];
        if (old != NO_PIECE) m_material.update(old, false);
        if (p != NO_PIECE) m_material.update(p, true);
        m_evalCacheValid = false;
        // ... existing logic
    }
    
    // Special move handling
    void makeMoveCastling(Move m) {
        // DON'T double-update rook!
        // Material unchanged
    }
    
    void makeMoveEnPassant(Move m) {
        // Remove captured pawn from correct square!
        Square capturedSq = /* not destination! */;
        m_material.update(pawnOn(capturedSq), false);
    }
    
    void makeMovePromotion(Move m) {
        // Remove pawn, add promoted piece
        m_material.update(PAWN, false);
        m_material.update(promotedPiece(m), true);
    }
};
```

#### 3.2 FEN Setup Material Reset
```cpp
void Board::setupPosition(const std::string& fen) {
    // CRITICAL: Reset material counts!
    m_material = Material{};
    // Then parse and rebuild
}
```

### Phase 4: Move Selection (3 hours)

#### 4.1 Simple Best Capture Selection
```cpp
// src/search/search.cpp
Move selectBestMove(const Board& board) {
    MoveList moves;
    generateLegalMoves(board, moves);
    
    Move bestMove = MOVE_NONE;
    Score bestScore = Score(-32000);
    
    for (Move m : moves) {
        Board copy = board;
        copy.makeMove(m);
        
        // Negate because we evaluate from opponent's perspective
        Score score = -copy.evaluate();
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
    }
    
    return bestMove != MOVE_NONE ? bestMove : moves[0];
}
```

### Phase 5: Testing Infrastructure (4 hours)

#### 5.1 Critical Test Positions
```cpp
const TestPosition MATERIAL_TESTS[] = {
    // Symmetry tests
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", 0},
    
    // Insufficient material (all should be 0)
    {"K2k4/8/8/8/8/8/8/8 w - -", 0},           // K vs K
    {"KB1k4/8/8/8/8/8/8/8 w - -", 0},          // KB vs K
    {"KN1k4/8/8/8/8/8/8/8 w - -", 0},          // KN vs K  
    {"KNNk4/8/8/8/8/8/8/8 w - -", 0},          // KNN vs K
    {"Kb1kB3/8/8/8/8/8/8/8 w - -", 0},         // Same colored bishops
    
    // NOT insufficient material
    {"Kb1kB3/8/8/8/8/8/8/7B w - -", 330},     // Opposite colored bishops
    
    // Special moves
    {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq -", 0},   // Castling ready
    {"8/2p5/3p4/KP4r1/8/8/8/8 w - c6", -400}, // En passant
    {"8/P7/8/8/8/8/8/k6K w - -", 850},        // Promotion imminent
    
    // Overflow test
    {"QQQQQQQQ/QQQQQQQQ/8/8/8/8/8/k6K w - -", 15200}
};
```

#### 5.2 Incremental Update Verification
```cpp
TEST(Material, IncrementalMatchesFromScratch) {
    for (const auto& fen : TEST_POSITIONS) {
        Board board(fen);
        
        // Generate random game
        for (int i = 0; i < 100; ++i) {
            MoveList moves;
            generateLegalMoves(board, moves);
            if (moves.empty()) break;
            
            Move m = moves[rand() % moves.size()];
            board.makeMove(m);
            
            // Verify incremental matches from-scratch
            Material scratch = recountMaterial(board);
            ASSERT_EQ(board.material(), scratch);
            
            // Verify symmetry
            Board flipped = board.flipped();
            ASSERT_EQ(board.evaluate(), -flipped.evaluate());
        }
    }
}
```

### Phase 6: SPRT Validation (2 hours)

#### 6.1 Test Configuration
```bash
fast-chess -engine cmd=./seajay_stage6 name=material \
           -engine cmd=./seajay_random name=random \
           -each tc=8+0.08 -rounds 1000 -repeat \
           -concurrency 4 -recover \
           -openings file=4moves_test.pgn format=pgn \
           -sprt elo0=0 elo1=200 alpha=0.05 beta=0.05
```

#### 6.2 Expected Results
- Win rate: >95% against random
- Elo gain: 200-400
- No time losses
- Captures all hanging pieces

## Risk Analysis and Mitigation

### Critical Risks:

1. **Sign/Perspective Errors** (HIGH)
   - **Mitigation**: Always return from side-to-move perspective
   - **Test**: Symmetry validation on every position
   - **Debug**: Assert(eval(pos) == -eval(pos.flipped()))

2. **Special Move Material Updates** (HIGH)
   - **Mitigation**: Explicit handlers for castling, en passant, promotion
   - **Test**: Specific test cases for each special move
   - **Debug**: Verify material unchanged for castling

3. **Integer Overflow** (MEDIUM)
   - **Mitigation**: Use int32_t internally, clamp for UCI
   - **Test**: Maximum material positions
   - **Debug**: Saturating arithmetic

4. **Incremental Update Bugs** (HIGH)
   - **Mitigation**: Debug mode full recount verification
   - **Test**: Random game incremental verification
   - **Debug**: Assert after every move

5. **Draw Detection Errors** (MEDIUM)
   - **Mitigation**: Comprehensive insufficient material checks
   - **Test**: All draw positions return 0
   - **Debug**: Bishop color square detection

## Success Criteria Checklist

- [ ] Material counting 100% accurate for all test positions
- [ ] Symmetry test passes: eval(pos) == -eval(flipped_pos)
- [ ] All insufficient material positions evaluate to 0
- [ ] Same-colored bishops detected as draw
- [ ] Special moves maintain correct material
- [ ] No integer overflow with maximum material
- [ ] Incremental updates match from-scratch recount
- [ ] SPRT passes with >200 Elo gain vs random
- [ ] Engine captures all hanging pieces
- [ ] No time control violations in 1000 games
- [ ] Zero compiler warnings with -Wall -Wextra
- [ ] Performance: <5% slowdown vs Phase 1

## Implementation Checklist

### Setup (30 minutes)
- [ ] Create src/evaluation/ directory structure
- [ ] Update CMakeLists.txt for new module
- [ ] Create test file structure

### Core Implementation (8 hours)
- [ ] Implement Score type with overflow protection
- [ ] Create Material class with incremental updates
- [ ] Add material tracking to Board class
- [ ] Implement static evaluation function
- [ ] Create best move selection
- [ ] Replace random mover in UCI

### Testing (4 hours)
- [ ] Unit tests for material counting
- [ ] Symmetry validation tests
- [ ] Special move tests
- [ ] Incremental update verification
- [ ] Integration tests with UCI
- [ ] SPRT validation run

### Documentation (1 hour)
- [ ] Code documentation
- [ ] Update project_status.md
- [ ] Update deferred_items_tracker.md
- [ ] Create development diary entry

## Total Timeline: 13.5 hours

## Code Organization

```
src/
├── evaluation/
│   ├── types.h          # Score type, constants
│   ├── material.h        # Material class
│   ├── material.cpp      # Material implementation
│   ├── evaluate.h        # Evaluation interface
│   └── evaluate.cpp      # Static evaluation
├── search/
│   ├── search.h          # Search interface
│   └── search.cpp        # Best move selection
└── core/
    └── board.h/cpp       # Material integration

tests/
├── evaluation/
│   ├── material_test.cpp # Material counting tests
│   ├── symmetry_test.cpp # Symmetry validation
│   └── evaluate_test.cpp # Evaluation tests
└── integration/
    └── sprt_test.sh      # SPRT validation script
```

## Key Implementation Notes

1. **Start Simple**: Basic material only, no positional factors
2. **Test First**: Write tests before implementation
3. **Debug Mode**: Heavy assertions in debug builds
4. **Incremental**: Never recalculate from scratch in production
5. **Perspective**: Always from side-to-move view
6. **Special Moves**: Explicit handlers, not generic code
7. **Documentation**: Comment tricky evaluation logic

## Post-Implementation Tasks

1. Run full perft suite to ensure move generation unchanged
2. Benchmark performance vs Phase 1
3. Run extended SPRT test (10,000 games)
4. Update project documentation
5. Create git commit with SPRT results
6. Prepare for Stage 7 (Negamax Search)

## Approval Gates

Before proceeding to implementation:
- [x] Expert reviews incorporated
- [x] Risk mitigation strategies defined
- [x] Test positions comprehensive
- [x] Success criteria clear
- [x] Deferred items documented
- [ ] Final review by human developer

**This plan is ready for implementation following approval.**