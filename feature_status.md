# SEE Cache Key Generation Performance Investigation

## Branch: test/20250823-see-cache-fallback

## Issue Analysis

An agent identified a performance issue in `/workspace/src/core/see.cpp` where the fallback hash generation for SEE cache keys was extremely expensive with:
- 64 iterations over all squares
- Modulo operations (`sq % 32`)
- 64-bit multiplication with magic constants
- Bitwise rotations (shift + OR)

This fallback was triggered when `board.zobristKey() == 0`.

## Investigation Results

### Finding 1: Zobrist Keys Are Properly Initialized
- `Board::rebuildZobristKey()` is called in `Board::clear()`
- All board constructors properly initialize zobrist tables
- Even empty boards have non-zero zobrist keys (due to side-to-move XOR)
- Test confirmed: Starting position zobrist = 0x11623d6631e1c087
- Test confirmed: Empty board zobrist = 0x2bb986b869b5b920

### Finding 2: Fallback Never Triggered in Normal Operation
- Ran benchmark: 19,191,913 nodes processed
- SEE statistics show 0 fallback hash uses
- The expensive fallback path is never hit during normal play

### Finding 3: The Issue Was Still Valid
The concern raised was legitimate because:
1. IF the fallback was ever triggered, it would be catastrophically expensive
2. The fallback code was unnecessarily complex for a "should never happen" case
3. No proper tracking or warning existed to detect if this occurred

## Improvements Implemented

### 1. Performance Counter Added
```cpp
std::atomic<uint64_t> fallbackHashUsed{0};  // Track when expensive fallback is used
```

### 2. Simplified Fallback Hash
Replaced expensive operations with simple XOR:
```cpp
// OLD: Expensive modulo, multiply, rotate
boardKey ^= (uint64_t(p) << (sq % 32)) * 0x9E3779B97F4A7C15ULL;
boardKey = (boardKey << 13) | (boardKey >> 51);  // Rotate

// NEW: Simple XOR operations only
boardKey ^= (uint64_t(p) << sq) ^ (uint64_t(sq) << 16);
```

### 3. Debug Assertions Added
```cpp
#ifdef DEBUG
    if (board.zobristKey() == 0) {
        std::cerr << "ERROR: SEE called with uninitialized board\n";
        assert(false && "SEE called with uninitialized board zobrist key");
    }
#endif
```

### 4. Warning System
Added one-time warning in fallback path to alert developers if it ever triggers.

## Performance Impact

### Before Fix
- Theoretical worst case: 64 iterations with expensive operations per SEE call
- Never actually triggered in practice

### After Fix  
- Fallback simplified to basic XOR operations
- Debug builds will catch initialization problems immediately
- Production builds have minimal overhead even if fallback triggers
- Benchmark shows no performance regression: 907,154 nps

## Conclusion

While the expensive fallback was never triggered in normal operation (zobrist keys are always properly initialized), the agent correctly identified a potential performance hazard. The improvements make the code more robust:

1. **Defense in depth**: If initialization ever fails, we'll know immediately
2. **Simpler fallback**: Even if triggered, impact is minimal
3. **Better monitoring**: Statistics track if fallback is ever used
4. **Debug safety**: Assertions catch problems in development

## Recommendation

This change is safe to merge as it:
- Adds defensive programming without performance cost
- Simplifies unnecessarily complex code
- Improves debugging capabilities
- Has no impact on normal operation (fallback never triggered)

The agent's concern was valid from a code quality perspective, even though the performance issue doesn't manifest in practice.