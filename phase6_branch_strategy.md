# Phase 6 Search API Refactor - Branch Strategy

## Overview
Phase 6 implements a new Search Node API to support future features (singular extensions, multi-cut, probcut) without changing current behavior.

## Branch Structure

```
main
 `-- integration/phase6-search-api-refactor (current)
     |-- feature/phase6-stage-6a1  (NodeContext structure)
     |-- feature/phase6-stage-6a2  (Negamax overload)
     |-- feature/phase6-stage-6b1  (Thread context - main)
     |-- feature/phase6-stage-6b2  (Thread context - qsearch)
     |-- feature/phase6-stage-6b3  (Thread context - helpers)
     |-- feature/phase6-stage-6c   (Replace excluded checks)
     |-- feature/phase6-stage-6d   (Verification helper)
     |-- feature/phase6-stage-6e   (TT hygiene)
     |-- feature/phase6-stage-6f   (PV clarity)
     `-- feature/phase6-stage-6g   (Integration)
```

## Workflow

### For Each Stage:
1. Create feature branch from integration branch:
   ```bash
   git checkout integration/phase6-search-api-refactor
   git checkout -b feature/phase6-stage-XXX
   ```

2. Implement stage following Phase6_Search_API_Refactor.md
3. Test locally with bench verification
4. Commit with format: `feat: [Stage 6X.Y] - description - bench XXXXX`
5. Push feature branch
6. If SPRT required, wait for results
7. Merge back to integration:
   ```bash
   git checkout integration/phase6-search-api-refactor
   git merge feature/phase6-stage-XXX --no-ff
   git push
   ```

### Final Integration:
Once all stages complete:
```bash
git checkout main
git merge integration/phase6-search-api-refactor --no-ff
git push
```

## Stage Status Tracking

| Stage | Branch | Status | SPRT | Bench | Notes |
|-------|--------|--------|------|-------|-------|
| 6a.1 | feature/phase6-stage-6a1 | Completed | Not Required | 2350511 | NodeContext structure |
| 6a.2 | feature/phase6-stage-6a2 | Completed | Not Required | 2350511 | Negamax overload wrapper (NodeContext) |
| 6b.1 | feature/phase6-stage-6b1 | Completed | Optional | 2350511 | Thread context - main (`UseSearchNodeAPIRefactor` toggle) |
| 6b.2 | feature/phase6-stage-6b2 | Completed | Not Required | 2350511 | Thread context - qsearch (toggle parity confirmed). |
| 6b.3 | feature/phase6-stage-6b3 | Completed | Optional | 2350511 | Thread context - helpers (LMR wrappers parity). |
| 6c | feature/phase6-stage-6c | Completed | Required | 2350511 | Replace stack-based excluded checks with NodeContext + `EnableExcludedMoveParam` toggle |
| 6d | feature/phase6-stage-6d | Completed | Not Required | 2350511 | Verification helper scaffold (NoOp) |
| 6e | feature/phase6-stage-6e | Completed | Not Required | 2350511 | TT hygiene guard refinements |
| 6f | feature/phase6-stage-6f | Completed | Not Required | 2350511 | PV clarity/root safety asserts merged after neutral SPRT |
| 6g | feature/phase6-stage-6g | Completed | Required | 2350511 | Integration + dev-mode bench parity (UseSearchNodeAPIRefactor ON/OFF). |

## Key Principles
- Each stage is atomic and independently testable
- No behavior changes when toggles are OFF
- Bench must remain identical for NoOp stages
- Clear rollback points documented for each stage
- SPRT testing required for stages that touch search logic

## Rollback Strategy
If any stage fails:
1. Identify the problematic stage from this tracking
2. Either fix forward on the feature branch, or
3. Skip the stage if dependencies allow
4. Document failure in feature_status.md
5. Continue with remaining stages if possible

## Success Criteria
- All unit tests pass
- Perft unchanged
- Bench matches baseline with all toggles OFF
- No performance regression (< 0.5% impact)
- Clean API ready for future features
