# SeaJay Chess Engine - Stage 14: Quiescence Search Implementation Plan

**Document Version:** 1.0  
**Date:** August 14, 2025  
**Stage:** Phase 3, Stage 14 - Quiescence Search  
**Prerequisites Completed:** Yes - Stage 13 Complete (Iterative Deepening with Aspiration Windows)  
**Author:** Brandon Harris  
**Co-authored by:** Claude Code  
**Input Sources:**
- `/workspace/project_docs/planning/stage_14_qsearch_analysis.md` - Comprehensive implementation analysis
- `/workspace/project_docs/planning/stage_14_implementation_safety.md` - C++ safety guidance

## Executive Summary

Stage 14 implements quiescence search to eliminate the horizon effect, the most critical tactical enhancement for SeaJay at this point. Without quiescence search, SeaJay makes catastrophic tactical errors by stopping evaluation in the middle of capture sequences. This implementation will boost SeaJay from ~1950 Elo to 2100+ Elo by ensuring tactical stability.

**Key Goals:**
- Implement capture-only quiescence search at leaf nodes
- Handle check evasions in quiescence for tactical completeness
- Maintain search performance while adding 70-90% more nodes
- Integrate seamlessly with existing search infrastructure
- Provide UCI controls for testing and debugging

**Implementation Approach:** Incremental development with three distinct phases, starting with minimal viable quiescence and progressively adding optimizations.

## Key Insights from Previous Analysis

### From Comprehensive Analysis (stage_14_qsearch_analysis.md):
- **Performance Impact:** Quiescence typically represents 70-90% of total nodes searched
- **Strength Gain:** Expected 150-250 Elo improvement with proper implementation
- **Critical Features:** Stand-pat evaluation, capture-only search, check handling
- **Stockfish Pattern:** Modern engines use sophisticated delta pruning and TT integration
- **Risk Areas:** Search explosion without proper pruning, stack overflow in deep tactics

### From Safety Analysis (stage_14_implementation_safety.md):
- **Stack Limits:** Conservative QSEARCH_MAX_PLY = 32 prevents overflow
- **Memory Safety:** Stack-allocated move lists and bounds checking
- **C++ Features:** SafePlyCounter wrapper, constexpr limits, modern error handling
- **Testing Strategy:** Incremental phases with comprehensive tactical validation
- **Debug Infrastructure:** Specialized logging and statistics for quiescence analysis

## Current State Analysis

### Completed Infrastructure (Stage 13)
- ✅ Robust negamax search with alpha-beta pruning
- ✅ Iterative deepening with aspiration windows
- ✅ Transposition table integration with proper bounds
- ✅ Comprehensive move ordering (TT move → MVV-LVA captures → quiet moves)
- ✅ Time management with stability-based adjustments
- ✅ Complete move generation including capture-only generation
- ✅ Legal move validation and check detection
- ✅ Statistical tracking for search efficiency

### Current Search Flow Entry Point
The existing `negamax()` function in `/workspace/src/search/negamax.cpp` line 188 currently returns static evaluation when `depth <= 0`. This is exactly where quiescence search will be integrated:

```cpp
// Terminal node - return static evaluation
if (depth <= 0) {
    return board.evaluate();
}
```

This will become:
```cpp
// Terminal node - enter quiescence search
if (depth <= 0) {
    return quiescence(board, ply, alpha, beta, searchInfo, info, tt);
}
```

### Available Infrastructure for Quiescence
- ✅ `MoveGenerator::generateCaptures()` - Capture-only move generation
- ✅ `inCheck()` function for check detection
- ✅ Move ordering system ready for capture prioritization
- ✅ Transposition table with proper depth handling
- ✅ SearchData statistics tracking extensible for qsearch metrics
- ✅ Alpha-beta framework compatible with quiescence structure

## Deferred Items Being Addressed

### From Stage 9 Planning (Originally Deferred):
1. **Quiescence Search Implementation**
   - Handle horizon effect through capture sequences
   - Capture-only search at leaf nodes
   - Status: Core requirement for Stage 14

2. **Check Extensions in Quiescence**
   - Extend search when in check during quiescence
   - Generate all legal moves (not just captures) when in check
   - Status: Essential for tactical completeness

### From Stage 11 MVV-LVA (Partial Overlap):
1. **Enhanced Capture Ordering**
   - MVV-LVA ordering already implemented and working
   - Will be leveraged directly in quiescence search
   - Status: Infrastructure ready

### Current Implementation Gaps Identified:
1. No specialized quiescence statistics tracking
2. No depth limits for quiescence recursion
3. No delta pruning for futile captures
4. No UCI controls for quiescence debugging

## Implementation Plan

### Phase 1: Minimal Viable Quiescence (Days 1-3)
**Goal:** Basic working quiescence search with essential safety features

#### Phase 1.1: Core Infrastructure (Day 1)
1. **Create quiescence module files:**
   - `/workspace/src/search/quiescence.h` - Public interface
   - `/workspace/src/search/quiescence.cpp` - Core implementation
   - `/workspace/src/search/quiescence_stats.h` - Statistics tracking

2. **Define safety constants and limits:**
   ```cpp
   // Prevent stack overflow
   constexpr int QSEARCH_MAX_PLY = 32;
   constexpr int QSEARCH_MAX_DEPTH = -16;  // Negative depth in quiescence
   
   // Performance safety
   constexpr uint64_t QSEARCH_NODE_LIMIT = 100000;  // Per-position limit
   ```

3. **Extend SearchData with quiescence metrics:**
   ```cpp
   // Add to SearchData in types.h
   uint64_t qsearchNodes = 0;
   uint64_t qsearchCutoffs = 0;
   uint64_t qsearchStandPats = 0;
   double qsearchRatio() const;
   ```

#### Phase 1.2: Minimal Quiescence Function (Day 1-2)
1. **Implement basic quiescence signature:**
   ```cpp
   eval::Score quiescence(Board& board, 
                         int ply,
                         eval::Score alpha,
                         eval::Score beta,
                         SearchInfo& searchInfo,
                         SearchData& info,
                         TranspositionTable* tt);
   ```

2. **Core algorithm structure:**
   - Stand-pat evaluation (use current position value)
   - Beta cutoff on stand-pat (position already good enough)
   - Generate captures only (unless in check)
   - Search captures with alpha-beta
   - Depth and ply limits for safety

3. **Integration point modification:**
   - Modify `negamax.cpp` line 188 to call quiescence
   - Add compile-time flag `ENABLE_QUIESCENCE` for A/B testing

#### Phase 1.3: Check Handling (Day 2-3)
1. **Implement quiescenceInCheck() function:**
   - Generate ALL legal moves when in check
   - No stand-pat when in check (must move)
   - Return checkmate score if no legal moves

2. **Safety and bounds checking:**
   - Maximum ply depth enforcement
   - Node count limits
   - Time checking integration

3. **Basic testing:**
   - Simple capture sequence positions
   - Check evasion positions
   - Depth limit verification

### Phase 2: Performance and Integration (Days 4-6)
**Goal:** Optimize performance and integrate with existing systems

#### Phase 2.1: Transposition Table Integration (Day 4)
1. **Quiescence TT storage:**
   - Use depth 0 for qsearch entries
   - Proper bound types (EXACT, UPPER, LOWER)
   - Avoid storing stand-pat as EXACT bounds

2. **TT probe in quiescence:**
   - Check for existing evaluations
   - Use TT moves for capture ordering
   - Handle mate score adjustments

#### Phase 2.2: Move Ordering Optimization (Day 5)
1. **Capture ordering in quiescence:**
   - Reuse MVV-LVA ordering system
   - TT move prioritization
   - Queen promotion prioritization

2. **Delta pruning preparation:**
   - Calculate futility margins
   - Skip obviously bad captures
   - Material balance considerations

#### Phase 2.3: Performance Tuning (Day 6)
1. **Profile quiescence performance:**
   - Measure node increase percentage
   - Benchmark tactical positions
   - Optimize hot paths

2. **Memory and cache optimization:**
   - Minimize stack usage
   - Efficient move list handling
   - Reduce function call overhead

### Phase 3: Advanced Features and Validation (Days 7-10)
**Goal:** Add advanced features and comprehensive testing

#### Phase 3.1: Delta Pruning (Day 7)
1. **Implement delta pruning:**
   - Large material deficit pruning
   - Position-specific margins
   - Endgame considerations

2. **Futility pruning in quiescence:**
   - Skip moves that can't improve alpha
   - Dynamic margins based on material

#### Phase 3.2: Advanced Check Handling (Day 8)
1. **Check extension limits:**
   - Maximum check depth in quiescence
   - Escape route prioritization
   - Discovered check handling

2. **Check generation optimization:**
   - Generate checks in quiescence (optional)
   - Priority ordering for check moves

#### Phase 3.3: Comprehensive Testing (Days 9-10)
1. **Tactical test positions:**
   - WAC (Win At Chess) test suite
   - Bratko-Kopec tactical positions
   - Engine-specific tactical scenarios

2. **Performance validation:**
   - SPRT testing vs. Stage 13 baseline
   - Node count analysis
   - Time management integration testing

3. **Edge case testing:**
   - Deep capture sequences
   - Underpromotion captures
   - Zugzwang positions

## Technical Considerations

### C++ Implementation Safety (cpp-pro agent input required)

**Review Request for cpp-pro:**
Please review the following aspects of the quiescence search implementation:

1. **Stack Safety Assessment:**
   - Are the proposed depth limits (QSEARCH_MAX_PLY = 32) sufficient?
   - Should we use explicit stack guards or rely on ply counting?
   - Any C++20 features that could improve stack safety?

2. **Performance Optimization Opportunities:**
   - Best practices for hot path optimization in recursive search
   - Template vs function pointer approaches for move generation
   - Memory layout optimizations for cache efficiency

3. **Error Handling Strategy:**
   - Exception safety in search algorithms
   - Graceful degradation patterns
   - Debug vs release mode considerations

4. **Modern C++ Features:**
   - C++20/23 features that would benefit quiescence implementation
   - Concepts for search function constraints
   - Coroutines applicability for search

### Current Implementation Safety Plan:
1. **Stack Safety:**
   - Maximum recursion depth enforcement
   - Stack usage monitoring
   - Tail call optimization where possible

2. **Performance Optimization:**
   - `[[likely]]` and `[[unlikely]]` attributes on common paths
   - Template specialization for hot functions
   - Minimize memory allocations in search

3. **Error Handling:**
   - Graceful degradation on depth limits
   - Time limit integration
   - Debug vs release behavior

4. **Modern C++20 Features:**
   - `constexpr` for compile-time constants
   - `std::span` for safe array access
   - Concepts for template constraints

### Memory Management:
- Use stack-allocated MoveList for captures
- Minimize heap allocations in search
- Efficient move copying and storage

### Debugging Infrastructure:
- Comprehensive logging system for quiescence
- Statistics collection and reporting
- UCI debug commands

## Chess Engine Considerations (chess-engine-expert agent input required)

**Review Request for chess-engine-expert:**
Please provide expert guidance on the following chess engine aspects:

1. **Tactical Completeness Assessment:**
   - Are there any critical tactical patterns that basic capture-only quiescence might miss?
   - Should we include check generation in quiescence search from the start?
   - What are the most common horizon effect scenarios we must handle?

2. **Common Implementation Pitfalls:**
   - What mistakes do chess engines typically make in quiescence search?
   - Are there any chess rules edge cases specific to quiescence?
   - How do strong engines handle zugzwang in quiescence?

3. **Performance vs Accuracy Trade-offs:**
   - What is the optimal balance between search depth and breadth in quiescence?
   - Should delta pruning be aggressive or conservative initially?
   - How important is move ordering within captures?

4. **Test Position Recommendations:**
   - Critical tactical positions for quiescence validation
   - Edge cases that commonly break quiescence implementations
   - Performance benchmark positions for node count analysis

5. **Integration Concerns:**
   - How should quiescence interact with existing time management?
   - Are there any concerns with TT pollution from quiescence nodes?
   - Should quiescence statistics be tracked separately?

### Current Chess Engine Plan:

### Tactical Accuracy:
1. **Horizon Effect Elimination:**
   - Complete capture sequences
   - Promotion threat handling
   - Pin and skewer resolution

2. **Check Handling Completeness:**
   - All legal moves in check
   - Checkmate detection
   - Stalemate avoidance

3. **Common Pitfalls:**
   - Search explosion prevention
   - Zugzwang handling
   - Repetition in quiescence

### Performance Characteristics:
1. **Expected Node Distribution:**
   - 70-90% quiescence nodes typical
   - 2-5x total node increase
   - <10% time increase with good pruning

2. **Branching Factor Management:**
   - Average 5-8 captures per position
   - Delta pruning reduces by 30-50%
   - Check positions higher branching

### Integration with Existing Features:
1. **Move Ordering Synergy:**
   - MVV-LVA directly applicable
   - TT move integration
   - Promotion prioritization

2. **Time Management:**
   - Quiescence time accounting
   - Stability impact on extensions
   - Early termination criteria

## Risk Mitigation

### High-Risk Areas:
1. **Stack Overflow Risk:**
   - **Mitigation:** Hard ply limits with assertions
   - **Testing:** Deep tactical positions
   - **Monitoring:** Stack usage instrumentation

2. **Search Explosion:**
   - **Mitigation:** Node limits and delta pruning
   - **Testing:** Positions with many captures
   - **Monitoring:** Performance benchmarks

3. **Tactical Blind Spots:**
   - **Mitigation:** Comprehensive test suite
   - **Testing:** Engine vs engine matches
   - **Validation:** Known tactical positions

4. **Performance Regression:**
   - **Mitigation:** SPRT testing framework
   - **Testing:** Time control validation
   - **Monitoring:** Node count tracking

### Integration Risks:
1. **Time Management Disruption:**
   - **Mitigation:** Separate quiescence time tracking
   - **Testing:** Various time controls
   - **Fallback:** Quiescence disable option

2. **TT Pollution:**
   - **Mitigation:** Careful bound type usage
   - **Testing:** TT hit rate monitoring
   - **Validation:** Search consistency checks

3. **Move Ordering Interference:**
   - **Mitigation:** Preserve existing MVV-LVA system
   - **Testing:** Move ordering efficiency metrics
   - **Validation:** Beta cutoff analysis

## Validation Strategy

### Unit Testing:
1. **Core Function Tests:**
   - Basic capture sequences
   - Check evasion scenarios
   - Depth limit enforcement
   - Stand-pat functionality

2. **Integration Tests:**
   - Full search with quiescence
   - TT interaction verification
   - Time management integration
   - Statistics accuracy

### Tactical Testing:
1. **Tactical Test Suites:**
   - WAC positions (300 positions)
   - Bratko-Kopec test (24 positions)
   - Custom tactical scenarios

2. **Performance Benchmarks:**
   - Node count analysis
   - Time usage profiling
   - Memory usage monitoring

3. **Engine vs Engine Testing:**
   - SPRT vs Stage 13 baseline
   - Fixed-time matches
   - Tactical position solving

### Validation Criteria:
1. **Correctness:** 100% pass rate on tactical test suites
2. **Performance:** <20% time increase with 150+ Elo gain
3. **Stability:** No crashes or infinite loops in 10,000+ positions
4. **Integration:** All existing tests continue to pass

### Specific Test Positions (from analysis documents):

#### Tactical Validation Positions:
1. **Basic Capture Sequences:**
   ```
   FEN: "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 4 4"
   Description: "Knight fork - should be found immediately in qsearch"
   Expected: Immediate tactical resolution without horizon effect
   ```

2. **Deep Capture Chains:**
   ```
   FEN: "rnbqkbnr/ppp2ppp/3p4/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3"
   Description: "Multiple capture sequences requiring deep quiescence"
   Expected: Complete tactical evaluation to quiet position
   ```

3. **Check Evasion in Quiescence:**
   ```
   FEN: "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3"
   Description: "King in check during quiescence search"
   Expected: All legal moves considered, not just captures
   ```

#### Performance Benchmark Positions:
1. **High Capture Density:**
   ```
   FEN: "r1b1kb1r/1pp2ppp/p1n2n2/3pp3/8/2NP1NP1/PPP1PP1P/R1BQKB1R w KQkq - 0 6"
   Description: "Many possible captures for branching factor testing"
   Expected: Efficient pruning without losing tactical accuracy
   ```

2. **Quiet Position Control:**
   ```
   FEN: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
   Description: "Starting position with no immediate tactics"
   Expected: Minimal quiescence nodes, fast stand-pat
   ```

## Items Being Deferred

### To Stage 14b (SEE - Static Exchange Evaluation):
1. **Advanced Capture Evaluation:**
   - Static exchange evaluation for capture ordering
   - Bad capture pruning based on material loss
   - X-ray attack consideration in exchanges
   - **Reason:** MVV-LVA sufficient for initial quiescence
   - **Impact:** Better capture ordering, fewer bad captures searched

### To Stage 15 (Search Extensions):
1. **Advanced Extensions:**
   - Recapture extensions
   - Passed pawn extensions in quiescence
   - One-reply extensions
   - **Reason:** Basic quiescence must be stable first
   - **Impact:** Deeper tactical analysis in specific scenarios

### To Future Phases:
1. **Multi-Cut in Quiescence:**
   - Multiple beta cutoffs at same node
   - Advanced pruning techniques
   - **Reason:** Complex optimization requiring tuning

2. **Parallel Quiescence:**
   - Multi-threaded quiescence search
   - Lock-free TT access in qsearch
   - **Reason:** Single-threaded performance must be optimal first

## Success Criteria

### Technical Success:
- [ ] Quiescence search integrates without breaking existing functionality
- [ ] All tactical test positions solve correctly
- [ ] Search depth limits respected without overflow
- [ ] TT integration maintains hit rates
- [ ] Statistics tracking provides meaningful insights

### Performance Success:
- [ ] SPRT testing shows 150+ Elo improvement vs Stage 13
- [ ] Total node increase <300% (target: 200-250%)
- [ ] Time increase <20% due to better tactical accuracy
- [ ] Beta cutoff efficiency maintained >80%

### Tactical Success:
- [ ] No horizon effect failures in test positions
- [ ] Check evasion positions solve correctly
- [ ] Deep tactical combinations found reliably
- [ ] No significant tactical blind spots introduced

### Integration Success:
- [ ] Existing tests continue to pass
- [ ] Time management functions correctly
- [ ] Move ordering efficiency preserved
- [ ] UCI interface maintains compatibility

## Timeline Estimate

### Development Phase (10 days):
- **Days 1-3:** Phase 1 - Minimal viable quiescence
- **Days 4-6:** Phase 2 - Performance and integration
- **Days 7-8:** Phase 3 - Advanced features
- **Days 9-10:** Phase 3 - Comprehensive testing

### Testing and Validation (3-5 days):
- **Days 11-12:** SPRT testing vs Stage 13
- **Days 13-14:** Tactical test suite validation
- **Day 15:** Final integration testing and documentation

### Buffer for Issues (2-3 days):
- **Days 16-17:** Bug fixes and performance tuning
- **Day 18:** Final validation and completion

**Total Estimated Duration:** 15-18 days

## Detailed Implementation Specifications

### Core Function Signatures:
```cpp
// Primary quiescence search function
eval::Score quiescence(Board& board, 
                      int ply,
                      eval::Score alpha,
                      eval::Score beta,
                      SearchInfo& searchInfo,
                      SearchData& info,
                      TranspositionTable* tt = nullptr);

// Specialized function for positions in check
eval::Score quiescenceInCheck(Board& board, 
                             int ply,
                             eval::Score alpha,
                             eval::Score beta,
                             SearchInfo& searchInfo,
                             SearchData& info,
                             TranspositionTable* tt = nullptr);
```

### Safety Constants (from safety analysis):
```cpp
namespace seajay::search::qsearch {
    // Stack safety limits
    constexpr int MAX_PLY = 32;              // Absolute maximum depth
    constexpr int MAX_CHECK_PLY = 8;         // Maximum check extensions
    constexpr int STACK_GUARD = 128;         // Stack safety margin
    
    // Performance limits
    constexpr uint64_t NODE_LIMIT = 100000;  // Per-position node limit
    constexpr int DELTA_MARGIN = 900;        // Delta pruning margin (queen value)
    
    // Compile-time safety
    static_assert(MAX_PLY < 64, "Ply limit must fit in search stack");
    static_assert(MAX_CHECK_PLY <= MAX_PLY, "Check extensions within total limit");
}
```

### Enhanced SearchData Structure:
```cpp
// Add to SearchData in types.h
struct QuiescenceStats {
    uint64_t qsearchNodes = 0;
    uint64_t qsearchCutoffs = 0;
    uint64_t standPatCutoffs = 0;
    uint64_t deltaFutilityPrunes = 0;
    uint64_t checkExtensions = 0;
    
    double qsearchRatio() const {
        uint64_t totalNodes = nodes + qsearchNodes;
        return totalNodes > 0 ? static_cast<double>(qsearchNodes) / totalNodes : 0.0;
    }
};
```

### File Structure:
```
src/search/
├── quiescence.h           # Public interface and constants
├── quiescence.cpp         # Main implementation
├── quiescence_check.cpp   # Check handling (separate for clarity)
├── quiescence_stats.h     # Statistics and debugging
└── quiescence_debug.h     # Debug logging system
```

### Integration Points:
1. **In negamax.cpp line 188:** Replace `return board.evaluate();` with quiescence call
2. **In types.h:** Add QuiescenceStats to SearchData
3. **In CMakeLists.txt:** Add new source files
4. **In UCI:** Add debug commands for quiescence testing

## Implementation Notes

### Key Design Decisions:
1. **Separate quiescence function** rather than inline in negamax
2. **Negative depth values** for quiescence depth tracking
3. **Stand-pat first** before generating moves
4. **Check handling** as separate function for clarity
5. **Statistics integration** with existing SearchData structure

### Testing Strategy:
1. **Incremental testing** after each phase
2. **Performance monitoring** throughout development
3. **Tactical validation** using proven test suites
4. **SPRT validation** for strength measurement

### Quality Assurance:
1. **Code review** checkpoints after each phase
2. **Memory safety** validation with sanitizers
3. **Performance profiling** for hot path optimization
4. **Documentation** updated with implementation details

---

This comprehensive plan provides a roadmap for implementing quiescence search that will significantly enhance SeaJay's tactical capabilities while maintaining performance and stability. The incremental approach ensures each component is thoroughly tested before moving to the next phase.