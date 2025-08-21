# Stage 23 CM4: Full Countermoves Implementation Plan

## Current Situation Analysis

### What We Have (CM3.5)
- ✅ Countermove updates on beta cutoffs (from CM2)
- ✅ Countermove lookups working
- ✅ Positioning after killers with bonus=8000
- ✅ +4.71 ELO improvement

### What's Expected
- Target: +25-40 ELO from literature
- Current: +4.71 ELO (~12-19% of expected)
- **Gap: Missing 20-35 ELO**

## Hypothesis: Why Aren't We Getting Full Benefit?

### Possible Issues

1. **Update Frequency**
   - Currently only updating on quiet beta cutoffs
   - Maybe should update on all beta cutoffs?
   - Check if updates are happening frequently enough

2. **Table Persistence**
   - Table is cleared at start of each search
   - Maybe countermoves should persist across searches?
   - Could maintain game-long history

3. **Bonus Too Low**
   - Current: 8000 (between killers and history)
   - Killers get ~16000 priority
   - Maybe countermoves need 12000-16000?

4. **Wrong Position in Ordering**
   - Currently: After killers, before history
   - Maybe should be: Before killers?
   - Or integrated with killers somehow?

5. **Statistics Issue**
   - Are we actually finding countermoves often?
   - Need to check hit rate statistics

## CM4 Implementation Phases

### Phase CM4.1: Add Diagnostics
**Goal:** Understand what's happening

```cpp
// Add to search output
if (info.counterMoveStats.updates > 0) {
    std::cerr << "Countermoves: " 
              << "updates=" << updates
              << " hits=" << hits  
              << " rate=" << (100.0 * hits / attempts) << "%\n";
}
```

### Phase CM4.2: Track Hit Rate
**Goal:** Measure effectiveness

```cpp
// In move ordering
if (counterMove != NO_MOVE) {
    info.counterMoveStats.hits++;
    auto it = std::find(moves.begin(), moves.end(), counterMove);
    if (it != moves.end()) {
        info.counterMoveStats.found++;;
        // Is it causing cutoffs?
        if (/* causes beta cutoff */) {
            info.counterMoveStats.cutoffs++;
        }
    }
}
```

### Phase CM4.3: Test Higher Bonuses
- Test with bonus=12000 (between killers)
- Test with bonus=16000 (equal to killers)
- Test with bonus=20000 (above killers)

### Phase CM4.4: Alternative Positioning
**Test different positions:**

#### Option A: Before Killers
```
1. TT Move
2. Good Captures
3. Countermove  ← NEW POSITION
4. Killer 1
5. Killer 2
6. History
```

#### Option B: Merged with Killers
```
1. TT Move
2. Good Captures
3. Killer 1 OR Countermove (whichever exists)
4. Killer 2
5. History
```

### Phase CM4.5: Persistence Options

#### Option A: Per-Game Persistence
```cpp
// Don't clear between searches
// info.counterMoves.clear();  // REMOVE THIS
```

#### Option B: Decay Instead of Clear
```cpp
// Gradually forget old countermoves
void decay() {
    // Keep 50% of entries randomly
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            if (rand() % 2 == 0) {
                m_counters[i][j] = NO_MOVE;
            }
        }
    }
}
```

## Testing Strategy

### CM4.1: Diagnostics First
1. Add statistics output
2. Run test games
3. Analyze:
   - Update frequency
   - Hit rate
   - Cutoff rate

### CM4.2-4.5: Based on Diagnostics
- If hit rate low → Fix updates
- If hit rate good but no cutoffs → Increase bonus
- If updates rare → Update on more move types

## Success Metrics

### Minimum Success
- +10-15 ELO (double current gain)
- Understand why full benefit not achieved
- Clear path forward

### Target Success  
- +25-40 ELO as expected
- Optimal bonus value found
- Clean, maintainable implementation

## Implementation Order

1. **Start with CM4.1** - Add diagnostics
2. **Analyze statistics** - Understand behavior
3. **Test higher bonuses** - Easy to test via UCI
4. **Try positioning changes** - If bonus doesn't help
5. **Consider persistence** - If table is too sparse

## Key Questions to Answer

1. How often are countermoves being updated?
2. How often do we find a countermove in the move list?
3. When found, how often do they cause cutoffs?
4. Is 8000 bonus enough to prioritize them properly?
5. Should countermoves persist across searches?

## Next Steps

1. Implement CM4.1 diagnostics
2. Run test games to gather statistics
3. Based on results, proceed with most promising approach
4. Target: Achieve +25-40 ELO expected from countermoves