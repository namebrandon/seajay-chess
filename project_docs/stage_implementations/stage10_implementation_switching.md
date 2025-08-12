# Stage 10: Implementation Switching Guide

**Purpose:** Document how to switch between ray-based and magic bitboard implementations  
**Date:** August 12, 2025  
**Status:** Both implementations coexist in production code  

## Overview

SeaJay currently maintains both ray-based and magic bitboard implementations for sliding piece attack generation. This dual-implementation approach provides:

1. **Safety:** Fallback to proven ray-based if issues arise
2. **Testing:** A/B comparison capabilities
3. **Debugging:** Validation mode to ensure consistency
4. **Migration:** Gradual transition path

## Switching Implementations

### Using Magic Bitboards (Recommended)

```bash
# Production build with magic bitboards
cd /workspace/build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_MAGIC_BITBOARDS=ON ..
make -j$(nproc)
```

### Using Ray-Based (Fallback)

```bash
# Production build with ray-based attacks
cd /workspace/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### Debug Validation Mode

```bash
# Magic bitboards with validation against ray-based
cd /workspace/build
cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_MAGIC_BITBOARDS=ON -DDEBUG_MAGIC=ON ..
make -j$(nproc)
```

In validation mode, every magic bitboard call is verified against the ray-based implementation. Any mismatch triggers an assertion.

## Performance Comparison

| Implementation | Rook Attack | Bishop Attack | Queen Attack | Memory |
|---------------|-------------|---------------|--------------|--------|
| Ray-Based | 186 ns | 134 ns | 320 ns | ~0 MB |
| Magic Bitboards | 3.3 ns | 2.4 ns | 5.7 ns | 2.25 MB |
| **Speedup** | **56.36x** | **55.83x** | **56.14x** | - |

## Code Architecture

### File Structure

```
/workspace/src/core/
├── bitboard.h           # Ray-based implementation (original)
├── magic_bitboards_v2.h # Magic implementation (new)
├── magic_constants.h    # Magic numbers and shifts
└── attack_wrapper.h     # Switching layer
```

### Wrapper Functions

The attack_wrapper.h provides three key functions:
- `getRookAttacks(square, occupied)`
- `getBishopAttacks(square, occupied)`
- `getQueenAttacks(square, occupied)`

These automatically use the correct implementation based on compile flags.

## Testing Both Implementations

### Run Tests with Ray-Based

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
./test_magic_bitboards  # Still tests ray-based when flag is off
```

### Run Tests with Magic

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_MAGIC_BITBOARDS=ON ..
make -j$(nproc)
./test_magic_bitboards  # Tests magic implementation
```

### A/B Performance Test

```bash
# Test ray-based
cmake -DCMAKE_BUILD_TYPE=Release ..
make magic_performance_test
./magic_performance_test > ray_results.txt

# Test magic
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_MAGIC_BITBOARDS=ON ..
make magic_performance_test
./magic_performance_test > magic_results.txt

# Compare
diff ray_results.txt magic_results.txt
```

## Migration Strategy

### Phase 1: Current State (August 2025)
- Both implementations coexist
- Magic bitboards opt-in via compile flag
- Full test coverage for both

### Phase 2: Validation Period (1-2 months)
- Enable magic bitboards in development builds
- Run extensive SPRT tests
- Monitor for any issues

### Phase 3: Default Switch (3-4 months)
- Make magic bitboards the default
- Ray-based becomes opt-in fallback
- Update documentation

### Phase 4: Deprecation (6+ months)
- Add deprecation warning for ray-based
- Ensure all users have migrated
- Final compatibility testing

### Phase 5: Removal (1 year)
- Remove ray-based implementation
- Clean up attack_wrapper.h
- Simplify build system

## Troubleshooting

### Issue: Incorrect attacks in specific positions

1. Enable validation mode:
   ```bash
   cmake -DDEBUG_MAGIC=ON -DUSE_MAGIC_BITBOARDS=ON ..
   ```

2. Run until assertion fails

3. The assertion will show:
   - Square where mismatch occurred
   - Occupied bitboard
   - Expected vs actual attacks

### Issue: Performance regression

1. Verify optimization flags:
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release -O3 -march=native ..
   ```

2. Check cache performance:
   ```bash
   ./magic_performance_test
   ```

3. Profile with perf:
   ```bash
   perf record ./seajay bench
   perf report
   ```

### Issue: Memory usage concerns

The 2.25MB used by magic tables is negligible on modern systems. However, if needed:

1. Consider fancy magic bitboards (saves ~1.7MB)
2. Use ray-based for memory-constrained environments
3. Implement on-demand table generation

## Developer Notes

### Adding New Attack Functions

If adding new attack-based functionality:

1. Implement in both bitboard.h (ray) and magic_bitboards_v2.h (magic)
2. Add wrapper in attack_wrapper.h
3. Include validation in DEBUG_MAGIC mode
4. Add tests for both implementations

### Benchmarking

Always benchmark with:
- Warm cache (multiple runs)
- Release build (-O3)
- Consistent CPU frequency (disable turbo boost)
- Same test positions

### Code Example

```cpp
#include "core/attack_wrapper.h"

// This code works with either implementation
Bitboard attacks = getRookAttacks(E4, occupied);

// The wrapper handles the switching:
#ifdef USE_MAGIC_BITBOARDS
    return magicRookAttacks(sq, occupied);
#else
    return rayRookAttacks(sq, occupied);
#endif
```

## Conclusion

The dual-implementation approach provides a safe migration path from ray-based to magic bitboard attack generation. The 55.98x performance improvement justifies the 2.25MB memory cost, but the option to fall back remains available if needed.

For production use, **magic bitboards are strongly recommended** due to their superior performance characteristics.

---

*Last updated: August 12, 2025*