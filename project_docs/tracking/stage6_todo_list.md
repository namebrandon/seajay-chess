# Stage 6: Material Evaluation - Implementation TODO List

**Created:** August 9, 2025  
**Status:** Ready to Begin Implementation  

## Pre-Implementation Checklist ✅
- [x] Pre-Stage Planning Process completed
- [x] Current state analysis done
- [x] Deferred items reviewed
- [x] Implementation plan created
- [x] cpp-pro technical review completed
- [x] chess-engine-expert domain review completed
- [x] Risk analysis completed
- [x] Final plan documented
- [x] Directory structure created

## Implementation Tasks

### Phase 1: Core Data Structures (3 hours)
- [ ] Create src/evaluation/types.h with Score type
- [ ] Implement saturating arithmetic for Score
- [ ] Create src/evaluation/material.h with Material class
- [ ] Implement piece counting and value tracking
- [ ] Add insufficient material detection
- [ ] Add same-colored bishop detection
- [ ] Create unit tests for Score type
- [ ] Create unit tests for Material class

### Phase 2: Evaluation Function (3 hours)
- [ ] Create src/evaluation/evaluate.h interface
- [ ] Implement src/evaluation/evaluate.cpp
- [ ] Add draw detection logic
- [ ] Implement material balance calculation
- [ ] Add perspective handling (side-to-move)
- [ ] Create debug verification functions
- [ ] Write evaluation unit tests

### Phase 3: Board Integration (4 hours)
- [ ] Add Material member to Board class
- [ ] Update setPiece() for material tracking
- [ ] Update removePiece() for material tracking
- [ ] Update movePiece() for material tracking
- [ ] Handle castling (no material change)
- [ ] Handle en passant capture correctly
- [ ] Handle pawn promotion material updates
- [ ] Add material reset in FEN parsing
- [ ] Implement evaluation cache
- [ ] Create integration tests

### Phase 4: Move Selection (3 hours)
- [ ] Create src/search/search.h interface
- [ ] Implement src/search/search.cpp
- [ ] Add selectBestMove() function
- [ ] Implement single-ply evaluation
- [ ] Add capture prioritization
- [ ] Integrate with UCI engine
- [ ] Replace random move selection
- [ ] Add time management (5% of remaining)
- [ ] Create move selection tests

### Phase 5: Testing Infrastructure (4 hours)
- [ ] Create material counting test suite
- [ ] Add symmetry validation tests
- [ ] Create special move tests
- [ ] Add incremental update verification
- [ ] Create overflow tests
- [ ] Add draw detection tests
- [ ] Write integration tests
- [ ] Create SPRT test script

### Phase 6: SPRT Validation (2 hours)
- [ ] Build Stage 6 version
- [ ] Keep random mover baseline
- [ ] Configure SPRT parameters
- [ ] Run initial 1000 game test
- [ ] Verify >95% win rate
- [ ] Document results
- [ ] Run extended 10,000 game test

### Phase 7: Documentation and Cleanup (1 hour)
- [ ] Add code documentation
- [ ] Update project_status.md
- [ ] Update deferred_items_tracker.md
- [ ] Create development diary entry
- [ ] Clean up debug code
- [ ] Optimize hot paths
- [ ] Create git commit

## Critical Reminders

### ⚠️ Most Common Bugs to Avoid:
1. **Sign errors**: Always return eval from side-to-move perspective
2. **Special moves**: Don't double-update material in castling
3. **En passant**: Captured pawn is NOT on destination square
4. **Promotion**: Remove pawn AND add promoted piece
5. **FEN setup**: MUST reset material counts to zero first

### 🔍 Debug Checks:
- After every move: `assert(incremental == from_scratch)`
- For every position: `assert(eval(pos) == -eval(flipped))`
- Material unchanged after castling
- Overflow protection working

### 📊 Success Metrics:
- Material counting 100% accurate
- SPRT shows >200 Elo gain
- Captures all hanging pieces
- No time losses in 1000 games
- <5% performance impact

## Notes
- Start with simplest implementation
- Test continuously during development
- Use DEBUG mode assertions liberally
- Keep Phase 1 functionality intact
- Document any deferred items