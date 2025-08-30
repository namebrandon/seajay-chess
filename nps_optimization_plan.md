# NPS Optimization Plan for SeaJay Chess Engine

## Problem Statement
SeaJay is running at **477K nps** while competitor engines achieve:
- Stash: **1.6M nps** (3.4x faster)
- Komodo: **1.5M nps** (3.1x faster)  
- Laser: **1.1M nps** (2.4x faster)

## Critical Development Constraints

### 1. Build System Documentation
📖 **REQUIRED READING: /workspace/docs/BUILD_SYSTEM.md**
- Contains essential build system information
- Details auto-detection of CPU capabilities
- Explains Makefile vs CMake usage
- Documents recent optimization improvements

### 2. Instruction Set Auto-Detection
⚠️ **Build System Uses `-march=native` for Auto-Detection**
- Makefile and CMake now use `-march=native` (see BUILD_SYSTEM.md)
- Automatically detects and uses best available CPU instructions
- Development environment limited to SSE 4.2, but this is handled automatically
- **For local builds**: `rm -rf build && ./build.sh`
- **OpenBench builds**: Uses Makefile with auto-detection

### 3. Build System Requirements
⚠️ **OpenBench Integration**
- OpenBench uses `make` to build from remote commits
- **ANY build optimizations MUST be added to Makefile**
- CMakeLists.txt changes alone won't affect OpenBench testing
- Both build systems must stay synchronized

### 3. Commit Requirements
⚠️ **MANDATORY: bench in commit messages**
```bash
# Get bench count:
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'

# Commit format:
git commit -m "feat: Description here

Detailed explanation...

bench 19191913"  # EXACT format required
```

### 4. Testing Protocol
⚠️ **STOP-AND-TEST after each phase**
- Commit and push each phase separately
- Wait for human to run SPRT test on OpenBench
- Human provides results before proceeding
- No proceeding without test confirmation

### 5. Thread Safety Requirements
⚠️ **CRITICAL: Design for Future LazySMP Multi-Threading**
- **ALL optimizations MUST be thread-safe** - no exceptions
- **No global mutable state** without atomic operations or mutexes
- **Prefer thread-local storage** for per-thread data
- **Avoid changes that complicate parallelization**
- **Consider data races** - multiple threads will search simultaneously
- **Board state must be copyable** - each thread needs independent state
- **Shared structures** (TT, history tables) need synchronization design
- **Cache-friendly access patterns** - avoid false sharing between threads

**LazySMP Requirements:**
- Each thread will run independent search from root position
- Threads share: transposition table, history heuristics
- Threads DON'T share: board state, move stacks, search stacks
- Optimizations should not introduce thread dependencies

---

## Phase-by-Phase Implementation Plan

### Phase 0: Baseline Analysis (No Code Changes)
**Goal**: Understand current performance characteristics

1. **Check competitor instruction sets**
   ```bash
   for engine in stash komodo-14.1-linux laser; do
     echo "=== $engine ==="
     objdump -d /workspace/external/engines/*/$engine | grep -c -E "(avx|bmi|pext|pdep)" || echo "No advanced instructions"
   done
   ```

2. **Profile current build**
   ```bash
   rm -rf build
   ./build.sh Release
   # Use the slow search position for consistent NPS measurement
   echo -e "position fen r2q1rk1/ppp1bppp/2np1n2/4p3/2B1P3/2NP1Q1P/PPP2PP1/R1B2RK1 b - - 0 9\ngo depth 14\nquit" | ./bin/seajay
   ```

3. **Component timing analysis**
   ```bash
   # Perft for move generation speed
   echo -e "position startpos\ngo perft 6\nquit" | ./bin/seajay
   
   # Create analysis script for the slow position
   echo -e "position fen r2q1rk1/ppp1bppp/2np1n2/4p3/2B1P3/2NP1Q1P/PPP2PP1/R1B2RK1 b - - 0 9\ngo depth 14\nquit" > slow_pos.txt
   time ./bin/seajay < slow_pos.txt
   ```

**Deliverable**: Performance baseline report
**No commit needed** (analysis only)

---

### Phase 1: Profiling and Hotspot Analysis
**Goal**: Identify specific code bottlenecks and potential bugs

1. **Build with profiling support**
   ```bash
   rm -rf build
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
   make -j seajay
   cd ..
   ```

2. **Run perf analysis on slow position**
   ```bash
   echo -e "position fen r2q1rk1/ppp1bppp/2np1n2/4p3/2B1P3/2NP1Q1P/PPP2PP1/R1B2RK1 b - - 0 9\ngo depth 14\nquit" > slow_pos.txt
   sudo perf record -g ./bin/seajay < slow_pos.txt
   sudo perf report > perf_report.txt
   ```

3. **Identify top hotspots**
   ```bash
   rm -rf build
   ./build.sh Release
   # Test with slow search position for NPS comparison
   echo -e "position fen r2q1rk1/ppp1bppp/2np1n2/4p3/2B1P3/2NP1Q1P/PPP2PP1/R1B2RK1 b - - 0 9\ngo depth 14\nquit" | ./bin/seajay
   # Also get bench count for commit message
   echo "bench" | ./bin/seajay | grep "Benchmark complete"
   ```

**Commit checkpoint**:
```bash
git add Makefile CMakeLists.txt
git commit -m "build: Optimize compiler flags for SSE 4.2 environments

- Add explicit SSE 4.2 and POPCNT support
- Enable O3 optimization level
- Add link-time optimization
- Synchronize Makefile and CMake flags

bench [ACTUAL_COUNT]"
git push
```

**WAIT FOR HUMAN**: SPRT test results before proceeding

---

### Phase 2: Board Representation & Attack Detection Optimization
**Goal**: Address the top bottlenecks identified in profiling

**Git Branch Strategy**:
- Each sub-phase gets its own feature branch from main
- Merge to main only after successful SPRT test
- Clean isolation of each optimization attempt

Based on Phase 1 profiling, we'll tackle each bottleneck in separate sub-phases:

#### Phase 2.1: isSquareAttacked Optimization (12.83% runtime)
**Branch**: `feature/20250829-attack-detection`
**Current**: 14.8M calls consuming 12.83% of runtime
**Target**: Reduce by 50% through caching or algorithm improvements

**Thread Safety Considerations**:
- Any caching must be per-thread or use atomic operations
- Attack maps cannot be global mutable state
- Incremental updates must not create race conditions
- Pre-computed patterns must be read-only or thread-local

##### Phase 2.1.a: Algorithmic Optimizations (Quick Wins)
**Goal**: Optimize attack detection algorithm without architectural changes
**Expected Gain**: 1.5-2x speedup of isSquareAttacked function

**Implementation Details**:
1. **Reorder piece type checks** for early exit:
   - Knights first (most common attackers in middlegame)
   - Pawns second (numerous and simple attacks)
   - Queens (combined bishop+rook attacks)
   - Bishops and Rooks separately only if no queen hits
   - King last (least likely attacker)

2. **Optimize queen handling**:
   - Compute bishop+rook attacks once for queens
   - Avoid duplicate sliding piece calculations
   - Cache occupancy bitboard calculation

3. **Early exit optimizations**:
   - Return immediately on first attacker found
   - Skip piece types with no pieces on board
   - Check piece existence before attack generation

4. **Remove runtime configuration overhead**:
   - Eliminate useMagicBitboards runtime checks in hot path
   - Use compile-time selection or direct calls

**Testing Requirements**:
- Local NPS measurement before/after
- Perft validation for correctness
- Bench count for commit
- SPRT test with bounds [-3.00, 3.00]
- **STOP**: Wait for human SPRT approval before proceeding to 2.1.b

##### Phase 2.1.b: Thread-Safe Attack Caching
**Goal**: Implement position-based attack caching with thread-local storage
**Expected Gain**: 3-5x speedup with good cache hit rates (combined with 2.1.a)
**Prerequisite**: Phase 2.1.a must pass SPRT

**Implementation Details**:
1. **Thread-local cache structure**:
   - Small cache size (16 entries) for L1 cache friendliness
   - Cache line aligned entries (64 bytes)
   - LRU or simple hash replacement policy
   - Store attacks for both colors per entry

2. **Cache entry design**:
   ```cpp
   struct CacheEntry {
       Hash zobristKey;
       Bitboard attackedByWhite;
       Bitboard attackedByBlack;
       uint8_t age;  // For replacement
   };
   ```

3. **Cache integration**:
   - Use thread_local storage for zero synchronization overhead
   - Hash position zobrist key for cache indexing
   - Compute both colors' attacks on cache miss (amortize cost)
   - Add statistics tracking for hit rates (debug builds)

4. **Memory considerations**:
   - Total memory: 16 entries * 64 bytes = 1KB per thread
   - Cache-line aligned to prevent false sharing
   - Consider NUMA awareness for large systems

**Testing Requirements**:
- Measure cache hit rates on typical positions
- Verify thread safety with concurrent searches
- Compare NPS improvement over 2.1.a baseline
- Full perft validation
- SPRT test with bounds [0.00, 5.00] (expecting positive gain)
- **STOP**: Wait for human SPRT approval before proceeding to Phase 2.2

**Expected Combined Impact of 2.1.a + 2.1.b**:
- Function speedup: 5-8x
- Overall NPS improvement: 15-25% (500K → 600-625K nps)
- Addresses the #1 performance bottleneck

#### Phase 2.2: makeMoveInternal Optimization - **ABANDONED** ❌
**Branch**: `feature/20250829-make-unmake-opt`
**Current**: 14.1M calls consuming 12.83% of runtime
**Target**: Was targeting 30-40% reduction
**Status**: **ABANDONED** - Multiple optimization attempts failed

**Results Summary**:
- **Phase 2.2.a (Safe optimizations)**: -11.75 ELO loss, no NPS gain
- **Phase 2.2.b (Remove fifty-move zobrist)**: -4.5% NPS decrease
- **Phase 2.2.c-f**: Deferred due to consistent failures

**Conclusion**: Compiler already highly optimized with -O3 -flto. Small changes 
cause negative interactions with CPU pipelining, cache behavior, and TT efficiency.

#### Phase 2.3: updateBitboards Optimization (10.57% runtime)
**Branch**: `feature/20250829-bitboard-updates`
**Current**: 60M calls consuming 10.57% of runtime
**Target**: Inline or optimize the hot path
**Approaches**:
- Option A: Force inline the function
- Option B: Use branchless bit manipulation
- Option C: Combine updates to reduce function calls
- Option D: Template specialization for common cases

**Status**: **IN PROGRESS**

**Phase 2.3.a Results**: ✅ **SUCCESS**
- **Commit**: de320d6
- **Changes**: Moved updateBitboards to header for inlining, split into add/remove functions
- **NPS**: 527K → 567K (7.6% improvement)
- **ELO**: +10-12 (SPRT confirmed)
- **Bench**: 19191913

**Phase 2.3.b Results**: ✅ **SUCCESS** 
- **Commit**: 14e628c
- **Changes**: Added movePieceInBitboards() for non-captures, prefetch hints
- **NPS**: 567K → 618K (9% additional, 17.3% total)
- **ELO**: +3.93 ± 7.82 (SPRT passed [0.00, 5.00])
- **Bench**: 19191913

**Phase 2.3.c Status**: ❌ **ABANDONED - FAILED PERFT**
- **Changes Attempted**:
  - Added `updateCastlingBitboards()` - optimized castling with XOR operations
  - Added `updateEnPassantBitboards()` - optimized en passant captures  
  - Added `unmakeEnPassantBitboards()` - optimized en passant unmake
  
- **Result**: Bench changed from 19191913 to 18892045 (INCORRECT)
- **Issue**: Logic error in specialized castling/en-passant functions
- **Decision**: Reverted all Phase 2.3.c changes, keeping only 2.3.a and 2.3.b
- **Conclusion**: The specialized functions for castling and en passant moves
  introduced subtle bugs. The complexity of handling these special moves
  makes them error-prone for optimization. The gains from 2.3.a and 2.3.b
  (17.3% NPS improvement, +14 ELO combined) are sufficient.

**Final Phase 2.3 Results**:
- **Total NPS Gain**: 527K → 618K (17.3% improvement)
- **Total ELO Gain**: ~14 ELO (10-12 from 2.3.a, 3.93 from 2.3.b)
- **Commits**: de320d6 (2.3.a), 14e628c (2.3.b)
- **Branch**: feature/20250829-bitboard-updates
- **Status**: COMPLETE (2.3.a and 2.3.b merged, 2.3.c abandoned)

#### Phase 2.4: HistoryHeuristic Access Optimization (7.92% runtime)
**Branch**: `feature/20250829-history-cache`
**Current**: 73.7M calls consuming 7.92% of runtime
**Target**: Improve cache locality and access patterns
**Approaches**:
- Option A: Optimize array layout for cache lines
- Option B: Reduce array dimensions if possible
- Option C: Use smaller data types to improve cache usage
- Option D: Consider compression or sparse representation

#### Phase 2.5: Evaluation Function Optimization (7.92% runtime)

**Phase 2.5.a Update (ABANDONED)**: Lazy evaluation was implemented with full UCI control but abandoned due to:
- ELO loss even with conservative settings (-6.75 ELO at 700cp threshold)
- Complex interdependencies with search (q-search, null move, futility pruning, TT)
- Minimal NPS gains (1-4%) on balanced positions
- Search accuracy more important than raw NPS for engine strength
- Decision: Focus on optimizations that don't affect search quality

### Phase 2.5: Evaluation Function Optimization (7.92% runtime)
**Branch**: `feature/20250830-eval-speedup`
**Current**: 1.5M calls consuming 7.92% of runtime
**Target**: 30-40% evaluation speedup without losing strength
**Expected NPS Impact**: +50-80K (evaluation is 7.92% of runtime)

##### Phase 2.5.a: UCI-Controlled Lazy Evaluation (NEW APPROACH)
**Goal**: Implement flexible lazy evaluation with UCI options for safe testing
**Branch**: `feature/20250830-lazy-eval-uci` (separate from other eval optimizations)
**Rationale**: Lazy evaluation is contentious - UCI control allows A/B testing without risk

**Implementation Strategy**:
1. **Phase 2.5.a-1**: Infrastructure with UCI options (default OFF)
   - Add `LazyEval` boolean UCI option (default: false)
   - Add `LazyEvalThreshold` integer UCI option (default: 500, range: 100-1000)
   - Implement basic material-only early exit when enabled
   - **Expected**: No regression (feature disabled by default)
   - **SPRT**: [-3.00, 3.00] to confirm no impact when OFF

2. **Phase 2.5.a-2**: Testing with option enabled
   - Test with LazyEval=true in OpenBench
   - Compare NPS gain vs ELO impact
   - Document tradeoffs for different time controls
   - **Expected**: 15-25% NPS gain, 0-10 ELO loss
   - **SPRT**: Multiple tests with different bounds

3. **Phase 2.5.a-3**: Threshold tuning (if base implementation successful)
   - Test thresholds: 300, 500, 700, 900 centipawns
   - Find optimal balance for SeaJay
   - Consider different thresholds for different game phases
   - **Expected**: Find sweet spot for NPS/ELO tradeoff

**Benefits of UCI Approach**:
- Safe to merge even if slightly negative ELO (users can disable)
- Allows real-world testing and feedback
- Can tune per hardware/time control
- No risk to existing strength when disabled
- Enables data collection on effectiveness

**Success Criteria**:
- Phase 1: Merges with no regression (option OFF)
- Phase 2: Documents clear NPS/ELO tradeoff
- Phase 3: Finds optimal threshold if viable
- Decision: Enable by default only if NPS gain > 20% with < 5 ELO loss

##### Phase 2.5.b: Remove Redundant Calculations (COMPLETED) ✅
**Goal**: Eliminate duplicate computations
**Implementation**:
- Cache game phase detection (currently called 3+ times)
- Reuse king square lookups throughout eval
- Eliminate duplicate bitboard operations
- Store frequently accessed values in locals
**Results**: 
- **NPS**: 560K → 575K (2.7% improvement)
- **Commit**: 38edb70
- **Status**: Awaiting SPRT results for merge to main

##### Phase 2.5.c: Progressive Lazy Evaluation (UCI-Controlled)
**Goal**: Implement staged evaluation with early exits (builds on 2.5.a)
**Prerequisite**: Phase 2.5.a must show promise
**Implementation**:
- Extend LazyEval UCI option to support staged evaluation
- Add `LazyEvalStages` UCI option (1-3 stages)
- Stage 1: Material + PST + basic pawn structure
- Stage 2: Add mobility if position unclear
- Stage 3: Add king safety only if needed
- Early exit thresholds between stages
**Expected**: 20-30% overall evaluation speedup when enabled
**Risk**: Medium - needs careful threshold tuning
**Note**: Only implement if basic lazy eval (2.5.a) shows positive results

##### Phase 2.5.d: Endgame-Aware Optimization
**Goal**: Skip irrelevant features based on game phase
**Implementation**:
- Skip king safety in endgame
- Skip passed pawn race logic in opening
- Phase-specific evaluation paths
**Expected**: 10% additional speedup
**Risk**: Low - logical optimization

##### Phase 2.5.e: SIMD Optimization (High Priority)
**Branch**: `feature/20250830-simd-optimization`
**Goal**: Vectorize hot evaluation and board operations using SSE4.2/AVX2
**Status**: **IN PROGRESS** - Initial analysis complete

**Implementation Strategy**:
1. **Phase 2.5.e-1: SSE4.2 Popcount Batching**
   - Target: Material counting in board.cpp (lines 320-335)
   - Batch multiple `std::popcount()` calls with SIMD
   - Use SSE4.2 for local dev, AVX2 path for OpenBench
   - Runtime CPU detection for optimal instruction set
   - **Expected**: 2-3x speedup on piece counting

2. **Phase 2.5.e-2: Vectorized Pawn Structure Evaluation**
   - Process multiple files simultaneously with SIMD
   - Parallel isolated/doubled/backward pawn detection
   - Vectorized passed pawn mask operations
   - **Expected**: 30-40% speedup on pawn evaluation

3. **Phase 2.5.e-3: Parallel Mobility Counting**
   - SIMD-accelerated bitboard iteration
   - Batch process multiple pieces' mobility
   - Vectorized attack generation helpers
   - **Expected**: 20-30% speedup on mobility evaluation

**Thread Safety & LazySMP Compatibility**:
- ✅ SIMD operations are inherently thread-safe (register-only)
- ✅ No shared memory access or synchronization needed
- ✅ Each thread gets independent SIMD register state
- ✅ Zero cache coherency issues
- ✅ Actually improves with multi-threading (better CPU utilization)

**Build System Considerations**:
- Local dev environment: SSE4.2 only (confirmed via gcc -march=native)
- OpenBench servers: AVX2/BMI2 available
- Use runtime CPU detection with function pointers
- Compile both SSE4.2 and AVX2 paths with preprocessor guards

**Expected Overall Impact**:
- **10-20% NPS improvement** on evaluation hot paths
- **No accuracy loss** - exact same results as scalar code
- **Better than LazyEval** - pure performance gain without search impact
- **LazySMP-ready** - perfect scaling with thread count

**Risk**: Low-Medium - well-understood optimization technique
**Priority**: HIGH - Significant NPS gains without ELO risk

**Git Workflow for Each Sub-Phase**:
1. **HUMAN**: Merge current branch to main (if successful)
2. Create new feature branch from main: `git feature <name>`
3. Implement optimization
4. Test locally and commit with bench count
5. Push branch to origin
6. **HUMAN**: Run SPRT test on OpenBench
7. **HUMAN**: If successful, approve merge to main
8. If failed: abandon branch and try different approach

**Testing Protocol for Each Sub-Phase**:
1. Implement optimization
2. Measure NPS improvement locally
3. Run bench and commit with exact count
4. Push to origin for OpenBench access
5. **HUMAN**: Run SPRT test with bounds [-3.00, 3.00] 
6. **HUMAN**: Review results and decide next steps
7. Document results before proceeding

**Success Criteria**:
- Each sub-phase should maintain or improve NPS
- No strength regression (SPRT pass)
- Combined target: 2x overall NPS improvement (1M+ nps)

---

### Phase 3: Move Generation Optimization  
**Goal**: Speed up move generation and fix potential bugs

**Investigation areas**:
- Profile move generation with perft
- Check for redundant legality checks
- Optimize bitboard operations
- Validate move ordering efficiency

---

### Phase 4: Evaluation Function Optimization
**Goal**: Speed up position evaluation

**Investigation areas**:
- Check for redundant calculations
- Profile PST lookups
- Optimize material counting
- Review pawn structure caching

---

### Phase 5: Search-Specific Optimizations
**Goal**: Reduce overhead in search loop

**Investigation areas**:
- Profile TT access patterns
- Check move ordering effectiveness
- Review LMR conditions
- Optimize repetition detection

---

### Phase 6: Memory and Cache Optimization
**Goal**: Improve cache usage and memory access patterns

**Investigation areas**:
- Reduce Board class size
- Optimize data structure alignment
- Improve memory locality
- Profile cache misses

---

### Phase 0.5: Compiler Optimization (COMPLETED ✅)
**Goal**: Optimize build flags with auto-detection

**Changes Made:**
1. **Makefile Updates**
   - Switched from hardcoded `-mavx2 -mbmi2` to `-march=native`
   - Upgraded from `-O2` to `-O3` for better optimization
   - Added `-flto` for link-time optimization
   - Now auto-detects CPU capabilities on any x86-64 CPU

2. **CMakeLists.txt Updates**
   - Added same optimizations for local builds
   - Release builds use `-O3 -march=native -flto`
   - Debug and RelWithDebInfo modes also updated

3. **Results**
   - **NPS Improvement**: 440K → 500K (14% gain)
   - **OpenBench**: Confirmed working on Ryzen, Xeon, and i9 processors
   - **Compatibility**: Auto-detection ensures optimal performance on any CPU

**Key Learning**: Using `-march=native` allows the compiler to automatically
detect and use the best available instruction sets (SSE4.2, AVX2, BMI2, etc.)
for each specific CPU, eliminating the need for hardcoded flags.

---

## Success Metrics

### Minimum Goals
- **2x speedup**: Reach 1M+ nps
- **No strength regression**: SPRT must pass
- **Thread-safe**: Ready for LazySMP
- **Stable**: No crashes or bugs

### Stretch Goals
- **3x speedup**: Reach 1.5M nps
- **Competitive**: Match other engines
- **Clean code**: Well-documented changes
- **Future-proof**: Easy to parallelize

---

## Testing Strategy

### Understanding Bench and Perft Validation

#### What is Perft?
Perft (performance test) validates move generation correctness by:
- Generating all legal moves from a position
- Recursively exploring all moves to a given depth
- Counting total leaf nodes
- Comparing against known correct values

#### How Bench Uses Perft
The `bench` command runs perft on 12 standard positions:
- Always produces **19,191,913 nodes** for correct move generation
- This node count is a "fingerprint" of correctness
- Required in every commit message for OpenBench validation
- Uses the engine's actual move generation code

#### Validation Tools
1. **Bench command**: `echo "bench" | ./bin/seajay`
   - Tests 12 positions, outputs total node count
   - Must always output 19,191,913 nodes

2. **Perft tool**: `./bin/perft_tool`
   - Standalone tool using same engine components
   - Can test individual positions to any depth
   - Includes test suite with known values

3. **Built-in test suite**: `./bin/perft_tool --suite --max-depth 4`
   - Validates standard positions
   - Tests special moves (castling, en passant, promotions)

### For Each Phase with Code Changes:

1. **Local testing**
   ```bash
   rm -rf build
   ./build.sh Release
   
   # Verify move generation correctness with perft
   echo "bench" | ./bin/seajay | grep "19191913"  # Must output this exact count
   ./bin/perft_tool --suite --max-depth 4  # Run validation suite
   
   # Test NPS on reference position
   echo -e "position fen r2q1rk1/ppp1bppp/2np1n2/4p3/2B1P3/2NP1Q1P/PPP2PP1/R1B2RK1 b - - 0 9\ngo depth 14\nquit" | ./bin/seajay | grep nps
   ```

2. **Correctness validation**
   ```bash
   # Quick perft validation (should complete without errors)
   ./bin/perft_tool kiwipete 3  # Complex position test
   ./bin/perft_tool startpos 5  # Should output 4865609 nodes
   
   # If any perft test fails, DO NOT COMMIT
   # Debug the issue before proceeding
   ```

3. **Commit with bench**
   ```bash
   BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}')
   # Verify bench is exactly 19191913
   if [ "$BENCH" != "19191913" ]; then
       echo "ERROR: Bench count incorrect! Expected 19191913, got $BENCH"
       echo "Move generation may be broken. Debug before committing."
       exit 1
   fi
   git commit -m "Description... bench $BENCH"
   git push
   ```

4. **OpenBench SPRT**
   - Branch: feature branch for the phase
   - Base: main (or previous phase)
   - Bounds: [-3.00, 3.00] for no regression
   - Bounds: [0.00, 5.00] for improvements

5. **Wait for results**
   - PASS: Continue to next phase
   - FAIL: Debug and fix
   - INCONCLUSIVE: Analyze and decide

### Why Perft Matters for Optimization
- **Perft exercises the optimized code paths** millions of times
- **Any bug in move generation** will change the node count
- **The bench count of 19,191,913** proves correctness
- **If optimization breaks something**, perft will catch it immediately
- **Thread-safety issues** may only appear under concurrent perft testing

---

## Important Reminders

### For Any Agents Called:
```
CRITICAL CONSTRAINTS:
1. MUST use: rm -rf build && ./build.sh for local development
2. OpenBench uses Makefile automatically
3. Build system auto-detects CPU capabilities via -march=native
4. Don't hardcode specific instruction sets (let compiler decide)
5. ALL changes MUST be thread-safe for LazySMP
6. Update BOTH Makefile and CMakeLists.txt
7. Include "bench [count]" in EVERY commit
8. Design for future LazySMP implementation

MANDATORY READING BEFORE ANY WORK:
- Read /workspace/nps_optimization_plan.md FIRST
- Read /workspace/docs/BUILD_SYSTEM.md SECOND
- DO NOT modify existing codebase unless explicitly instructed
- Analysis and planning only unless told otherwise
```

---

## Phase Tracking

| Phase | Description | Branch | Status | NPS Before | NPS After | SPRT |
|-------|-------------|--------|--------|------------|-----------|------|
| 0 | Baseline Analysis | - | Complete | - | 440K | N/A |
| 0.5 | Compiler Optimization | (merged) | Complete | 440K | 500K | ✅ PASSED |
| 1 | Profiling & Hotspots | feature/20250829-slow-search-analysis | Complete | 500K | 500K | Analysis |
| 2.1 | isSquareAttacked Opt | feature/20250829-attack-detection | **Merged** | 500K | 540K | ✅ PASSED |
| ~~2.2~~ | ~~makeMoveInternal Opt~~ | feature/20250829-make-unmake-opt | **ABANDONED** | 540K | - | ❌ FAILED |
| 2.2.a | - Safe optimizations | - | FAILED | 540K | 540K | -11.75 ELO |
| 2.2.b | - Remove fifty-move zobrist | - | FAILED | 540K | 513K | -4.5% NPS |
| 2.3 | updateBitboards Opt | feature/20250829-bitboard-updates | Pending | - | - | - |
| 2.4 | HistoryHeuristic Opt | feature/20250829-history-cache | **Complete** | 540K | 560K | ~Neutral |
| 2.4.a | - Array reordering | - | Complete | 540K | 591K | -4.29 ELO |
| 2.4.b | - int16_t conversion | - | Complete | 591K | 560K | -0.68 ELO |
| 2.5 | Evaluation Opt | feature/20250830-eval-speedup | In Progress | 560K | - | - |
| 2.5.a | - UCI Lazy Eval | feature/20250830-lazy-eval-uci | **ABANDONED** | 560K | - | ELO loss, search dependencies |
| 2.5.b | - Remove redundancy | - | **Complete** | 560K | 575K | Awaiting |
| 2.5.c | - Progressive lazy (UCI) | - | Planned | - | - | - |
| 2.5.d | - Endgame-aware | - | Planned | - | - | - |
| 2.5.e | - SIMD (optional) | - | Planned | - | - | - |
| 3 | Move Generation | TBD | Pending | - | - | - |
| 4 | Search Optimizations | TBD | Pending | - | - | - |
| 5 | Memory & Cache | TBD | Pending | - | - | - |

---

## Notes Section

### Findings from Analysis:
1. **Compiler Optimization Impact**: Simply switching from -O2 to -O3 with -march=native gave 14% speedup
2. **Instruction Sets**: Competitors use SSE/BMI but not AVX in their binaries
3. **Baseline NPS**: 440K nps on test position (still 2.3-3.7x slower than competitors)

### Successful Optimizations:
1. **Phase 0.5 - Compiler Flags** (2025-08-29)
   - Changed: -O2 → -O3, added -march=native and -flto
   - Result: 14% NPS improvement (440K → 500K)
   - Verified on multiple CPU architectures via OpenBench

2. **Phase 2.4 - History Heuristic Optimization** (2025-08-29)
   - **Phase 2.4.a**: Array reordering [2][64][64] → [64][64][2]
     - Both colors in same cache line (50% fewer cache misses)
     - Replaced global aging with saturating arithmetic
     - Result: 9.4% NPS gain (540K → 591K), -4.29 ELO
   - **Phase 2.4.b**: Convert to int16_t
     - Halved memory footprint (32KB → 16KB)
     - Entire table fits in L1 cache
     - Inlined getScore function
     - Combined result: 560K NPS, -0.68 ELO (essentially neutral)
   - **Critical Achievement**: LazySMP-ready (eliminated catastrophic cache invalidation)
   - **Memory Efficiency**: 50% reduction enables better overall cache usage

3. **Phase 2.5.b - Remove Redundant Calculations** (2025-08-30)
   - **Commit**: 38edb70
   - **Changes**: 
     - Cached game phase detection (3 calls → 1)
     - Cached king squares (multiple calls → 1)
     - Cached side to move (4 calls → 1)
     - Cached isPureEndgame check (2 calls → 1)
   - **Result**: 2.7% NPS improvement (560K → 575K)
   - **Status**: Awaiting SPRT test results
   - **Expected**: Neutral to small positive ELO

### Phase 1 Profiling Results (2025-08-29):

**Top Hotspots Identified (gprof analysis):**
1. **isSquareAttacked** (12.83%) - Called 14.8M times
2. **makeMoveInternal** (12.83%) - Called 14.1M times  
3. **updateBitboards** (10.57%) - Called 60M times
4. **HistoryHeuristic::getScore** (7.92%) - Called 73.7M times
5. **evaluate** (7.92%) - Called 1.5M times

**Key Findings:**
- Move generation and board operations dominate runtime (36.23% combined)
- History heuristic lookup is extremely hot (73.7M calls)
- Attack detection is a major bottleneck
- SEE calculations and move ordering also significant

**NPS Status:** ~500K (no change from Phase 0.5)

### Next Investigation Areas:
- ~~Optimize history heuristic access patterns~~ ✅ COMPLETE (Phase 2.4)
- Implement lazy evaluation in eval function (Phase 2.5)
- Consider incremental attack updates
- Review board copy vs copy-make tradeoffs
- Optimize remaining hotspots after Phase 2.5

### Build System Notes:
- Makefile now uses -march=native for auto-detection
- Works on Ryzen (AVX2/BMI2), Xeon, and Core i9 processors
- Development environment limited to SSE4.2 + POPCNT

---

Last Updated: 2025-08-30
Current Branch: feature/20250830-eval-speedup (Phase 2.5 in progress)
Completed: Phase 2.5.b - Remove redundant calculations (awaiting SPRT)