# NPS Optimization Plan for SeaJay Chess Engine

## Problem Statement
SeaJay is running at **477K nps** while competitor engines achieve:
- Stash: **1.6M nps** (3.4x faster)
- Komodo: **1.5M nps** (3.1x faster)  
- Laser: **1.1M nps** (2.4x faster)

## Critical Development Constraints

### 1. Instruction Set Limitations
⚠️ **MAXIMUM: SSE 4.2** (no AVX/AVX2/BMI2)
- Development environment only supports up to SSE 4.2
- Cannot use Makefile directly (has AVX2/BMI2 flags)
- **MUST use**: `rm -rf build && ./build.sh`

### 2. Build System Requirements
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
⚠️ **Design for Future LazySMP**
- All optimizations must be thread-safe
- No global mutable state without proper design
- Prefer thread-local storage where needed
- Avoid changes that complicate parallelization

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
**Approaches**:
- Option A: Add attack map caching per position
- Option B: Use incremental attack updates during make/unmake
- Option C: Optimize attack generation with better bitboard algorithms
- Option D: Pre-compute common attack patterns

#### Phase 2.2: makeMoveInternal Optimization (12.83% runtime)
**Branch**: `feature/20250829-make-unmake-opt`
**Current**: 14.1M calls consuming 12.83% of runtime
**Target**: Reduce overhead by 30-40%
**Approaches**:
- Option A: Reduce unnecessary state updates
- Option B: Optimize piece list maintenance
- Option C: Streamline zobrist key updates
- Option D: Consider copy-make vs make-unmake tradeoffs

#### Phase 2.3: updateBitboards Optimization (10.57% runtime)
**Branch**: `feature/20250829-bitboard-updates`
**Current**: 60M calls consuming 10.57% of runtime
**Target**: Inline or optimize the hot path
**Approaches**:
- Option A: Force inline the function
- Option B: Use branchless bit manipulation
- Option C: Combine updates to reduce function calls
- Option D: Template specialization for common cases

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
**Branch**: `feature/20250829-eval-speedup`
**Current**: 1.5M calls consuming 7.92% of runtime
**Target**: Speed up by 20-30% without losing strength
**Approaches**:
- Option A: Lazy evaluation (compute only what's needed)
- Option B: Incremental evaluation updates
- Option C: Better PST access patterns
- Option D: SIMD for material/PST calculations

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

### For Each Phase with Code Changes:
1. **Local testing**
   ```bash
   rm -rf build
   ./build.sh Release
   echo "bench" | ./bin/seajay
   ./tools/analyze_position.sh "[test FEN]" depth 14
   ```

2. **Commit with bench**
   ```bash
   BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}')
   git commit -m "Description... bench $BENCH"
   git push
   ```

3. **OpenBench SPRT**
   - Branch: feature/20250829-slow-search-analysis
   - Base: main (or previous phase)
   - Bounds: [-3.00, 3.00] for no regression
   - Bounds: [0.00, 5.00] for improvements

4. **Wait for results**
   - PASS: Continue to next phase
   - FAIL: Debug and fix
   - INCONCLUSIVE: Analyze and decide

---

## Important Reminders

### For Any Agents Called:
```
CRITICAL CONSTRAINTS:
1. MUST use: rm -rf build && ./build.sh
2. CANNOT use: make (except for OpenBench)
3. Maximum instruction set: SSE 4.2 + POPCNT
4. No AVX, AVX2, or BMI2 instructions
5. All changes must be thread-safe
6. Update BOTH Makefile and CMakeLists.txt
7. Include "bench [count]" in EVERY commit
8. Design for future LazySMP implementation
```

---

## Phase Tracking

| Phase | Description | Branch | Status | NPS Before | NPS After | SPRT |
|-------|-------------|--------|--------|------------|-----------|------|
| 0 | Baseline Analysis | - | Complete | - | 440K | N/A |
| 0.5 | Compiler Optimization | (merged) | Complete | 440K | 500K | ✅ PASSED |
| 1 | Profiling & Hotspots | feature/20250829-slow-search-analysis | Complete | 500K | 500K | Analysis |
| **2.1** | isSquareAttacked Opt | feature/20250829-attack-detection | **Ready** | 500K | - | - |
| 2.2 | makeMoveInternal Opt | feature/20250829-make-unmake-opt | Pending | - | - | - |
| 2.3 | updateBitboards Opt | feature/20250829-bitboard-updates | Pending | - | - | - |
| 2.4 | HistoryHeuristic Opt | feature/20250829-history-cache | Pending | - | - | - |
| 2.5 | Evaluation Opt | feature/20250829-eval-speedup | Pending | - | - | - |
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
- Optimize isSquareAttacked with better caching or algorithm
- Reduce updateBitboards overhead in make/unmake
- Optimize history heuristic access patterns
- Consider incremental attack updates
- Review board copy vs copy-make tradeoffs

### Build System Notes:
- Makefile now uses -march=native for auto-detection
- Works on Ryzen (AVX2/BMI2), Xeon, and Core i9 processors
- Development environment limited to SSE4.2 + POPCNT

---

Last Updated: 2025-08-29
Branch: feature/20250829-slow-search-analysis