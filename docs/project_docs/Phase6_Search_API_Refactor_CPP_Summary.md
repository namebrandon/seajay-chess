# Phase 6 Search API Refactor - C++ Performance Summary

## Executive Summary
This document provides C++ performance recommendations for the Phase 6 Search API Refactor, focusing on achieving zero-overhead abstractions while preparing for advanced pruning techniques in a thread-safe manner for LazySMP.

## Key Design Decisions

### 1. NodeContext Structure (4 bytes, register-passed)
```cpp
struct alignas(8) NodeContext {
    Move excluded;     // 2 bytes (uint16_t)
    uint8_t flags;     // 1 byte: isPv (bit 0), isRoot (bit 1)
    uint8_t padding;   // 1 byte padding

    // Zero-cost inline accessors
    ALWAYS_INLINE bool isPv() const { return flags & 0x01; }
    ALWAYS_INLINE bool isRoot() const { return flags & 0x02; }
};
```

**Benefits:**
- Fits in single register (x86-64 ABI)
- No pointer indirection
- Cache-line efficient
- Thread-local (stack allocated)

### 2. Compile-Time Feature Toggling
```cpp
template<bool EnableSingular = false>
ALWAYS_INLINE eval::Score verify_exclusion_impl(...) {
    if constexpr (!EnableSingular) {
        return eval::Score::zero(); // Compiled out entirely
    }
    // Actual implementation
}
```

**Benefits:**
- Zero overhead when disabled
- Dead code elimination
- No runtime branching
- Allows aggressive compiler optimization

### 3. LazySMP Thread Safety
- **NodeContext**: Thread-local, stack allocated
- **TT Access**: Relaxed atomics, lock-free
- **No Shared State**: Each thread has independent context
- **Root Coordination**: Atomic<Move> for best move only

### 4. Performance Targets
- **Disabled Features**: < 0.5% performance impact
- **Enabled Features**: Measurable strength gain
- **Cache Misses**: < 5% L1 miss rate in search loop
- **Branch Prediction**: < 2% misprediction rate
- **Memory**: No heap allocations in hot path

### 5. Critical Optimizations

#### Cache Optimization
- NodeContext packed to 4 bytes
- 16-byte aligned TTEntry prevents false sharing
- Prefetch hints for predictable access patterns

#### Branch Optimization
- Branchless excluded move checking
- Template specialization for PV/non-PV paths
- [[likely]]/[[unlikely]] attributes for C++20

#### Memory Layout
- Structure packing with explicit padding
- Alignment directives for SIMD compatibility
- Stack allocation for temporary data

### 6. Implementation Strategy

#### Phase 1: Add Infrastructure
- Introduce NodeContext with feature flags OFF
- Add overloaded negamax signature
- Maintain backward compatibility

#### Phase 2: Migration
- Update all call sites to use NodeContext
- Verify zero behavioral change
- Profile for performance regression

#### Phase 3: Enable Features
- Turn on features one at a time
- SPRT test each feature
- Monitor performance metrics

### 7. Validation Checklist
- [ ] Static assertions for size/alignment
- [ ] Assembly inspection for inlining
- [ ] Perf profiling for cache behavior
- [ ] Valgrind for memory correctness
- [ ] Thread sanitizer for race conditions
- [ ] Benchmark parity when disabled
- [ ] Strength improvement when enabled

### 8. Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| Cache pollution | Pack structures, align to cache lines |
| Register pressure | Pass small structs by value |
| Branch misprediction | Use templates for static dispatch |
| Thread contention | Lock-free algorithms, thread-local data |
| Compiler variance | Test with GCC, Clang, MSVC |

### 9. Measurement Tools
- `perf stat` - Cache misses, branch prediction
- `perf record` - Hot spot analysis
- `valgrind --tool=cachegrind` - Cache simulation
- `objdump -d` - Assembly verification
- `rdtsc` - Cycle-accurate timing

### 10. Code Review Focus Areas
1. Ensure all hot path functions are inlined
2. Verify no hidden allocations
3. Check for unnecessary copies
4. Validate memory ordering for atomics
5. Confirm feature flags compile out cleanly

## Conclusion
The Phase 6 refactor can achieve zero-overhead abstractions through careful C++ design. By focusing on cache efficiency, compile-time optimization, and thread-local design, we can prepare for advanced search features without impacting current performance.