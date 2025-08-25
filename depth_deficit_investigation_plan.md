# SeaJay Depth Deficit Root Cause Investigation Plan

## Problem Statement
SeaJay searches 10-11 ply while opponents reach 19-22 ply with similar node counts, indicating severe search inefficiency. Time management fixes (Phase 1a-1c) showed no improvement, suggesting other root causes.

## Investigation Hypotheses

### H1: Missing Search Extensions
**Theory**: Lack of check extensions and other search extensions causes shallow tactical searches.
**Expected Impact**: 2-4 ply depth loss in tactical positions

### H2: Overly Aggressive LMR (Late Move Reductions)
**Theory**: Reducing too many moves or reducing too aggressively, missing important variations.
**Expected Impact**: 1-3 ply effective depth loss

### H3: Poor Move Ordering
**Theory**: Bad move ordering causes more nodes to be searched, reducing efficiency.
**Expected Impact**: 2-3x node waste, 2-3 ply depth loss

### H4: Overly Aggressive Pruning
**Theory**: Null move, futility, or other pruning cutting off good lines.
**Expected Impact**: Missing tactics, 1-2 ply depth loss

### H5: Quiescence Search Explosion
**Theory**: Too many nodes spent in quiescence, not enough for main search.
**Expected Impact**: 50%+ nodes in qsearch, 3-5 ply main search depth loss

## Investigation Methodology

### Test Positions
We'll use tactical positions where depth matters:
1. **Forced mate sequences** - Should find with extensions
2. **Deep tactics** - Combinations 5+ moves deep
3. **Check sequences** - Where check extensions critical
4. **Quiet positions** - Where pruning/LMR should work well
5. **Complex middlegames** - Real game positions from PGNs

### Test Suite Sources
- Extract critical positions from the 4ku/Laser games where SeaJay blundered
- Standard tactical test suites (if available)
- Positions where Stockfish finds tactics at specific depths

---

## Test Protocols

### Test 1: Search Extensions Analysis

**Objective**: Confirm if missing extensions cause depth deficit in tactical positions

**Method**:
1. Select 5 positions with forcing sequences (checks, captures)
2. Compare SeaJay vs Stockfish at fixed time (5 seconds):
   - Record depth reached
   - Record if tactic found
   - Note selective depth
3. Analyze positions with checks specifically:
   ```bash
   # Position with many checks (example)
   echo -e "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -\\ngo movetime 5000\\nquit" | ./build/seajay
   echo -e "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -\\ngo movetime 5000\\nquit" | ./external/engines/stockfish/stockfish
   ```

**Expected Results**:
- If extensions missing: SeaJay depth significantly lower in tactical positions
- Stockfish should reach 3-5 ply deeper in forcing positions

**Data to Collect**:
- Depth reached
- Move found
- Time to find best move
- Node count

---

### Test 2: LMR Aggressiveness Analysis

**Objective**: Determine if LMR is reducing too many important moves

**Method**:
1. Enable detailed LMR statistics in UCI output
2. Run same position with LMR on vs off:
   ```bash
   # With LMR (current)
   echo -e "setoption name LMREnabled value true\\nposition startpos\\ngo depth 10\\nquit" | ./build/seajay
   
   # Without LMR
   echo -e "setoption name LMREnabled value false\\nposition startpos\\ngo depth 10\\nquit" | ./build/seajay
   ```
3. Compare:
   - Nodes searched
   - Depth reached  
   - Move quality

**Expected Results**:
- If LMR too aggressive: Disabling LMR finds better moves despite fewer plies
- Normal: LMR saves 40-60% nodes with minimal quality loss

**Data to Collect**:
- LMR reduction statistics (how many plies reduced on average)
- Percentage of moves reduced
- Quality difference with/without LMR

---

### Test 3: Move Ordering Efficiency

**Objective**: Measure move ordering quality via beta cutoff statistics

**Method**:
1. Analyze UCI output for move ordering efficiency (already reported):
   ```bash
   echo -e "position startpos\\ngo depth 12\\nquit" | ./build/seajay 2>&1 | grep "moveeff"
   ```
2. Compare with theoretical ideal (best move first = 90%+ efficiency)
3. Test specific move ordering components:
   - TT move hit rate
   - Killer move effectiveness
   - History heuristic effectiveness

**Expected Results**:
- Good ordering: 85%+ first move beta cutoffs
- Poor ordering: <70% first move beta cutoffs

**Data to Collect**:
- Move ordering efficiency percentage
- Beta cutoff distribution (1st move, 2nd move, etc.)
- TT hit rate

---

### Test 4: Pruning Aggressiveness Analysis

**Objective**: Determine if pruning is cutting important lines

**Method**:
1. Test with null move on/off:
   ```bash
   # Compare depths and moves found
   echo -e "setoption name UseNullMove value true\\nposition startpos moves e2e4 e7e5\\ngo depth 12\\nquit" | ./build/seajay
   echo -e "setoption name UseNullMove value false\\nposition startpos moves e2e4 e7e5\\ngo depth 12\\nquit" | ./build/seajay
   ```
2. Check pruning statistics if available
3. Test on tactical positions where pruning might fail

**Expected Results**:
- Normal: Null move saves 30-40% nodes with same move quality
- Too aggressive: Missing tactics, different moves found

**Data to Collect**:
- Nodes with/without pruning
- Moves found with/without
- Depth reached

---

### Test 5: Quiescence Search Analysis

**Objective**: Determine if qsearch is consuming too many nodes

**Method**:
1. Add node counting for main search vs quiescence
2. Run on various position types:
   ```bash
   # Quiet position
   echo -e "position fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq -\\ngo depth 10\\nquit" | ./build/seajay
   
   # Tactical position (many captures possible)
   echo -e "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -\\ngo depth 10\\nquit" | ./build/seajay
   ```
3. Calculate qsearch percentage of total nodes

**Expected Results**:
- Normal: 20-40% nodes in qsearch
- Explosion: >60% nodes in qsearch

**Data to Collect**:
- Main search nodes
- Quiescence nodes
- Ratio by position type
- Maximum qsearch depth reached

---

## Diagnostic Output Requirements

We need to add/enable these UCI info outputs:
```cpp
info depth 10 seldepth 25 nodes 1234567 nps 500000 time 2469
info string Main nodes: 800000 QSearch nodes: 434567 (35.2%)
info string LMR reductions: 45000/50000 moves (90%), avg reduction: 2.3 ply
info string Beta cutoffs: 1st move: 87%, 2nd-3rd: 8%, rest: 5%
info string TT hits: 65%, TT cutoffs: 12000
info string Null move cutoffs: 8500 (saved ~300000 nodes)
info string Extensions: check: 0, singular: 0, one-reply: 0
```

---

## Test Execution Plan

### Phase 1: Baseline Measurements (Current main branch)
1. Run all 5 test protocols on main branch
2. Document baseline metrics
3. Identify positions where SeaJay fails

### Phase 2: Comparative Analysis
1. Run same tests on Stockfish for reference
2. Compare metrics to identify anomalies
3. Focus on largest discrepancies

### Phase 3: Targeted Testing
1. Deep dive into identified problem areas
2. Create specific test cases for failures
3. Validate hypotheses with evidence

---

## Success Criteria

A hypothesis is CONFIRMED if:
1. **Extensions**: SeaJay reaches 3+ ply less than Stockfish in tactical positions
2. **LMR**: Disabling LMR improves move quality significantly
3. **Move Ordering**: Efficiency below 75%
4. **Pruning**: Missing obvious tactics that appear without pruning
5. **Quiescence**: >50% of nodes spent in qsearch

---

## Expected Outcomes

Based on investigation, we'll have:
1. **Confirmed root causes** with evidence
2. **Quantified impact** of each issue  
3. **Priority order** for fixes
4. **Specific test positions** demonstrating each problem
5. **Clear success metrics** for validating fixes

This investigation should take 2-3 hours but will save days of wrong fixes.