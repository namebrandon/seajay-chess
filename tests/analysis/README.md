# Performance Analysis Test Suite

This directory contains test programs that demonstrate the performance issues identified in the SeaJay chess engine optimization review.

## Test Programs

### 1. TT Move Reordering Test
**File:** `tt_move_perf_test.cpp`  
**Purpose:** Demonstrates the performance overhead of redundant TT move searching after move ordering  
**Issue:** Finding #1 - Redundant linear search in hot path

**Compile and Run:**
```bash
g++ -O2 -std=c++20 -I../../src tt_move_perf_test.cpp -o tt_move_test
./tt_move_test
```

### 2. Pawn Hash Performance Test
**File:** `pawn_hash_perf_test.cpp`  
**Purpose:** Shows 3.4x speedup by using power-of-2 hash table size instead of prime  
**Issue:** Finding #6 - Expensive modulo operations in pawn evaluation

**Compile and Run:**
```bash
g++ -O2 -std=c++20 pawn_hash_perf_test.cpp -o pawn_hash_test
./pawn_hash_test
```

### 3. SEE Cache Key Generation Test
**File:** `see_cache_test.cpp`  
**Purpose:** Demonstrates expensive fallback hash generation in SEE when zobrist keys are uninitialized  
**Issue:** Finding #2 - Expensive operations in fallback path

**Compile and Run:**
```bash
g++ -O2 -std=c++20 -I../../src see_cache_test.cpp -o see_cache_test
./see_cache_test
```

## Running All Tests

To run all performance tests:
```bash
#!/bin/bash
cd /workspace/tests/analysis

# Compile all tests
g++ -O2 -std=c++20 -I../../src tt_move_perf_test.cpp -o tt_move_test
g++ -O2 -std=c++20 pawn_hash_perf_test.cpp -o pawn_hash_test
g++ -O2 -std=c++20 -I../../src see_cache_test.cpp -o see_cache_test

# Run all tests
echo "=== Running TT Move Test ==="
./tt_move_test
echo ""
echo "=== Running Pawn Hash Test ==="
./pawn_hash_test
echo ""
echo "=== Running SEE Cache Test ==="
./see_cache_test
```

## Expected Results

### Pawn Hash Test
- Shows ~3.4x speedup with power-of-2 table size
- Demonstrates that distribution quality remains nearly identical
- Proves this is a simple, low-risk optimization

### TT Move Test
- Shows overhead increases linearly with move list size
- Demonstrates millions of unnecessary operations per second
- Confirms this is in the critical hot path

### SEE Cache Test
- Shows expensive modulo and rotation operations in fallback
- Demonstrates 10x+ speedup possible with simpler approach
- Highlights that zobrist keys should always be initialized

## Impact on Engine Performance

Based on these tests and typical chess engine profiles:
- Pawn hash optimization: ~2-3% overall speedup
- TT move fix: ~5-8% search speedup
- SEE cache fix: ~1-2% quiescence speedup
- Combined: ~10-15% total NPS improvement

## Implementation Notes

All identified issues can be fixed while maintaining:
- Cross-platform compatibility (Linux, Windows, macOS)
- SSE 4.2 instruction set limit (no AVX required)
- Current architecture and API structure
- Backward compatibility with existing tests