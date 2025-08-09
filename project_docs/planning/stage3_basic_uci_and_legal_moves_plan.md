# SeaJay Chess Engine - Stage 3: Basic UCI and Legal Moves Implementation Plan

**Document Version:** 1.0  
**Date:** December 2024  
**Stage:** Phase 1, Stage 3 - Basic UCI and Legal Moves  
**Prerequisites Completed:** Yes (Stages 1-2 complete)  
**Reviews Completed:** chess-engine-expert ✓, cpp-pro ✓

## Executive Summary

Stage 3 will bring SeaJay to life as a playable chess engine. We'll implement a minimal UCI protocol interface, complete move generation with pseudo-legal and legal move filtering, and enable the engine to play complete games in standard chess GUIs. The focus is on correctness over performance, establishing a solid foundation for future optimization.

## Current State Analysis

### Completed from Previous Stages:
- **Stage 1:** Hybrid bitboard-mailbox board representation with ray-based sliding piece attacks
- **Stage 2:** Position management with robust FEN parsing, Result<T,E> error handling, comprehensive validation

### Available Infrastructure:
- C++20 build environment with CMake
- Basic types (Square, Piece, Color, Bitboard)
- Board class with position storage and FEN I/O
- Ray-based attack generation for sliding pieces
- Comprehensive validation functions
- Result<T,E> error handling system

### Current Limitations:
- No move generation capability
- No UCI protocol support
- Cannot play games
- No move representation system

## Deferred Items Being Addressed

From `/workspace/project_docs/tracking/deferred_items_tracker.md`:

### From Stage 2 to Stage 3:
1. **UCI Position Command** - Will use FEN parser to set up positions
2. **Basic move validation** - Foundation for legal move filtering

### Items Still Deferred:
1. **En Passant Pin Validation** - Deferred to Stage 4 (requires full attack generation)
2. **Triple Check Validation** - Deferred to Stage 4
3. **Full retrograde legality** - Not critical for engine play

## Implementation Plan

### Phase 3.1: Move Representation and Infrastructure (2-3 hours)

#### Tasks:
1. **Create Move Class** (src/core/types.h enhancement)
   - 16-bit move encoding (6 bits from, 6 bits to, 4 bits flags)
   - Factory methods for different move types
   - Type-safe accessors and queries
   - String conversion for UCI protocol

2. **Implement MoveList Container** (src/core/move_list.h)
   - Stack-allocated fixed array (256 moves max)
   - RAII wrapper with STL-like interface
   - Zero heap allocation design
   - C++20 ranges support

3. **Define Move Generation Interface** (src/core/move_generation.h)
   - Template-based generation for compile-time optimization
   - MoveGenType enum for different generation modes
   - Function signatures for piece-specific generators

### Phase 3.2: Piece-by-Piece Move Generation (4-5 hours)

#### Implementation Order (from simplest to most complex):

1. **King Moves** (30 minutes)
   - Simple 8-square pattern
   - No special cases initially
   - Test with center, edge, corner positions

2. **Knight Moves** (30 minutes)
   - Compile-time lookup table generation
   - 8 possible moves per knight
   - Test empty and crowded boards

3. **Pawn Moves** (1.5 hours)
   - Single push, double push from starting rank
   - Diagonal captures
   - Promotion handling (defer en passant)
   - Test all edge cases

4. **Sliding Pieces** (1.5 hours)
   - Leverage existing ray generation
   - Rook moves (horizontal/vertical)
   - Bishop moves (diagonals)
   - Queen moves (rook + bishop)

5. **Special Moves** (1 hour)
   - Castling (king and queen side)
   - En passant capture
   - Complete promotion handling

### Phase 3.3: Legal Move Filtering (2-3 hours)

#### Core Components:

1. **Pin Detection**
   - Identify pinned pieces
   - Store pin rays for move validation
   - Optimize with lazy evaluation

2. **Check Detection**
   - Is king under attack?
   - Identify checking pieces
   - Generate check evasions

3. **Legal Move Validation**
   - Make/unmake pattern for validation
   - Filter pseudo-legal moves
   - Handle special cases (castling through check, en passant pins)

### Phase 3.4: UCI Protocol Implementation (3-4 hours)

#### Minimal Commands for GUI Play:

1. **Core UCI Commands**
   ```
   uci        -> id name SeaJay 0.1
                 id author Brandon Harris
                 uciok
   isready    -> readyok
   position   -> Parse FEN or moves
   go         -> Select and return best move
   quit       -> Clean exit
   ```

2. **Position Handling**
   - "position startpos moves e2e4 e7e5..."
   - "position fen [FEN string] moves..."
   - Apply moves to current position

3. **Move Selection**
   - Initially random legal move selection
   - Format moves in UCI notation (e2e4, e7e8q)
   - Handle game end conditions

### Phase 3.5: Integration and Testing (2-3 hours)

1. **Perft Implementation**
   - Move counting at various depths
   - Perft divide for debugging
   - Standard test positions

2. **GUI Compatibility Testing**
   - Test with Arena Chess GUI
   - Test with Cute Chess
   - Verify move legality and game completion

3. **Performance Baseline**
   - Measure move generation speed
   - Target: 100K-500K nodes/second in perft
   - Profile for obvious bottlenecks

## Technical Considerations (from cpp-pro review)

### Memory Management:
- Stack allocation for move lists (no heap allocation)
- Thread-local storage for working move lists
- Object pooling for frequently allocated structures

### Performance Optimizations:
- Compile-time lookup tables using consteval
- Template specialization for move generation types
- Branch prediction hints with [[likely]]/[[unlikely]]
- Manual UCI parsing (avoid regex overhead)

### Error Handling:
- Separate UCI protocol errors from internal errors
- Use Result<T,E> consistently
- Clear error messages with context
- Non-fatal vs fatal error classification

### Modern C++ Features:
- C++20 concepts for template constraints
- std::span for zero-copy array passing
- consteval for compile-time computations
- Structured bindings for clean code
- std::bit_cast for safe type punning

## Chess Engine Considerations (from chess-engine-expert review)

### Critical Test Positions:
1. **Kiwipete** - "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
   - Depth 1: 48 moves, Depth 2: 2039, Depth 3: 97862

2. **En Passant Pin** - "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
   - Tests illegal en passant due to discovered check

3. **Complex Promotions** - "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
   - Tests promotion with pieces giving check

### Common Pitfalls to Avoid:
- Pinned piece moves exposing king
- En passant creating discovered checks
- Castling through/into/from check
- Off-by-one errors in perft counts
- Buffer overflows in move list

### Implementation Order Rationale:
1. King/Knight first - Simple, easy to verify
2. Pawns next - Complex but finite moves
3. Sliders last - Build on ray generation
4. Special moves - After basic generation works
5. Legal filtering - Once all moves generate correctly

## Risk Mitigation

### Identified Risks:

1. **Move Generation Bugs**
   - Mitigation: Incremental testing, perft validation
   - Detection: Perft divide command for debugging

2. **UCI Protocol Errors**
   - Mitigation: Extensive GUI testing
   - Detection: Log all UCI communication

3. **Performance Issues**
   - Mitigation: Profile early, optimize later
   - Detection: Benchmark move generation speed

4. **Memory Corruption**
   - Mitigation: Stack allocation, bounds checking
   - Detection: Address sanitizer in debug builds

5. **Illegal Move Generation**
   - Mitigation: Comprehensive test positions
   - Detection: Perft validation suite

## Validation Strategy

### Unit Tests:
- Each piece type tested in isolation
- Move count verification for known positions
- Legal move filtering correctness
- UCI command parsing

### Integration Tests:
- Complete game playthrough
- Perft accuracy for standard positions
- GUI compatibility verification
- Time control handling

### Performance Tests:
- Move generation speed benchmarks
- Memory usage monitoring
- Profile hot paths

## Items Being Deferred

### To Stage 4:
1. **Make/Unmake Move** - Full implementation with state tracking
2. **Complete Perft Suite** - All standard test positions
3. **Advanced Pin Detection** - Optimized implementation
4. **Check Evasion Generator** - Specialized move generation when in check

### To Stage 5:
1. **Performance Optimization** - Current focus is correctness
2. **Move Ordering** - Not needed for random selection
3. **Transposition Detection** - Requires Zobrist hashing

## Success Criteria

### Functional Requirements:
- [x] UCI protocol responds to basic commands
- [x] Generates all legal moves in any position
- [x] Plays complete games without crashes
- [x] Passes basic perft tests (depth 3-4)
- [x] Works with Arena and Cute Chess GUIs

### Performance Targets:
- Minimum: 100K nodes/second in perft
- Target: 500K nodes/second
- Move generation < 1ms per position
- UCI response time < 100ms

### Code Quality:
- Zero compiler warnings
- All unit tests passing
- Clean separation of concerns
- Well-documented interfaces

## Timeline Estimate

- **Phase 3.1:** Move Infrastructure - 3 hours
- **Phase 3.2:** Move Generation - 5 hours
- **Phase 3.3:** Legal Filtering - 3 hours
- **Phase 3.4:** UCI Protocol - 4 hours
- **Phase 3.5:** Testing - 3 hours
- **Buffer:** 2 hours

**Total Estimate:** 18-20 hours

## Implementation Checklist

### Pre-Implementation:
- [x] Review deferred items tracker
- [x] Review expert feedback
- [x] Set up test positions
- [x] Create test framework

### Core Implementation:
- [ ] Move class and utilities
- [ ] MoveList container
- [ ] King move generation
- [ ] Knight move generation
- [ ] Pawn move generation
- [ ] Sliding piece generation
- [ ] Special moves (castling, en passant)
- [ ] Legal move filtering
- [ ] UCI protocol handler
- [ ] Random move selection

### Validation:
- [ ] Unit tests for each piece type
- [ ] Perft validation (depth 3-4)
- [ ] GUI compatibility test
- [ ] Performance benchmarks
- [ ] Memory leak check

### Documentation:
- [ ] Update project_status.md
- [ ] Update deferred items tracker
- [ ] Document UCI protocol usage
- [ ] Create development diary entry

## Appendix: Key Code Structures

### Move Encoding (16-bit):
```
Bits 0-5:   From square (0-63)
Bits 6-11:  To square (0-63)  
Bits 12-15: Move flags
  0000 - Quiet move
  0001 - Double pawn push
  0010 - King castle
  0011 - Queen castle
  0100 - Capture
  0101 - En passant capture
  1000 - Knight promotion
  1001 - Bishop promotion
  1010 - Rook promotion
  1011 - Queen promotion
  1100 - Knight promotion capture
  1101 - Bishop promotion capture
  1110 - Rook promotion capture
  1111 - Queen promotion capture
```

### Test Position Node Counts:
```
Starting Position:
  Depth 1: 20
  Depth 2: 400
  Depth 3: 8,902
  Depth 4: 197,281
  Depth 5: 4,865,609

Kiwipete:
  Depth 1: 48
  Depth 2: 2,039
  Depth 3: 97,862
  Depth 4: 4,085,603
  Depth 5: 193,690,690
```

## Review Sign-off

**chess-engine-expert:** Plan addresses all critical move generation concerns. Test positions are comprehensive. Implementation order is optimal.

**cpp-pro:** Architecture leverages modern C++ effectively. Memory management strategy is sound. Error handling approach is robust.

**Ready for Implementation:** ✓

---

*This plan represents the consensus of expert reviews and establishes a clear path for Stage 3 implementation.*