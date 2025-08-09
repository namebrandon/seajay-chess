# SeaJay Chess Engine - SPRT Testing Process & Governance

**Document Version:** 1.0  
**Date:** August 9, 2025  
**Author:** Brandon Harris with Claude AI  
**Purpose:** Establish rigorous process and tracking for SPRT testing  

## Overview

This document defines the standardized process for conducting SPRT (Sequential Probability Ratio Test) validation of chess engine improvements. It ensures consistency, traceability, and historical tracking of all engine strength changes throughout development.

## SPRT Testing Requirements

### When SPRT Testing is MANDATORY

1. **All Phase 2+ Code Changes** that affect:
   - Evaluation function
   - Search algorithm
   - Move ordering
   - Time management
   - Pruning techniques
   - Extension decisions

2. **Any Change Claiming Strength Improvement**
   - New features
   - Optimizations
   - Parameter tuning
   - Bug fixes affecting play

3. **Major Refactoring**
   - Code simplifications
   - Architecture changes
   - Performance optimizations

### When SPRT Testing is OPTIONAL

1. **Phase 1 Changes** (move generation, UCI protocol)
2. **Non-functional changes**:
   - Documentation
   - Comments
   - Code formatting
   - Build system updates
3. **Debug/diagnostic features**
4. **Testing infrastructure**

## Version Numbering System

### Version Format: `MAJOR.MINOR.PATCH-IDENTIFIER`

- **MAJOR**: Phase number (1-7)
- **MINOR**: Stage within phase
- **PATCH**: Incremental improvements within stage
- **IDENTIFIER**: Feature or test identifier

### Examples:
- `2.1.0-material`: Phase 2, Stage 1, initial material evaluation
- `2.1.1-material-tuned`: Tuned piece values
- `2.2.0-negamax`: Phase 2, Stage 2, negamax search
- `3.1.0-magic`: Phase 3, Stage 1, magic bitboards

### Special Identifiers:
- `-dev`: Development version (not tested)
- `-rc`: Release candidate (passed SPRT)
- `-master`: Current master branch
- `-exp`: Experimental feature

## SPRT Testing Process

### Step 1: Pre-Test Checklist

Before running SPRT:

- [ ] Code compiles without warnings
- [ ] Perft tests pass (no move generation bugs)
- [ ] Bench command runs successfully
- [ ] Version number assigned
- [ ] Changes documented in git commit
- [ ] Base version identified and built

### Step 2: Test Configuration

#### Select Appropriate Parameters

| Phase | Test Type | elo0 | elo1 | alpha | beta | Time Control |
|-------|-----------|------|------|-------|------|--------------|
| 2 | Feature | 0 | 5 | 0.05 | 0.05 | 10+0.1 |
| 2 | Tuning | 0 | 3 | 0.05 | 0.05 | 10+0.1 |
| 3 | Optimization | 0 | 3 | 0.05 | 0.05 | 10+0.1 |
| 3 | Simplification | -2 | 2 | 0.05 | 0.10 | 10+0.1 |
| 4 | NNUE | 0 | 5 | 0.05 | 0.05 | 20+0.2 |
| 5+ | Fine-tuning | 0 | 2 | 0.05 | 0.05 | 60+0.6 |

#### Document Test Setup

Create entry in SPRT log:
```markdown
Test ID: SPRT-2025-001
Date: 2025-08-09
Version: 2.1.0-material vs 2.0.0-master
Feature: Material evaluation
Parameters: [0, 5] α=0.05 β=0.05
Time Control: 10+0.1
Opening Book: 8moves_v3.pgn
Concurrency: 4
Hardware: [CPU/RAM specs]
```

### Step 3: Run SPRT Test

#### Standard Command Template
```bash
python3 /workspace/tools/scripts/run_sprt.py \
    /workspace/bin/seajay_[version] \
    /workspace/bin/seajay_[base] \
    --elo0 [ELO0] --elo1 [ELO1] \
    --alpha [ALPHA] --beta [BETA] \
    --time-control "[TC]" \
    --opening-book [BOOK] \
    --output-dir /workspace/sprt_results/[TEST_ID] \
    --concurrency [N]
```

#### Monitor Progress
- Check status every 15-30 minutes
- Note any anomalies
- Don't interrupt unless necessary
- Let SPRT decide when to stop

### Step 4: Record Results

#### Capture All Output
Save to `/workspace/sprt_results/[TEST_ID]/`:
- `sprt_status.json` - Final statistics
- `sprt_games.pgn` - All games played
- `console_output.txt` - Full terminal output
- `test_summary.md` - Human-readable summary

#### Update SPRT Log
Record in `/workspace/project_docs/testing/SPRT_Results_Log.md`:
```markdown
## Test SPRT-2025-001
- **Result**: PASS ✓
- **Games**: 1,234
- **Score**: 654.5/1234 (53.1%)
- **ELO**: +21.7 ± 8.3
- **LLR**: 2.95 (2.89)
- **Duration**: 2h 34m
- **Decision**: Merge to master
```

### Step 5: Post-Test Actions

#### If PASSED ✓
1. Update version in master
2. Tag git commit with version
3. Update project status
4. Document in changelog
5. Archive test results

#### If FAILED ✗
1. Analyze failure reasons
2. Document lessons learned
3. Revert or fix changes
4. Consider different approach
5. Archive test results

#### If INCONCLUSIVE ⋯
1. Review partial results
2. Consider extending test
3. Adjust parameters if needed
4. Document borderline nature

## Results Tracking System

### Directory Structure
```
/workspace/sprt_results/
├── SPRT-2025-001/
│   ├── sprt_status.json
│   ├── sprt_games.pgn
│   ├── console_output.txt
│   └── test_summary.md
├── SPRT-2025-002/
│   └── ...
└── archive/
    └── phase2_complete/
```

### Master SPRT Log Format

Location: `/workspace/project_docs/testing/SPRT_Results_Log.md`

```markdown
# SeaJay SPRT Results Log

## Summary Statistics
- Total Tests: 47
- Passed: 31 (66%)
- Failed: 14 (30%)
- Inconclusive: 2 (4%)
- Current Version: 2.7.3-master
- Estimated Strength: 1523 ELO

## Test History

### Test SPRT-2025-047 (Latest)
- Date: 2025-08-09
- Version: 2.7.3-lmr vs 2.7.2-master
- Feature: Late move reductions
- Parameters: [0, 5] α=0.05 β=0.05
- Result: PASS ✓
- Games: 2,156
- Score: 1,134/2,156 (52.6%)
- ELO: +18.2 ± 7.1
- LLR: 2.91
- Duration: 4h 12m
```

### Version History Tracking

Maintain in `/workspace/project_docs/testing/Version_History.md`:

```markdown
# SeaJay Version History

## Current Master: 2.7.3-master
- Strength: ~1523 ELO
- Last Updated: 2025-08-09

## Version Progression

### Phase 2: Basic Search and Evaluation
- 2.0.0: Baseline (random moves) - 0 ELO
- 2.1.0: Material evaluation - +812 ELO (SPRT-2025-001)
- 2.2.0: Negamax 4-ply - +287 ELO (SPRT-2025-002)
- 2.3.0: Alpha-beta pruning - +156 ELO (SPRT-2025-003)
- 2.4.0: Piece-square tables - +134 ELO (SPRT-2025-004)
```

## Quality Standards

### Minimum Requirements for Master Merge

1. **SPRT PASS** with appropriate parameters
2. **No perft regression** - All tests still pass
3. **No bench regression** > 10% without justification
4. **Code review** completed
5. **Documentation** updated

### Regression Testing

Before each SPRT test:
```bash
# Mandatory regression check
bash /workspace/tools/scripts/run_regression_tests.sh

# Performance baseline check
python3 /workspace/tools/scripts/benchmark_baseline.py
```

### Hardware Consistency

**Important**: Always use same hardware for testing within a phase

Document hardware in each test:
```
CPU: Intel i7-9750H @ 2.6GHz (12 threads)
RAM: 16GB DDR4
OS: Ubuntu 22.04 Docker Container
Turbo: Disabled
Power: Performance mode
```

## Statistical Significance Guidelines

### Understanding SPRT Results

| LLR Range | Interpretation | Action |
|-----------|---------------|--------|
| LLR ≥ 2.89 | Strong evidence for H1 | Merge confidently |
| 2.0 < LLR < 2.89 | Good evidence for H1 | Usually merge |
| -2.0 < LLR < 2.0 | Insufficient evidence | Continue testing |
| -2.89 < LLR < -2.0 | Evidence against | Usually reject |
| LLR ≤ -2.89 | Strong evidence for H0 | Reject confidently |

### Sample Size Guidelines

Typical games needed for detection:

| True ELO Diff | Games Needed | Time @ 10+0.1 |
|---------------|--------------|---------------|
| +20 ELO | 500-1,000 | 1-2 hours |
| +10 ELO | 2,000-4,000 | 4-8 hours |
| +5 ELO | 8,000-16,000 | 16-32 hours |
| +3 ELO | 20,000-40,000 | 40-80 hours |

## Troubleshooting SPRT Tests

### Common Issues

1. **Test never converges**
   - Change provides exactly elo0 improvement
   - Solution: Stop after reasonable games, analyze manually

2. **Opposite result on re-test**
   - Normal statistical variation
   - Solution: Trust SPRT, not intuition

3. **Very quick PASS/FAIL**
   - Large difference (good or bad)
   - Solution: Verify engines aren't swapped

4. **Crashes during test**
   - Save partial results
   - Fix bug and restart
   - Note in documentation

## Reporting Requirements

### Monthly SPRT Summary

Create monthly report including:
- Total tests run
- Pass/fail ratio
- Version progression
- Strength estimate
- Notable improvements
- Failed experiments

### Phase Completion Report

At end of each phase:
- Compile all SPRT results
- Calculate total ELO gain
- Identify successful techniques
- Document failed approaches
- Archive all test data

## Best Practices

1. **One Change Per Test**
   - Isolate improvements
   - Clear attribution
   - Easier debugging

2. **Batch Related Tests**
   - Test all parameters together
   - Use same base version
   - Run in parallel if possible

3. **Document Everything**
   - Test rationale
   - Unexpected results
   - Hardware issues
   - Lessons learned

4. **Regular Baselines**
   - Update baseline weekly
   - Track performance over time
   - Detect gradual regression

5. **Peer Review**
   - Share interesting results
   - Get second opinion on borderline cases
   - Learn from failures

## Continuous Improvement

### Process Reviews

Quarterly review of:
- SPRT parameter effectiveness
- Time control appropriateness
- Hardware utilization
- Process efficiency

### Automation Opportunities

Future improvements:
- Automated SPRT on git push
- Cloud-based testing farm
- Result dashboard
- Strength tracking graph
- Automated reports

## Compliance

**All developers MUST:**
1. Follow this process for covered changes
2. Document all SPRT tests
3. Archive results properly
4. Update version history
5. Maintain hardware consistency

**Exceptions require:**
- Written justification
- Project lead approval
- Documentation in log

## References

- [SPRT How-To Guide](SPRT_How_To_Guide.md) - Practical usage
- [Chess Programming Wiki - SPRT](https://www.chessprogramming.org/SPRT)
- [Stockfish Testing Framework](https://github.com/official-stockfish/fishtest)
- [Statistical Methods in Computer Chess](https://www.3dkingdoms.com/chess/elo.htm)

---

*This process ensures SeaJay's improvements are statistically validated and historically tracked, maintaining scientific rigor throughout development.*