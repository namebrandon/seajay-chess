# SeaJay Chess Engine - Stage 7: Negamax Search Implementation Plan

**Document Version:** 1.0  
**Date:** August 9, 2025  
**Stage:** Phase 2, Stage 7 - Negamax Search  
**Prerequisites Completed:** Yes (Stages 1-6 complete)  

## Executive Summary

Stage 7 implements the core search algorithm that transforms SeaJay from a material-counting engine into one capable of multi-ply tactical calculation. Using the negamax algorithm (an elegant variant of minimax), the engine will search 4 plies deep, enabling it to find basic tactics, avoid blunders, and discover mate-in-2 positions. This stage is expected to add 200-300 Elo strength.

## Current State Analysis

### What Exists from Previous Stages
- **Complete Legal Move Generation** (Stage 3-4): 99.974% perft accuracy
- **Material Evaluation** (Stage 6): Centipawn-based evaluation with draw detection
- **UCI Protocol** (Stage 3): Full implementation ready for search integration
- **Board Infrastructure**: Efficient makeMove/unmakeMove with UndoInfo
- **Simple 1-ply Search**: selectBestMove() provides foundation to build upon
- **Testing Framework** (Stage 5): SPRT and perft validation tools ready

### Current Search Capabilities
```cpp
// Current implementation (1-ply lookahead only)
Move selectBestMove(Board& board) {
    // Evaluates immediate captures and moves
    // No recursive search
    // No time management
    // No search statistics
}
```

## Deferred Items Being Addressed

### From Deferred Items Tracker
- No specific items deferred TO Stage 7
- Stage 6 properly deferred multi-ply search to this stage

### From Previous Stages
- Multi-ply recursive evaluation (deferred from Stage 6)
- Minimax tree traversal (deferred from Stage 6)
- Search depth beyond 1 ply (implicit deferral)

## Implementation Plan

### Phase 1: Core Negamax Algorithm (Day 1 Morning)

#### 1.1 Search Infrastructure
```cpp
namespace seajay::search {

// Search statistics tracking
struct SearchInfo {
    uint64_t nodes = 0;           // Total nodes searched
    int depth = 0;                 // Current search depth
    int seldepth = 0;              // Maximum depth reached
    Move bestMove;                 // Best move found
    eval::Score bestScore;         // Score of best move
    
    // Time management
    std::chrono::steady_clock::time_point startTime;
    std::chrono::milliseconds timeLimit{0};
    bool stopped = false;
    
    // Statistics methods
    uint64_t nps() const;
    std::chrono::milliseconds elapsed() const;
    bool checkTime();
};

// Main search function
eval::Score negamax(Board& board, int depth, int ply, SearchInfo& info);
}
```

#### 1.2 Basic Negamax Implementation
```cpp
eval::Score negamax(Board& board, int depth, int ply, SearchInfo& info) {
    // Node counting
    info.nodes++;
    
    // Time check (every 4096 nodes)
    if ((info.nodes & 0xFFF) == 0 && info.checkTime()) {
        info.stopped = true;
        return eval::Score::zero();
    }
    
    // Update selective depth
    if (ply > info.seldepth) {
        info.seldepth = ply;
    }
    
    // Terminal node - return static evaluation
    if (depth <= 0) {
        return board.evaluate();
    }
    
    // Generate legal moves
    MoveList moves = generateLegalMoves(board);
    
    // Handle checkmate and stalemate
    if (moves.empty()) {
        if (isInCheck(board, board.sideToMove())) {
            // Checkmate - return mate score
            return eval::Score::mated_in(ply);
        }
        // Stalemate
        return eval::Score::draw();
    }
    
    // Search all moves
    eval::Score bestScore = eval::Score::minus_infinity();
    Move bestMove;
    
    for (const Move& move : moves) {
        // Make move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Recursive search with negation
        eval::Score score = -negamax(board, depth - 1, ply + 1, info);
        
        // Unmake move
        board.unmakeMove(move, undo);
        
        // Check for search interruption
        if (info.stopped) {
            return bestScore;
        }
        
        // Update best score
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            
            // Store best move at root
            if (ply == 0) {
                info.bestMove = move;
                info.bestScore = score;
            }
        }
    }
    
    return bestScore;
}
```

### Phase 2: Alpha-Beta Framework (Day 1 Afternoon)

#### 2.1 Add Alpha-Beta Parameters
```cpp
eval::Score negamax(Board& board, int depth, int ply, 
                   eval::Score alpha, eval::Score beta, 
                   SearchInfo& info);
```

#### 2.2 Prepare for Future Pruning
- Add alpha-beta window parameters
- Pass infinite bounds initially (-INF, +INF)
- Structure ready for Stage 8 pruning implementation
- Add beta cutoff infrastructure (inactive)

### Phase 3: Iterative Deepening (Day 1 Evening)

#### 3.1 ID Framework
```cpp
Move search(Board& board, const SearchLimits& limits) {
    SearchInfo info;
    info.startTime = std::chrono::steady_clock::now();
    info.timeLimit = calculateTimeLimit(limits, board);
    
    Move bestMove;
    
    // Iterative deepening loop
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        info.depth = depth;
        
        eval::Score score = negamax(board, depth, 0, 
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(), 
                                   info);
        
        // Only update best move if search completed
        if (!info.stopped) {
            bestMove = info.bestMove;
            
            // Send UCI info
            sendSearchInfo(info);
            
            // Stop if mate found
            if (score.is_mate_score()) {
                break;
            }
        } else {
            break;  // Time's up
        }
    }
    
    return bestMove;
}
```

### Phase 4: Time Management (Day 2 Morning)

#### 4.1 Time Allocation
```cpp
std::chrono::milliseconds calculateTimeLimit(const SearchLimits& limits, 
                                            const Board& board) {
    // Fixed move time
    if (limits.movetime > 0) {
        return limits.movetime;
    }
    
    // Infinite analysis
    if (limits.infinite) {
        return std::chrono::milliseconds::max();
    }
    
    // Game time - use 5% of remaining
    Color stm = board.sideToMove();
    auto remaining = limits.time[stm];
    
    // Simple allocation: 5% of remaining time + 75% of increment
    auto allocated = remaining / 20 + limits.inc[stm] * 3 / 4;
    
    // Safety bounds
    return std::clamp(allocated, 
                     std::chrono::milliseconds(5),     // Minimum 5ms
                     remaining - std::chrono::milliseconds(50)); // Keep 50ms buffer
}
```

#### 4.2 Time Checking
```cpp
bool SearchInfo::checkTime() {
    auto elapsed = std::chrono::steady_clock::now() - startTime;
    return elapsed >= timeLimit;
}
```

### Phase 5: UCI Integration (Day 2 Afternoon)

#### 5.1 Search Info Output
```cpp
void sendSearchInfo(const SearchInfo& info) {
    std::cout << "info"
              << " depth " << info.depth
              << " seldepth " << info.seldepth
              << " score cp " << info.bestScore.to_cp()
              << " nodes " << info.nodes
              << " nps " << info.nps()
              << " time " << info.elapsed().count()
              << " pv " << moveToUCI(info.bestMove)
              << std::endl;
}
```

#### 5.2 Go Command Handling
- Support `go depth <x>`
- Support `go movetime <x>`
- Support `go wtime <x> btime <x> winc <x> binc <x>`
- Support `go infinite`

### Phase 6: Testing and Validation (Day 2 Evening)

#### 6.1 Mate Finding Tests
```cpp
const TestPosition MATE_IN_ONE[] = {
    {"6k1/5ppp/8/8/8/8/8/R7 w - - 0 1", "a1a8", 1},  // Back rank mate
    {"7k/8/8/8/8/8/1Q6/7K w - - 0 1", "b2b8", 1},   // Queen mate
    {"r3k3/8/8/8/8/8/8/2KR4 w - - 0 1", "d1d8", 1},  // Rook mate
};

const TestPosition MATE_IN_TWO[] = {
    {"6k1/5ppp/8/8/8/8/R7/6K1 w - - 0 1", "a2a8", 3},  // Back rank in 2
    {"7k/8/8/8/8/8/8/KQ6 w - - 0 1", "b1b7", 3},     // K+Q mate
};
```

#### 6.2 Performance Benchmarks
- Starting position to depth 4: < 60 seconds
- Tactical positions depth 4: Find best moves
- Mate positions: 100% accuracy

## Technical Considerations

### C++20 Features to Use
1. **std::chrono** improvements for time management
2. **Designated initializers** for SearchInfo
3. **std::span** for move lists (if beneficial)
4. **Concepts** for type safety (future-proofing)
5. **std::stop_token** preparation (not yet implemented)

### Performance Optimizations
1. **Node counting with bit masking** for time checks
2. **Inline critical functions** (negamax inner loop)
3. **Pass board by reference** throughout
4. **Pre-allocate move lists** where possible
5. **Cache-friendly data structures**

## Chess Engine Considerations

### Critical Chess-Specific Details
1. **Mate Score Handling**
   - Use ply-relative mate scores
   - Propagate with distance adjustment
   - MATE_SCORE = 32000, MATE_THRESHOLD = 31000

2. **Move Generation Order**
   - No ordering initially (Stage 7)
   - Natural move generation order
   - Prepare infrastructure for Stage 8 ordering

3. **Draw Detection**
   - Defer repetition detection to Stage 8
   - Defer 50-move rule to Stage 8
   - Focus on core search first

4. **Search Depth**
   - Use plies (half-moves) not full moves
   - 4 ply depth is appropriate
   - Track seldepth for future extensions

## Risk Mitigation

### Identified Risks and Mitigations

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Sign/negation bugs | High | High | Add score validation asserts |
| Make/unmake corruption | Medium | High | Hash validation after unmake |
| Infinite recursion | Low | High | Depth check at entry |
| Time overrun | Medium | Medium | Conservative time allocation |
| Mate score errors | Medium | High | Comprehensive mate tests |
| Stack overflow | Low | High | Limit max depth to 64 |

### Debugging Strategy
1. **Add extensive assertions** in debug builds
2. **Validate board state** after each unmake
3. **Log first 3 plies** of search tree for debugging
4. **Compare with reference** engine on test positions
5. **Unit test each component** before integration

## Validation Strategy

### Test Progression
1. **Fixed depth 1**: Should match current selectBestMove()
2. **Fixed depth 2**: Should avoid hanging pieces
3. **Mate-in-1**: 100% accuracy required
4. **Fixed depth 4**: Basic tactics found
5. **Mate-in-2**: 80%+ accuracy expected
6. **Time management**: Respect time limits ±10%
7. **SPRT test**: vs Stage 6 material-only

### Success Metrics
- ✅ Find all mate-in-1 positions
- ✅ Find 80%+ mate-in-2 positions
- ✅ Complete depth 4 search in <60s from startpos
- ✅ 50K-200K NPS without optimizations
- ✅ SPRT pass with +200 Elo vs material-only
- ✅ No crashes or hangs in 1000 game test

## Items Being Deferred

### To Stage 8 (Alpha-Beta Pruning)
1. Active alpha-beta cutoffs
2. Move ordering (MVV-LVA, killers)
3. Aspiration windows
4. Search tree statistics

### To Stage 9 (Positional Evaluation)
1. Quiescence search
2. Check extensions
3. Passed pawn extensions
4. Singular extensions

### To Future Phases
1. Transposition tables
2. Null move pruning
3. Late move reductions
4. Multi-threading

## Success Criteria

Stage 7 is complete when:
1. ✅ Negamax search to fixed depth works correctly
2. ✅ Iterative deepening framework operational
3. ✅ Time management respects limits
4. ✅ UCI info output during search
5. ✅ All mate-in-1 tests pass
6. ✅ 80%+ mate-in-2 tests pass
7. ✅ SPRT shows +200 Elo improvement
8. ✅ No memory leaks or crashes
9. ✅ Performance within expected range (50K-200K NPS)
10. ✅ Documentation updated

## Timeline Estimate

**Total: 2 days**

Day 1:
- Morning: Core negamax implementation (4 hours)
- Afternoon: Alpha-beta framework and ID (4 hours)
- Evening: Initial testing and debugging (2 hours)

Day 2:
- Morning: Time management (2 hours)
- Afternoon: UCI integration (3 hours)
- Evening: Comprehensive testing and validation (3 hours)

## Implementation Order

1. Start with fixed-depth negamax (no time, no ID)
2. Validate with simple positions
3. Add mate detection and test
4. Implement iterative deepening
5. Add time management
6. Integrate with UCI
7. Run comprehensive tests
8. SPRT validation

## Quality Gates

Before proceeding with implementation:
- [x] All Stage 6 tests still passing
- [x] Expert reviews incorporated (cpp-pro and chess-engine-expert)
- [x] Risk mitigation strategies defined
- [x] Test positions comprehensive
- [x] Success criteria clear
- [x] Deferred items documented
- [ ] Final review by human developer

**This plan is ready for implementation following approval.**

## Appendix: Key Code Patterns

### Canonical Negamax Structure
```cpp
eval::Score negamax(Board& board, int depth, int ply, 
                   eval::Score alpha, eval::Score beta,
                   SearchInfo& info) {
    // Time check
    if ((info.nodes & 0xFFF) == 0 && info.checkTime()) {
        info.stopped = true;
        return eval::Score::zero();
    }
    
    // Node counting
    info.nodes++;
    
    // Seldepth tracking
    if (ply > info.seldepth) {
        info.seldepth = ply;
    }
    
    // Terminal node
    if (depth <= 0) {
        return board.evaluate();
    }
    
    // Move generation
    MoveList moves = generateLegalMoves(board);
    
    // Checkmate/Stalemate detection
    if (moves.empty()) {
        if (isInCheck(board, board.sideToMove())) {
            return eval::Score::mated_in(ply);
        }
        return eval::Score::draw();
    }
    
    // Search moves
    eval::Score bestScore = eval::Score::minus_infinity();
    for (const Move& move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        eval::Score score = -negamax(board, depth - 1, ply + 1,
                                    -beta, -alpha, info);
        
        board.unmakeMove(move, undo);
        
        if (info.stopped) return bestScore;
        
        if (score > bestScore) {
            bestScore = score;
            if (ply == 0) {
                info.bestMove = move;
                info.bestScore = score;
            }
            
            // Alpha-beta framework (inactive in Stage 7)
            if (score > alpha) {
                alpha = score;
                if (score >= beta) {
                    break;  // Will be activated in Stage 8
                }
            }
        }
    }
    
    return bestScore;
}
```

---

*Document prepared following comprehensive expert review and risk analysis. Ready for Stage 7 implementation.*