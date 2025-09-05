# Node Explosion Diagnostic - Overall Feature Status

## Branch: `bugfix/20250905-node-explosion-diagnostic`
**Created**: 2025-09-05  
**Issue**: SeaJay searches 5-10x more nodes than comparable engines  
**Goal**: Reduce node explosion from average 12.9x to under 2x vs Stash

## Investigation Summary

### Phase 1: Baseline Analysis [COMPLETED]
- Identified 38.5x node explosion vs Stash at depth 10
- Established diagnostic infrastructure for tracking

### Phase 2: Diagnostic Infrastructure [COMPLETED]
- Added comprehensive `NodeExplosionStats` structure
- Implemented thread-local statistics for UCI compatibility
- Added move type tracking for beta cutoffs
- Created diagnostic scripts for automated testing

### Phase 3: Root Cause Analysis [COMPLETED]
- Found move ordering mechanics were correct (TT moves always placed first)
- Identified the real issue: pruning parameters too conservative
- First-move cutoff only 69% (should be 90%+)

### Phase 4: Move Ordering Investigation [COMPLETED]
**Sub-branch**: `bugfix/nodexp/20250905-move-ordering`
- Enhanced diagnostics to track move type effectiveness
- Discovered TT moves only cause 26% of cutoffs despite being searched first
- Applied SPSA-tuned pruning parameters
- See detailed analysis: [feature_status_move_ordering.md](feature_status_move_ordering.md)

## SPRT Test Results

### Move Ordering + SPSA Parameters vs Main
**Result**: +38.06 ± 14.51 ELO (PASS)
- SPRT: 10.0+0.10s Threads=1 Hash=8MB
- LLR: 2.98 (-2.94, 2.94) [-1.00, 5.00]
- Games: N=1228 W=455 L=321 D=452
- Penta: [32, 105, 250, 151, 76]
- Link: https://openbench.seajay-chess.dev/test/447/

## Key Achievements

### Diagnostic Improvements
1. **Move type tracking** - Shows which types cause beta cutoffs (TT/killer/capture/quiet)
2. **TT effectiveness tracking** - Shows how often TT moves are found, valid, and placed first
3. **Cutoff position analysis** - Distribution of which move index causes cutoffs
4. **Pruning effectiveness metrics** - Tracks futility, move count, LMR success rates

### Performance Improvements (with SPSA parameters)
- **Total nodes**: 637K → 175K (72% reduction!)
- **First-move cutoff**: 69.1% → 78.1%
- **Late cutoffs**: 1556 → 318 (80% reduction)
- **TT cutoff effectiveness**: 26% → 47%

### Applied SPSA Parameters
All pruning parameters updated based on 250k game tuning session:
- More aggressive LMR (depth 3→2, moves 4→2)
- Deeper null move reductions (base 2→4)
- Much tighter move count limits (e.g., 12→7 at depth 3)
- Higher futility margins (Base=240, Scale=73)

## Files Created/Modified

### Documentation
- `feature_status.md` - This overall tracking document
- `feature_status_move_ordering.md` - Detailed move ordering investigation
- `move_ordering_investigation.md` - Technical analysis notes

### Test Scripts
- `test_spsa_params.sh` - Tests SPSA parameters vs defaults
- `apply_spsa_tuning.sh` - Helper to apply SPSA values

### Source Changes
- `src/search/node_explosion_stats.h/cpp` - Diagnostic infrastructure
- `src/search/negamax.cpp` - Enhanced beta cutoff tracking
- `src/search/types.h` - Updated SearchLimits defaults
- `src/uci/uci.h/cpp` - Updated UCI option defaults

## Next Steps

While we've achieved a 72% reduction in nodes searched, further optimization is possible:

1. **Evaluation improvements** - Better eval = better move selection
2. **Aspiration window tuning** - More aggressive windows could help
3. **Quiet move generation** - Investigate base move generation order
4. **Singular extensions** - Could help identify critical moves

## Conclusion

The node explosion issue has been significantly improved through:
1. Comprehensive diagnostic infrastructure
2. Systematic root cause analysis  
3. Application of SPSA-tuned parameters

The engine now searches approximately 3-5x more nodes than Stash (down from 38.5x), with a proven +38 ELO improvement over the previous main branch.