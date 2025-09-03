# SPSA Foundation Tuning - Status Tracking

**Branch:** tune/20250903-foundation  
**Base Commit:** 8dc207e (main with all UCI options)  
**Start Date:** 2025-09-03  
**Purpose:** Tune foundational search efficiency parameters (move ordering + aspiration windows)

## Overview

This tuning session focuses on the most fundamental parameters that affect alpha-beta search efficiency:
1. **Move Ordering** - Better ordering leads to more alpha-beta cutoffs
2. **Aspiration Windows** - Optimizes the search window for better pruning

These parameters form the foundation of search efficiency and should be tuned before more aggressive pruning techniques.

## Tuning Parameters (Option A)

### SPSA Configuration
```
CountermoveBonus, int, 8000.0, 4000.0, 16000.0, 500.0, 0.002
AspirationWindow, int, 16.0, 5.0, 50.0, 3.0, 0.002
AspirationMaxAttempts, int, 5.0, 3.0, 10.0, 1.0, 0.002
```

### Parameter Details

| Parameter | Current | Min | Max | Step (C) | Learning Rate (R) | Purpose |
|-----------|---------|-----|-----|----------|------------------|---------|
| CountermoveBonus | 8000 | 4000 | 16000 | 500.0 | 0.002 | History bonus for countermoves that cause cutoffs |
| AspirationWindow | 16 | 5 | 50 | 3.0 | 0.002 | Initial window size in centipawns |
| AspirationMaxAttempts | 5 | 3 | 10 | 1.0 | 0.002 | Max re-searches before infinite window |

### OpenBench Settings
- **Games:** 40000 (3 parameters)
- **Time Control:** 10+0.1
- **Book:** UHO_4060_v2.epd
- **Base Branch:** main
- **Dev Branch:** tune/20250903-foundation
- **Test Type:** SPSA

## Expected Impact

### CountermoveBonus
- **Current Theory:** Improves move ordering → more alpha-beta cutoffs
- **Expected Range:** 5-15 ELO improvement
- **Risk:** Too high values might over-prioritize recent history

### AspirationWindow  
- **Current Theory:** Narrow windows = more cutoffs but more re-searches
- **Expected Range:** 3-8 ELO improvement
- **Risk:** Too narrow causes excessive re-searches

### AspirationMaxAttempts
- **Current Theory:** Balances re-search cost vs. window accuracy
- **Expected Range:** 1-3 ELO improvement
- **Risk:** Minor parameter, limited impact

## Testing Plan

### Phase 1: Foundation Tuning
1. Run SPSA with above parameters
2. Monitor convergence after 20000 games
3. Continue to 40000 games for stable results

### Phase 2: Validation
1. After SPSA completes, test tuned values with SPRT
2. Use bounds [0.0, 5.0] for validation
3. Expected combined improvement: 8-20 ELO

### Phase 3: Next Steps
Based on results:
- If successful (>5 ELO): Proceed to null move tuning
- If moderate (2-5 ELO): Consider adding LMR parameters
- If minimal (<2 ELO): Re-examine parameter ranges

## Status Log

### 2025-09-03: Initial Setup
- Created branch tune/20250903-foundation
- Base: commit 8dc207e
- All UCI options implemented and documented
- Ready for OpenBench submission

## Notes

### Why These Parameters?
1. **Move ordering is fundamental** - It determines which moves get examined first, directly affecting alpha-beta efficiency
2. **Aspiration windows optimize search** - They narrow the search window based on expected scores
3. **These work together** - Good move ordering makes aspiration windows more effective

### Tuning Strategy
- Starting conservative with well-understood parameters
- These parameters have minimal risk of tactical blindness
- Success here validates our UCI implementation before aggressive pruning

### Future Tuning Phases
After this foundation tuning:
1. **Phase 2:** Null move parameters (highest pruning impact)
2. **Phase 3:** LMR parameters (reduction strategy)
3. **Phase 4:** Futility/Razoring (shallow pruning)
4. **Phase 5:** Endgame PST values (evaluation tuning)

## Commands for OpenBench

When submitting to OpenBench, use:

```
Base: main
Dev: tune/20250903-foundation
SPSA Parameters:
CountermoveBonus, int, 8000.0, 4000.0, 16000.0, 500.0, 0.002
AspirationWindow, int, 16.0, 5.0, 50.0, 3.0, 0.002
AspirationMaxAttempts, int, 5.0, 3.0, 10.0, 1.0, 0.002

Games: 40000
TC: 10+0.1
Book: UHO_4060_v2.epd
```

---
*This is a temporary tracking document for the tuning session. Will be removed when merging back to main.*