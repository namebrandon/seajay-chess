# Stage 23: Countermove Heuristic Implementation Plan

## Overview
Countermoves are a move ordering heuristic that tracks which moves work well as responses to specific opponent moves. When the opponent plays move A→B, and we respond with C→D that causes a beta cutoff, we remember C→D as the "countermove" for A→B.

**Expected Total Gain:** +25-40 ELO at 2400 strength level
**Implementation Date:** 2025-08-21
**Branch:** feature/20250821-countermoves

## Critical Requirements

### ⚠️ MANDATORY: Hard Stop After Each Phase
- **EVERY phase MUST be followed by OpenBench testing**
- **NO proceeding to next phase without human validation**
- **ALL commits MUST include "bench <node-count>" for OpenBench parsing**
- Even 0 ELO phases get tested (they catch bugs!)

### Bench Count Requirement
Every commit message MUST include the bench count:
```bash
# Get bench count before committing
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'

# Example commit message:
git commit -m "feat: Add countermove infrastructure (Phase CM1) - bench 19191913"
```

## Phased Implementation Approach

### Phase CM1: Infrastructure (0 ELO expected)
**Goal:** Create data structures with no integration
**Validation:** Compilation and unit tests only

1. Create `src/search/countermoves.h`:
   - 64×64 table structure [from_sq][to_sq] → counter_move
   - Clear/initialization methods
   - Getter/setter methods
   - Thread-safe design (each thread gets own instance)

2. Create `src/search/countermoves.cpp`:
   - Basic implementation
   - Memory alignment (cache-line aligned)
   - Total size: 8KB per thread

3. Add unit tests to verify operations

**Commit:** `feat: Add countermove heuristic infrastructure (Phase CM1) - bench [count]`
**🛑 HARD STOP FOR OPENBENCH TESTING**

### Phase CM2: Shadow Mode (0 ELO expected)
**Goal:** Update logic without using for ordering
**Validation:** Statistics collection, no performance impact

1. Integrate update logic in `negamax.cpp`:
   - Update on quiet move beta cutoffs only
   - Skip null moves, captures, promotions
   - Add statistics collection

2. Log countermove updates but don't use them

**Commit:** `feat: Add countermove shadow mode updates (Phase CM2) - bench [count]`
**🛑 HARD STOP FOR OPENBENCH TESTING**

### Phase CM3: Basic Integration (-1 to +1 ELO expected)
**Goal:** Minimal integration to verify no crashes
**Validation:** Stable operation with minimal bonus

1. Add UCI option:
   ```cpp
   options["CountermoveBonus"] = Option(1000, 0, 20000);
   ```

2. Integrate in `move_ordering.cpp`:
   - Add minimal bonus (+1000)
   - Position between killers and history

3. Verify stable sorting and no crashes

**Commit:** `feat: Integrate countermoves with minimal bonus (Phase CM3) - bench [count]`
**🛑 HARD STOP FOR OPENBENCH TESTING**

### Phase CM4: Full Activation (+25-40 ELO expected)
**Goal:** Full performance with tuned parameters
**Validation:** SPRT testing for ELO gain

1. Update default bonus to 8000
2. Fine-tune integration
3. Performance optimization

**Commit:** `feat: Activate countermoves with tuned bonus (Phase CM4) - bench [count]`
**🛑 HARD STOP FOR OPENBENCH TESTING**

### Phase CM5: SPSA Tuning (Optional, +2-5 ELO additional)
**Goal:** Find optimal CountermoveBonus value
**Method:** OpenBench SPSA run

## Implementation Details

### Data Structure Design
```cpp
class CounterMoves {
public:
    static constexpr int DEFAULT_BONUS = 8000;  // Between killers (16K) and history
    
    CounterMoves() { clear(); }
    
    void clear() {
        std::memset(m_counters, 0, sizeof(m_counters));
    }
    
    void update(Move prevMove, Move counterMove) {
        if (prevMove != NO_MOVE && !isCapture(counterMove)) {
            Square from = moveFrom(prevMove);
            Square to = moveTo(prevMove);
            m_counters[from][to] = counterMove;
        }
    }
    
    Move getCounterMove(Move prevMove) const {
        if (prevMove == NO_MOVE) return NO_MOVE;
        Square from = moveFrom(prevMove);
        Square to = moveTo(prevMove);
        return m_counters[from][to];
    }
    
private:
    alignas(64) Move m_counters[64][64];  // 8KB per thread
};
```

### Move Ordering Integration
Priority order (with typical bonuses):
1. **Hash move** - 1,000,000+
2. **Good captures** (SEE >= 0) - 50,000
3. **Killer moves** - 16,000 (existing)
4. **Countermove** - 8,000 (NEW)
5. **History heuristic** - 0-8,192 (existing)
6. **Bad captures** (SEE < 0) - negative
7. **Quiet moves** - base score

### Update Rules
```cpp
// In negamax.cpp, after beta cutoff:
if (score >= beta) {
    // Update only for quiet moves
    if (!isCapture(bestMove) && !isPromotion(bestMove)) {
        // Update killers (existing)
        updateKillers(bestMove, ply);
        
        // Update history (existing)
        updateHistory(bestMove, depth);
        
        // Update countermove (NEW)
        if (ply > 0 && prevMove != NULL_MOVE) {
            searchData.counterMoves.update(prevMove, bestMove);
        }
    }
    return score;
}
```

### Special Cases to Handle
- **Skip updates for:**
  - Null moves (no previous move to counter)
  - Captures (tactical, not positional)
  - Promotions (tactical)
  - Root position (no previous move)
  
- **Handle carefully:**
  - Castling (can have countermoves)
  - En passant (skip - it's a capture)
  - Check evasions (allow countermoves)

## Testing Strategy

### Unit Tests (Phase CM1)
- Table initialization
- Get/set operations
- Clear functionality
- Edge cases (NO_MOVE, NULL_MOVE)

### Integration Tests (Phase CM2-3)
- Update counting statistics
- Verify no crashes
- Check move ordering stability

### Performance Tests (Phase CM4)
Test positions where countermoves excel:
```
// Tactical sequences
"r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3"  // Italian
"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 1 4"  // Four Knights

// Clear best responses
"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"  // Lasker position
"rnbqkb1r/pp1ppppp/5n2/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq c6 0 3"  // Sicilian
```

### SPRT Settings
- TC: 15+0.05
- Confidence: 95%
- Elo bounds: [0, 5] for infrastructure phases
- Elo bounds: [10, 35] for activation phase

## Expert Insights from Top Engines

### Stockfish Approach
- Compact Move storage
- Integration with continuation history
- Updates on all beta cutoffs

### Ethereal Approach  
- Simple 64×64 table
- Clean move picker integration
- No complex replacement schemes

### Weiss Approach
- Piece-to-square indexing for memory efficiency
- Depth-based bonus updates
- Positioned between killers and history

## Common Pitfalls to Avoid

1. **Memory alignment** - Ensure cache-line alignment
2. **Thread safety** - Each thread needs own table
3. **Overwriting good counters** - Consider depth-based replacement
4. **Interaction conflicts** - Balance with killer/history scores
5. **Special move handling** - Proper treatment of castling/promotions

## Alternative Design (Memory Optimized)
For memory-constrained environments:
```cpp
// Piece-to-square: 6 × 64 × 2 = 768 bytes vs 8KB
Move m_counters[6][64];  // [piece_type][to_square]
```

## Success Criteria

### Per Phase:
- **CM1:** Clean compilation, unit tests pass, 0 ELO regression
- **CM2:** Update statistics collected, 0 ELO regression  
- **CM3:** Stable integration, -1 to +1 ELO
- **CM4:** +25-40 ELO gain confirmed via SPRT
- **CM5:** Optimal parameter found via SPSA

### Overall:
- Thread-safe implementation
- No performance regression in phases CM1-3
- Significant ELO gain in phase CM4
- Clean integration with existing heuristics
- Tunable via UCI option

## Progress Tracking

| Phase | Status | Commit Hash | Bench Count | ELO Result | Date |
|-------|--------|-------------|-------------|------------|------|
| CM1   | Pending | - | - | - | - |
| CM2   | - | - | - | - | - |
| CM3   | - | - | - | - | - |
| CM4   | - | - | - | - | - |
| CM5   | - | - | - | - | - |

## Notes
- Each phase requires OpenBench test completion before proceeding
- Human validation required between phases
- All commits must include "bench <count>" for OpenBench
- UCI option enables runtime tuning without recompilation
- Follows SeaJay principle: no #ifdef for core features