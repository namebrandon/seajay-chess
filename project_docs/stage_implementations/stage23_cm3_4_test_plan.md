# CM3.4: Testing with Original Bonus (1000)

## Test Configuration
- **Branch:** integration/countermoves-micro-phasing  
- **Commit:** 8532051 (same as CM3.3)
- **UCI Options:** Threads=1 Hash=8 CountermoveBonus=1000
- **Change:** Same code, just higher bonus value

## Why This Test Matters

The original CM3 failed catastrophically with bonus=1000:
- **Original:** -72.69 ± 21.52 ELO
- **Our approach:** Position-based ordering instead of score-based

Since CM3.3 with bonus=100 showed no regression, we're testing if:
1. The problem was the implementation approach (score vs position)
2. The problem appears only at higher bonus values
3. Our cleaner implementation avoids the original bug

## Expected Outcomes

### Success Scenario (0 to +10 ELO)
- Our position-based approach works correctly
- Original implementation had a scoring bug
- Continue testing with higher values (2000, 4000, 8000)

### Failure Scenario (-50+ ELO)
- Problem manifests at higher bonus values
- Countermove is overriding too many good moves
- Need to investigate interaction with killers/history

## Move Ordering with bonus=1000

With bonus=1000, countermove gets higher priority:
```
1. TT Move
2. Good Captures (MVV-LVA)
3. Promotions
4. Killer Move 1
5. Killer Move 2  
6. **Countermove** ← Positioned here when bonus > 0
7. History Moves
8. Remaining Quiet
```

The key difference from original:
- **Original:** Applied bonus in scoring calculation (unclear how)
- **Ours:** Explicit position in ordering sequence

## Critical Analysis

The micro-phasing has revealed:
- CM3.1 (UCI only): No regression ✓
- CM3.2 (Lookup): Small overhead ✓
- CM3.3 (bonus=100): No additional regression ✓
- CM3.4 (bonus=1000): **Testing now...**

If CM3.4 succeeds where the original failed, it proves the bug was in the implementation, not the concept.

## Next Steps

If CM3.4 succeeds:
1. Test CM3.5 with bonus=2000
2. Test CM3.6 with bonus=4000
3. Test CM3.7 with bonus=8000 (target)
4. Find optimal value via SPSA

If CM3.4 fails:
1. The issue is bonus-value dependent
2. Try intermediate values (500, 750)
3. Consider different ordering position
4. May need to cap at lower value