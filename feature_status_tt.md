# Transposition Table Investigation - Feature Status

## Branch: `bugfix/nodexp/20250905-tt-optimization`
**Created**: 2025-09-05  
**Parent Branch**: `bugfix/20250905-node-explosion-diagnostic`  
**Issue**: Low TT hit rates (38-47%) contributing to node explosion  
**Goal**: Improve TT effectiveness to reduce search tree size

## Executive Summary

The investigation revealed that **TT size is NOT the limiting factor** for poor hit rates. Instead, we discovered fundamental issues with search stability and TT replacement strategy that cause hit rates to paradoxically **decrease** with search depth.

## Key Findings

### 1. Hash Table Size Analysis

Tested hash sizes from 1MB to 1GB on tactical position:

| Hash Size | TT Hit Rate | Nodes | Hash Full | Collisions |
|-----------|------------|-------|-----------|------------|
| 1MB | 55.6% | 19,369 | 120% | 0 |
| 8MB | 37.5% | 91,612 | 96% | 0 |
| 32MB | 38.0% | 91,048 | 29% | 0 |
| 64MB | 38.0% | 91,044 | 15% | 0 |
| 128MB | 38.1% | 91,044 | 8% | 0 |
| 256MB | 38.1% | 91,044 | 5% | 0 |
| 512MB | 38.1% | 91,044 | 4% | 0 |
| 1024MB | 38.1% | 91,044 | 4% | 0 |

**Conclusion**: Hit rate plateaus at ~38% regardless of size. The problem is structural, not capacity-based.

### 2. Position-Dependent Performance

| Position Type | TT Hit Rate | Notes |
|--------------|-------------|-------|
| Starting position | 65% | Good performance on simple positions |
| Tactical middlegame | 38% | Poor performance on complex positions |
| Endgame | ~45% | Moderate performance |

### 3. Critical Discovery: Hit Rate Decreases with Depth

**This is the opposite of expected behavior!**

| Search Depth | TT Hit Rate |
|--------------|------------|
| Depth 6 | 65.3% |
| Depth 8 | 57.3% |
| Depth 10 | 38.8% |
| Depth 12 | 34.2% |

**Expected**: Hit rate should increase as the table fills with useful entries  
**Actual**: Hit rate decreases dramatically with depth  
**Implication**: Search instability or overly aggressive replacement

### 4. Collision Detection Issue

- **0 collisions reported** even at 120% hashfull
- This is mathematically impossible with a full hash table
- Indicates a bug in collision tracking or fundamental misunderstanding

## Implementation Changes

### Commit: 5d86e26
**Message**: "feat: Improve TT replacement strategy and fix collision tracking"

### Changes Made:

1. **Enhanced Replacement Strategy** (`transposition_table.cpp`):
   - Added 2-ply grace period for old generation entries
   - Implemented depth-preferred replacement for collisions
   - More nuanced decision making based on entry age and depth

2. **Fixed Collision Detection**:
   - Moved collision check before isEmpty() call
   - Should now properly detect when different positions map to same index

3. **Added Diagnostics** (`types.h`):
   ```cpp
   uint64_t ttReplaceEmpty = 0;     // Replaced empty entries
   uint64_t ttReplaceOldGen = 0;    // Replaced old generation entries
   uint64_t ttReplaceDepth = 0;     // Replaced shallower entries
   uint64_t ttReplaceSkipped = 0;   // Skipped replacement
   ```

### Performance Impact:
- **2% node reduction** (89,146 vs 91,044 baseline)
- Minimal improvement suggests deeper issues

## Root Cause Analysis

### Primary Issue: Search Instability

The decreasing hit rate with depth indicates severe search instability:

1. **Different paths each iteration** - The search explores different move orders at each depth
2. **Aspiration window failures** - Frequent re-searches pollute the TT
3. **Move ordering variance** - Inconsistent move ordering causes different cutoffs

### Secondary Issues:

1. **TT Entry Quality**:
   - Only 75% of TT hits produce cutoffs when tried first
   - Suggests stored moves aren't optimal or bounds aren't tight

2. **Replacement Strategy Flaws**:
   - Still too aggressive in replacing old entries
   - May need separate tables for different search phases

3. **Generation Handling**:
   - Single generation counter may be insufficient
   - Need to preserve valuable entries across searches better

## Comparison with Strong Engines

### Typical Strong Engine TT Performance:
- **Hit Rate**: 60-70% at depth 10
- **First-move cutoff from TT**: 40-50%
- **Collisions**: 5-10% of stores
- **Hit rate trend**: Increases with depth

### SeaJay Current Performance:
- **Hit Rate**: 38% at depth 10
- **First-move cutoff from TT**: 26%
- **Collisions**: 0% (bug)
- **Hit rate trend**: Decreases with depth (!)

## Recommendations for Next Steps

### Immediate Actions:

1. **Fix Collision Tracking**:
   - Debug why collisions aren't being detected
   - May need to track at probe time, not just store time

2. **Investigate Search Stability**:
   - Add move ordering consistency tracking
   - Monitor aspiration window failure rate
   - Track how often the PV changes between iterations

3. **Test Iterative Deepening**:
   - Run searches from depth 1-10 sequentially
   - Monitor how TT builds up
   - Check if entries are being preserved

### Longer-term Improvements:

1. **Two-Tier Replacement**:
   - Always replace if much deeper (4+ ply)
   - Preserve if similar depth (within 2 ply)
   - Consider entry value/bound type

2. **Separate TT for Phases**:
   - PV nodes vs non-PV nodes
   - Quiescence vs main search
   - Different replacement strategies for each

3. **Entry Quality Improvements**:
   - Store killer/history information
   - Track how often entry produces cutoff
   - Age-based replacement consideration

## Test Scripts Created

1. **`test_tt_sizes.sh`** - Tests TT performance across different hash sizes
2. **`test_tt_improvements.sh`** - Validates improvements and collision detection

## Metrics for Success

| Metric | Current | Target | Strong Engine |
|--------|---------|--------|---------------|
| TT Hit Rate (depth 10) | 38% | 55% | 65% |
| Hit Rate Trend | Decreasing | Increasing | Increasing |
| TT Cutoff Rate | 26% | 40% | 50% |
| Collision Detection | 0% (broken) | 5-10% | 5-10% |
| Node Reduction | Baseline | -30% | -50% |

## Conclusion

The TT investigation revealed that the problem is not capacity but **search instability**. The decreasing hit rate with depth is a critical symptom of a search that explores different parts of the tree at each iteration, preventing effective TT reuse.

While we've improved the replacement strategy (2% node reduction), the fundamental issues remain:
1. Search instability causing poor TT reuse
2. Collision detection not working
3. Hit rates that decrease instead of increase with depth

The TT problems are a **symptom** of broader search issues, not the root cause of node explosion.

## Next Investigation Target

Based on these findings, the next area to investigate should be:
1. **Search stability** - Why the search explores different paths each iteration
2. **Aspiration windows** - May be too narrow, causing excessive re-searches
3. **Move ordering consistency** - Ensure deterministic ordering across iterations