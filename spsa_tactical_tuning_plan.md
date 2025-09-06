# SPSA Tactical Tuning Plan

## Branch: tune/20250906-core-tactical

## Objective
Improve SeaJay's tactical awareness based on WAC test suite analysis showing:
- Missing direct checks/checkmates (Qxh7+, Qxg7+, etc.)
- Preferring defensive moves over tactical strikes
- 76% success at 800ms → 83% at 2000ms on WAC
- 40 positions consistently failed at both time controls

## Key Patterns Identified
1. **Check/Checkmate Blindness** - Missing forcing moves
2. **Over-defensive Play** - King safety too conservative
3. **Quiet Move Preference** - Playing g2f1 instead of winning captures
4. **Weak Forcing Move Recognition** - Poor check extensions or move ordering

## SPSA Test Groups

### Test 1: Comprehensive Tactical Awareness (HIGHEST PRIORITY)
**Goal:** Improve tactical search and reduce defensive bias

```
# Core tactical search
MaxCheckPly, int, 8.0, 4.0, 10.0, 1.0, 0.002

# LMR (less reduction on tactics)
LMRMinMoveNumber, int, 8.0, 4.0, 15.0, 1.0, 0.002
LMRHistoryThreshold, int, 30.0, 10.0, 70.0, 5.0, 0.002
LMRBaseReduction, int, 30.0, 10.0, 80.0, 5.0, 0.002

# Futility (don't prune tactics)
FutilityBase, int, 200.0, 100.0, 400.0, 20.0, 0.002
FutilityScale, int, 80.0, 40.0, 150.0, 10.0, 0.002
FutilityMaxDepth, int, 5.0, 3.0, 8.0, 1.0, 0.002

# Null move (less aggressive)
NullMoveStaticMargin, int, 150.0, 100.0, 300.0, 20.0, 0.002
NullMoveMinDepth, int, 4.0, 3.0, 6.0, 1.0, 0.002

# Move ordering
CountermoveBonus, int, 10000.0, 5000.0, 20000.0, 1000.0, 0.002

# King safety (reduce defensive bias)
KingSafetyDirectShieldMg, int, 12.0, 0.0, 25.0, 2.0, 0.002
RootKingPenalty, int, 100.0, 0.0, 300.0, 20.0, 0.002
```

**Games:** 150,000  
**Time Control:** 10+0.1  
**Expected Impact:** High - addresses all major tactical weaknesses

### Test 2: Alternative - Focused Groups

#### Group A: Core Tactical Search
```
MaxCheckPly, int, 8.0, 4.0, 10.0, 1.0, 0.002
CountermoveBonus, int, 10000.0, 5000.0, 20000.0, 1000.0, 0.002
MoveCountHistoryBonus, int, 8.0, 2.0, 15.0, 1.0, 0.002
MoveCountHistoryThreshold, int, 2000.0, 500.0, 4000.0, 200.0, 0.002
LMRMinMoveNumber, int, 8.0, 4.0, 15.0, 1.0, 0.002
LMRHistoryThreshold, int, 30.0, 10.0, 70.0, 5.0, 0.002
LMRBaseReduction, int, 30.0, 10.0, 80.0, 5.0, 0.002
LMRDepthFactor, int, 200.0, 100.0, 350.0, 20.0, 0.002
```
**Games:** 120,000

#### Group B: All Pruning Margins
```
FutilityBase, int, 200.0, 100.0, 400.0, 20.0, 0.002
FutilityScale, int, 80.0, 40.0, 150.0, 10.0, 0.002
FutilityMaxDepth, int, 5.0, 3.0, 8.0, 1.0, 0.002
FutilityMargin1, int, 150.0, 75.0, 250.0, 15.0, 0.002
FutilityMargin2, int, 200.0, 125.0, 350.0, 20.0, 0.002
RazorMargin1, int, 400.0, 200.0, 600.0, 30.0, 0.002
RazorMargin2, int, 600.0, 300.0, 900.0, 40.0, 0.002
NullMoveStaticMargin, int, 150.0, 100.0, 300.0, 20.0, 0.002
NullMoveMinDepth, int, 4.0, 3.0, 6.0, 1.0, 0.002
NullMoveReductionBase, int, 2.0, 1.0, 4.0, 0.5, 0.002
```
**Games:** 100,000

#### Group C: King Safety & Evaluation
```
KingSafetyDirectShieldMg, int, 12.0, 0.0, 25.0, 2.0, 0.002
KingSafetyAdvancedShieldMg, int, 3.0, 0.0, 15.0, 2.0, 0.002
king_mg_e1, int, 10.0, -30.0, 30.0, 5.0, 0.002
king_mg_b1, int, 0.0, -30.0, 30.0, 5.0, 0.002
king_mg_g1, int, 10.0, -20.0, 40.0, 5.0, 0.002
RootKingPenalty, int, 100.0, 0.0, 300.0, 20.0, 0.002
BishopValueMg, int, 344.0, 320.0, 370.0, 5.0, 0.002
KnightValueMg, int, 325.0, 310.0, 340.0, 5.0, 0.002
```
**Games:** 80,000

## Pre-Tuning Setup

### 1. Baseline WAC Test
```bash
./tools/run_wac_test.sh ./bin/seajay 2000
```
Record baseline success rate and save failed positions CSV.

### 2. Enable Tactical Features
Before tuning, consider setting:
```
setoption name SEEMode value production
setoption name SEEPruning value aggressive
```

### 3. OpenBench Configuration
- Branch: tune/20250906-core-tactical
- Base: bugfix/20250905-node-explosion-diagnostic
- Book: UHO_4060_v2.epd
- TC: 10+0.1
- Threads: 1
- Hash: 16-32 MB

## Success Metrics

### Primary (Tactical)
- WAC success rate improvement (target: 85%+ at 2000ms)
- Reduction in specific pattern failures (check blindness)
- Node efficiency in tactical positions

### Secondary (General)
- Elo gain or neutrality
- No regression in positional play
- Improved time-to-depth in tactical positions

## Post-Tuning Validation

### 1. WAC Retest
```bash
./tools/run_wac_test.sh ./bin/seajay 2000
```
Compare with baseline, analyze improvements in failed positions.

### 2. Specific Pattern Tests
Test positions that showed check blindness:
- WAC.014 (Qxh7+)
- WAC.055 (Qxg7+)
- WAC.141 (Qxf4)
- WAC.074 (Qf1+)

### 3. Node Efficiency Check
Run node explosion diagnostic on tactical positions to verify efficiency gains.

## Risk Mitigation

1. **Over-tuning for Tactics**: Monitor positional test suites to ensure no regression
2. **Time Control Bias**: Consider testing at 60+0.6 for validation
3. **Parameter Interactions**: Use comprehensive test (Test 1) to capture interactions

## Timeline

1. **Day 1**: Launch comprehensive tactical test
2. **Day 2-4**: Monitor progress, 150k games
3. **Day 5**: Analyze results, run WAC validation
4. **Day 6**: Decide on follow-up tests or merge

## Notes

- Current branch includes all e8f7 investigation work and tactical test infrastructure
- RootKingPenalty already at 0 from previous testing
- Consider creating separate test for piece values if time permits