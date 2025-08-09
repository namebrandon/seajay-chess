# SeaJay Chess Engine - Stage 6: Material Evaluation Implementation Plan

**Document Version:** 1.0  
**Date:** August 9, 2025  
**Stage:** Phase 2, Stage 6 - Material Evaluation  
**Prerequisites Completed:** Yes - Phase 1 Complete  

## Executive Summary

Stage 6 marks the beginning of Phase 2 and the transition from random move selection to intelligent play based on material evaluation. This stage implements basic piece values using the centipawn scale and enables the engine to evaluate positions based on material balance. The implementation will be validated through SPRT testing against the random mover baseline.

## Current State Analysis

### Existing Infrastructure from Phase 1:
1. **Complete Board Representation** (`src/core/board.h/cpp`)
   - Hybrid bitboard-mailbox architecture
   - Full piece tracking and position management
   - Zobrist hashing infrastructure

2. **Legal Move Generation** (`src/core/move_generation.h/cpp`)
   - Complete legal move generation for all piece types
   - Move encoding (16-bit format)
   - Special moves (castling, en passant, promotions)

3. **UCI Protocol** (`src/uci/uci.h/cpp`)
   - Full UCI implementation
   - Position setup and move execution
   - Currently uses random move selection

4. **Testing Infrastructure**
   - Perft validation (99.974% accuracy)
   - fast-chess tournament manager installed
   - Benchmark command implemented
   - SPRT testing scripts ready

### What's Missing:
- No evaluation function exists
- No material counting logic
- No score representation
- No move selection based on evaluation

## Deferred Items Being Addressed

From `/workspace/project_docs/tracking/deferred_items_tracker.md`:
- **BUG #001** (Position 3 Perft): Deferred to later in Phase 2 (low priority)
- No specific items deferred TO Stage 6

## Implementation Plan

### Phase 1: Core Data Structures (2 hours)
1. Create `src/evaluation/types.h`
   - Define Score type (centipawn representation)
   - Define piece value constants
   - Create evaluation result structure

2. Create `src/evaluation/material.h/cpp`
   - Material counting functions
   - Incremental material tracking structure
   - Material balance calculation

### Phase 2: Static Evaluation Function (3 hours)
1. Create `src/evaluation/evaluate.h/cpp`
   - Static evaluation entry point
   - Material-only evaluation initially
   - Side-to-move perspective handling
   - Draw detection (insufficient material)

2. Integration points:
   - Board class material tracking
   - Make/unmake move material updates
   - UCI info output with scores

### Phase 3: Move Selection Logic (3 hours)
1. Update `src/search/search.h/cpp` (new files)
   - Single-ply material evaluation
   - Best capture selection algorithm
   - Move scoring based on material gain/loss
   - Simple quiescence (capture evaluation)

2. UCI Integration:
   - Replace random move selection
   - Add evaluation display in info output
   - Implement simple time management (5% of remaining)

### Phase 4: Testing and Validation (4 hours)
1. Unit Tests:
   - Material counting accuracy
   - Evaluation symmetry
   - Incremental update correctness
   - Special position evaluation

2. Integration Tests:
   - Capture selection validation
   - Draw detection tests
   - Time management tests

3. SPRT Validation:
   - Configure test: material eval vs random
   - Expected 100% win rate
   - Document results

### Phase 5: Optimization and Cleanup (2 hours)
1. Performance optimization:
   - Incremental material updates
   - Cached evaluation scores
   - Move ordering by material gain

2. Code cleanup:
   - Documentation
   - Const correctness
   - Error handling

## Technical Considerations

### C++ Implementation Details:
1. **Score Representation:**
   ```cpp
   using Score = int16_t;  // Centipawn scale
   constexpr Score SCORE_ZERO = 0;
   constexpr Score SCORE_MATE = 32000;
   constexpr Score SCORE_DRAW = 0;
   ```

2. **Piece Values:**
   ```cpp
   constexpr Score PAWN_VALUE = 100;
   constexpr Score KNIGHT_VALUE = 320;
   constexpr Score BISHOP_VALUE = 330;
   constexpr Score ROOK_VALUE = 500;
   constexpr Score QUEEN_VALUE = 900;
   constexpr Score KING_VALUE = 0;  // Or very high for checkmate
   ```

3. **Material Structure:**
   ```cpp
   struct Material {
       int16_t pieces[PIECE_NB];  // Count per piece type
       Score value[COLOR_NB];      // Material value per side
       
       void add(Piece p);
       void remove(Piece p);
       Score balance() const;
   };
   ```

4. **Integration with Board:**
   - Add Material member to Board class
   - Update in setPiece/removePiece/movePiece
   - Provide getter for evaluation

## Chess Engine Considerations

### Material Evaluation Best Practices:
1. **Centipawn Scale:**
   - Industry standard (100 = 1 pawn)
   - Allows fractional pawn advantages
   - Compatible with UCI protocol

2. **Draw Detection:**
   - K vs K
   - K+B vs K
   - K+N vs K
   - K+N+N vs K (usually)

3. **Special Cases:**
   - Promotion evaluation
   - Exchange evaluation (R vs B+N)
   - Piece pairs (bishop pair bonus later)

4. **Common Pitfalls:**
   - Forgetting perspective (white vs black)
   - Integer overflow with large advantages
   - Not handling insufficient material
   - Asymmetric evaluation

## Risk Mitigation

### Identified Risks:

1. **Risk:** Incorrect material counting
   - **Mitigation:** Comprehensive unit tests
   - **Validation:** Compare with starting position known values

2. **Risk:** Evaluation sign errors (perspective)
   - **Mitigation:** Always evaluate from side-to-move perspective
   - **Validation:** Symmetry tests (position and color-flipped)

3. **Risk:** Performance regression
   - **Mitigation:** Incremental updates only
   - **Validation:** Benchmark before/after

4. **Risk:** UCI protocol break
   - **Mitigation:** Maintain backward compatibility
   - **Validation:** GUI testing with Arena/CuteChess

5. **Risk:** Time management failures
   - **Mitigation:** Simple percentage-based initially
   - **Validation:** No timeouts in 1000 games

## Validation Strategy

### Unit Test Suite:
1. Material counting from FEN positions
2. Incremental update verification
3. Draw detection accuracy
4. Evaluation symmetry

### Integration Tests:
1. Best capture selection
2. Hanging piece detection
3. Simple tactics (1-ply)
4. Time control compliance

### SPRT Testing:
```bash
fast-chess -engine cmd=./seajay_material name=material \
           -engine cmd=./seajay_random name=random \
           -each tc=8+0.08 -rounds 1000 -repeat \
           -concurrency 4 -recover \
           -openings file=4moves_test.pgn format=pgn \
           -sprt elo0=0 elo1=100 alpha=0.05 beta=0.05
```

### Expected Outcomes:
- 100% win rate vs random mover
- ~800 Elo strength estimate
- Consistent hanging piece capture
- No time losses

## Items Being Deferred

### To Stage 7 (Negamax):
1. Multi-ply search
2. Recursive evaluation
3. Minimax tree traversal

### To Stage 9 (Positional):
1. Piece-square tables
2. Positional bonuses
3. Pawn structure evaluation

### To Future Phases:
1. Endgame-specific values
2. Material imbalance tables
3. Bishop pair bonus

## Success Criteria

1. ✅ Material counting 100% accurate
2. ✅ Evaluation function returns correct scores
3. ✅ Engine captures hanging pieces consistently
4. ✅ SPRT test passes vs random (100% win rate)
5. ✅ No time control violations
6. ✅ All unit tests passing
7. ✅ Benchmark shows <5% performance impact
8. ✅ UCI protocol compatibility maintained

## Timeline Estimate

- **Total Estimate:** 14 hours
- **Phase 1:** 2 hours (Data structures)
- **Phase 2:** 3 hours (Evaluation function)
- **Phase 3:** 3 hours (Move selection)
- **Phase 4:** 4 hours (Testing/validation)
- **Phase 5:** 2 hours (Optimization)

## Implementation Order

1. Create evaluation module structure
2. Implement piece values and material counting
3. Create static evaluation function
4. Add material tracking to Board
5. Implement single-ply best move selection
6. Replace random mover in UCI
7. Add comprehensive tests
8. Run SPRT validation
9. Optimize and document

## Notes for Implementation

- Start with simplest possible implementation
- Focus on correctness over performance initially
- Maintain backward compatibility with UCI
- Keep evaluation symmetric and deterministic
- Document all design decisions
- Create extensive test coverage early