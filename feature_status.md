# Node Explosion Diagnostic - Feature Status

## Overview
**Branch**: `bugfix/20250905-node-explosion-diagnostic`  
**Created**: 2025-09-05  
**Issue**: SeaJay searches 5-10x more nodes than comparable engines  
**Goal**: Reduce node explosion from average 12.9x to under 2x vs Stash

## Phase 1: Baseline Analysis [COMPLETED]

### Test Results Summary
Average explosion ratios across all positions and depths:
- **vs Stash: 12.9x** (worst: 38.5x in middlegame at depth 10)
- **vs Komodo: 3.9x** (worst: 9.7x in middlegame at depth 5)
- **vs Laser: Similar to Komodo

### Key Observations
1. Explosion worsens with depth (exponential growth)
2. Middlegame positions show worst explosion (38.5x)
3. Even simple positions have 4-14x explosion

## Phase 2: Instrumentation [COMPLETED]

### Added Components
1. ✅ `node_explosion_stats.h` - Comprehensive diagnostic statistics structure
2. ✅ `node_explosion_stats.cpp` - Thread-local implementation for thread safety
3. ✅ `nodeExplosionDiagnostics` flag in SearchLimits
4. ✅ Build system integration (CMakeLists.txt)
5. ⏳ Instrumentation hooks in search functions (next step)

### Statistics Being Tracked
- **Depth Distribution**: Nodes at each ply, EBF calculations
- **Pruning Effectiveness**: Futility, move count, LMR by depth
- **Quiescence Explosion**: Entry counts, stand-pat rates, capture rates
- **Move Ordering Failures**: Late cutoffs, bad captures searched
- **SEE Analysis**: Call counts, false positives/negatives, expensive captures

## Phase 3: Root Cause Analysis [COMPLETED]

### Implementation Status
✅ Added `NodeExplosionStats` struct with comprehensive tracking
✅ Added thread-local instance for UCI compatibility  
✅ Added UCI option `NodeExplosionDiagnostics`
✅ Added instrumentation hooks in negamax and quiescence
✅ Fixed compilation issues and node counting bugs
✅ Successfully collected diagnostic data

### Key Findings from Diagnostics

#### Test Position: `r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17`
- **SeaJay**: 316,554 nodes at depth 10
- **Stash**: 8,228 nodes (SeaJay uses **38.5x more**)
- **Komodo**: 39,741 nodes (SeaJay uses **8.0x more**)

#### Root Causes Identified

1. **🔴 CRITICAL: Move Ordering Completely Broken**
   - First-move cutoff rate: 0% (should be 90%+)
   - Top-3 cutoff rate: 0% (should be 95%+)
   - Beta cutoffs happening extremely late or not at all
   - This is THE primary cause of node explosion

2. **Suspicious Pruning Statistics**
   - Futility pruning: 100% "success" rate (likely a bug)
   - Move count pruning: 100% rate (seems incorrect)
   - These metrics suggest the tracking is broken, not the pruning

3. **Quiescence Search Issues**
   - 50% of nodes in qsearch (reasonable but could be better)
   - Stand-pat and capture metrics not tracked properly
   - SEE metrics showing 0 for bad captures

## Phase 4: Fix Priority [NEXT]

### Priority Suspects (Updated)
1. **Quiescence explosion** - Likely not pruning bad captures effectively
2. **Move ordering failures** - Beta cutoffs happening very late
3. **Pruning too conservative** - Not pruning enough moves
4. **LMR not aggressive enough** - Not reducing enough moves
5. **SEE issues** - Removed from highest priority pending evidence

### Next Steps
1. Add instrumentation hooks to negamax and quiescence functions
2. Add UCI option to enable diagnostics
3. Run test positions with diagnostics enabled
4. Analyze collected data to identify root causes
5. Fix highest-impact issues first

## Test Command Reference
```bash
# Run baseline comparison
./tools/analyze_position.sh "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" depth 10

# Run comprehensive diagnostic
./tools/node_explosion_diagnostic.sh

# Build with diagnostics
rm -rf build && ./build.sh Release
```

## Success Metrics
| Metric | Current | Target |
|--------|---------|--------|
| Avg vs Stash | 12.9x | < 2x |
| Avg vs Komodo | 3.9x | < 1.5x |
| Worst case | 38.5x | < 3x |

## Notes
- Using thread-local storage for diagnostic stats to ensure thread safety with UCI
- Diagnostic mode will have minimal impact when disabled (flag check only)
- Focus on quiescence and move ordering as primary suspects based on explosion patterns