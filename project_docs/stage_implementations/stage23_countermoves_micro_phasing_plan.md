# Stage 23: Countermoves Micro-Phasing Recovery Plan

## Summary of Current Situation

### What We Know
1. **Infrastructure works**: Phases CM1 (data structures) and CM2 (shadow updates) showed no regression
2. **Integration fails catastrophically**: Phase CM3 with any bonus (1000 or 12000) causes -72 to -78 ELO regression
3. **Bug fixes made it worse**: Changing indexing from [from][to] to [piece_type][to] worsened the regression
4. **Even zero bonus now regresses**: After the "fixes", even CountermoveBonus=0 shows -2.52 ELO

### Important Context
**Known Invalid Move Bug**: There is an existing bug in main (and all branches) where invalid moves are generated approximately 1-2 times per 500 games. This pre-existing issue is documented but likely NOT the cause of the countermoves regression, given the magnitude of the failure.

## Micro-Phasing Strategy

Instead of implementing CM3 as a single change, we will break it into micro-phases to isolate the exact cause:

### Starting Point
- **Reset to CM2** (commit 40490c6): Last known good state with shadow updates but no ordering impact
- **Branch**: `integration/countermoves-micro-phasing`
- **Testing**: EVERY micro-phase gets OpenBench testing before proceeding

### Micro-Phase Breakdown

#### CM3.1: UCI Option Only
- Add UCI option for CountermoveBonus
- NO integration with move ordering
- NO usage of countermove data
- Expected: 0 ELO change
- Purpose: Verify UCI infrastructure doesn't cause issues

#### CM3.2: Lookup Without Bonus
- Add countermove lookup in move_ordering.cpp
- Retrieve the countermove but DON'T apply any bonus (effectively no-op)
- Expected: 0 to -1 ELO (minimal overhead)
- Purpose: Verify lookup logic and array access patterns

#### CM3.3: Minimal Bonus (100 units)
- Apply tiny bonus (100, compared to history's 0-8192)
- Expected: 0 to +1 ELO
- Purpose: Verify basic scoring integration works

#### CM3.4: Gradual Bonus Increase
- Test with 500, 1000, 2000, 4000, 8000
- Stop at first sign of regression
- Expected: Identify breaking point
- Purpose: Find if there's a threshold where it breaks

### Decision Tree

```
If CM3.1 fails → UCI implementation issue
If CM3.2 fails → Array indexing or lookup bug
If CM3.3 fails → Move ordering integration bug
If CM3.4 fails at specific value → Scoring conflict or overflow
If all pass → Original implementation had compound bugs
```

## Action Plan

### Phase 1: Documentation & Setup (Current)
1. ✓ Create integration branch
2. ✓ Document micro-phasing strategy
3. Update stage23 implementation plan
4. Document known invalid move bug for context

### Phase 2: Reset & Prepare
1. Reset to CM2 commit (40490c6)
2. Verify clean compilation and bench count
3. Create first micro-phase commit

### Phase 3: Systematic Testing
1. Implement CM3.1 → OpenBench test → Wait for results
2. Implement CM3.2 → OpenBench test → Wait for results
3. Implement CM3.3 → OpenBench test → Wait for results
4. Implement CM3.4 → OpenBench test → Wait for results

### Phase 4: Analysis & Decision
Based on where the regression appears:
- **Fix the specific bug** if isolated to clear issue
- **Rewrite with Ethereal approach** if fundamental design flaw
- **Defer to later stage** if related to invalid move bug

## Success Metrics

- Each micro-phase shows expected behavior (0 ELO or small positive)
- Root cause identified to specific code change
- Clear path forward (fix or rewrite) determined
- No compound bugs hiding other issues

## Timeline

- Each micro-phase: ~1-2 hours implementation + OpenBench testing
- Total estimated: 2-3 days to complete all micro-phases
- Decision point: After all micro-phase results

## Risk Mitigation

1. **If invalid move bug interferes**: Document correlation but continue
2. **If all micro-phases pass**: Original had compound bug, proceed with gradual integration
3. **If early micro-phase fails**: Stop immediately and debug that specific change
4. **If regression is intermittent**: Run longer SPRT tests to confirm

## Commit Message Format

```
feat: Countermoves micro-phase CM3.X - [description] - bench [count]

Micro-phase testing to isolate regression cause.
Expected: [0 ELO / minimal overhead / etc]
```

## Micro-Phase Testing Results

| Phase | Status | Commit Hash | Bench Count | ELO Result | Notes |
|-------|--------|-------------|-------------|------------|-------|
| CM3.1 | Complete | 97984d3 | 19191913 | +11.11 ± 12.04 | UCI only, within noise |
| CM3.2 | Complete | b33ad41 | 19191913 | -8.25 ± 12.59 | Lookup overhead, acceptable |
| CM3.3 | Complete | 8532051 | 19191913 | -8.04 ± 10.08 | Minimal bonus (100) - stable |
| CM3.4 | Complete | 8532051 | 19191913 | -0.96 ± 10.33 | bonus=1000 - SUCCESS! |
| CM3.5 | Complete | 8532051 | 19191913 | +4.71 ± 10.16 | bonus=8000 - POSITIVE! |