# Stage 23: Countermoves Micro-Phase Analysis

## Current Testing: CM3.3 with CountermoveBonus=100

### What We're Testing
- **Branch:** integration/countermoves-micro-phasing
- **Commit:** 8532051
- **UCI Options:** Threads=1 Hash=8 CountermoveBonus=100
- **Change:** Countermove positioned after killers when found

### Expected Outcomes & Interpretations

#### Scenario A: Success (0 to +5 ELO)
- **Meaning:** Basic ordering logic is correct
- **Next Step:** Proceed to CM3.4 with gradual bonus increases (500, 1000, 2000...)
- **Goal:** Find optimal bonus value or breaking point

#### Scenario B: Small Regression (-5 to -15 ELO)
- **Meaning:** Ordering works but overhead or priority issue
- **Next Step:** Try different positions (before killers, after history)
- **Goal:** Find correct position in move ordering hierarchy

#### Scenario C: Massive Regression (-50+ ELO)
- **Meaning:** Fundamental bug in countermove lookup or application
- **Possible Causes:**
  1. Indexing bug (wrong moves being prioritized)
  2. Illegal moves being selected
  3. Countermove overriding better moves
  4. Interaction with killer moves causing conflicts
- **Next Step:** Add extensive debugging/logging to identify issue

### Critical Difference from Original CM3

**Original CM3 Approach:**
- Applied bonus in scoring function (not clear how)
- Massive regression: -72.69 ELO with bonus=1000

**Current CM3.3 Approach:**
- Explicitly positions countermove after killers
- No scoring calculation, just position-based ordering
- Minimal bonus (100) to test basic logic first

### Move Ordering Hierarchy (CM3.3)

```
1. TT Move           (from transposition table)
2. Good Captures     (MVV-LVA ordered)
3. Promotions        
4. Killer Move 1     (quiet move that caused cutoff)
5. Killer Move 2     
6. Countermove       ← NEW with bonus=100
7. History Moves     (ordered by history score)
8. Remaining Quiet   
```

### Why Micro-Phasing Matters

The original implementation jumped straight to bonus=1000 and failed catastrophically. By breaking it down:

1. **CM3.1:** UCI only → Verified no infrastructure issues
2. **CM3.2:** Lookup only → Small overhead but functional
3. **CM3.3:** Minimal ordering → Testing basic logic
4. **CM3.4:** Gradual increase → Will find breaking point

This systematic approach isolates the exact cause of failure.

## Historical Context

### Failed Attempts
- **Original CM3:** -72.69 ELO with bonus=1000
- **CM3.5 "Fix":** -78.68 ELO with bonus=12000 (made it worse!)
- **Changed indexing:** from [from][to] to [piece_type][to] (incorrect)

### Key Lessons
1. Never apply large bonuses without testing small ones first
2. Position-based ordering may be safer than score-based
3. Micro-phasing catches bugs early
4. The "obvious fix" can make things worse

## Decision Tree After CM3.3 Results

```
CM3.3 Result?
├── Success (0 to +5 ELO)
│   └── Continue to CM3.4 with bonus=500
├── Small Regression (-5 to -15 ELO)  
│   └── Try alternate position (before killers)
└── Large Regression (-50+ ELO)
    ├── Add debug logging
    ├── Verify move legality
    └── Consider Ethereal's simpler approach
```

## Next Steps

Awaiting OpenBench results for CM3.3...

If successful, CM3.4 will test:
- bonus=500
- bonus=1000 (original failure point)
- bonus=2000
- bonus=4000
- bonus=8000 (target value)

Each will be tested separately to find the exact breaking point or optimal value.