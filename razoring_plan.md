# Razoring Implementation Plan

Author: SeaJay Development Team  
Date: 2025-09-02  
Branch: feature/20250902-pruning-optimization  
Status: Phase R2 Implementation

## Executive Summary

Razoring is a shallow-depth pruning technique that drops to quiescence search when the static evaluation is far below alpha. This is a well-understood, low-risk optimization that complements our successful futility pruning improvements.

**Phase R1 Results:** -5 ELO loss (minor) - likely due to lack of tactical guards  
**Phase R2 Goal:** Add tactical awareness to prevent pruning in sharp positions

## Technical Design

### Core Concept
- **Trigger:** At depths 1-2, when staticEval + margin <= alpha
- **Action:** Run qsearch(alpha, alpha+1) instead of full search
- **Result:** If qsearch still fails low, return immediately

### Implementation Location
In `negamax()` after:
- TT probe completed
- Draw checks done  
- Static evaluation computed
- In-check status known

Before:
- Move generation
- LMR decisions
- Other pruning techniques

### Safety Conditions
1. **Never razor when:**
   - isPvNode (PV nodes need full search)
   - weAreInCheck (tactical positions)
   - Near mate bounds (abs(alpha) >= MATE_BOUND - MAX_PLY)

2. **Additional guards:**
   - Tactical presence: If SEE >= 0 captures exist, skip or use stronger margin
   - Phase-aware: Stricter in endgames (zugzwang risk)

### Margin Strategy

#### Phase 1: Conservative Baseline
- Depth 1: 300 cp
- Depth 2: 500 cp

#### Phase 2: Phase-Adjusted (if Phase 1 succeeds)
- Endgame: +100 cp (more zugzwang risk)
- Open middlegame: -50 cp (more tactical fluidity)

#### Phase 3: Tactical Guards (if needed)
- If parent has SEE >= 0 captures: +150 cp or skip
- If many checks available: skip razoring

## Implementation Phases

### Phase R1: Basic Razoring (Conservative)
**Goal:** Establish baseline with maximum safety

```cpp
// After staticEval computed, before move generation
if (!isPvNode && !weAreInCheck && depth <= 2) {
    if (abs(alpha.value()) < MATE_BOUND - MAX_PLY) {
        int razorMargin = (depth == 1) ? 300 : 500;
        if (staticEval + razorMargin <= alpha) {
            Score qScore = quiescence(alpha, alpha + 1, 0);
            if (qScore <= alpha) {
                // Update telemetry
                info.razorCutoffs++;
                info.razorDepthBuckets[depth-1]++;
                return qScore;
            }
        }
    }
}
```

**Testing:**
- SPRT bounds: [0.00, 5.00] nELO
- Monitor: razor cutoff rate, PVS re-search rate
- Success criteria: Positive ELO, no tactical regression

### Phase R2: Tactical Awareness (CURRENT IMPLEMENTATION)
**Goal:** Add SEE-based guard for tactical positions to address Phase R1's minor ELO loss

**Implementation Details:**

1. **Tactical Guard Implementation**
```cpp
// Generate captures only to check for tactical presence
MoveList caps;
MoveGenerator::generateCaptures(board, caps);

// Check if any capture has SEE >= 0
bool hasTacticalMoves = false;
for (const Move& m : caps) {
    if (isCapture(m) && seajay::seeGE(board, m, 0)) {
        hasTacticalMoves = true;
        break;
    }
}

// Apply stronger margin if tactical moves exist
int razorMargin = (depth == 1) ? limits.razorMargin1 : limits.razorMargin2;
if (hasTacticalMoves) {
    razorMargin += 150;  // Add tactical bonus
}
```

2. **TT Storage Improvements**
```cpp
// Wrap TT store with enable check
if (tt && tt->isEnabled()) {
    // Adjust mate scores before storing
    int16_t scoreToStore = qScore.value();
    if (qScore >= MATE_BOUND - MAX_PLY) {
        scoreToStore = qScore.value() + ply;  // Adjust mate score
    } else if (qScore <= -MATE_BOUND + MAX_PLY) {
        scoreToStore = qScore.value() - ply;  // Adjust mated score
    }
    
    tt->store(zobristKey, NO_MOVE, scoreToStore, TT_EVAL_NONE,
             static_cast<uint8_t>(depth), Bound::UPPER);
}
```

3. **UCI Options Visibility**
Add to handleUci() options output:
```cpp
std::cout << "option name UseRazoring type check default false" << std::endl;
std::cout << "option name RazorMargin1 type spin default 300 min 100 max 800" << std::endl;
std::cout << "option name RazorMargin2 type spin default 500 min 200 max 1200" << std::endl;
```

4. **Optional Safety Refinements**
- **Endgame guard:** Increase margins by +100cp when non-pawn material < ENDGAME_THRESHOLD
- **SEE threshold:** Use seeGE(move, +50) for stricter tactical detection if needed
- **Telemetry:** Track razorTacticalSkips when tactical guard triggers

**Testing:**
- SPRT bounds: [0.00, 5.00] nELO (expect positive after tactical guards)
- Focus on tactical test positions
- Monitor razor cutoff rate changes with tactical guard

### Phase R3: Phase-Aware Margins (If R1-R2 pass)
**Goal:** Optimize margins based on game phase

```cpp
int getPhaseAdjustedRazorMargin(int depth, const Board& board) {
    int base = (depth == 1) ? 300 : 500;
    
    // Endgame adjustment
    if (board.getPhase() <= 4) {  // Endgame
        base += 100;  // Stricter for zugzwang
    } else if (board.getPhase() >= 20) {  // Opening/early middle
        base -= 50;   // More aggressive
    }
    
    return base;
}
```

**Testing:**
- SPRT bounds: [0.00, 5.00] nELO
- Test on endgame positions specifically
- Monitor zugzwang detection

## UCI Options

```cpp
// Phase R1
option name UseRazoring type check default false
option name RazorMargin1 type spin default 300 min 100 max 800
option name RazorMargin2 type spin default 500 min 200 max 1200

// Phase R2 (if implemented)
option name RazorTacticalBonus type spin default 150 min 0 max 400

// Phase R3 (if implemented)  
option name RazorEndgameBonus type spin default 100 min 0 max 300
option name RazorOpeningReduction type spin default 50 min 0 max 200
```

## Telemetry Integration

### New SearchData fields:
```cpp
struct SearchData {
    // Existing fields...
    
    // Razoring statistics
    int razorAttempts = 0;
    int razorCutoffs = 0;
    std::array<int, 2> razorDepthBuckets = {0, 0};  // [d1, d2]
    int razorTacticalSkips = 0;  // Phase R2
    int razorPhaseAdjustments = 0;  // Phase R3
};
```

### Reporting format:
```
razor: att=X cut=Y cut%=Z.Z% razor_b=[d1,d2]
```

## TT Interaction

When razoring causes a cutoff:
```cpp
ttEntry.store(
    board.getHash(),
    NO_MOVE,  // Or best qsearch move if tracked
    qScore,   // Mate-adjusted
    depth,    // Current depth
    TT::UPPER_BOUND,
    age
);
```

## Testing Protocol

### Local Validation (Each Phase)
```bash
# Node count baseline
echo -e "position startpos\\ngo depth 10" | ./bin/seajay | grep nodes

# Tactical safety check
echo -e "position fen r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/1R2R1K1 w kq - 2 17\\ngo depth 10" | ./bin/seajay

# With telemetry
echo -e "setoption name SearchStats value true\\nsetoption name UseRazoring value true\\nposition startpos\\ngo depth 10\\nquit" | ./bin/seajay
```

### SPRT Configuration
```
Dev Branch: feature/20250902-pruning-optimization
Base Branch: <previous-phase-commit>
Time Control: 10+0.1
Hash: 8MB
Book: UHO_4060_v2.epd
Test Type: SPRT
Bounds: [0.00, 5.00] nELO (Phase R1)
UCI Options: UseRazoring=true
```

## Success Metrics

### Phase R1 (Conservative)
- Node reduction: 3-5% at depth 10
- Razor cutoff rate: 5-10% at depths 1-2
- PVS re-search rate: < 7%
- SPRT: Positive ELO gain

### Phase R2 (Tactical)
- Maintained node reduction from R1
- Reduced tactical blindness
- razorTacticalSkips: 10-20% of razor attempts

### Phase R3 (Phase-aware)
- Additional 1-2% node reduction
- Better endgame performance
- No zugzwang failures

## Risk Assessment

### Low Risk
- Conservative margins prevent most tactical issues
- Multiple safety guards in place
- Well-understood technique used by many engines

### Potential Issues
- Zugzwang in endgames (mitigated by phase adjustment)
- Tactical blindness (mitigated by SEE guard)
- Interaction with other pruning (razoring runs first)

## Implementation Timeline

1. **Hour 1:** Implement Phase R1 basic razoring
2. **Hour 2:** Add telemetry, UCI options, testing
3. **SPRT Test:** 2-4 hours for Phase R1
4. **If R1 passes:** Implement R2 tactical guards
5. **If R2 passes:** Implement R3 phase awareness

## Key Design Decisions

1. **Depths 1-2 only:** Deeper razoring too risky without extensive tuning
2. **Conservative margins:** Start safe, optimize later
3. **Qsearch window:** (alpha, alpha+1) for minimal qsearch work
4. **TT handling:** Store as UPPER bound with current depth
5. **Ordering:** Razoring before other pruning for maximum effect

## Expected Interaction with Existing Pruning

- **Futility pruning:** Complementary - razoring at node level, futility per-move
- **Null move:** Independent - different depth ranges
- **LMR:** Independent - razoring before move generation
- **Delta pruning:** Works together in qsearch

## Next Steps After Razoring

Based on success of razoring implementation:
1. **If strong gains:** Consider depth 3 razoring with larger margins
2. **If modest gains:** Move to probcut or history-based pruning
3. **If neutral:** Focus on LMR tuning instead