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

### Phase 2: Board Representation Optimization  
**Goal**: Fix potential bugs and optimize make/unmake or copy-make operations

**Investigation areas**:
- Check for unnecessary board copies
- Validate zobrist key updates
- Look for redundant state updates
- Check move generation correctness

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

### Phase 7: Compiler Optimization Flags (LAST)
**Goal**: Optimize build flags within SSE 4.2 limits

1. **Update Makefile for OpenBench**
   - Add `-O3` if using `-O2`
   - Add `-msse4.2 -mpopcnt` explicitly
   - Consider `-flto` for link-time optimization
   - Add `-march=x86-64-v2` (includes up to SSE4.2)

2. **Synchronize with CMakeLists.txt**
   - Match optimization flags
   - Ensure both build systems produce similar binaries

3. **Benchmark changes**
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

**WAIT FOR HUMAN**: SPRT test results

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

| Phase | Description | Status | NPS Before | NPS After | SPRT Result |
|-------|-------------|--------|------------|-----------|-------------|
| 0 | Baseline Analysis | Complete | - | 440K | N/A |
| 1 | Profiling & Hotspots | Pending | 440K | - | N/A |
| 2 | Board Optimization | Pending | - | - | Awaiting |
| 3 | Move Generation | Pending | - | - | Awaiting |
| 4 | Evaluation | Pending | - | - | Awaiting |
| 5 | Search Loop | Pending | - | - | Awaiting |
| 6 | Memory & Cache | Pending | - | - | Awaiting |
| 7 | Compiler Flags (LAST) | Pending | - | - | Awaiting |

---

## Notes Section

### Findings from Analysis:
(To be filled during investigation)

### Bottlenecks Identified:
(To be documented during profiling)

### Successful Optimizations:
(To be tracked as we progress)

### Failed Attempts:
(Important for learning)

---

Last Updated: 2025-08-29
Branch: feature/20250829-slow-search-analysis