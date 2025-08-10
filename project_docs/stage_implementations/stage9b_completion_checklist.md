# SeaJay Chess Engine - Stage 9b: Draw Detection and Repetition Handling Completion Checklist

**Stage:** Phase 2, Stage 9b
**Started:** August 10, 2025
**Target Completion:** August 10, 2025
**Developer:** Brandon Harris

## Overview
This checklist tracks the implementation of draw detection mechanisms, particularly threefold repetition and fifty-move rule detection, which are critical for proper game termination and SPRT testing accuracy.

## Pre-Implementation Review
- [x] Reviewed Stage 9b implementation plan
- [x] Verified all prerequisites complete (Stages 1-9)
- [x] Confirmed Zobrist hashing infrastructure ready
- [x] Verified make/unmake with UndoInfo working
- [x] Confirmed halfmove counter already tracked
- [x] Reviewed expert recommendations (cpp-pro, chess-engine-expert)
- [ ] Created feature branch for Stage 9b

## Phase 1: Position History Infrastructure (2-3 hours)

### 1.1 History Tracking
- [ ] Add position history vector to Board class
- [ ] Add m_lastIrreversiblePly tracking
- [ ] Implement pushGameHistory() method
- [ ] Add clearHistoryBeforeIrreversible() method
- [ ] Create separate SearchStack structure
- [ ] Add history management to makeMove()
- [ ] Add history management to unmakeMove()
- [ ] Implement fixed-size ring buffer for long games
- [ ] Add RAII MoveGuard pattern
- [ ] Unit tests for history tracking

### 1.2 Repetition Detection Logic
- [ ] Implement isRepetitionDraw() method
- [ ] Add separate game vs search history logic
- [ ] Implement 1-repetition rule for search
- [ ] Implement 2-repetition rule for game history
- [ ] Optimize to only check within halfmove clock
- [ ] Unit tests for repetition detection

### 1.3 Fifty-Move Rule
- [ ] Implement isFiftyMoveRule() method
- [ ] Verify halfmove counter reset on captures
- [ ] Verify halfmove counter reset on pawn moves
- [ ] Add isDraw() convenience method
- [ ] Unit tests for fifty-move rule

## Phase 2: Search Integration (2-3 hours)

### 2.1 Draw Detection in Search
- [ ] Add draw checks to negamax() BEFORE evaluation
- [ ] Ensure checkmate checked BEFORE repetition
- [ ] Return score 0 for all draw types
- [ ] Add search path tracking to SearchInfo
- [ ] Implement isRepetitionInSearch() method
- [ ] Unit tests for search draw detection

### 2.2 Root Move Filtering
- [ ] Implement filterRootMoves() function
- [ ] Filter moves that cause immediate repetition
- [ ] Integrate with iterative deepening
- [ ] Unit tests for root move filtering

### 2.3 Performance Optimization
- [ ] Profile repetition detection overhead
- [ ] Ensure <5% NPS impact
- [ ] Optimize hot paths if needed

## Phase 3: UCI Protocol Integration (1-2 hours)

### 3.1 Game State Management
- [ ] Add game history tracking to UCI handler
- [ ] Update position command to track history
- [ ] Clear history on "ucinewgame"
- [ ] Clear history on new position

### 3.2 Draw Reporting
- [ ] Add info string output for draws
- [ ] Report draw type (repetition/fifty-move/material)
- [ ] Handle game termination properly
- [ ] Test with GUI (Arena/CuteChess)

## Phase 4: Testing and Validation (2-3 hours)

### 4.1 Unit Tests
- [ ] Test basic threefold repetition
- [ ] Test castling rights false repetition
- [ ] Test en passant phantom repetition
- [ ] Test 50-move rule edge cases
- [ ] Test root position repetition
- [ ] Test search vs game history
- [ ] Test checkmate vs repetition priority
- [ ] Test halfmove boundary conditions

### 4.2 Test Positions (Stockfish Validated)
- [ ] Position 1: Basic threefold (Nc3-Nb1 repetition)
- [ ] Position 2: Castling rights change (NOT repetition)
- [ ] Position 3: En passant ghost (NOT repetition)
- [ ] Position 4: 50-move rule at move 100
- [ ] Position 5: Root position already repeated
- [ ] Position 6: Search vs game history difference
- [ ] Position 7: Mate vs repetition (find mate)
- [ ] Position 8: Halfmove boundary test

### 4.3 Integration Tests
- [ ] Run perft tests (ensure no regression)
- [ ] Play test games vs random mover
- [ ] Verify draws detected properly
- [ ] Check no infinite loops in endgames

### 4.4 Performance Validation
- [ ] Benchmark before implementation
- [ ] Benchmark after implementation
- [ ] Verify <5% NPS impact
- [ ] Profile with valgrind if >5% impact

## Phase 5: SPRT Validation

### 5.1 SPRT Test Setup
- [ ] Create SPRT test: Stage 9b vs Stage 9
- [ ] Configure for draw rate measurement
- [ ] Target: <30% draw rate (from 40-50%)
- [ ] Run with standard time control (10+0.1)

### 5.2 SPRT Results
- [ ] Test ID: SPRT-2025-___
- [ ] Games played: ___
- [ ] Draw rate achieved: ___% 
- [ ] LLR: ___
- [ ] Result: PASS/FAIL
- [ ] Notes: ___

## Code Quality Checklist

### C++ Best Practices
- [ ] RAII patterns used for state management
- [ ] No dynamic allocation in hot paths
- [ ] Fixed-size buffers for history
- [ ] Const-correctness maintained
- [ ] No memory leaks (valgrind clean)
- [ ] Proper error handling

### Chess-Specific Requirements
- [ ] Zobrist does NOT include halfmove clock
- [ ] En passant square in Zobrist when relevant
- [ ] Castling rights properly tracked
- [ ] Side to move matches for repetition
- [ ] Checkmate priority over repetition

## Documentation

- [ ] Updated project_status.md
- [ ] Created development diary entry
- [ ] Updated inline code documentation
- [ ] Committed with proper attribution
- [ ] Archived this checklist

## Sign-off

### Developer Confirmation
- [ ] All tests passing
- [ ] No known bugs
- [ ] Performance acceptable
- [ ] Code review complete
- [ ] Ready for Stage 10

**Developer Signature:** _________________
**Date Completed:** _________________
**Total Time Spent:** _________________

## Notes
<!-- Add any additional notes, observations, or issues encountered during implementation -->

## Deferred Items
<!-- List any items deferred to future stages -->
- Threefold repetition hash table (Stage 10+)
- 75-move rule (rarely needed)
- Perpetual check detection (complex)

---
*This checklist is part of the SeaJay Chess Engine documentation and should be archived upon stage completion.*