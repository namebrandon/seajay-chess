# SeaJay Pruning Optimization Plan

**Branch:** feature/analysis/20250826-pruning-aggressiveness  
**Created:** August 26, 2025  
**Status:** IMPLEMENTATION READY  
**Parent Analysis:** pruning_aggressiveness_analysis.md  

## Executive Summary

SeaJay exhibits overly aggressive pruning resulting in:
- **20% fewer nodes** searched compared to peer engines
- **171 tactical blunders** in 29 games (vs <50 expected)
- **Missing standard techniques** used by all modern engines

This plan provides a systematic approach to fix pruning issues through conservative adjustments and implementation of missing techniques.

## Critical Requirements

### OpenBench Compatibility
**EVERY commit MUST include bench count in exact format:**
```bash
# Get bench count after building
echo "bench" | ./seajay | grep "Benchmark complete" | awk '{print $4}'

# Commit message format
git commit -m "feat: [description]

bench 19191913"  # EXACT format required
```

### Testing Protocol
1. **Local validation** before pushing
2. **SPRT testing** for each phase
3. **Tactical suite** verification
4. **NO merging to main** without human approval

---

## PHASE 1: Conservative Parameter Adjustments [HIGH PRIORITY]

**Goal:** Reduce tactical blindness by making existing pruning less aggressive  
**Expected Impact:** -5 to 0 Elo short term, but 50%+ reduction in blunders  
**Time Estimate:** 4-6 hours

### Phase 1.1: Null Move Static Margin Reduction
**Reference:** Laser uses 70cp, SeaJay uses 120cp (71% more aggressive)

#### Implementation Steps:
```cpp
// File: src/search/negamax.cpp, line ~351
// Current:
eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth);  // 120

// Phase 1.1a: Reduce to 90cp (intermediate)
eval::Score margin = eval::Score(90 * depth);

// Phase 1.1b: Match Laser at 70cp (if 90cp tests well)
eval::Score margin = eval::Score(70 * depth);
```

#### Validation:
1. Run tactical suite before/after change
2. Count nodes on standard positions
3. SPRT test: bounds `[-3, 3]` (expect small regression)

#### Commit Process:
```bash
# Build and get bench
./build.sh
BENCH=$(echo "bench" | ./seajay | grep "Benchmark complete" | awk '{print $4}')

# Commit Phase 1.1a
git add -A
git commit -m "fix: Reduce null move static margin from 120cp to 90cp

Conservative adjustment to reduce tactical blindness. Laser uses 70cp,
this is an intermediate step toward that target.

bench $BENCH"

git push origin feature/analysis/20250826-pruning-aggressiveness
```

### Phase 1.2: SEE Pruning Conservative Default
**Reference:** Aggressive mode prunes at -75cp, conservative at -100cp

#### Implementation:
```cpp
// File: src/uci/uci.cpp, line ~90
// Change default from "off" to "conservative"
std::cout << "option name SEEPruning type combo default conservative var off var conservative var aggressive" << std::endl;

// Also update member initialization
m_seePruning = "conservative";  // was "off"
```

#### Validation:
- Verify captures aren't over-pruned
- Test tactical positions with hanging pieces
- SPRT: `[-2, 3]`

### Phase 1.3: Null Move Verification Search
**Reference:** Laser verifies null move at depth >= 10

#### Implementation:
```cpp
// File: src/search/negamax.cpp, after line ~406
if (nullScore >= beta) {
    info.nullMoveStats.cutoffs++;
    
    // Phase 1.3: Add verification search for deep nodes
    if (depth >= 10) {
        // Verification search at reduced depth
        eval::Score verifyScore = negamax(
            board,
            depth - nullMoveReduction - 1,  // Even shallower
            ply,
            beta - eval::Score(1),
            beta,
            searchInfo,
            info,
            limits,
            tt,
            false
        );
        
        if (verifyScore < beta) {
            // Verification failed, don't trust null move
            info.nullMoveStats.verificationFails++;
            // Continue with normal search instead of returning
        } else {
            // Verification passed, null move cutoff is valid
            if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                return nullScore;
            }
        }
    } else {
        // Shallow depth, trust null move without verification
        if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
            return nullScore;
        }
    }
}
```

---

## PHASE 2: Futility Pruning Implementation [CRITICAL]

**Goal:** Implement standard futility pruning (missing in SeaJay)  
**Expected Impact:** +10-15% node reduction, +5-10 Elo  
**Time Estimate:** 6-8 hours  
**Reference:** Laser formula: `staticEval <= alpha - 115 - 90*depth`

### Phase 2.1: Basic Futility Pruning Structure

#### Implementation:
```cpp
// File: src/search/negamax.cpp, after null move pruning (~line 420)

// Phase 2.1: Futility Pruning
// Only for non-PV nodes at shallow depths
if (!isPvNode && depth <= 6 && !weAreInCheck 
    && std::abs(alpha.value()) < MATE_BOUND - MAX_PLY
    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
    
    // Get static evaluation (use cached if available)
    eval::Score staticEval = eval::Score::zero();
    if (ply > 0) {
        int cachedEval = searchInfo.getStackEntry(ply).staticEval;
        if (cachedEval != 0) {
            staticEval = eval::Score(cachedEval);
        }
    }
    
    if (staticEval == eval::Score::zero()) {
        staticEval = board.evaluate();
        searchInfo.setStaticEval(ply, staticEval);
    }
    
    // Phase 2.1a: Conservative futility margin
    int futilityMargin = 100 + 80 * depth;  // Start conservative
    
    if (staticEval + eval::Score(futilityMargin) <= alpha) {
        info.futilityPruned++;  // Add counter to SearchData
        return staticEval;  // Position is futile
    }
}
```

### Phase 2.2: Extended Futility Pruning

#### Implementation:
```cpp
// Extended futility for depth 1-3
if (!isPvNode && depth <= 3 && depth >= 1 && !weAreInCheck) {
    // More aggressive margins for very shallow depths
    int extendedMargin = 200 + 100 * depth;
    
    if (staticEval + eval::Score(extendedMargin) <= alpha) {
        // Do a quiescence search to verify
        eval::Score qScore = quiescence(board, ply, alpha, beta, 
                                       searchInfo, info, limits, *tt, 0, false);
        if (qScore <= alpha) {
            info.extendedFutilityPruned++;
            return qScore;
        }
    }
}
```

### Phase 2.3: Add UCI Options for Tuning

#### Implementation:
```cpp
// File: src/uci/uci.cpp
// Add futility options
std::cout << "option name FutilityPruning type check default true" << std::endl;
std::cout << "option name FutilityMarginBase type spin default 100 min 50 max 200" << std::endl;
std::cout << "option name FutilityMarginDepth type spin default 80 min 50 max 150" << std::endl;

// Add to SearchLimits structure
bool useFutility = true;
int futilityBase = 100;
int futilityDepthMultiplier = 80;
```

### Phase 2.4: Testing and Tuning

#### Test Positions:
```
// Quiet positions where futility should trigger
position fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4
go depth 10

// Tactical positions where futility should NOT trigger
position fen r1b1kb1r/pp1n1ppp/2p1p3/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R b KQkq - 0 7
go depth 10
```

---

## PHASE 3: Move Count Pruning [MEDIUM PRIORITY]

**Goal:** Implement late move pruning based on move count  
**Expected Impact:** +10-15% node reduction  
**Time Estimate:** 5-6 hours  
**Reference:** Laser uses depth/improvement-based tables

### Phase 3.1: Basic Move Count Framework

#### Implementation:
```cpp
// File: src/search/negamax.cpp, in move loop after moveCount++

// Phase 3.1: Move Count Pruning (Late Move Pruning)
if (!isPvNode && !weAreInCheck && depth <= 8 && moveCount > 1) {
    // Don't prune captures, promotions, or killers
    if (!isCapture(move) && !isPromotion(move) && !info.killers.isKiller(ply, move)) {
        
        // Depth-based move count limits
        static const int moveCountLimit[9] = {
            0,   // depth 0 (not used)
            3,   // depth 1
            6,   // depth 2  
            12,  // depth 3
            18,  // depth 4
            24,  // depth 5
            32,  // depth 6
            40,  // depth 7
            50   // depth 8
        };
        
        // Check if we're improving (compare to previous ply's eval)
        bool improving = false;
        if (ply >= 2) {
            int prevEval = searchInfo.getStackEntry(ply - 2).staticEval;
            int currEval = searchInfo.getStackEntry(ply).staticEval;
            if (prevEval != 0 && currEval != 0) {
                improving = (currEval > prevEval);
            }
        }
        
        // Adjust limit based on improvement
        int limit = moveCountLimit[depth];
        if (!improving) {
            limit = limit * 3 / 4;  // Reduce limit if not improving
        }
        
        if (moveCount > limit) {
            info.moveCountPruned++;
            continue;  // Skip this move
        }
    }
}
```

### Phase 3.2: History-Based Adjustment

#### Implementation:
```cpp
// Adjust move count limit based on history score
int historyScore = info.history.get(board.sideToMove(), moveFrom(move), moveTo(move));
int historyThreshold = 1000;  // Tune this value

// Good history moves get extra chances
if (historyScore > historyThreshold) {
    limit += 5;  // Allow 5 more moves if good history
}
```

### Phase 3.3: Countermove Consideration

```cpp
// Don't prune countermoves
Move counterMove = info.counterMoves.getCounterMove(prevMove);
if (move == counterMove) {
    continue;  // Don't prune this move
}
```

---

## PHASE 4: Razoring Implementation [LOW PRIORITY]

**Goal:** Prune at very shallow depths when far below alpha  
**Expected Impact:** +5% node reduction  
**Time Estimate:** 3-4 hours  
**Reference:** Laser uses 300cp margin at depth <= 2

### Phase 4.1: Basic Razoring

#### Implementation:
```cpp
// File: src/search/negamax.cpp, before move generation

// Phase 4.1: Razoring
if (!isPvNode && !weAreInCheck && depth <= 2) {
    // Get static eval
    eval::Score staticEval = board.evaluate();
    
    // Razoring margin
    int razorMargin = 300;  // Conservative start
    
    if (staticEval + eval::Score(razorMargin) < alpha) {
        // Drop into quiescence search
        eval::Score razorScore = quiescence(board, ply, alpha, beta, 
                                           searchInfo, info, limits, *tt, 0, false);
        if (razorScore <= alpha) {
            info.razoringCutoffs++;
            return razorScore;
        }
    }
}
```

---

## PHASE 5: Advanced Techniques [FUTURE]

These require more complex implementation:

### Phase 5.1: ProbCut
- Shallow search to predict deep failures
- Requires careful tuning

### Phase 5.2: History-Based Pruning
- Prune moves with consistently bad history
- Needs mature history tables

### Phase 5.3: SEE-Based Move Ordering
- Order all moves by SEE value
- Currently only used for captures

---

## Testing and Validation Protocol

### For Each Phase:

#### 1. Pre-Implementation Baseline:
```bash
# Record current performance
echo "position fen [standard test positions]" | ./seajay
# Save node counts, depth, time

# Run tactical suite
./run_tactical_suite.sh > baseline.txt
```

#### 2. Post-Implementation Testing:
```bash
# Same positions, compare metrics
echo "position fen [same positions]" | ./seajay
# Compare node counts (should change as expected)

# Tactical suite should maintain or improve
./run_tactical_suite.sh > after.txt
diff baseline.txt after.txt
```

#### 3. SPRT Testing Guidelines:

| Phase | Expected Impact | SPRT Bounds | Time Control |
|-------|----------------|-------------|--------------|
| 1.1 Null Move | Small regression | `[-3, 3]` | 10+0.1 |
| 1.2 SEE Conservative | Neutral to positive | `[-2, 3]` | 10+0.1 |
| 1.3 Verification | Neutral | `[-2, 2]` | 10+0.1 |
| 2.x Futility | Positive | `[0, 5]` | 10+0.1 |
| 3.x Move Count | Positive | `[0, 5]` | 10+0.1 |
| 4.x Razoring | Small positive | `[0, 3]` | 10+0.1 |

### Tactical Test Positions:

```bash
# Create test file: tactical_suite.epd
cat > tactical_suite.epd << 'EOF'
# Positions where SeaJay currently fails
r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - bm Bxc6
r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - am Nf6
# Add more positions from the 171 blunders
EOF
```

---

## Statistics and Monitoring

### Add Pruning Counters:

```cpp
// In SearchData struct
struct PruningStats {
    uint64_t futilityPruned = 0;
    uint64_t extendedFutilityPruned = 0;
    uint64_t moveCountPruned = 0;
    uint64_t razoringCutoffs = 0;
    uint64_t nullMoveVerificationFails = 0;
    
    void reset() {
        futilityPruned = 0;
        extendedFutilityPruned = 0;
        moveCountPruned = 0;
        razoringCutoffs = 0;
        nullMoveVerificationFails = 0;
    }
    
    void report() const {
        std::cerr << "Pruning Stats:\n";
        std::cerr << "  Futility: " << futilityPruned << "\n";
        std::cerr << "  Extended Futility: " << extendedFutilityPruned << "\n";
        std::cerr << "  Move Count: " << moveCountPruned << "\n";
        std::cerr << "  Razoring: " << razoringCutoffs << "\n";
        std::cerr << "  Null Verification Fails: " << nullMoveVerificationFails << "\n";
    }
};
```

### UCI Info String Updates:

```cpp
// Report pruning effectiveness periodically
if (depth >= 5) {
    std::cout << "info string pruning futility " << info.pruningStats.futilityPruned
              << " movecount " << info.pruningStats.moveCountPruned
              << " razor " << info.pruningStats.razoringCutoffs << std::endl;
}
```

---

## Risk Mitigation

### Potential Issues and Solutions:

1. **Over-pruning Tactics**
   - Solution: Conservative margins initially
   - Validate with tactical suite after each change
   - Keep safety conditions (check, mate scores)

2. **Performance Regression**
   - Solution: Incremental changes with SPRT
   - Profile before/after for bottlenecks
   - Ensure compile optimizations enabled

3. **Integration Conflicts**
   - Solution: Test each phase independently
   - Maintain clear phase boundaries
   - Document all dependencies

### Rollback Plan:

```bash
# If a phase fails badly
git checkout HEAD~1  # Revert last commit
git push --force-with-lease  # Update remote

# Or create recovery branch
git checkout -b recovery/pruning-phase-X
git revert [bad-commit]
```

---

## Success Criteria

### Primary Metrics:
1. **Tactical Blunders:** Reduce from 171 to <50 per 29 games
2. **Node Count:** Increase by 15-20% (match peer engines)
3. **Search Depth:** Maintain or improve average depth
4. **Elo:** No regression > 5 Elo

### Secondary Metrics:
1. **Tactical Puzzle Solve Rate:** >80% on standard suite
2. **Time to Depth:** Similar or better than baseline
3. **Move Ordering Efficiency:** Improve from 76.8% to >80%

---

## Implementation Schedule

### Week 1 (Immediate):
- Day 1: Phase 1.1-1.2 (Parameter adjustments)
- Day 2: Phase 1.3 (Verification search)
- Day 3-4: Phase 2.1-2.2 (Basic futility)
- Day 5: Testing and validation

### Week 2:
- Day 1-2: Phase 2.3-2.4 (Futility tuning)
- Day 3-4: Phase 3.1-3.2 (Move count pruning)
- Day 5: Phase 3.3 and testing

### Week 3:
- Day 1: Phase 4 (Razoring)
- Day 2-3: Integration testing
- Day 4-5: SPRT validation

---

## References

### Source Code:
- **Laser:** https://github.com/jeffreyan11/laser-chess-engine/blob/master/src/search.cpp
- **Stockfish:** https://github.com/official-stockfish/Stockfish/blob/master/src/search.cpp
- **Ethereal:** https://github.com/AndyGrant/Ethereal/blob/master/src/search.c

### Papers and Articles:
- "Null Move Pruning" - https://www.chessprogramming.org/Null_Move_Pruning
- "Futility Pruning" - https://www.chessprogramming.org/Futility_Pruning
- "Late Move Reductions" - https://www.chessprogramming.org/Late_Move_Reductions
- "Move Count Based Pruning" - https://www.chessprogramming.org/Movecount_Based_Pruning

### Testing Resources:
- Strategic Test Suite (STS) - https://www.chessprogramming.org/Strategic_Test_Suite
- Bratko-Kopec Test - https://www.chessprogramming.org/Bratko-Kopec_Test
- OpenBench Framework - https://github.com/AndyGrant/OpenBench

---

## Appendix A: Common Pitfalls

1. **Don't prune in check positions**
2. **Don't prune when close to mate scores**
3. **Always validate TT moves before using**
4. **Test with both tactical and positional suites**
5. **Profile after adding new pruning (overhead matters)**

## Appendix B: Debugging Commands

```bash
# Compare node counts
echo -e "position startpos\\ngo depth 10" | ./seajay | grep "nodes"

# Check pruning stats
echo -e "setoption name Debug value true\\nposition startpos\\ngo depth 10" | ./seajay

# Validate no crashes on extreme positions
echo -e "position fen 8/8/8/8/8/8/8/8 w - - 0 1\\ngo depth 10" | ./seajay
```

---

**Document Status:** Complete and ready for implementation  
**Next Action:** Begin Phase 1.1 - Reduce null move static margin