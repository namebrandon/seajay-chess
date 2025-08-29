# Triangular PV Array Implementation Plan for SeaJay

## Overview
Implementation of full Principal Variation (PV) display for UCI compliance, using triangular PV arrays for thread-safety and future multi-threading support.

## Design Decisions
- **Architecture**: Triangular PV array (not TT extraction)
- **Memory**: Stack-allocated, ~17KB per thread
- **Thread Safety**: Thread-local storage, no synchronization needed
- **Cache Optimization**: Flattened array with cache-line alignment
- **Performance Target**: <2% overhead

## Implementation Phases

### Phase 1: Basic Infrastructure ✅ COMPLETED
**Status**: Completed  
**Expected ELO**: 0 ± 5 (infrastructure only)  
**SPRT Bounds**: [-5.00, 3.00]  
**Commit**: `2e497c8` - `feat: Add triangular PV infrastructure (Phase PV1) - bench 19191913`
**Branch**: `feature/20250828-uci-display`

**Tasks**:
- [x] Create `/workspace/src/search/principal_variation.h`
- [x] Implement TriangularPV class with flattened array
- [x] Add cache-aligned memory layout
- [x] Include header in negamax.cpp
- [x] Add stack-allocated PV (pass nullptr for now)
- [x] Verify compilation
- [x] Run bench for node count (19191913)
- [x] Commit and push for SPRT testing

**Results**: Awaiting SPRT test results from OpenBench

---

### Phase 2: Root PV Collection Only
**Status**: Not Started  
**Expected ELO**: 0 ± 5 (display change only)  
**SPRT Bounds**: [-5.00, 3.00]  
**Commit Message**: `feat: Enable root PV collection (Phase PV2) - bench [count]`

**Tasks**:
- [ ] Update PV only at root (ply == 0)
- [ ] Collect best move sequence at root
- [ ] Keep existing single-move UCI output
- [ ] Verify PV collection works
- [ ] Run bench for node count
- [ ] Commit and push for SPRT testing

---

### Phase 3: Full Tree PV Tracking
**Status**: Not Started  
**Expected ELO**: -2 ± 5 (1-2% overhead expected)  
**SPRT Bounds**: [-5.00, 3.00]  
**Commit Message**: `feat: Full tree PV tracking (Phase PV3) - bench [count]`

**Tasks**:
- [ ] Pass PV through all recursive negamax calls
- [ ] Update PV at all depths in PV nodes
- [ ] Implement child PV copying logic
- [ ] Keep UCI output unchanged (risk mitigation)
- [ ] Run bench for node count
- [ ] Commit and push for SPRT testing

---

### Phase 4: UCI Full PV Output
**Status**: Not Started  
**Expected ELO**: 0 ± 5 (display only)  
**SPRT Bounds**: [-5.00, 3.00]  
**Commit Message**: `feat: Enable full PV display in UCI (Phase PV4) - bench [count]`

**Tasks**:
- [ ] Modify sendIterationInfo() to extract full PV
- [ ] Update UCI info output format
- [ ] Add PV validation before output
- [ ] Match Komodo's output style
- [ ] Test with various GUIs
- [ ] Run bench for node count
- [ ] Commit and push for SPRT testing

---

### Phase 5: Optimizations (Optional)
**Status**: Not Started  
**Expected ELO**: +1 ± 5 (minor optimization)  
**SPRT Bounds**: [-3.00, 5.00]  
**Commit Message**: `opt: Optimize PV operations (Phase PV5) - bench [count]`

**Tasks**:
- [ ] Template PV/non-PV nodes
- [ ] Add prefetching hints
- [ ] Optimize small PV copies
- [ ] Profile cache behavior
- [ ] Run bench for node count
- [ ] Commit and push for SPRT testing

---

## Technical Design Details

### Data Structure
```cpp
class TriangularPV {
    static constexpr int MAX_DEPTH = 128;
    alignas(64) std::array<Move, (MAX_DEPTH * (MAX_DEPTH + 1)) / 2> pvArray;
    alignas(64) std::array<uint8_t, MAX_DEPTH> pvLength;
};
```

### Memory Layout
- Flattened triangular array for cache efficiency
- Cache-line aligned to prevent false sharing
- Stack allocated, no heap allocation
- ~17KB per thread total

### Integration Points
1. `searchIterativeTest()` - Stack allocate PV
2. `negamax()` - Pass PV by reference, update on best move
3. `sendIterationInfo()` - Extract and display full PV
4. UCI output - Format as space-separated moves

### Testing Protocol
Each phase requires:
1. Run `echo "bench" | ./seajay` for node count
2. Include "bench [count]" in commit message
3. Push to feature branch
4. Submit to OpenBench for SPRT testing
5. Wait for test completion before proceeding

## Performance Metrics
- **Target Overhead**: <2% in single-threaded search
- **Memory per Thread**: ~17KB
- **Cache Impact**: Minimal (fits in L2)
- **Multi-threading**: Zero contention (thread-local)

## Risk Mitigation
- Each phase independently testable
- No functional changes until Phase 4
- Existing UCI output preserved as fallback
- Bench stability verified at each step

## Success Criteria
- [ ] Full PV display matching Komodo behavior
- [ ] No measurable ELO loss (<2 ELO)
- [ ] Thread-safe design ready for Lazy SMP
- [ ] Clean integration with existing code
- [ ] GUI compatibility verified

## Notes
- Following Stockfish/Ethereal approach (industry standard)
- Avoiding TT extraction due to multi-threading issues
- Stack allocation chosen for simplicity and performance
- Designed for future 16+ thread scalability