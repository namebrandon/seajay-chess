# SeaJay Chess Engine - Stage 9: Positional Evaluation Implementation Plan

**Document Version:** 1.1  
**Date:** August 10, 2025  
**Updated:** Added critical edge cases and validation requirements from chess-engine-expert review  
**Stage:** Phase 2, Stage 9 - Positional Evaluation  
**Prerequisites Completed:** Yes (Stages 1-8 complete)  

## Executive Summary

Stage 9 introduces piece-square tables (PST) to add positional evaluation to SeaJay's existing material-only evaluation. This enhancement will teach the engine basic positional concepts like piece centralization, pawn advancement, and king safety positioning. Expected improvement is +150-200 Elo. Implementation will be kept simple and focused, with more advanced evaluation features deferred to future stages.

## Current State Analysis

### Existing Capabilities (from previous stages)
- **Move Generation:** Complete and validated (99.974% perft accuracy)
- **Search:** 4-6 ply alpha-beta with 90% node reduction
- **Evaluation:** Material-only with draw detection for insufficient material
- **Performance:** 1.49M NPS, reaches depth 6 in <1 second
- **Move Ordering:** Basic (promotions → captures → quiet)
- **Current Strength:** ~1200 Elo (estimated from Stage 7 SPRT)

### Current Evaluation System
- `evaluate()` function returns material balance
- Material tracking with incremental updates
- Score type with centipawn representation
- Symmetric evaluation property maintained

## Deferred Items Being Addressed

From `/workspace/project_docs/tracking/deferred_items_tracker.md`:
- **Quiescence Search:** Still deferred to Stage 9b or later
- **MVV-LVA Ordering:** Will be partially addressed (basic implementation)
- **Check Extensions:** Deferred to future stages

## Implementation Plan

### Phase 1: PST Infrastructure (2 hours)

#### 1.1 Create PST Data Structures
```cpp
// src/evaluation/pst.h
namespace seajay::eval {

// Score for both game phases
struct MgEgScore {
    Score mg;  // middlegame
    Score eg;  // endgame
    
    constexpr MgEgScore(Score middlegame, Score endgame) noexcept;
    constexpr MgEgScore operator+(MgEgScore rhs) const noexcept;
};

// Piece-square table for one piece type
template<typename ValueType>
class PieceSquareTable {
    std::array<ValueType, 64> m_values{};
public:
    constexpr ValueType get(Square sq, Color c) const noexcept;
    constexpr ValueType mirror(Square sq) const noexcept;
};

// PST manager
class PST {
    static constexpr std::array<PieceSquareTable<MgEgScore>, 6> s_tables;
public:
    static constexpr Score value(PieceType pt, Square sq, Color c) noexcept;
    static constexpr MgEgScore diff(PieceType pt, Square from, Square to, Color c) noexcept;
};

}
```

#### 1.2 PST Values
- Use simplified PeSTO values (modern, well-tested)
- Compile-time constexpr tables
- Values range: -50 to +50 centipawns
- Separate middlegame and endgame tables

### Phase 2: Board Integration (2 hours)

#### 2.1 Add PST Score Tracking to Board
```cpp
class Board {
private:
    eval::MgEgScore m_pstScore{};  // Incremental PST score
    
public:
    const eval::MgEgScore& pstScore() const noexcept;
    void updatePSTScore(Move move, Piece captured);
    void recalculatePSTScore();  // For validation
};
```

#### 2.2 Update Make/Unmake Functions
- Track PST changes during piece movement
- Handle captures, promotions, castling
- Store PST delta in UndoInfo for unmake

### Phase 3: Evaluation Integration (1 hour)

#### 3.1 Enhanced Evaluation Function
```cpp
Score evaluate(const Board& board) {
    const Color stm = board.sideToMove();
    
    // Material score
    Score material = board.material().balance(stm);
    
    // PST score (no tapering yet)
    const auto& pst = board.pstScore();
    Score positional = (stm == WHITE) ? pst.mg : -pst.mg;
    
    // Combined evaluation
    return material + positional;
}
```

#### 3.2 Add Basic MVV-LVA for Captures
```cpp
// Simple victim-attacker ordering
int mvvLvaScore(Move move) {
    static constexpr int VICTIM_VALUE[7] = {0, 100, 300, 300, 500, 900, 0};
    static constexpr int ATTACKER_VALUE[7] = {0, 10, 30, 30, 50, 90, 0};
    
    if (isCapture(move)) {
        return VICTIM_VALUE[captured(move)] - ATTACKER_VALUE[piece(move)];
    }
    return 0;
}
```

### Phase 4: Testing & Validation (3 hours)

#### 4.1 Unit Tests
```cpp
// Test symmetry property
TEST(PST, EvaluationSymmetry) {
    // Position and its mirror must evaluate identically
}

// Test incremental accuracy
TEST(PST, IncrementalVsRecalculation) {
    // Incremental must match full recalculation
}

// Test PST values are applied correctly
TEST(PST, CorrectValues) {
    // Verify knight on e5 scores higher than a1
}
```

#### 4.2 Integration Tests
- Perft tests must remain unchanged
- Search should prefer central moves from start position
- Evaluation should show positional improvements

#### 4.3 SPRT Validation
```bash
# Material-only vs Material+PST
./tools/scripts/run-sprt.sh seajay_material seajay_pst
# Expected: +150-200 Elo improvement
```

## Technical Considerations

### C++20/23 Features (from cpp-pro review)
1. **constexpr/consteval** for compile-time PST initialization
2. **std::array** instead of C arrays for bounds checking
3. **Concepts** for type safety in evaluation components
4. **alignas(64)** for cache line optimization
5. **[[nodiscard]]** and **[[likely]]/[[unlikely]]** attributes

### Memory & Performance
- Align PST tables to cache lines (64 bytes)
- Pack middlegame/endgame values together
- Use XOR 56 for efficient rank mirroring
- Incremental updates to avoid recalculation

## Chess Engine Considerations (from chess-engine-expert review)

### Critical Requirements
1. **Evaluation Symmetry:** Position and its color-flip must evaluate identically
2. **Rank Mirroring:** Use `sq ^ 56` not `63 - sq` for black pieces
3. **PST Value Ranges:** Keep conservative (-50 to +50 centipawns)
4. **No Pawn Values on 1st/8th Ranks:** Indicates indexing bug

### Critical Edge Cases to Handle
1. **Castling:** Update PST for BOTH king and rook movements
2. **En Passant:** Captured pawn is on different rank (to ± 8)
3. **Promotion:** Remove pawn PST from destination, add promoted piece PST
4. **FEN Loading:** MUST recalculate PST from scratch, never trust incremental
5. **Sign Convention:** Store white perspective, negate for black (avoid double negation)

### Testing Positions
```cpp
// Central knight should score higher
"r1bqkb1r/pppp1ppp/2n2n2/4N3/4P3/8/PPPP1PPP/RNBQKB1R w KQkq -"

// Rook on 7th rank bonus
"8/R7/8/8/8/8/pppppppp/rnbqkbnr w - -"

// King safety differential after castling
"r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq -"
```

## Risk Mitigation

### Identified Risks & Mitigations

1. **Risk:** Breaking evaluation symmetry
   - **Mitigation:** Comprehensive symmetry tests
   - **Detection:** Assert in debug builds
   - **Critical:** Use `sq ^ 56` for rank mirroring, NOT `sq ^ 63` or `63 - sq`

2. **Risk:** Incorrect incremental updates
   - **Mitigation:** Validate against recalculation after EVERY move
   - **Detection:** Debug assertions and runtime validation
   - **Store full PST state in UndoInfo for perfect unmake**

3. **Risk:** PST indexing errors
   - **Mitigation:** Static_assert on known positions
   - **Detection:** Unit tests for each piece type
   - **Use INVALID sentinel values for illegal pawn positions (rank 1/8)**

4. **Risk:** Performance regression
   - **Mitigation:** Benchmark before/after
   - **Detection:** Track NPS in tests
   - **Use alignas(64) for cache line optimization**

5. **Risk:** Special move PST bugs
   - **Castling:** Must update both king AND rook PST
   - **En Passant:** Captured pawn is NOT on destination square
   - **Promotion:** Remove pawn PST, add promoted piece PST
   - **Mitigation:** Explicit test cases for each special move type

6. **Risk:** Make/Unmake asymmetry
   - **Mitigation:** Store complete PST state in UndoInfo
   - **Detection:** Validate PST restoration after unmake
   - **Test with make/unmake/make sequences**

## Validation Strategy

### Success Criteria
1. ✅ All existing tests continue to pass
2. ✅ Evaluation symmetry maintained
3. ✅ SPRT shows +100-200 Elo improvement
4. ✅ NPS remains above 1M
5. ✅ Perft results unchanged
6. ✅ Incremental PST matches recalculation

### Test Plan
1. Unit test each PST component
2. Integration test with search
3. Symmetry validation on 100+ positions
4. SPRT test vs material-only baseline (20,000+ games)
5. Manual analysis of game positions
6. **Incremental Update Torture Test:** Test sequence with all special moves
7. **PST-specific perft:** Validate PST after every move in perft
8. **Differential testing:** Compare against known-good implementation
9. **FEN loading validation:** Recalculate PST from scratch after FEN

## Items Being Deferred

### To Stage 9b
1. **Quiescence Search** - Requires separate implementation
2. **Draw Detection** - Repetitions and 50-move rule
3. **Advanced MVV-LVA** - SEE (Static Exchange Evaluation)

### To Phase 3
1. **Tapered Evaluation** - Smooth middlegame/endgame transition
2. **King Safety** - Pawn shields, attack detection
3. **Pawn Structure** - Passed pawns, isolated pawns
4. **Piece Mobility** - Count legal moves
5. **Proper Zobrist Random Numbers** - Replace debug sequential values

## Success Criteria

Stage 9 is complete when:
1. PST infrastructure implemented and tested
2. Incremental PST tracking working correctly
3. Evaluation maintains symmetry property
4. SPRT test shows significant improvement
5. All validation tests pass
6. Documentation updated

## Timeline Estimate

- **Phase 1 (PST Infrastructure):** 2 hours
- **Phase 2 (Board Integration):** 2 hours
- **Phase 3 (Evaluation Integration):** 1 hour
- **Phase 4 (Testing & Validation):** 3 hours
- **Total Estimate:** 8 hours

## Implementation Checklist

### Pre-Implementation
- [x] Review Master Project Plan requirements
- [x] Check deferred items tracker
- [x] Get cpp-pro agent review
- [x] Get chess-engine-expert review
- [ ] Create feature branch

### Implementation
- [ ] Create PST header with data structures
- [ ] Add PST values (PeSTO tables)
- [ ] Integrate PST tracking in Board class
- [ ] Update make/unmake for PST
- [ ] Modify evaluation function
- [ ] Add MVV-LVA capture ordering
- [ ] Create unit tests
- [ ] Create integration tests
- [ ] Run SPRT validation

### Post-Implementation
- [ ] Update project_status.md
- [ ] Update deferred_items_tracker.md
- [ ] Create stage completion report
- [ ] Merge to main branch
- [ ] Tag release v2.9.0

## Notes

- Keep implementation simple - resist adding features
- PST alone provides measurable improvement
- Quiescence search is complex enough for separate stage
- Focus on correctness over optimization
- Maintain backwards compatibility with UCI

---

**Approval Required:** This plan requires human review before implementation begins.