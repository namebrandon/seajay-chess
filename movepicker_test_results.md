# MovePicker Performance Testing Results

## Test Configuration
- Position: startpos
- Time: 5000ms per test
- Date: 2025-09-08

## Baseline Reference (commit 6cc37db - last compilable)

### Legacy Ordering (MovePicker disabled)
```
Depth reached: 19
Nodes searched: 4,588,159
NPS: 1,081,857
Score: cp 28
Time: 4241ms
Key moves: e2e4 e7e5 g1f3 g8f6
```

This represents our target performance - any fix should get close to these numbers.

## Regression Symptoms (what we're trying to fix)
- Severely reduced node counts (8K vs 22K at depth 8)
- Inflated scores (+77 cp vs -8 cp)
- Poor move selection
- -1100 Elo in gameplay

## Test Results by Commit

### Commit 6cc37db - Validation diagnostics (BASELINE)
**Status**: Compiles with fix
**Date tested**: 2025-09-08

#### Legacy ordering (baseline)
```
Depth: 18 (in 5 seconds)
Nodes: 2,932,718
Score: cp 25
NPS: 1,071,899
```

#### MovePicker Standard Mode (SHOWS REGRESSION)
```
Depth: 12 (in 5 seconds) - only depth 12!
Nodes: 135,032 - severely reduced!
Score: cp 17
NPS: 30,871 - very low!
At depth 8: 8,085 nodes vs 19,733 legacy
```

#### MovePicker Minimal Mode (BROKEN)
```
Depth: 64 (clearly broken)
Nodes: 3,550
Score: cp 0
```

### Commit 0f0ac53 - Initial safeguards
**Status**: Needs compilation fixes
**Date tested**: Not yet tested
```
Config: TBD
Depth: TBD
Nodes: TBD
Score: TBD
NPS: TBD
```

### Commit 963c4b3 - Comprehensive safeguards (BEST RESULT!)
**Status**: Compiles with fixes  
**Date tested**: 2025-09-08

#### MovePicker Standard Mode with Safeguards
```
Depth: 10 (limited but better than minimal)
Nodes: 68,204 (in 2.3 seconds)
Score: cp 32 (reasonable)
NPS: 29,209
At depth 8: 14,660 nodes (74% of legacy!)
```

**Analysis**: BEST RESULT SO FAR! The comprehensive safeguards achieve 74% of legacy performance at depth 8 (14,660 vs 19,712 nodes). This is significantly better than:
- Broken baseline: 8,085 nodes (41% of legacy)
- Hybrid approach: 13,412 nodes (68% of legacy)
- **This commit: 14,660 nodes (74% of legacy)**

### Commit fa8dd78 - Hybrid ordering (PARTIAL IMPROVEMENT)
**Status**: Compiles with fixes
**Date tested**: 2025-09-08

#### Legacy ordering (baseline)
```
Depth: 18
Nodes: 3,622,726
Score: cp 22
NPS: 1,083,027
```

#### MovePicker Standard Mode (Hybrid approach)
```
Depth: 12 (still limited but same as before)
Nodes: 196,608 (better than 135K before!)
Score: cp 7 (more reasonable than cp 17)
NPS: 39,313
At depth 8: 13,412 nodes (vs 8,085 broken, 19,712 legacy)
```

**Analysis**: The hybrid approach shows improvement! Node count at depth 8 is 66% better than the broken version (13K vs 8K), though still 32% below legacy (13K vs 19K). This suggests the hybrid approach partially addresses the issue but doesn't fully solve it.

## Success Criteria
A commit is considered successful if:
1. Nodes > 3,500,000 (>75% of baseline)
2. Depth >= 18
3. Score between cp 0 and cp 40 (reasonable range)
4. No crashes or illegal moves