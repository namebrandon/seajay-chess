# LMR (Late Move Reduction) Analysis for SeaJay

## Current Implementation Summary

### Formula (Linear)
```
reduction = baseReduction + (depth - minDepth) / depthFactor + lateMoveBonus
```

Where:
- `baseReduction = 1` (default)
- `minDepth = 3` (don't reduce if depth < 3)
- `depthFactor = 3`
- `lateMoveBonus = (moveNumber - 8) / 4` for moves > 8
- `minMoveNumber = 6` (don't reduce first 5 moves)

### Current Conditions (what's NOT reduced)
1. First 5 moves (moveNumber < 6)
2. Captures
3. Moves when in check
4. Moves that give check
5. Depth < 3

### Missing Conditions (problems identified)
1. **No killer move exemption** - Killers are good moves, shouldn't reduce
2. **No history score consideration** - High history = good move
3. **No PV node special handling** - PV nodes need careful reduction
4. **Linear formula too simplistic** - Most engines use logarithmic
5. **No improving/non-improving logic** - Should reduce more when not improving
6. **No countermove consideration** - Countermoves often good

## Problems Found

### 1. Overly Simplistic Linear Formula
Current: `reduction = 1 + (depth-3)/3 + late_move_bonus`

This gives reductions like:
- Depth 3: 1 ply reduction
- Depth 6: 2 ply reduction  
- Depth 9: 3 ply reduction
- Depth 12: 4 ply reduction

Most strong engines use logarithmic tables based on depth AND move number.

### 2. Missing Move Quality Indicators
The code doesn't check:
- Killer moves (from move ordering)
- History heuristic scores
- Countermoves
- Whether position is improving

### 3. Test Results from Investigation
From `depth_deficit_investigation_results.md`:
- LMR changes best move (d2d4 vs b1c3) - TOO AGGRESSIVE
- Saves 78% nodes but may miss good moves
- Contributing to 20% of depth deficit

## Proposed Improvements

### Phase 3a: Logarithmic Reduction Table
Replace linear formula with:
```cpp
// Stockfish-inspired logarithmic table
int reductionTable[64][64]; // [depth][moveNumber]

// Initialize with logarithmic formula
for (int d = 1; d < 64; d++) {
    for (int m = 1; m < 64; m++) {
        reductionTable[d][m] = int(0.5 + log(d) * log(m) / 2.0);
    }
}
```

### Phase 3b: Add Move Quality Conditions
Don't reduce or reduce less for:
1. Killer moves
2. Moves with good history scores (top 25%)
3. Countermoves
4. Tactical moves (even non-captures)

### Phase 3c: Improving/Non-improving Logic
Track if eval is improving:
```cpp
bool improving = (eval > previousEval);
if (!improving) {
    reduction += 1; // Reduce more when not improving
}
```

## Test Results

### Comparison Test (startpos moves e2e4 e7e5)
**With LMR enabled:**
- Depth 10: 818,120 nodes
- Depth 9: 212,516 nodes  
- Score: cp -302 at depth 10
- Move ordering: 83.1% efficiency

**With LMR disabled:**
- Depth 9: 1,634,437 nodes (7.7x more!)
- Depth 8: 563,911 nodes
- Score: cp -279 at depth 9
- Could only reach depth 9 in same time

**Key Findings:**
1. LMR saves ~87% of nodes (good!)
2. Both find same best move (b1c3)
3. LMR allows 1 extra ply in same time
4. Score slightly different (-302 vs -279) but acceptable

## Expected Impact
- Better move selection (fix wrong best move issue)
- 1-2 ply deeper searches from efficiency
- +30-50 Elo total from all LMR improvements