# Node Explosion Diagnostic Plan

## Problem Statement
SeaJay is searching 5-10x more nodes than other engines at the same depth, indicating fundamental inefficiencies in the search algorithm. This diagnostic plan will help identify the root causes.

## Diagnostic Implementation Plan

### Phase 1: Baseline Comparison
Use `tools/analyze_position.sh` to establish baseline comparisons with other engines (Komodo, Stash, Laser) at various depths.

#### Test Positions
1. **Starting position**: `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`
2. **Middle game**: `r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17`
3. **Endgame**: `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1`
4. **Tactical**: `r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4`

Run each position at depths 1-10 and record node counts.

### Phase 2: Search Statistics Collection

#### A. Pruning Effectiveness Instrumentation
Add counters to track:
```cpp
struct PruningStats {
    // Futility Pruning
    uint64_t futility_attempts = 0;
    uint64_t futility_prunes = 0;
    uint64_t futility_nodes_saved = 0;
    
    // Razoring
    uint64_t razor_attempts = 0;
    uint64_t razor_prunes = 0;
    uint64_t razor_nodes_saved = 0;
    
    // Null Move Pruning
    uint64_t null_move_attempts = 0;
    uint64_t null_move_cutoffs = 0;
    uint64_t null_move_nodes_saved = 0;
    
    // Move Count Pruning
    uint64_t move_count_attempts[9] = {0}; // depths 0-8
    uint64_t move_count_prunes[9] = {0};
    uint64_t move_count_nodes_saved[9] = {0};
    
    // LMR
    uint64_t lmr_reductions = 0;
    uint64_t lmr_re_searches = 0;  // Failed reductions requiring full search
    uint64_t lmr_nodes_saved = 0;
};
```

#### B. Move Ordering Quality Metrics
```cpp
struct MoveOrderingStats {
    uint64_t beta_cutoffs[64] = {0};  // Indexed by move number
    uint64_t first_move_cutoffs = 0;
    uint64_t total_cutoffs = 0;
    
    // Move sources that caused cutoffs
    uint64_t tt_move_cutoffs = 0;
    uint64_t killer_cutoffs = 0;
    uint64_t counter_cutoffs = 0;
    uint64_t history_cutoffs = 0;
    uint64_t other_cutoffs = 0;
};
```

#### C. Depth Distribution Analysis
```cpp
struct DepthStats {
    uint64_t nodes_at_depth[MAX_DEPTH] = {0};
    uint64_t qsearch_nodes = 0;
    uint64_t main_search_nodes = 0;
    double effective_branching_factor[MAX_DEPTH] = {0.0};
};
```

### Phase 3: Comparative Analysis Script

Create `tools/node_explosion_diagnostic.sh`:
```bash
#!/bin/bash
# Runs comparative analysis between SeaJay and reference engines

POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    "r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17"
    # ... more positions
)

for pos in "${POSITIONS[@]}"; do
    for depth in {1..10}; do
        # Use analyze_position.sh to get node counts
        ./tools/analyze_position.sh "$pos" depth $depth --save-report
        
        # Extract node ratios
        # SeaJay_nodes / Komodo_nodes = explosion_factor
    done
done

# Generate explosion factor graph per depth
```

### Phase 4: Hot Path Analysis

#### Identify Problem Patterns
1. **Position types with worst explosion**:
   - Open positions vs closed
   - Tactical vs quiet
   - Endgame vs middlegame

2. **Move types consuming most nodes**:
   - Captures that don't get pruned
   - Checks in quiescence
   - Late moves that should be reduced

### Phase 5: Debug Output Mode

Add UCI option for diagnostic output:
```cpp
setoption name NodeDiagnostics value true
```

This enables real-time output showing:
```
info string depth 5: main=45234 qsearch=89234 (66.4% in qsearch)
info string depth 5: futility=234/1000 (23.4% success)
info string depth 5: first_move_cutoff=67.8%
info string depth 5: EBF=4.73 (target: 2.5)
```

### Phase 6: Root Cause Analysis

#### Common Culprits to Check:
1. **SEE Implementation (HIGH PRIORITY)**:
   - SEE has historically caused issues in SeaJay
   - May not be properly pruning bad captures
   - Could be incorrectly evaluating exchanges
   - Check if SEE is actually being used in quiescence
   - Verify SEE calculations match expected values
   - Test with SEE disabled vs enabled to measure impact
   - Note: Previous fixes may have been masking SEE as the true culprit

2. **Quiescence explosion**: 
   - Not pruning bad captures (likely SEE-related)
   - Following checks too deep
   - Not limiting qsearch nodes
   - Delta pruning not working

3. **Poor move ordering**:
   - TT move not tried first
   - Killers/history not working
   - MVV-LVA issues
   - SEE not being used for capture ordering

4. **Pruning bugs**:
   - Conditions too conservative
   - Wrong depth calculations
   - Side-to-move confusion

5. **LMR issues**:
   - Not reducing enough moves
   - Re-searching too often
   - Wrong reduction amounts

### Phase 7: Fix Priority Matrix

| Issue | Node Impact | Fix Difficulty | Priority |
|-------|------------|----------------|----------|
| SEE implementation | Very High (3-5x) | Medium-High | 1 |
| Quiescence explosion | Very High (3-5x) | Medium | 2 |
| Move ordering | High (2-3x) | Low | 3 |
| Futility pruning | Medium (1.5-2x) | Low | 4 |
| LMR tuning | Medium (1.5x) | Low | 5 |
| Null move | Low-Medium (1.3x) | Medium | 6 |

## Implementation Steps

1. **Create diagnostic branch**: `bugfix/<date>-node-explosion-diagnostic`
2. **Add statistics collection** without changing search behavior
3. **Run baseline tests** with analyze_position.sh
4. **Identify worst offender** from statistics
5. **Fix highest-impact issue** first
6. **Re-test and measure** improvement
7. **Iterate** until node counts are reasonable

## Success Metrics

Target: Reduce node explosion from 5-10x to under 2x compared to Komodo at depth 10.

| Depth | Current Ratio | Target Ratio |
|-------|--------------|--------------|
| 5 | ~5x | <2x |
| 7 | ~7x | <2x |
| 10 | ~10x | <2.5x |

## Expected Outcomes

1. **Identify primary cause**: Usually 1-2 major issues cause 80% of explosion
2. **Quick wins**: Often simple fixes (e.g., qsearch depth limit) give huge improvements
3. **Better understanding**: Know exactly where nodes are being wasted
4. **Measurable progress**: Track improvement with each fix

## Tools and Scripts

- `tools/analyze_position.sh` - Multi-engine comparison
- `tools/node_explosion_diagnostic.sh` - To be created
- UCI diagnostic mode - To be added
- Real-time statistics output - To be implemented

## Timeline Estimate

- Phase 1-2: 2-3 hours (instrumentation)
- Phase 3-4: 1-2 hours (analysis)
- Phase 5-6: 2-3 hours (debugging)
- Phase 7: Varies by issue complexity

Total: 1-2 days to identify and fix primary causes