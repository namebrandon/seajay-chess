# SeaJay Chess Engine - Deferred Items Tracker

**Purpose:** Track items deferred from earlier stages and items being deferred to future stages  
**Last Updated:** August 14, 2025  

## Items FROM Stage 1 TO Stage 2

After reviewing Stage 1 implementation, the following was already completed but needs enhancement:

### Already Implemented (But Needs Enhancement):
1. **Basic FEN Parser** (`Board::fromFEN`)
   - ✅ Basic parsing exists
   - ⚠️ Needs: Error reporting with Result<T,E> type
   - ⚠️ Needs: Parse-to-temp-validate-swap pattern
   - ⚠️ Needs: Buffer overflow protection

2. **FEN Generator** (`Board::toFEN`) 
   - ✅ Basic implementation exists
   - ✅ Appears complete

3. **Board Display** (`Board::toString`)
   - ✅ Basic ASCII display exists
   - ⚠️ Could add: Unicode piece display option
   - ⚠️ Could add: Debug display with bitboards

4. **Basic Validation**
   - ✅ `validatePosition()` - basic implementation
   - ✅ `validateKings()` - both kings present
   - ✅ `validatePieceCounts()` - material limits
   - ✅ `validateEnPassant()` - basic EP validation
   - ✅ `validateCastlingRights()` - basic castling validation
   - ⚠️ Missing: Side not to move in check validation
   - ⚠️ Missing: Bitboard/mailbox sync validation
   - ⚠️ Missing: Zobrist validation

### Not Yet Implemented:
5. **Zobrist Rebuild After FEN**
   - Currently incrementally updates during parsing
   - Needs: Complete rebuild from scratch after successful parse

## Items DEFERRED FROM Stage 2 TO Future Stages

### To Stage 4 (Move Generation):
1. **En Passant Pin Validation**
   - Complex validation where EP capture would expose king to check
   - Test positions documented in stage2_position_management_plan.md
   - Reason: Expensive computation, needs attack generation

2. **Triple Check Validation**
   - Validating position doesn't have impossible triple check
   - Reason: Requires attack generation to verify

3. **Full Legality Check**
   - Checking if position is reachable from starting position
   - Reason: Complex retrograde analysis, not critical

### To Stage 3 (UCI):
1. **UCI Position Command**
   - Will use FEN parser to set up positions
   - Dependency: Robust FEN parsing from Stage 2

### To Stage 5 (Testing):
1. **Perft Test Positions**
   - Will use FEN parser to load test positions
   - Dependency: 100% accurate FEN parsing

## Tracking Mechanism

### How We Ensure Nothing Is Forgotten:

1. **Code Comments**: Add TODO comments at relevant locations
```cpp
// TODO(Stage4): Implement en passant pin validation
// See: deferred_items_tracker.md for details
```

2. **Stage Planning Documents**: Each stage plan should review this tracker
```markdown
## Prerequisites Check
- [ ] Review deferred_items_tracker.md for incoming items
- [ ] Update tracker with newly deferred items
```

3. **Test Placeholders**: Create disabled tests for deferred items
```cpp
TEST(Board, DISABLED_EnPassantPinValidation) {
    // TODO(Stage4): Enable when attack generation available
    const char* fen = "8/2p5/3p4/KP5r/8/8/8/8 w - c6 0 1";
    // Test will verify b5xc6 is illegal (exposes king)
}
```

4. **Project Status Updates**: Reference this tracker in project_status.md

## Phase 1 Completion Status

**Phase 1 Complete:** August 9, 2025
- All 5 stages completed successfully
- 99.974% perft accuracy achieved
- Full UCI protocol implementation
- Comprehensive testing infrastructure established

## Items DEFERRED FROM Phase 1 TO Phase 2

### To Phase 2 (Search and Evaluation):
1. **Position 3 Perft Discrepancy Resolution** - **BUG #001**
   - **Status:** Identified but unresolved (documented in `/workspace/project_docs/tracking/known_bugs.md`)
   - **Impact:** 0.026% accuracy deficit at depth 6 (2,871 missing nodes out of 11,030,083)
   - **Priority:** Low (affects only advanced validation, not functionality)
   - **Analysis:** Systematic deficit in rook moves at depth 5+; all other moves perfect
   - **Tools Created:** Complete debugging infrastructure in `/workspace/tools/perft_debug.cpp`
   - **Resolution Strategy:** Use comparative Stockfish analysis to pinpoint exact divergent move
   - **Estimated Effort:** 2-4 hours focused debugging with systematic tools
   - **Validation Criteria:** Position 3 must achieve exactly 11,030,083 nodes at depth 6
   - **Technical Notes:** 99.974% accuracy demonstrates fundamentally sound move generation

## Items DEFERRED FROM Stage 7 TO Future Stages

### Stage 8 COMPLETED Items:
1. **Active Alpha-Beta Cutoffs** ✅
   - Beta cutoffs successfully activated
   - Achieving 90% node reduction
   - Working correctly with fail-soft

2. **Basic Move Ordering** ✅
   - Promotions → Captures → Quiet moves implemented
   - 94-99% move ordering efficiency achieved
   
3. **Search Tree Statistics** ✅
   - Beta cutoff counting implemented
   - Move ordering effectiveness tracking
   - Effective branching factor calculation

### Items Deferred FROM Stage 8:
1. **Advanced Move Ordering (MVV-LVA)** ✅ COMPLETED IN STAGE 11
   - Most Valuable Victim - Least Valuable Attacker
   - More sophisticated capture ordering
   - **COMPLETED:** August 13, 2025 in Stage 11
   - **Result:** 100% ordering efficiency for captures
   - **Performance:** 2-30 microseconds per position

2. **Killer Move Heuristic**
   - Track moves that cause cutoffs
   - Order killer moves early
   - Deferred to Phase 3

3. **History Heuristic**
   - Track move success rates
   - Statistical move ordering
   - Deferred to Phase 3

4. **Aspiration Windows**
   - Search with narrow window around previous score
   - Re-search on fail high/low
   - Deferred to Phase 3

### To Stage 9 (Positional Evaluation):
1. **Quiescence Search**
   - Search captures at leaf nodes
   - Resolve tactical sequences
   - Eliminate horizon effect

2. **Check Extensions**
   - Extend search when in check
   - Don't count check evasions against depth
   - Find deeper checkmates

3. **Search Enhancements**
   - Passed pawn extensions
   - Recapture extensions
   - One-reply extensions

### To Future Phases (Phase 3+):
1. **Transposition Tables**
   - Cache search results
   - Avoid re-searching positions
   - Major performance improvement

2. **Advanced Pruning**
   - Null move pruning
   - Late move reductions (LMR)
   - Futility pruning
   - Delta pruning in quiescence

3. **Parallel Search**
   - Multi-threading support
   - Lazy SMP or YBWC
   - Shared transposition table

4. **Advanced Time Management**
   - Dynamic time allocation
   - Move instability detection
   - Pondering support

5. **Zobrist Random Values Enhancement**
   - Replace sequential debug values (1, 2, 3...) with proper random values
   - Current implementation uses sequential values for debugging
   - Should use high-quality random 64-bit values for production
   - Improves hash distribution and reduces collisions
   - Reference: src/core/zobrist.cpp lines 13-75

## Stage 2 Specific Enhancements Needed

Based on review, Stage 2 needs to:

### High Priority (Core Requirements):
1. Implement Result<T,E> error handling system
2. Add parse-to-temp-validate-swap pattern
3. Add "side not to move in check" validation
4. Implement validateBitboardSync()
5. Implement validateZobrist() with full rebuild
6. Add buffer overflow protection in FEN parsing
7. Create comprehensive test suite with expert positions

### Medium Priority (Improvements):
1. Enhanced error messages with position info
2. Position hash function for testing
3. FEN normalization for testing
4. Debug assertions throughout

### Low Priority (Nice to Have):
1. Unicode piece display option
2. Debug display showing bitboards
3. Performance benchmarks

## Review Schedule

This tracker should be reviewed:
- At the start of each new stage
- During stage completion review
- When adding new deferred items

## Notes

- Items marked with ⚠️ need attention in current stage
- Items marked with ✅ are complete
- TODO comments in code should reference this document
- Keep this document updated as items are completed or deferred

## Items DEFERRED FROM Stage 7 TO Future Stages

### To Stage 8 (Alpha-Beta Pruning):
1. **Active Alpha-Beta Cutoffs**
   - Framework is in place with parameters
   - Actual pruning logic to be activated
   - Move ordering for better cutoffs

2. **Search Tree Statistics**
   - Cutoff rates and efficiency metrics
   - Branch factor analysis

### To Stage 9 (Positional Evaluation):
1. **Quiescence Search**
   - Handle horizon effect
   - Capture-only search at leaf nodes

2. **Search Extensions**
   - Check extensions
   - Passed pawn extensions
   - One-reply extensions

## Items DEFERRED FROM Stage 11 (MVV-LVA) TO Future Stages

**Date:** August 13, 2025  
**Status:** STAGE 11 COMPLETE ✅  
**Source:** stage11_mvv_lva_plan.md

### CRITICAL NOTE (August 20, 2025): 
**⚠️ SEQUENCING ERROR DISCOVERED IN STAGE 18 ⚠️**
- Stage 11 correctly implemented MVV-LVA for **capture ordering only**
- **Quiet moves were left unordered** (return score 0) - this was CORRECT for Stage 11
- However, **LMR (Stage 18) was implemented before quiet move ordering**
- This causes LMR to reduce essentially random quiet moves
- **Correct sequence should have been:**
  - Stage 11: MVV-LVA (captures) ✅
  - Stages 12-17: Various optimizations ✅
  - **Stage 18: History Heuristic** (quiet move ordering) ❌ MISSING
  - **Stage 19: Killer Moves** (additional quiet ordering) ❌ MISSING
  - **Stage 20: LMR** (requires ordered quiet moves) ❌ TOO EARLY
- **Impact:** LMR achieving 91% node reduction but losing 10 ELO

### To Stage 14b (SEE):
1. **Static Exchange Evaluation**
   - Better capture ordering using exchange sequences
   - Pruning of losing captures (e.g., QxP defended by pawn)
   - X-ray attacks in exchanges
   - **Reason:** MVV-LVA doesn't consider recaptures
   - **Impact:** Will filter out bad captures that MVV-LVA ranks highly

### To Future Phases (Phase 3+):
1. **Killer Move Heuristic** (~~Stage 22~~ Should be Stage 19)
   - Track moves that cause beta cutoffs
   - Order killer moves after good captures
   - **Reason:** Requires statistics tracking infrastructure
   - **CRITICAL:** Should be implemented BEFORE LMR

2. **History Heuristic** (~~Stage 23~~ Should be Stage 18)
   - Statistical move ordering based on past success
   - **Reason:** Requires history tables and aging mechanism
   - **CRITICAL:** MUST be implemented BEFORE LMR for it to work

3. **Counter-Move History**
   - Track good responses to specific moves
   - **Reason:** Advanced technique for later phases

4. **Continuation History**
   - Multi-ply move sequence tracking
   - **Reason:** Complex implementation for marginal gain

5. **Position-Specific Adjustments**
   - Endgame material threshold adjustments
   - **Reason:** Requires endgame detection

### Stage 11 Completion Notes:

✅ **Stage 11 Successfully Completed (August 13, 2025):**
- MVV-LVA ordering implemented with formula-based scoring
- Special cases handled: en passant, promotions, underpromotions
- Avoided critical promotion-capture bug (attacker is PAWN, not promoted piece)
- Deterministic ordering with stable sort and from-square tiebreaking
- Debug infrastructure with statistics tracking
- Feature flag for easy A/B testing
- 7 clean git commits, one per phase
- Expected benefits: 15-30% node reduction, +50-100 Elo

**Technical Decisions:**
- Used formula approach vs 2D lookup table (cleaner, same performance)
- Kept scores separate from Move class (preserves 16-bit encoding)
- Implemented IMoveOrderingPolicy interface for future extensibility
- Added comprehensive debug assertions and logging

### To Future Phases:
1. **Transposition Tables** (Phase 3)
2. **Null Move Pruning** (Phase 3)
3. **Late Move Reductions** (Phase 3)
4. **Multi-threading** (Phase 3+)
5. **Repetition Detection** (Phase 2/3)
6. **50-Move Rule** (Phase 2/3)

## Items DEFERRED FROM Stage 9b Performance Analysis

**Analysis Date:** August 11, 2025
**Source:** stage9b_regression_analysis.md

### Architecture Refactoring (Phase 3+):

1. **Proper Separation of Game vs Search History**
   - **Current:** Board class mixes game and search history
   - **Target:** Separate GameController and SearchThread classes
   - **Benefit:** Clean architecture, no search/game state mixing
   - **Reference:** Lines 164-194 of regression analysis
   ```cpp
   // Future architecture:
   // GameController - handles game moves and history
   // SearchThread - handles search with pre-allocated stack
   // Board - pure position state, no history
   ```

2. **Pre-allocated Search Stack Pattern**
   - **Current:** Using SearchInfo with fixed array (good)
   - **Enhancement:** Full Stockfish-style StateInfo pattern
   - **Benefit:** Zero heap allocations during search
   - **Reference:** Lines 27-32 of regression analysis

### Performance Optimizations (Phase 3):

3. **Incremental Material Tracking**
   - **Current:** Material recalculated on demand
   - **Target:** Update incrementally during make/unmake
   - **Benefit:** Avoid redundant calculations
   - **Reference:** Line 200 of regression analysis

4. **Smart Eval Cache Invalidation**
   - **Current:** Cache invalidated on every move
   - **Target:** Selective invalidation based on move type
   - **Benefit:** Better cache utilization
   - **Reference:** Line 201 of regression analysis

5. **Remove Bounds Checking in Release Builds**
   - **Current:** Vector bounds checks always active
   - **Target:** Use asserts only in debug builds
   - **Benefit:** Reduced overhead in hot path
   - **Reference:** Line 202 of regression analysis

6. **Ensure No Virtual Function Calls in Hot Path**
   - **Current:** makeMove/unmakeMove are not virtual (good)
   - **Verify:** Keep monitoring this in future refactoring
   - **Reference:** Line 203 of regression analysis

### Advanced Performance Features (Phase 4+):

7. **Hash Table Prefetching**
   - **Future:** Prefetch transposition table entries
   - **Benefit:** Hide memory latency
   - **Reference:** Line 206 of regression analysis

8. **Branch Prediction Hints**
   - **Target:** Use [[likely]]/[[unlikely]] C++20 attributes
   - **Benefit:** Better CPU branch prediction
   - **Reference:** Line 207 of regression analysis

9. **SIMD Bitboard Operations**
   - **Target:** Use AVX2/SSE for bitboard operations
   - **Benefit:** Parallel bit manipulation
   - **Reference:** Line 208 of regression analysis

10. **Copy-Make Pattern Investigation**
    - **Research:** Test copy-make vs make/unmake
    - **Benefit:** May be faster for simple positions
    - **Reference:** Lines 209, 251 of regression analysis

### Monitoring and Testing Infrastructure:

11. **Regular Performance Profiling**
    - **Tool:** perf on Linux, Instruments on macOS
    - **Frequency:** After each major change
    - **Target:** makeMove < 5% of runtime
    - **Reference:** Lines 221-224 of regression analysis

12. **Nodes Per Second Benchmarking**
    - **Target:** 2-5M nps on modern hardware
    - **Current:** Need to measure after fix
    - **Reference:** Lines 217-219 of regression analysis

### Stage 9b Deferred Items (August 11, 2025):

13. **Fifty-Move Rule Implementation**
    - **Reason:** Not critical for initial draw detection
    - **Complexity:** Requires halfmove counter tracking
    - **Impact:** Would catch additional drawn positions
    - **Target:** Phase 3 or later

14. **UCI Draw Claim Handling**
    - **Reason:** Not required for engine vs engine testing
    - **Complexity:** UCI protocol extension needed
    - **Impact:** Important for GUI integration
    - **Target:** When GUI compatibility becomes priority

### Already Implemented from Analysis:

✅ **Stage 9b Completed (August 11, 2025):**
- Implemented dual-mode history system (vector for game, array for search)
- Zero heap allocations during search
- Threefold repetition detection working correctly
- Debug instrumentation wrapped in DEBUG guards
- **Result:** Draw detection works correctly, performance optimized

### Priority Ranking for Future Work:

**High Priority (Phase 3):**
- Items 3, 4, 5 (Performance optimizations)
- Item 11 (Regular profiling)

**Medium Priority (Phase 3-4):**
- Items 1, 2 (Architecture refactoring)
- Items 7, 8 (Advanced performance)

**Low Priority (Research):**
- Items 9, 10 (SIMD, Copy-make)
- Item 12 (NPS benchmarking)

### Notes:
- The immediate fix has been applied and is ready for SPRT testing
- Further architectural improvements deferred to avoid scope creep
- Focus remains on recovering the -70 Elo regression first
- All Stockfish/Ethereal/Leela patterns documented for future reference

## Search Architecture Refactoring (CRITICAL)

**Date Added:** August 25, 2025  
**Status:** DEFERRED - Major architectural issue identified  
**Source:** Depth deficit investigation and singular extensions implementation  
**Severity:** HIGH - Fundamental design pattern mismatch with successful engines  

### The Problem:

During implementation of singular extensions, we discovered that SeaJay's search architecture fundamentally differs from successful engines (Stockfish, Laser, Ethereal) in how it handles move exclusion and special searches.

#### Current SeaJay Approach (Problematic):
```cpp
// Single recursive call with excluded move mechanism
searchInfo.setExcludedMove(ply, move);
eval::Score singularScore = negamax(board, singularDepth, ply,
                                   singularBeta - 1, singularBeta,
                                   searchInfo, info, limits, tt, false);
searchInfo.setExcludedMove(ply, NO_MOVE);

// Move loop checks for exclusion
for (const Move& move : moves) {
    if (searchInfo.isExcluded(ply, move)) {
        continue;  // Skip excluded move
    }
    // ... rest of move processing
}
```

**Issues with this approach:**
1. **Excluded move affects move counting** - moveCount is wrong when moves are skipped
2. **Breaks move ordering statistics** - excluded moves don't contribute to ordering efficiency metrics
3. **Complex state management** - must track excluded moves across recursive calls
4. **Different from proven implementations** - no successful engine uses this pattern
5. **Hard to debug** - exclusion state is implicit and distributed

#### Successful Engine Approach (Stockfish/Laser/Ethereal):
```cpp
// Iterate through moves and test each one explicitly
bool isSingular = true;
for (unsigned int i = 0; i < legalMoves.size(); i++) {
    Move testMove = legalMoves.get(i);
    if (testMove == ttMove) continue;  // Skip the move being tested
    
    Board copyBoard = board.staticCopy();
    if (!copyBoard.makeMove(testMove)) continue;
    
    // Search this specific move with reduced depth
    int score = -search(copyBoard, singularDepth, -singularBeta, -singularBeta + 1, ...);
    
    if (score >= singularBeta) {
        isSingular = false;  // Found alternative that's good enough
        break;
    }
}
```

**Advantages of the proven approach:**
1. **Explicit and clear** - each move is tested individually
2. **No state pollution** - no excluded move tracking needed
3. **Correct statistics** - all moves counted properly
4. **Easier to debug** - can log each move tested
5. **Proven to work** - used by all top engines

### Impact Beyond Singular Extensions:

This architectural issue affects multiple advanced search features:

1. **Multi-cut pruning** - needs to test multiple moves to prove cutoff
2. **Probcut** - requires searching specific moves at reduced depth
3. **IID (Internal Iterative Deepening)** - needs special searches
4. **Singular extensions** - already failed due to this issue
5. **Enhanced null move** - verification searches need clean state

### Recommended Refactoring:

#### Phase 1: Create Search Variants
```cpp
// Add specialized search functions instead of using flags/exclusions
eval::Score searchSingular(Board& board, int depth, int ply, 
                          eval::Score alpha, eval::Score beta,
                          Move excludeMove, ...);

eval::Score searchProbcut(Board& board, int depth, int ply, ...);

eval::Score searchNull(Board& board, int depth, int ply, ...);
```

#### Phase 2: Explicit Move Testing Pattern
```cpp
// For features that need to test multiple moves
template<typename Predicate>
bool testMoves(Board& board, const MoveList& moves, 
               Predicate shouldTest, int depth, ...) {
    for (const Move& move : moves) {
        if (!shouldTest(move)) continue;
        
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        // ... search this move
        board.unmakeMove(move, undo);
        // ... check result
    }
}
```

#### Phase 3: Remove Excluded Move Mechanism
- Delete `excludedMove` from SearchStack
- Remove `isExcluded()`, `setExcludedMove()` methods
- Simplify move loop to always process all moves

### Priority and Timeline:

**Priority:** HIGH - Blocking multiple advanced features  
**When to implement:** Before any of:
- Singular extensions (already blocked)
- Multi-cut pruning
- Probcut
- Advanced null move techniques

**Estimated effort:** 10-15 hours
- 2-3 hours: Design new search architecture
- 4-6 hours: Implement search variants
- 2-3 hours: Refactor existing code
- 2-3 hours: Testing and validation

### Temporary Workarounds:

Until refactoring is complete:
1. **Avoid features requiring move exclusion** - they won't work properly
2. **Keep check extensions only** - simple and proven to work
3. **Focus on features that don't need special searches** - move ordering, time management
4. **Document all deferred features** - maintain list of blocked improvements

### Success Criteria for Refactoring:

1. **No excluded move mechanism** - completely removed from codebase
2. **Explicit move testing** - all special searches clearly visible
3. **Correct statistics** - move counts, ordering efficiency accurate
4. **Feature parity** - can implement same features as top engines
5. **Performance maintained** - no regression from refactoring

### References:

1. **Laser Implementation:** Lines 849-891 of search.cpp - clear iteration pattern
2. **Stockfish:** Uses explicit loops for all special searches
3. **Ethereal:** Similar pattern with separate search functions
4. **Our failed attempt:** commit 5a13f1c (feature/20250825-depth-deficit-investigation)

### Lessons Learned:

1. **Copy successful patterns** - don't try to be clever with different approaches
2. **Explicit > Implicit** - clear code is better than "elegant" state management
3. **Test infrastructure early** - would have caught the move counting issue
4. **Study reference implementations** - before designing core architecture

## Items DEFERRED FROM Stage 10 (Magic Bitboards) TO Future Stages

**Date:** August 12, 2025  
**Status:** STAGE 10 COMPLETE ✅  
**Source:** stage10_magic_bitboards_plan.md

### To Future Phases (Phase 5+):

1. **X-Ray Attack Generation**
   - **Description:** Attacks that go "through" pieces (e.g., rook attacking through another rook)
   - **Current:** Magic bitboards provide raw attacks only (stop at first blocker)
   - **Use Case:** Advanced tactics, pin detection
   - **Complexity:** Requires separate x-ray tables or on-demand calculation
   - **Priority:** Low - only needed for advanced evaluation features
   - **Note:** Basic magic bitboards are sufficient for move generation

2. **Magic Number Generation Algorithm**
   - **Description:** Custom generation of magic numbers instead of using Stockfish's
   - **Current:** Using proven magic numbers from Stockfish (with attribution)
   - **Reason:** Generation adds complexity without performance benefit
   - **Priority:** Very Low - current numbers work perfectly
   - **Note:** Only consider if licensing becomes an issue

3. **Fancy Magic Bitboards**
   - **Description:** Variable-size attack tables to save memory
   - **Current:** Using plain magic (fixed-size tables, ~2.3MB)
   - **Reason:** Added complexity not worth ~1.7MB memory savings
   - **Priority:** Very Low - memory usage is acceptable
   - **Expert Opinion:** Plain magic recommended by chess-engine-expert

4. **SIMD Optimizations for Magic Lookups**
   - **Description:** Using AVX2/SSE instructions for parallel operations
   - **Current:** Standard C++ implementation
   - **Reason:** Focus on correctness first, optimization later
   - **Target:** Phase 6 after all core features complete
   - **Priority:** Medium - could provide additional speedup

5. **Kindergarten Bitboards Alternative**
   - **Description:** Alternative to magic bitboards using different approach
   - **Current:** Not implementing (magic bitboards chosen)
   - **Reason:** Magic bitboards are industry standard
   - **Priority:** Very Low - only if magic bitboards prove insufficient

### Stage 10 Completion Notes:

✅ **Stage 10 Successfully Completed (August 12, 2025):**
- Achieved 55.98x speedup (far exceeding 3-5x target)
- Header-only implementation avoids static initialization issues
- 155,388 symmetry tests all passing
- Zero memory leaks verified with valgrind
- Both ray-based and magic implementations coexist
- Production-ready code with all debug output removed
- Memory usage: 2.25MB (as expected)

**Technical Decisions:**
- Kept ray-based implementation as fallback (conservative approach)
- Used `USE_MAGIC_BITBOARDS` compile flag for switching
- `DEBUG_MAGIC` flag available for validation mode
- Can remove ray-based after extended production testing

## Items DEFERRED FROM Stage 12 (Transposition Tables) TO Future Stages

**Date:** August 14, 2025  
**Status:** STAGE 12 COMPLETE ✅ (Phases 0-5 implemented)  
**Source Documents:** 
- Planning: `/workspace/project_docs/planning/stage12_transposition_tables_plan.md`
- Implementation: `/workspace/project_docs/stage_implementations/stage12_transposition_tables_implementation.md`
- Status: `/workspace/project_docs/stage_implementations/stage12_current_status.md`
- Expert Review: Chess-engine-expert analysis (August 14, 2025)

### Core Achievement (Phases 0-5):
✅ **Successfully Implemented:**
- Comprehensive test infrastructure with 19 killer positions
- Proper Zobrist hashing with fifty-move counter (949 unique keys)
- Basic TT with always-replace strategy (16-byte entries)
- Full search integration (probe and store)
- Correct draw detection order (checkmate → repetition → fifty-move → TT)
- Mate score adjustment for ply distance
- **Performance:** 25-30% node reduction, 87% hit rate
- **Status:** Production-ready, valgrind clean, all tests passing

### Phase 6: Three-Entry Clusters (DEFERRED)

**Description:** Implement multiple entries per hash index to reduce collisions

**Detailed Implementation Guidance (from expert review):**
```cpp
// Optimal 4-entry cluster (64 bytes, cache-line aligned)
struct alignas(64) TTCluster {
    TTEntry entries[4];  // 4x16 = 64 bytes exactly
};

// Replacement strategy within cluster:
int selectVictim(const TTCluster& cluster, uint32_t key32, uint8_t currentGen) {
    int victimIdx = 0;
    int minScore = INT_MAX;
    
    for (int i = 0; i < 4; i++) {
        // Empty slot - use immediately
        if (!cluster[i].key32) return i;
        
        // Matching key - always replace
        if (cluster[i].key32 == key32) return i;
        
        // Scoring: higher = more valuable to keep
        int score = 0;
        if (cluster[i].generation() == currentGen) score += 1000;
        score += cluster[i].depth * 10;
        if (cluster[i].bound() == EXACT) score += 100;
        
        if (score < minScore) {
            minScore = score;
            victimIdx = i;
        }
    }
    return victimIdx;
}
```

**Why Deferred:**
- Current always-replace working better than expected (87% hit rate)
- Adds memory management complexity
- Risk of subtle bugs affecting entire search
- Only 5-8% improvement expected (not 10% as originally thought)

**When to Implement:**
- Phase 4+ when collision patterns are better understood
- After SPRT testing shows specific collision problems
- If hit rate drops below 80% in certain position types

**Testing Required:**
- Collision chain length monitoring
- Victim selection distribution analysis
- Cache-line alignment verification
- False sharing detection in SMP

### Phase 7: Advanced Features (DEFERRED)

#### 7A: Generation/Aging Mechanism

**Detailed Implementation Guidance:**
```cpp
class TranspositionTable {
    uint8_t generation = 0;  // Wraps naturally 0-255
    
    void newSearch() { 
        generation++;  // Let it wrap
    }
    
    bool isPreferred(const TTEntry& existing, int newDepth) {
        uint8_t ageDiff = generation - existing.generation();
        
        // Always replace very old entries (128+ generations)
        if (ageDiff > 128) return true;
        
        // Recent generations: prefer depth
        if (ageDiff > 8) {
            return newDepth >= existing.depth - 2;
        }
        
        return newDepth > existing.depth;
    }
};
```

**Tuning Parameters:**
- Standard time control: 70% depth, 30% age weight
- Bullet/blitz: 50/50 split
- Analysis mode: 90% depth, 10% age

#### 7B: PV Extraction

**Safe Implementation (avoiding loops):**
```cpp
std::vector<Move> extractPV(Board& board, int maxDepth) {
    std::vector<Move> pv;
    std::unordered_set<uint64_t> seen;  // Critical: detect loops
    Board tempBoard = board;  // Work on copy!
    
    while (pv.size() < maxDepth) {
        uint64_t hash = tempBoard.getZobristKey();
        
        // Loop detection - CRITICAL
        if (!seen.insert(hash).second) break;
        
        TTEntry* tte = probe(hash);
        if (!tte || tte->move == Move::none()) break;
        
        // Validate move legality
        if (!tempBoard.isLegalMove(tte->move)) break;
        
        pv.push_back(tte->move);
        tempBoard.makeMove(tte->move);
    }
    
    return pv;
}
```

**Edge Cases to Test:**
- Forced repetition positions
- Mate sequences with underpromotion
- Positions after null move (should not extract through)
- Corrupted TT entries

#### 7C: Depth-Preferred Replacement

**Implementation Details:**
- Keep exact bounds over fail-high/fail-low
- Weight recent searches higher
- Consider bound type in replacement decision
- Track "always keep" flag for critical positions

#### 7D: Prefetching

**Modern CPU Considerations:**
- CPUs 2020+: Minimal benefit (2-3% at best)
- Implementation: `__builtin_prefetch(&tt[nextHash & mask], 0, 1)`
- Only worth it if profiling shows cache misses >10%

**Why Phase 7 Deferred:**
- Aging adds complexity with minimal gain (+10-15 Elo)
- PV extraction risky (loops, corruption)
- Current simple strategy effective
- Better to implement after gathering real-world data

### Phase 8: Optimization and Polish (DEFERRED)

**Optimization Priority (from expert):**

1. **Batch Clear with memset** (Easy win)
   ```cpp
   void clear() {
       std::memset(entries.data(), 0, sizeMB * 1024 * 1024);
   }
   ```

2. **Lock-free SMP Preparation** (XOR trick)
   ```cpp
   // For future SMP - XOR old and new data
   void atomicStore(TTEntry* tte, TTEntry newEntry) {
       uint64_t oldData = tte->asUint64();
       uint64_t newData = newEntry.asUint64();
       tte->asUint64() = oldData ^ newData ^ oldData;
   }
   ```

3. **Huge Pages Support** (2MB pages)
   ```cpp
   #ifdef __linux__
   void* mem = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
   #endif
   ```

4. **Platform-Specific Alignment**
   - x86-64: 64-byte cache lines
   - ARM: Consider 128-byte for newer chips
   - Use `std::hardware_destructive_interference_size`

**SIMD Opportunities:**
- Limited for TT (random access pattern)
- Useful only for batch operations (clear, resize)
- Not worth complexity for marginal gains

**Why Phase 8 Deferred:**
- Current implementation already fast
- Optimizations are incremental improvements
- Risk of platform-specific bugs
- Better done after profiling shows specific bottlenecks

### Expert's Final Verdict (August 14, 2025)

**Strong Recommendation: DEFER ALL REMAINING PHASES**

**Rationale:**
> "A working transposition table is worth two theoretical improvements"

- Current implementation achieves PRIMARY goals
- 25-30% node reduction already captured
- Risk/reward strongly favors moving forward
- Phases 6-8 provide only +35-50 Elo (optimistic) for 10-15 hours work
- Better to implement new features with higher impact

**Expected Total Improvement from Phases 6-8:**
- Phase 6 (Clusters): +20-30 Elo maximum
- Phase 7 (Aging/PV): +10-15 Elo
- Phase 8 (Optimization): +5-10 Elo
- **Total:** +35-50 Elo (vs +130-175 already achieved)

### Killer Test Positions for Future Validation

When implementing deferred phases, use these positions:

```cpp
// High collision stress tests
const char* collisionTests[] = {
    // After 50+ moves from start position
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 50 75",
    
    // Near fifty-move rule
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 99 100",
    
    // After 1000 random moves (high collision rate)
    "7k/8/8/8/8/8/8/K6R w - - 0 1",
    
    // The Transposition Trap (same position, different history)
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",  // After Ke1-e2-e1
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 2 2",  // Same pos, different fifty
};
```

### Implementation Priority When Revisiting

**If forced to implement some features:**
1. Generation/aging only (easiest, least risky)
2. Batch clearing optimization (simple win)
3. Skip clusters entirely (complexity not worth it)
4. Skip PV extraction (UCI already shows PV)

**Cherry-pick Approach:**
- Implement only features showing clear need in profiling
- Add incrementally with full testing between
- Maintain ability to disable each feature

### Success Metrics for Future Implementation

**Clusters (Phase 6):**
- Hit rate improvement: >5%
- Collision rate reduction: >50%
- No performance regression
- Memory overhead acceptable

**Aging (Phase 7):**
- Old entry replacement rate: >10%
- Search quality maintained
- No tactical blindness
- PV stability improved

**Optimization (Phase 8):**
- Clear operation: <1ms for 128MB
- Platform-specific gains: >5%
- No compatibility issues
- Maintainable code

### References for Future Implementation

1. **Stockfish Implementation:** Modern reference for all features
2. **CPW Transposition Table:** Comprehensive theory and pitfalls
3. **Expert Review:** This document's Phase 6-8 sections
4. **Test Infrastructure:** `/workspace/tests/unit/test_transposition_table.cpp`
5. **Killer Positions:** `/workspace/tests/integration/test_tt_search.cpp`

### Tracking and Review

**Review Triggers:**
- SPRT testing shows <80% hit rate
- Profiling reveals >15% time in TT operations
- Collision rate exceeds 5%
- Phase 4 planning begins
- SMP implementation requires lock-free TT

**Current State for Reference:**
- Implementation: `/workspace/src/core/transposition_table.h/cpp`
- Tests: `/workspace/tests/unit/test_transposition_table.cpp`
- Statistics: Integrated in UCI info output
- Memory: 128MB default, configurable via UCI

### Stage 12 Summary

**What We Built (Phases 0-5):** A robust, production-ready transposition table with proper Zobrist hashing, search integration, and comprehensive testing.

**What We Deferred (Phases 6-8):** Advanced optimizations that provide marginal gains with significant complexity risk.

**The Right Decision:** Expert consensus strongly supports deferring remaining phases to focus on higher-impact features in upcoming stages.

**Next Stage:** Ready to proceed with Stage 13 (likely Null Move Pruning) with confidence in our TT foundation

## PRIORITY ITEMS - Performance Optimizations

### CPU Optimization Flags (HIGH PRIORITY)
**Date Added:** August 15, 2025  
**Requested By:** User (comparison with Elixir engine)  
**Status:** Ready to implement  

**Description:** Add CPU-specific optimization flags `-mpopcnt -msse4.2` to improve performance

**Analysis Complete:**
- Tested and verified **4.2x speedup** for popcount operations
- No code changes required - SeaJay already uses `std::popcount` which leverages these instructions
- Compiles cleanly with these flags
- Safe for modern CPUs (2008+ support)

**Implementation Options:**
1. **Option A - Add to CMakeLists.txt (Recommended):**
```cmake
# Add after line 86 in CMakeLists.txt
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    option(USE_CPU_OPTIMIZATIONS "Enable CPU-specific optimizations (popcnt, SSE4.2)" ON)
    if(USE_CPU_OPTIMIZATIONS)
        add_compile_options(-mpopcnt -msse4.2)
        message(STATUS "CPU optimizations enabled: POPCNT, SSE4.2")
    endif()
endif()
```

2. **Option B - Build script modification (Non-invasive):**
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-march=x86-64 -mpopcnt -msse4.2" ..
```

**Expected Benefits:**
- Significant speedup in bitboard operations (popcount, lsb, msb)
- Better move generation performance
- No compatibility issues with target x86-64 architecture
- Matches optimization level of Elixir engine

**Priority:** HIGH - Easy win with significant performance gain

---

## UCI Parameter Tuning - SPSA Optimization Opportunities

**Date Added:** August 26, 2025  
**Status:** DEFERRED - Multiple tuning opportunities identified  
**Priority:** MEDIUM - Significant ELO gains possible through tuning  

### Parameters Ready for SPSA Tuning:

#### 1. Aspiration Window Settings (HIGH POTENTIAL)
**Current UCI Options:**
- `AspirationWindow`: Default 16cp (range: 5-50)
- `AspirationMaxAttempts`: Default 5 (range: 3-10)
- `AspirationGrowth`: Default "exponential" (options: linear, moderate, exponential, adaptive)

**Tuning Opportunities:**
- **Initial window size:** Current 16cp is very narrow (Stockfish-style), but may cause excessive re-searches
- **Wider windows (20-30cp):** Better for tactical positions, fewer re-searches
- **Narrower windows (10-15cp):** More aggressive pruning, deeper searches
- **Growth mode tuning:** Linear vs exponential growth patterns
- **Depth-based adjustment:** Different window sizes for different depths

**Expected Impact:** 10-30 ELO from optimal aspiration settings
**Testing Strategy:** SPSA with 20,000+ games, monitor re-search rates

#### 2. Null Move Parameters
**Current UCI Options:**
- `NullMoveReduction`: Default 3 (range: 2-4)
- `NullMoveStaticMargin`: Default 120 (range: 50-300)
- `UseNullMove`: Default true

**Note:** Static margin tuning already planned in Phase 1.6 of pruning optimization

#### 3. LMR Parameters
**Current UCI Options:**
- `LMRMinDepth`: Default 3
- `LMRMinMoveNumber`: Default 6
- `LMRBaseReduction`: Default 1
- `LMRDepthFactor`: Default 3

**Expected Impact:** 5-15 ELO from optimal reduction formula

#### 4. SEE Pruning Thresholds
**Current UCI Options:**
- `SEEPruning`: Default "off" (should test "conservative" vs "aggressive")
- Conservative threshold: -100cp
- Aggressive threshold: -75cp

**Note:** Currently disabled by default, but tuning thresholds could make it viable

#### 5. Quiescence Search Parameters
**Current UCI Options:**
- `DeltaPruningMargin`: Fixed at 900cp (could be tunable)
- `QSearchNodeLimit`: Default 0 (unlimited)

#### 6. Move Ordering Bonuses
**Current UCI Options:**
- `KillerMoveBonus`: Default 8000 (range: 0-20000)
- `CountermoveBonus`: Default 8000 (range: 0-20000)

#### 7. Time Management Parameters
**Current UCI Options:**
- `StabilityThreshold`: Default 6 (range: 3-12)
- `OpeningStability`: Default 4 (range: 2-8)
- `MiddlegameStability`: Default 6 (range: 3-10)
- `EndgameStability`: Default 8 (range: 4-12)

### General SPSA Tuning Strategy:

1. **Batch Related Parameters:** Tune aspiration settings together
2. **Use OpenBench SPSA:** Built-in support for parameter tuning
3. **Start Conservative:** Begin with small ranges around current values
4. **Monitor Side Effects:** Watch for tactical blindness or time losses
5. **Phase-Specific Tuning:** Different settings for different game phases

### Implementation Priority:

**Phase 1 (Immediate after current work):**
- Aspiration window settings (high impact, easy to test)
- Null move static margin (already planned)

**Phase 2 (After initial tuning):**
- LMR parameters (once move ordering is stable)
- Move ordering bonuses (killer/countermove values)

**Phase 3 (Future optimization):**
- SEE thresholds (if SEE becomes viable)
- Time management parameters
- Quiescence margins

### Expected Total Gain from Tuning:
- Conservative estimate: 30-50 ELO
- Optimistic estimate: 50-80 ELO
- Depends heavily on current parameter quality

### Testing Protocol:
```bash
# Example OpenBench SPSA configuration
Parameter: AspirationWindow
Start: 16
Min: 8
Max: 32
Step: 2
Games: 20000
Time Control: 10+0.1
```

### Notes:
- Some parameters interact (e.g., aspiration width affects null move effectiveness)
- Tuning should be done iteratively, not all at once
- Re-tune periodically as engine strength increases
- Document optimal values for different time controls

---

## Items DEFERRED FROM Stage 14 (Quiescence Search) TO Future Stages

**Date:** August 15, 2025  
**Status:** STAGE 14 COMPLETE ✅ (Basic quiescence with captures and check evasions)  
**Updated:** August 19, 2025 - Confirmed Q-search is ACTIVE and working  
**Source Documents:**
- Regression Analysis: `/workspace/project_docs/stage_investigations/stage14_regression_analysis.md`
- Candidates Summary: `/workspace/project_docs/stage_implementations/stage14_candidates_summary.md`
- Decision Document: `/workspace/project_docs/stage_implementations/stage14_quiet_checks_decision.md`

### Core Achievement:
✅ **Successfully Implemented and ACTIVE:**
- Basic quiescence search with captures and check evasions **[VERIFIED ACTIVE - August 19, 2025]**
- Delta pruning with conservative margins (900cp/600cp)
- MVV-LVA move ordering in quiescence
- Transposition table integration
- Time pressure panic mode
- **Performance:** +300 ELO over Stage 13
- **Status:** Production-ready, ENABLE_QUIESCENCE flag properly set in CMakeLists.txt
- **Binary Size:** 455KB (confirms all features compiled in)
- **UCI Control:** UseQuiescence option defaults to true
- **Verification:** seldepth reaches 25+ while depth at 6 (clear sign of active quiescence)

### To Stage 16+ (After Prerequisites):

#### 1. Quiet Checks in Quiescence (DEFERRED to Stage 16)

**Description:** Include quiet (non-capture) checking moves at depth 0 of quiescence

**Why Deferred:**
- **Missing Prerequisites:** ~~No SEE to filter bad checks~~ SEE NOW AVAILABLE (Stage 15 complete)
- **Recent C9 Catastrophe:** Need stability after aggressive pruning failure
- **High Risk:** Could cause search explosion without proper filtering
- **Complexity:** Requires efficient `givesCheck()` function

**⚠️ CRITICAL NOTE FOR STAGE 16 IMPLEMENTATION:**
- **SEE is COMPLETE (Stage 15)** but currently **DISABLED BY DEFAULT**
- **UCI Settings:** SEEMode defaults to "off", SEEPruning defaults to "off"
- **Reason:** OpenBench testing showed SEE overhead exceeds benefit at ~2200 ELO
- **Action Required:** When implementing Stage 16, must enable SEE in UCI options
- **Testing:** Will need to test quiet checks with SEE enabled vs disabled

**Prerequisites Status:**
1. **SEE (Static Exchange Evaluation)** - ✅ COMPLETE but disabled by default
2. **Efficient Check Detection** - Fast `givesCheck()` implementation needed
3. **Stable Quiescence** - ✅ Current implementation fully validated  
4. **Better Time Management** - Minor 1-2% time losses acceptable

**Implementation When Ready:**
```cpp
// ONLY at depth 0, with strict limits
if (ply == 0 && !isInCheck && ENABLE_QUIET_CHECKS) {
    MoveList quietChecks;
    generateQuietChecks(board, quietChecks);
    
    int checksAdded = 0;
    const int MAX_QUIET_CHECKS = 2;  // Very conservative
    
    for (Move check : quietChecks) {
        if (see(board, check) >= 0 &&  // Requires SEE!
            checksAdded++ < MAX_QUIET_CHECKS) {
            moves.push_back(check);
        }
    }
}
```

**Expected Gain:** 15-25 ELO (but only after prerequisites)

**Expert Opinion (chess-engine-expert):**
> "DO NOT IMPLEMENT quiet checks at Stage 14. SeaJay needs stability and consolidation after the C9 catastrophe, not more experimental features."

### To Future Phases:

#### 2. Graduated Delta Margin Tuning

**Description:** Carefully tune delta margins between 900cp and lower values

**Current State:**
- Using conservative 900cp (can see queen captures)
- C9's aggressive 200cp caused catastrophic failure

**Future Approach:**
- Test incrementally: 900 → 800 → 700 → 600
- Use SPRT at each step
- Find break-even point for SeaJay's evaluation

#### 3. SEE-Based Pruning in Quiescence

**Description:** Use Static Exchange Evaluation to prune bad captures

**Benefit:** Can safely use lower delta margins with SEE filtering

**Implementation:** Stage 16 or later (after SEE implementation)

#### 4. Singular Reply Extension in Quiescence

**Description:** Extend when only one legal move in check evasion

**Benefit:** Better tactical resolution in forcing lines

**Complexity:** Low, but needs careful testing

### Critical Lessons from Stage 14:

#### The C9 Catastrophe:
- Delta margin of 200cp pruned away winning captures
- With 200cp margin, being down 201cp meant missing:
  - Queen captures (900cp)
  - Rook captures (500cp)
  - Minor piece captures (320cp)
- Result: Lost 0-11 against Golden C1

#### The Recovery:
- C10 reverted to conservative 900cp margins
- Performance restored to Golden C1 level
- Lesson: Delta margins MUST exceed capturable piece values

#### The ENABLE_QUIESCENCE Mystery:
- Spent 4 hours debugging "regression" 
- Discovered quiescence was never compiled in (missing flag)
- Binary size difference (384KB vs 411KB) was the critical clue
- Lesson: Never use compile-time flags for core features

### Stage 14 Summary (COMPLETE - August 15, 2025):

**What We Built:** 
- Robust quiescence search with conservative parameters
- +300 ELO improvement over static evaluation
- All features compile in, UCI-controlled
- Stable 51% performance against baseline after 137+ SPRT games

**What We Deferred:**
- Quiet checks (confirmed deferred to Stage 16 after expert re-consultation)
- Aggressive delta margin tuning
- Advanced pruning techniques

**Expert Re-Assessment (August 15, 2025):**
User requested reconsideration of quiet checks given increased stability and code familiarity. Chess-engine-expert strongly reaffirmed original decision:
> "DO NOT implement quiet checks now. Without SEE, you'll examine losing moves. Ship Stage 14 as complete, implement SEE in Stage 15, then enhance quiescence in Stage 16."

**The Right Decision Confirmed:** 
Despite temptation to add quiet checks, expert consensus and historical engine experience (Fruit, Glaurung) confirms that SEE is a hard prerequisite.

**Next Stage:** Stage 15 - Static Exchange Evaluation (SEE) - Foundation for Stage 16 enhanced quiescence

## Items DEFERRED FROM Stages 19-20 (Killer Moves & History Heuristic) TO Future Stages

**Date:** August 20, 2025  
**Status:** STAGES 19-20 COMPLETE ✅  
**Source:** Implementation and testing of move ordering improvements

### Stage 19-20 Completion Summary

✅ **Successfully Implemented:**
- **Killer Moves (Stage 19):** +31.42 ± 10.49 ELO
- **History Heuristic (Stage 20):** +6.78 ± 8.96 ELO
- **Combined Improvement:** ~38 ELO in move ordering
- **Move Ordering Efficiency:** Sufficient for LMR implementation

### Critical Lessons Learned from History Heuristic Implementation

#### The Complexity Trap (Phase B4 Investigation)
After B3 underperformed (only +6.78 vs expected +25-35 ELO), we tested several "improvements":

1. **History Persistence (B4.1):** -0.55 ELO (neutral)
   - Preserving history across iterations didn't help
   - History is position-dependent, not generally useful across searches

2. **Aggressive Butterfly Updates (B4.2):** -5.25 ELO (regression)
   - Penalty of depth²/2 was too harsh
   - Over-penalized moves that were good in other contexts

3. **Gentle Butterfly Updates (B4.3):** -0.97 ELO (neutral)
   - Reduced penalty to depth²/4
   - Still no improvement over simple implementation

**Key Insight:** Simple history (reward cutoffs only) works best for SeaJay's current strength level. Complex techniques that work in 3000+ ELO engines may actually hurt at 2200 ELO.

### Implementation Best Practices Discovered

1. **Phased Development Works**
   - Infrastructure → Integration → Activation pattern catches bugs early
   - Expected regressions (like B2's -9 ELO) are normal and documented
   - Test EVERY phase, even "no impact" ones

2. **Simple > Complex at Lower ELO**
   - Advanced techniques require strong foundation
   - Butterfly updates need better evaluation to differentiate moves
   - Keep it simple until engine reaches higher strength

3. **Trust Original Implementation**
   - B3's simple approach was actually optimal
   - Remediation attempts made things worse
   - Sometimes "underperformance" is actually correct performance

### Additional Move Ordering Improvements for LMR Support

With killer moves (+31 ELO) and history heuristic (+15-25 ELO expected) complete, the following additional move ordering techniques could further improve LMR effectiveness:

#### 1. Countermove Heuristic (Stage 21+)
**Expected Gain:** +5-10 ELO

**Description:** Store the best response to each opponent move
- "If opponent plays e2-e4, we usually respond well with e7-e5"
- Indexed by [piece][to_square] of opponent's last move
- Complements killers (ply-based) and history (global)

**Implementation Approach:**
```cpp
class CountermoveTable {
    Move countermoves[NUM_PIECE_TYPES][64];  // [piece_type][to_square]
    
    void update(PieceType piece, Square to, Move response) {
        countermoves[piece][to] = response;
    }
    
    Move getCountermove(PieceType piece, Square to) const {
        return countermoves[piece][to];
    }
};
```

**Priority:** Medium - Good complement to existing ordering

#### 2. Continuation History (Stage 22+)
**Expected Gain:** +10-15 ELO

**Description:** Track move pair sequences
- "After I play Nf3, Bg5 is often good"
- More context-aware than simple history
- Used extensively in Stockfish

**Implementation Approach:**
```cpp
class ContinuationHistory {
    // History indexed by [prev_piece][prev_to][curr_piece][curr_to]
    int16_t history[NUM_PIECE_TYPES][64][NUM_PIECE_TYPES][64];
    
    void update(Move prevMove, Move currMove, int bonus) {
        // Update based on move sequence
    }
};
```

**Priority:** Medium-Low - More complex but proven effective

#### 3. Capture History (Stage 23+)
**Expected Gain:** +3-5 ELO

**Description:** Separate history table for captures
- Helps order captures beyond MVV-LVA
- Useful when SEE values are equal
- Tracks which specific captures work well

**Implementation Approach:**
```cpp
class CaptureHistory {
    // Indexed by [attacker_type][victim_type][to_square]
    int16_t history[NUM_PIECE_TYPES][NUM_PIECE_TYPES][64];
};
```

**Priority:** Low - Minor improvement over existing MVV-LVA

#### 4. Threat Detection for LMR (Stage 24+)
**Expected Gain:** Variable (prevents tactical oversights)

**Description:** Identify tactical moves that should resist reduction
- Checks, forks, pins, discovered attacks
- These moves marked as "non-reducible" in LMR
- Prevents LMR from missing critical tactics

**Implementation Approach:**
- Add threat detection during move generation
- Mark threatening moves with special flag
- LMR checks flag before reducing

**Priority:** High (when implementing LMR) - Critical for LMR safety

### LMR Implementation Readiness

**Current State:**
✅ TT move ordering (highest priority)
✅ MVV-LVA for captures  
✅ Killer moves (+31 ELO)
✅ History heuristic (+15-25 ELO expected)
✅ SEE available (but disabled by default)

**Sufficient for LMR:** YES
- Current ordering distinguishes good/bad moves well enough
- LMR can now reduce late moves with confidence
- Expected +20-30 ELO from LMR with current ordering

**Recommended Implementation Order:**
1. Complete history heuristic testing (Stage 20)
2. Implement basic LMR (Stage 21)
3. Add countermove heuristic if needed (Stage 22)
4. Consider continuation history later (Stage 23+)
5. Capture history only if profiling shows need

### Notes on Implementation

**Key Insights:**
- Killer + History provides ~80% of move ordering benefit
- Additional techniques have diminishing returns
- LMR effectiveness depends more on reduction formula than perfect ordering
- Better to implement LMR sooner with good-enough ordering

**Testing Approach:**
- Each addition should be tested independently
- Use SPRT to verify ELO gains
- Monitor time-to-depth impact
- Check for tactical blindness

**Memory Considerations:**
- Countermove: ~2KB (6×64×4 bytes)
- Continuation History: ~1MB (6×64×6×64×2 bytes)
- Capture History: ~24KB (6×6×64×2 bytes)
- All are cache-friendly access patterns

## Game Phase Interpolation for Evaluation Terms

**Date Added:** August 25, 2025  
**Status:** DEFERRED - Interesting concept for future implementation  
**Source:** Analysis of Stash engine evaluation approach  

### Concept Overview:

Instead of discrete game phases (OPENING/MIDDLEGAME/ENDGAME) with hard boundaries, implement smooth interpolation between midgame and endgame evaluation values based on remaining material.

### Implementation Approach:

1. **Paired Score Structure:**
```cpp
struct ScorePair {
    int mg;  // midgame value
    int eg;  // endgame value
};
```

2. **Continuous Phase Calculation:**
```cpp
// Based on material: Queens=4, Rooks=2, Bishops/Knights=1
int phase = popCount(queens) * 4 + popCount(rooks) * 2 + 
            popCount(bishops) + popCount(knights);
// Clamp between 4 (endgame) and 24 (midgame)
```

3. **Linear Interpolation:**
```cpp
int interpolate(ScorePair score, int phase) {
    return (score.mg * (phase - 4) + score.eg * (24 - phase)) / 20;
}
```

### Benefits:
- **Smooth transitions:** No evaluation jumps at phase boundaries
- **More accurate:** Values naturally scale as pieces come off
- **Better tuning:** Can tune midgame/endgame independently for each term

### Example - Knight Outposts:
- Current: Flat 35cp regardless of phase
- With interpolation: 35cp midgame → 30cp endgame
- Reflects reality that outposts are less valuable in simplified positions

### Implementation Considerations:
- Would require refactoring all evaluation terms to use ScorePairs
- Increases complexity but improves accuracy
- Could start with just a few terms (knight outposts, passed pawns) as test

### Why Deferred:
- Current discrete phase system is working adequately
- Mixing with knight outpost feature would complicate testing
- Better to implement as separate feature for clean A/B testing
- Estimated gain: 10-20 ELO from smoother evaluation

### Priority: Medium-Low
- Nice to have but not critical
- Consider after core search improvements complete
- Good candidate for Phase 4+ when fine-tuning evaluation

## Move Ordering Enhancements - Detailed Analysis

**Date Added:** August 25, 2025  
**Status:** ANALYZED - Multiple improvements identified  
**Branch:** feature/analysis/20250825-move-ordering  
**Analysis Document:** /workspace/move_ordering_analysis.md  

### Current Move Ordering Efficiency Problem

**Measured Performance:**
- Depth 6: 86.0% efficiency
- Depth 10: 75.9% efficiency (TARGET: >85%)
- **Degradation:** -10.1 percentage points from shallow to deep depths
- **Impact:** ~30% wasted nodes, causing 1-2 ply depth loss

### Identified Missing Features and Issues

#### 1. Piece-Type History (MISSING - HIGH PRIORITY)

**Current Implementation:**
- Uses only from-square and to-square: `history[side][from][to]`
- Total entries: 2 × 64 × 64 = 8,192 entries
- Cannot distinguish between different pieces moving to same squares

**Proposed Enhancement:**
```cpp
class PieceToHistory {
    // Include piece type for better discrimination
    int16_t history[NUM_COLORS][NUM_PIECE_TYPES][64][64];
    // Or more memory-efficient: [color][piece_type][to_square]
    int16_t history[NUM_COLORS][NUM_PIECE_TYPES][64];
    
    void update(Color side, PieceType piece, Square from, Square to, int bonus) {
        history[side][piece][from][to] += bonus;
        // Age if needed
    }
};
```

**Benefits:**
- Better discrimination between moves (Knight to e5 vs Bishop to e5)
- More accurate history tracking
- Reduced collision in history table

**Implementation Complexity:** Medium
- Requires modifying history table structure
- Update all history access points
- Increase memory usage (6× current for full, 1.5× for compact)

**Expected Gain:** +10-15 ELO

#### 2. Continuation History (MISSING - MEDIUM PRIORITY)

**Description:** Track move pair sequences that work well together

**Current State:** Not implemented at all

**Proposed Implementation:**
```cpp
class ContinuationHistory {
    // Track sequences: "After move A, move B is often good"
    // Indexed by [prev_piece][prev_to][curr_piece][curr_to]
    int16_t history[NUM_PIECE_TYPES][64][NUM_PIECE_TYPES][64];
    
    // Alternative compact version (1-ply continuation)
    int16_t history[NUM_PIECE_TYPES][64][64];  // [prev_piece][prev_to][curr_to]
    
    void update(Move prevMove, Move currMove, int bonus) {
        PieceType prevPiece = getPieceType(prevMove);
        Square prevTo = moveTo(prevMove);
        PieceType currPiece = getPieceType(currMove);
        Square currTo = moveTo(currMove);
        
        history[prevPiece][prevTo][currPiece][currTo] += bonus;
    }
};
```

**Use Cases:**
- "After Nf3, Bg5 is often strong"
- "After e4, d4 follows well"
- Captures context from previous move

**Memory Requirements:**
- Full version: ~1.5MB (6×64×6×64×2 bytes)
- Compact version: ~48KB (6×64×64×2 bytes)

**Implementation Complexity:** High
- Requires tracking move sequences
- Complex aging mechanism needed
- Integration with existing move ordering

**Expected Gain:** +15-20 ELO

#### 3. Capture History (MISSING - LOW PRIORITY)

**Description:** Separate history table specifically for captures

**Current State:** Captures use MVV-LVA only, no historical learning

**Proposed Implementation:**
```cpp
class CaptureHistory {
    // Track which specific captures work well
    // Indexed by [attacker_type][victim_type][to_square]
    int16_t history[NUM_PIECE_TYPES][NUM_PIECE_TYPES][64];
    
    void updateCapture(PieceType attacker, PieceType victim, 
                       Square to, int bonus) {
        history[attacker][victim][to] += bonus;
        // Separate aging from quiet history
    }
    
    int getScore(PieceType attacker, PieceType victim, Square to) {
        return history[attacker][victim][to];
    }
};
```

**Benefits:**
- Learns which captures are actually good beyond material value
- Helps order equal-value captures (e.g., NxB vs BxN)
- Can learn bad capture patterns (e.g., QxP defended by pawn)

**Memory Requirements:** ~24KB (6×6×64×2 bytes)

**Implementation Complexity:** Low
- Simple addition to existing capture ordering
- Easy to integrate with MVV-LVA

**Expected Gain:** +3-5 ELO

#### 4. Statistical Tracking Bug (CONFIRMED BUG)

**Issue:** Countermove hit rate shows >100% (e.g., 114.1% at depth 10)

**Evidence from testing:**
```
info depth 10 ... info string Countermoves: updates=33187 hits=37853 hitRate=114.1%
```

**Root Cause Analysis:**
- Hit count (37,853) exceeds update count (33,187)
- Suggests double-counting or incorrect increment location
- May be counting hits in quiescence but not updates

**Likely Bug Location:**
```cpp
// In negamax.cpp or SearchData struct
// Probably incrementing hits multiple times per node
// Or not incrementing updates in all code paths
```

**Impact:**
- Incorrect statistics mislead optimization efforts
- Cannot trust countermove effectiveness metrics
- May hide actual performance issues

**Fix Required:**
1. Audit all countermove stat increment locations
2. Ensure updates++ happens exactly once per countermove update
3. Ensure hits++ happens exactly once per countermove hit
4. Add assertion: hits <= updates

**Priority:** HIGH - Need accurate statistics for optimization

### Implementation Priority Order

#### Phase 1: Fix Bugs (Immediate)
1. Fix countermove statistics tracking
2. Verify all move ordering statistics are accurate

#### Phase 2: Piece-Type History (Next)
1. Implement piece-to history
2. Test memory vs discrimination tradeoff
3. Validate with SPRT

#### Phase 3: Continuation History (Later)
1. Start with 1-ply continuation (simpler)
2. Test memory impact
3. Consider 2-ply if successful

#### Phase 4: Capture History (Optional)
1. Only if profiling shows bad capture ordering
2. Low priority due to small expected gain

### Testing Strategy

**For each enhancement:**
1. Implement in isolation on separate branch
2. Measure efficiency improvement at depths 6-12
3. Run SPRT with bounds [0.00, 5.00]
4. Check for tactical blindness
5. Monitor memory usage and NPS impact

**Success Criteria:**
- Move ordering efficiency >85% at depth 10
- No tactical regressions
- Memory usage acceptable (<10MB total)
- NPS degradation <5%

### References and Examples

**Stockfish Implementation:**
- Uses all four types of history
- Complex aging with multiple tables
- ~10MB total memory for history tables

**Ethereal Implementation:**
- Simpler piece-to history
- No continuation history
- Still achieves good ordering efficiency

**Our Current Implementation:**
- Basic from-to history only
- Aggressive aging (may be too aggressive)
- No learning from captures

### Notes for Implementation

**Memory Alignment:**
- Ensure cache-line alignment for hot tables
- Consider separating read-heavy and write-heavy data

**Aging Strategies:**
- Current: Divide by 2 when >10% entries near max
- Consider: Gradual decay each iteration
- Alternative: Separate aging for different tables

**Integration Points:**
- Move ordering function in move_ordering.cpp
- History update in negamax.cpp after cutoffs
- Statistics tracking in SearchData struct

## CRITICAL: Static Null Move Pruning - Fundamental Architecture Problem

**Date Added:** August 26, 2025  
**Status:** CRITICAL ISSUE - Imposing ELO ceiling on engine  
**Priority:** HIGH - Must fix to unlock engine's potential  
**Source:** Phase 1.2b implementation attempts (feature/analysis/20250826-pruning-aggressiveness)  
**Evidence:** Two catastrophic failures (-1107 and -1199 ELO) when attempting fixes  

### Executive Summary

SeaJay's static null move pruning is fundamentally broken and unfixable without architectural changes. This is likely costing 30-80 ELO and imposing a hard ceiling on the engine's strength. The issue is not a simple bug but a deep architectural problem with how evaluation, caching, and pruning are coupled.

### The Core Problem

The static null move pruning implementation has multiple interrelated issues that cannot be fixed independently:

1. **Material Balance Check is Backwards**
   ```cpp
   // Line 346 in negamax.cpp - This is WRONG
   if (board.material().balance(board.sideToMove()).value() - beta.value() > -200) {
       // Only evaluates when NOT significantly behind
       // But static null move should trigger when AHEAD!
   }
   ```

2. **Cache Ambiguity Problem**
   ```cpp
   int cachedEval = searchInfo.getStackEntry(ply).staticEval;
   if (cachedEval != 0) {  // 0 means "not cached"
       staticEval = eval::Score(cachedEval);
   }
   // But what if the actual eval IS 0? We can't tell!
   ```

3. **Deeply Coupled Control Flow**
   - Pruning only happens inside evaluation block
   - Evaluation only happens inside material balance check
   - Can't change one without breaking the others

### Why This Is Critical

**Current Performance Loss:**
- Static null move pruning should provide 50-100 ELO
- Current implementation likely provides <20 ELO
- We're missing ~80% of pruning opportunities
- The material balance check PREVENTS pruning when winning

**Comparison with Successful Engines:**
| Engine | Implementation | ELO Gain |
|--------|---------------|----------|
| Stockfish | Simple, early eval | ~80 ELO |
| Laser | Clean, no complex checks | ~70 ELO |
| Publius | Straightforward | ~60 ELO |
| **SeaJay** | **Broken, complex** | **<20 ELO** |

### Evidence of the Problem

1. **Phase 1.2a Success:** Extending depth from 3 to 6 worked (simple change)
2. **Phase 1.2b Catastrophe #1:** Removing material check = -1107 ELO (0 wins in 294 games)
3. **Phase 1.2b Catastrophe #2:** "Proper" fix = -1199 ELO (0 wins in 40 games)

The fact that ANY attempt to fix the material balance check causes complete engine failure proves this is architectural, not a simple bug.

### The Required Solution

#### Option 1: Adopt Publius/Stockfish Pattern (RECOMMENDED)
```cpp
// Evaluate ONCE at node entry (like all successful engines)
eval::Score staticEval = weAreInCheck ? eval::Score(-32000) : board.evaluate();
searchInfo.setStaticEval(ply, staticEval);

// Then for static null move - SIMPLE and CLEAR:
if (!isPvNode && depth <= 6 && !weAreInCheck && 
    std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
    
    eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth);
    if (staticEval - margin >= beta) {
        return staticEval - margin;
    }
}
```

#### Option 2: Fix Cache System First
```cpp
struct CachedEval {
    bool valid = false;
    eval::Score value = eval::Score::zero();
};
```
Then refactor to use proper cache validation.

### Implementation Plan

#### Phase 1: Analysis (2-3 hours)
1. Profile current static null move effectiveness
2. Count how often material balance check prevents pruning
3. Measure actual ELO contribution of current implementation

#### Phase 2: Architectural Refactor (8-10 hours)
1. Create feature branch for major refactor
2. Implement early evaluation pattern
3. Remove complex nested conditions
4. Simplify cache system
5. Extensive testing at each step

#### Phase 3: Validation (4-5 hours)
1. Ensure no behavioral changes initially
2. Then fix the material balance logic
3. SPRT testing with bounds [30, 80]
4. Expect significant ELO gain

### Risk Assessment

**If We Don't Fix This:**
- Engine is stuck with ~20 ELO from broken static null move
- Missing 30-80 ELO that competitors have
- Cannot implement other pruning techniques properly
- Permanent ceiling on engine strength

**If We Do Fix This:**
- Expect 30-80 ELO immediate gain
- Unlock ability to add futility pruning (+20 ELO)
- Enable proper razoring (+10 ELO)
- Remove complexity that causes bugs

### Why Previous Attempts Failed

Both attempts failed because they tried to fix the problem locally without addressing the architectural issues:

1. **Attempt 1:** Removed material check but broke control flow
2. **Attempt 2:** Fixed control flow but cache ambiguity caused invalid pruning

The ONLY way to fix this is architectural refactoring.

### Temporary Workaround

Until this is fixed:
- Keep the broken implementation (at least it doesn't crash)
- Focus on features that don't depend on evaluation architecture
- Document all pruning-related features as blocked

### Success Criteria

1. Static null move triggers when position is good (not when behind)
2. Clean, understandable code matching successful engines
3. 30-80 ELO gain in SPRT testing
4. Ability to add other pruning techniques easily

### References

1. `/workspace/phase_1_2b_complete_analysis.md` - Detailed failure analysis
2. `/workspace/static_null_comparison.md` - Comparison with working engines
3. OpenBench tests 214 and 215 - Catastrophic failures
4. Publius source - Example of correct implementation

### Priority: CRITICAL

This is not a "nice to have" - it's a fundamental issue that:
- Imposes a hard ceiling on engine strength
- Blocks implementation of other features
- Makes the codebase fragile and error-prone
- Costs 30-80 ELO compared to proper implementation

**This should be the next major work item after current testing completes.**

## Items FROM Stage 18 (LMR) - CRITICAL ISSUES

**Date:** August 20, 2025  
**Status:** IN PROGRESS - CRITICAL ISSUES IDENTIFIED  
**Branch:** feature/20250819-lmr  

### Critical Issues Discovered:

1. **No Quiet Move Ordering** (ROOT CAUSE)
   - **Issue:** Quiet moves have NO ordering (score = 0 in MVV-LVA)
   - **Impact:** LMR reduces essentially random quiet moves
   - **Solution:** MUST implement history heuristic before LMR can work
   - **Severity:** CRITICAL - makes LMR ineffective

2. **Wrong Re-search Condition** (BUG)
   - **Current:** `if (score > alpha)` - INCORRECT!
   - **Correct:** `if (score > alpha && score < beta)`
   - **Impact:** Re-searching at wrong times
   - **Solution:** Fix the condition in negamax.cpp

3. **Sequencing Error** (PLANNING MISTAKE)
   - **What happened:** LMR implemented before quiet move ordering
   - **Should have been:** History → Killers → LMR
   - **Was actually:** LMR → (nothing for quiet moves)
   - **Impact:** -10 ELO despite 91% node reduction

### Option 1 - Immediate Critical Fix (RECOMMENDED):

**Step 1: Fix re-search condition bug**
```cpp
// In negamax.cpp around line 388-398
if (reduction > 0) {
    eval::Score nullBeta = alpha + eval::Score(1);
    score = -negamax(board, depth - 1 - reduction, ply + 1,
                    -nullBeta, -alpha, searchInfo, info, limits, tt);
    
    // FIXED: Check both bounds!
    if (score > alpha && score < beta) {
        info.lmrStats.reSearches++;
        score = -negamax(board, depth - 1, ply + 1,
                        -beta, -alpha, searchInfo, info, limits, tt);
    } else {
        info.lmrStats.successfulReductions++;
    }
}
```

**Step 2: Make LMR less aggressive**
- Change minMoveNumber from 4 to 6 or 8
- Only reduce very late moves until history heuristic exists

**Step 3: Test and validate**
- Commit fixes with bench
- Run OpenBench test
- Expected: Should at least not lose ELO

### Options for Moving Forward:

1. **Option 1:** Immediate fix + retest (5 minutes work, might salvage some ELO)
2. **Option 2:** Disable LMR, implement history heuristic first (correct sequence)
3. **Option 3:** Full Phase 4 - Add history heuristic + fix bugs (most thorough)
4. **Option 4:** Ultra-conservative LMR with minMoveNumber=10 (minimal risk)

### Lessons Learned:

1. **Move ordering is CRITICAL for LMR** - cannot work with random quiet moves
2. **Sequencing matters** - features have dependencies
3. **Stage 11 was correct** - MVV-LVA properly scoped to captures only
4. **Planning error** - didn't recognize LMR dependency on quiet move ordering
5. **Binary size is a clue** - 27KB difference revealed missing features

### What Should Have Been:

**Correct Stage Sequence:**
- Stage 11: MVV-LVA (captures) ✅
- Stage 12: Transposition Tables ✅
- Stage 13: Iterative Deepening ✅
- Stage 14: Quiescence Search ✅
- Stage 15: SEE ✅
- Stage 16: Enhanced Quiescence
- Stage 17: Null Move Pruning
- **Stage 18: History Heuristic** ← Should be here
- **Stage 19: Killer Moves** ← Then this
- **Stage 20: LMR** ← THEN LMR
- Stage 21: Aspiration Windows
- Stage 22: Other optimizations

## Search Stack Static Eval Sentinel Issue (DEFERRED FROM Phase 4.2.c)

**Date Added:** August 31, 2025  
**Status:** DEFERRED - Lower priority architectural improvement needed  
**Source:** Expert review of Phase 4.2.c TT static eval storage implementation  
**Priority:** MEDIUM - Affects improving position detection accuracy  

### The Problem

The search stack uses 0 as a sentinel value to indicate "no static eval stored", but 0 is a legitimate evaluation value. This creates ambiguity that can affect search quality, particularly for improving position detection used in various pruning decisions.

### Current Implementation Issues

1. **Zero-as-Sentinel Ambiguity**
   ```cpp
   // In SearchStack structure
   int staticEval;  // 0 means "unknown" but could also be legitimate eval
   
   // In improving position detection
   if (prevEval != 0 && currEval != 0) {  // Can't distinguish 0 eval from "not set"
       bool improving = currEval > prevEval;
   }
   ```

2. **Impact on Pruning Decisions**
   - Futility pruning uses improving status to adjust margins
   - LMR uses improving to determine reduction amounts
   - Null move pruning may use improving for decisions
   - When static eval legitimately equals 0, these features malfunction

3. **Comparison with TT Fix**
   - Same issue existed in TT (fixed in Phase 4.2.c with TT_EVAL_NONE sentinel)
   - TT now uses INT16_MIN as sentinel, clearly distinguishing from real evals
   - Search stack still has the ambiguity problem

### Recommended Solution

#### Option 1: Use Sentinel Value (Preferred)
```cpp
// In search_info.h or wherever SearchStack is defined
struct SearchStackEntry {
    static constexpr int EVAL_NONE = std::numeric_limits<int>::min();
    
    int staticEval = EVAL_NONE;  // Clear sentinel value
    // ... other members
    
    bool hasStaticEval() const { 
        return staticEval != EVAL_NONE; 
    }
    
    void setStaticEval(int eval) {
        staticEval = eval;  // Can now store 0 legitimately
    }
};

// In negamax.cpp for improving detection
int prevEval = searchInfo.getStackEntry(ply - 2).staticEval;
int currEval = searchInfo.getStackEntry(ply).staticEval;

if (searchInfo.getStackEntry(ply - 2).hasStaticEval() && 
    searchInfo.getStackEntry(ply).hasStaticEval()) {
    bool improving = currEval > prevEval;  // Now reliable even when eval is 0
    // Use improving for pruning decisions
}
```

#### Option 2: Add Explicit Flag
```cpp
struct SearchStackEntry {
    int staticEval = 0;
    bool staticEvalValid = false;  // Explicit validity flag
    
    void setStaticEval(int eval) {
        staticEval = eval;
        staticEvalValid = true;
    }
    
    bool hasStaticEval() const { 
        return staticEvalValid; 
    }
};
```

### Why This Was Deferred

1. **Lower Impact Than Other Fixes**
   - TT sentinel bug was critical (affected eval reuse)
   - Quiescence pollution was storing invalid data
   - This issue is more subtle, affecting edge cases

2. **Requires Broader Changes**
   - Need to update all places that check/set static eval
   - Must modify improving detection logic throughout
   - Risk of introducing bugs in working code

3. **Current Workaround Exists**
   - Positions with true 0 eval are relatively rare
   - Most positions have non-zero evaluation
   - Impact estimated at 1-3 ELO maximum

### Implementation Considerations

1. **Memory Impact**
   - Sentinel approach: No additional memory
   - Flag approach: 1 extra byte per search stack entry
   - Search stack is small (MAX_PLY entries), so impact minimal

2. **Code Changes Required**
   - Update SearchStack/SearchInfo structure definition
   - Modify all static eval setters to use new method
   - Update improving detection in negamax.cpp
   - Update any other code checking for static eval presence
   - Add assertions to catch misuse

3. **Testing Requirements**
   - Verify improving detection works correctly with 0 evals
   - Test positions with material balance exactly 0
   - Check pruning decisions in balanced positions
   - Run SPRT to ensure no regression

### When to Implement

**Recommended Timeline:**
- After current Phase 4 optimizations are complete and stable
- Before implementing features that heavily rely on improving detection
- Could be bundled with other search stack improvements

**Triggers for Implementation:**
- If profiling shows many positions with 0 eval
- If new pruning technique needs accurate improving detection
- As part of general search stack cleanup/refactoring

### Expected Benefits

- **Correctness**: Accurate improving detection in all positions
- **Reliability**: No edge cases with 0 evaluations
- **Consistency**: Matches approach used in TT (already fixed)
- **Future-proofing**: Enables more sophisticated pruning techniques
- **Estimated ELO**: 1-3 ELO from better pruning decisions

### References

1. **Expert Review**: Phase 4.2.c feedback identified this issue
2. **TT Fix Example**: See commit 0aa0f4a for similar fix in TT
3. **Current Code**: 
   - Search stack: `/workspace/src/search/search_info.h`
   - Improving detection: `/workspace/src/search/negamax.cpp`
4. **Similar Engines**: Stockfish uses explicit "value is valid" tracking

### Risk Assessment

**Low Risk:**
- Well-understood problem with clear solution
- Similar fix already successful in TT
- Can be implemented incrementally
- Easy to test and validate

**Potential Issues:**
- Must ensure all code paths updated
- Need to handle uninitialized stack entries
- Backward compatibility with existing code

### Success Criteria

1. Zero evaluations correctly distinguished from "not set"
2. Improving detection accurate for all eval values
3. No performance regression in SPRT testing
4. Clean, maintainable code matching TT approach
5. All tests pass including edge cases

## Phase 4.3.a - Counter-Move History (CMH) - DEFERRED

**Date Added:** August 31, 2025  
**Status:** IMPLEMENTED BUT DISABLED - No ELO gain demonstrated  
**Source:** Phase 4.3.a implementation and extensive testing  
**Priority:** LOW - Revisit only after engine reaches higher strength  

### Summary

Counter-Move History was fully implemented with multiple optimization rounds based on expert feedback:
- Fixed double decay issue (decay only on positive updates)
- Implemented pure integer arithmetic to avoid float overhead
- Added overflow prevention with int32_t scoring
- Increased depth gate from 4 to 6 for better time control performance
- Tested with weights 0.0, 1.0, and 1.5

### Testing Results

Despite extensive optimization and testing:
- **Weight 1.5:** -6 ELO regression
- **Weight 1.0:** +5-7 ELO initially, but ultimately neutral in extended testing
- **Weight 0.0:** Best performance (CMH disabled)
- **Depth gate 6:** No improvement over disabled state

### Likely Causes

1. **Time Control Sensitivity:** CMH tables need time to warm up, may only show benefit at longer time controls
2. **Engine Strength:** At ~2200 ELO, SeaJay may not generate consistent enough move patterns for CMH to learn effectively
3. **Table Pollution:** Fast time controls don't provide enough samples for meaningful statistics
4. **Implementation Overhead:** Even with optimizations, overhead exceeds benefit at current strength

### Future Considerations

**When to Revisit:**
- When engine reaches 2400+ ELO
- When testing at longer time controls (60+0 or slower)
- After improving base move ordering accuracy
- If implementing SMP (CMH benefits from cross-thread learning)

**Implementation Notes:**
- Code is complete and working in move_ordering.cpp and countermove_history.h
- UCI parameter counterMoveHistoryWeight defaults to 0.0 (disabled)
- Can be re-enabled for testing via UCI: `setoption name CounterMoveHistoryWeight value 1.0`
- All optimizations from expert review are implemented and can be reactivated

### Technical Details Preserved

The implementation includes:
- Proper integer arithmetic (3/2 scaling for 1.5x weight)
- Overflow prevention with int32_t intermediate values
- Depth gating at depth >= 6
- Double decay fix (only on positive updates)
- Pre-computed scoring for efficiency

All code remains in place for future testing when conditions are more favorable.

## PST Phase Interpolation - Phase Weight Tuning (DEFERRED)

**Date Added:** August 31, 2025  
**Status:** DEFERRED - Phase 1 implementation complete and working  
**Source:** PST phase interpolation implementation (feature/20250831-pst-interpolation)  
**Priority:** MEDIUM - Could provide additional 5-10 ELO  

### Summary

PST phase interpolation has been successfully implemented with:
- Continuous phase calculation using material weights (N=1, B=1, R=2, Q=4)
- Fixed-point arithmetic interpolation between mg/eg values
- Differentiated endgame PST values for all piece types
- UCI option UsePSTInterpolation (default: true)

### SPSA Tuning Opportunities

**Phase Weights for Material-Based Calculation:**
```cpp
// Current implementation (not tuned)
constexpr int PHASE_WEIGHT[6] = { 0, 1, 1, 2, 4, 0 };  // P, N, B, R, Q, K
```

**Tuning Parameters:**
- Knight weight: Currently 1, range [1-3]
- Bishop weight: Currently 1, range [1-3]  
- Rook weight: Currently 2, range [2-5]
- Queen weight: Currently 4, range [3-6]

**SPSA Configuration for OpenBench:**
```
KnightPhaseWeight, int, 1.0, 1.0, 3.0, 0.5, 0.002
BishopPhaseWeight, int, 1.0, 1.0, 3.0, 0.5, 0.002
RookPhaseWeight, int, 2.0, 2.0, 5.0, 0.5, 0.002
QueenPhaseWeight, int, 4.0, 3.0, 6.0, 0.5, 0.002
```

### Why Deferred

1. **Phase 1 Already Successful:** Initial implementation with default weights expected to pass SPRT
2. **Diminishing Returns:** Fine-tuning weights provides smaller gains than implementing new features
3. **Testing Resources:** Better to use OpenBench for testing new features first
4. **Current Weights Reasonable:** Based on traditional material values, likely close to optimal

### When to Implement

**Recommended Timeline:**
- After core search/evaluation features are complete
- When looking for incremental improvements via tuning
- Could be combined with other SPSA tuning sessions

**Expected Gain:** 5-10 ELO from optimal phase weights

### Implementation Notes

Would require:
1. Making phase weights configurable via UCI
2. Modifying phase0to256() function to use UCI parameters
3. SPSA tuning session with 40,000-50,000 games
4. Validation that interpolation still works correctly with new weights

## PV Tracking in Quiescence Search (DEFERRED)

**Date Added:** September 1, 2025  
**Status:** DEFERRED - Nice-to-have quality improvement  
**Source:** Code review and discussion during futility pruning work  
**Priority:** LOW - No strength impact, only usability improvement  

### Summary

SeaJay currently does NOT track the Principal Variation (PV) in quiescence search. When entering quiescence from a PV node, tactical sequences found (like forcing captures or checks) are not added to the PV display.

### Current State

- **Negamax** has PV tracking via `TriangularPV* pv` parameter
- **Quiescence** does NOT have a PV parameter
- When negamax calls quiescence at depth 0, the PV cannot be extended
- Tactical sequences in quiescence are not visible to users

### Implementation Required

```cpp
// Add PV parameter to quiescence function signature
eval::Score quiescence(
    Board& board,
    int ply,
    eval::Score alpha,
    eval::Score beta,
    SearchInfo& searchInfo,
    SearchData& data,
    const SearchLimits& limits,
    TranspositionTable& tt,
    TriangularPV* pv,      // ADD THIS
    bool isPvNode,         // ADD THIS
    int checkPly,
    bool inPanicMode)

// Update PV when finding best move in quiescence
if (score > alpha) {
    alpha = score;
    if (pv && isPvNode) {
        pv->updatePV(ply, move);
    }
}

// Update negamax call to quiescence to pass PV
return quiescence(board, ply, alpha, beta, searchInfo, info, limits, *tt, 
                  pv, isPvNode, 0, inPanicMode);
```

### Benefits

- **Complete PV reporting**: Shows tactical continuations beyond last full-width move
- **Better debugging**: Can see what tactical sequence engine is planning
- **User experience**: More informative output in GUIs
- **Standard practice**: Most strong engines track PV through quiescence

### Why Deferred

1. **Zero strength impact**: This is purely cosmetic/informational
2. **Current PV adequate**: Main search PV shows the critical moves
3. **Low priority**: Many higher-impact features to implement first
4. **Small overhead**: Minor performance cost for PV updates

### When to Implement

- When polishing the engine for release
- If users request more detailed PV output
- When debugging tactical issues becomes difficult
- As part of general code cleanup/standardization

### Expected Effort

- 1-2 hours to implement and test
- Simple mechanical changes to function signatures
- Need to update all quiescence call sites
- Testing to ensure PV correctly concatenates search and quiescence moves

