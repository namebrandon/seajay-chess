# Move Ordering Investigation - Node Explosion Fix

## Branch: `bugfix/nodexp/20250905-move-ordering`
**Parent Branch**: `bugfix/20250905-node-explosion-diagnostic`  
**Created**: 2025-09-05  
**Issue**: Move ordering completely broken - 0% first-move cutoff rate

## Problem Statement

Diagnostic analysis revealed that SeaJay's move ordering is completely broken:
- **First-move cutoff rate**: 0% (should be 90%+)
- **Top-3 cutoff rate**: 0% (should be 95%+)  
- **Result**: 38.5x more nodes than Stash at depth 10

This is the primary cause of the node explosion problem.

## Investigation Plan

### Phase 1: Diagnostic Enhancement
- [ ] Add more detailed move ordering diagnostics
- [ ] Track which move (1st, 2nd, 3rd, etc.) causes beta cutoff
- [ ] Track TT move hit rate and position in move list
- [ ] Track killer move effectiveness

### Phase 2: Component Testing
- [ ] Test TT move extraction and ordering
- [ ] Test killer moves are being stored and retrieved
- [ ] Test history heuristic updates and scoring
- [ ] Test MVV-LVA ordering for captures
- [ ] Test countermove implementation

### Phase 3: Root Cause Analysis
- [ ] Identify why cutoffs aren't happening early
- [ ] Check if moves are being ordered at all
- [ ] Verify beta cutoff logic is correct
- [ ] Check for inverted scoring or ordering

### Phase 4: Fix Implementation
- [ ] Fix identified issues
- [ ] Test each fix incrementally
- [ ] Measure improvement with diagnostics

## Expected Outcomes

With proper move ordering:
- First-move cutoff rate: >90%
- Node reduction: 10-40x
- Performance comparable to other engines

## Test Command

```bash
echo -e "uci
setoption name NodeExplosionDiagnostics value true
position fen r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17
go depth 10
quit" | ./bin/seajay 2>&1 | grep -A 10 "Node Explosion"
```

## Progress Log

### 2025-09-05: Investigation Started
- Created branch hierarchy for organized fix approach
- Identified move ordering as primary culprit
- 0% first-move cutoff rate is catastrophic for alpha-beta efficiency

### 2025-09-05: Fixed Tracking Issue
- **Found**: Beta cutoff tracking was broken in diagnostics
- **Reality**: First-move cutoff is 69.1% (not 0%)
- **Still a problem**: Should be 90%+ for good move ordering
- **Added**: Enhanced diagnostics to track cutoff types (TT, killer, capture, quiet)

### Current Status (Updated)
- First-move cutoff: 69.1% (needs +20% improvement)
- Top-3 cutoff: 85.2% (needs +10% improvement)  
- Late cutoffs: 1556 (too many)
- Node explosion: Still 38.5x vs Stash

### Latest Findings (2025-09-05)

**TT Move Analysis:**
- TT moves are found 20,100 times and ALWAYS placed first (100%)
- But TT moves only cause 15,199 cutoffs (75.6% effectiveness)
- This means TT moves are placed correctly but often don't cause cutoffs
- Possible causes:
  - TT storing moves from different depths
  - TT pollution from sibling nodes
  - Shallow searches polluting deep search TT entries

**Move Type Cutoff Breakdown:**
- Captures: 48.2% (highest - good MVV-LVA ordering)
- TT moves: 26.1% (should be higher)
- Killers: 19.7% (reasonable)
- Quiet: 6.1% (expected to be low)

### Root Causes (Refined):
1. **TT moves not always good** - 25% of TT moves don't cause cutoffs
2. **First-move cutoff at 69%** - Even with TT moves first, still searching too many moves
3. **Move generation ordering** - May need to check if quiet moves are ordered properly