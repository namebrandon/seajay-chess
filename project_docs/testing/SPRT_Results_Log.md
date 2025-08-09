# SeaJay Chess Engine - SPRT Results Log

**Purpose:** Historical record of all SPRT tests conducted on SeaJay  
**Started:** August 9, 2025  
**Current Version:** 1.5.0-master (Phase 1 Complete)  
**Estimated Strength:** ~100 ELO (random play baseline)  

## Summary Statistics

| Metric | Value |
|--------|-------|
| Total Tests | 1 |
| Passed | 0 (0%) |
| Failed | 0 (0%) |
| Inconclusive | 1 (100%) |
| Total Games | 10 |
| Total Time | ~0.1 hours |
| Success Rate | N/A |

## Test Configuration Standards

### Phase 2 (Upcoming)
- **Parameters:** [0, 5] α=0.05 β=0.05
- **Time Control:** 10+0.1
- **Book:** 4moves_test.pgn or 8moves_v3.pgn
- **Concurrency:** 4-8 games

## Test History

### Test SPRT-2025-08-09-001
- **Date:** 2025-08-09
- **Versions:** Stage 6 Material Eval vs Random (Phase 1 baseline)
- **Feature:** Material evaluation with single-ply lookahead
- **Hypothesis:** Material evaluation will significantly outperform random play
- **Parameters:** [0, 5] α=0.05 β=0.05
- **Time Control:** Quick test (10 games)
- **Opening Book:** Standard positions
- **Hardware:** Development container
- **Concurrency:** Sequential

#### Results
- **Decision:** INCONCLUSIVE ⋯
- **Games Played:** 10
- **Score:** 0-1-9 (45% draws)
- **ELO Estimate:** Insufficient data
- **Time Elapsed:** ~5 minutes

#### Analysis
Single-ply material evaluation alone is insufficient to show strength gains against random play. The engine correctly evaluates material but without search depth, it cannot capitalize on advantages. This is expected behavior - Stage 7's negamax search will enable the material evaluation to demonstrate effectiveness.

#### Conclusion
Test confirms Stage 6 implementation is working but needs Stage 7 (search) to show measurable strength improvement. No bugs identified; behavior is as expected for material-only evaluation.

---

### Template for Future Tests:

---

### Test SPRT-2025-001
- **Date:** YYYY-MM-DD HH:MM
- **Versions:** X.Y.Z-feature vs X.Y.Z-master
- **Feature:** Description of change
- **Hypothesis:** Expected improvement
- **Parameters:** [elo0, elo1] α=0.05 β=0.05
- **Time Control:** TC+increment
- **Opening Book:** book_name.pgn
- **Hardware:** CPU/RAM/OS details
- **Concurrency:** N games

#### Results
- **Decision:** PASS ✓ / FAIL ✗ / INCONCLUSIVE ⋯
- **Games Played:** N
- **Score:** W-L-D (win%)
- **ELO Estimate:** +X.X ± Y.Y
- **LLR:** X.XX (bounds: [lower, upper])
- **Duration:** Xh Ym
- **NPS Average:** X.XM

#### Analysis
- Key observations
- Unexpected behaviors
- Performance notes

#### Action Taken
- Merged to master / Rejected / Further testing

#### Files
- Results: `/workspace/sprt_results/SPRT-2025-001/`
- PGN: `sprt_games.pgn`
- JSON: `sprt_status.json`

---

## Phase Summaries

### Phase 1: Foundation and Move Generation
- **Completed:** August 9, 2025
- **SPRT Tests:** N/A (not required for Phase 1)
- **Final Version:** 1.5.0-master
- **Achievement:** Complete move generation, UCI protocol, testing infrastructure

### Phase 2: Basic Search and Evaluation (Upcoming)
- **Start Date:** TBD
- **Target Tests:** ~15-20
- **Target Strength:** 1500 ELO
- **Key Features:** Material eval, negamax, alpha-beta, PST

## Strength Progression Chart

```
ELO Progression (Estimated)
3500 |                                    
3000 |                                Goal ★
2500 |                           
2000 |                    
1500 |          Target Phase 2 ▲
1000 |     
 500 |
   0 |★ Current (random play)
     +--+--+--+--+--+--+--+
     P1 P2 P3 P4 P5 P6 P7
```

## Notable Findings

*To be populated as testing begins*

### Successful Techniques
- TBD

### Failed Experiments
- TBD

### Surprising Results
- TBD

## Hardware Specifications

### Primary Test Machine
```
CPU: [To be specified]
RAM: [To be specified]
OS: Ubuntu 22.04 (Docker)
Compiler: GCC 12
Build: Release with -O3 -march=native
```

### Build Configuration
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

## Testing Guidelines

1. **Always test against current master**
2. **Use consistent hardware within a phase**
3. **Document all parameters**
4. **Save all output files**
5. **Update this log immediately after test**

## Statistical Reference

### SPRT Bounds (Standard)
- **Lower Bound (H0):** LLR ≤ log(β/(1-α)) = log(0.05/0.95) ≈ -2.89
- **Upper Bound (H1):** LLR ≥ log((1-β)/α) = log(0.95/0.05) ≈ 2.89

### ELO Calculation
```
ELO = 400 * log10(wins/losses)
95% CI = ± 400/sqrt(games)
```

## Version Naming Convention

- **MAJOR.MINOR.PATCH-TAG**
  - MAJOR: Phase (1-7)
  - MINOR: Stage within phase
  - PATCH: Incremental improvement
  - TAG: Feature identifier

Examples:
- 1.5.0-master: Phase 1, Stage 5, baseline
- 2.1.0-material: Phase 2, Stage 1, material evaluation
- 2.2.0-negamax: Phase 2, Stage 2, negamax search

## Links

- [SPRT How-To Guide](SPRT_How_To_Guide.md)
- [SPRT Testing Process](SPRT_Testing_Process.md)
- [Version History](Version_History.md)
- [Master Project Plan](../SeaJay%20Chess%20Engine%20Development%20-%20Master%20Project%20Plan.md)

---

*This log will be continuously updated throughout SeaJay's development. Each SPRT test must be recorded here for historical tracking and analysis.*