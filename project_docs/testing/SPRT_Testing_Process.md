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

## SPRT Testing Process - SeaJay Implementation

### Step 1: Prepare Test Binaries Using Git

**IMPORTANT**: Always use git to retrieve the actual code from the previous stage's completion commit. Never attempt to disable features in current code to simulate an older version.

Create two binaries for comparison:

1. **Save Current Work** (if any uncommitted changes):
   ```bash
   # First, save your current work
   git stash save "Current stage development - preparing for SPRT test"
   # Or commit your changes if ready:
   # git add -A && git commit -m "Stage N implementation complete"
   ```

2. **Build Base Binary** from previous stage's completion commit:
   ```bash
   # Find the commit for the previous stage completion
   git log --oneline | grep -i "stage"  # Find the Stage N-1 completion commit
   
   # Checkout the previous stage's code
   git checkout <stage-n-1-commit-hash>
   
   # Build the base binary
   cd /workspace/build
   cmake .. && make clean && make -j
   
   # Save it with a descriptive name
   cp bin/seajay ../bin/seajay_stage<N-1>_<feature>
   # Example: cp bin/seajay ../bin/seajay_stage7_no_alphabeta
   ```

3. **Build Test Binary** from current stage:
   ```bash
   # Return to your current stage code
   git checkout <your-branch>  # or git stash pop if you stashed
   
   # Build the test binary
   cd /workspace/build
   cmake .. && make clean && make -j
   
   # Save it with a descriptive name
   cp bin/seajay ../bin/seajay_stage<N>_<feature>
   # Example: cp bin/seajay ../bin/seajay_stage8_alphabeta
   ```

4. **Verify Both Binaries Work**:
   ```bash
   # Test the base binary
   echo -e "position startpos\ngo depth 1\nquit" | ../bin/seajay_stage<N-1>_<feature>
   
   # Test the new binary
   echo -e "position startpos\ngo depth 1\nquit" | ../bin/seajay_stage<N>_<feature>
   ```

5. **Restore Your Working State** (if needed):
   ```bash
   # If you stashed changes earlier
   git stash pop
   # Your working directory is now back to where you were
   ```

### Why This Approach is Critical

1. **Accuracy**: You're testing the actual code that was marked complete for each stage
2. **Reproducibility**: Anyone can recreate your test by checking out the same commits
3. **Reliability**: No risk of accidentally leaving features partially disabled
4. **History**: Git maintains the complete development history for reference
5. **Simplicity**: No need to modify code to create test versions

### Example: Testing Stage 8 (Alpha-Beta) vs Stage 7 (Negamax)

```bash
# Save current work
git stash save "Stage 8 alpha-beta implementation"

# Build Stage 7 binary (without alpha-beta)
git checkout c09a377  # Stage 7 completion commit
cd /workspace/build && cmake .. && make clean && make -j
cp bin/seajay ../bin/seajay_stage7_no_alphabeta

# Build Stage 8 binary (with alpha-beta)
git stash pop  # Return to Stage 8 code
cd /workspace/build && cmake .. && make clean && make -j
cp bin/seajay ../bin/seajay_stage8_alphabeta

# Now you have both binaries ready for SPRT testing
```

### Step 2: Pre-Test Validation

- [ ] Both binaries compile without warnings
- [ ] Perft tests pass for test binary (if move generation changed)
- [ ] Both engines respond to UCI commands
- [ ] Fast-chess is available at `/workspace/external/testers/fast-chess/fastchess`
- [ ] Opening book exists at `/workspace/external/books/4moves_test.pgn`

### Step 3: Configure SPRT Parameters

#### Select Appropriate Parameters Based on Expected Improvement

| Expected Improvement | elo0 | elo1 | alpha | beta | Time Control | Notes |
|---------------------|------|------|-------|------|--------------|-------|
| Major feature (>100 Elo) | 0 | 50-200 | 0.05 | 0.05 | 10+0.1 | e.g., adding search to eval-only |
| Significant feature | 0 | 10-30 | 0.05 | 0.05 | 10+0.1 | e.g., pruning, extensions |
| Minor improvement | 0 | 5-10 | 0.05 | 0.05 | 10+0.1 | e.g., move ordering |
| Tuning/refinement | 0 | 3-5 | 0.05 | 0.05 | 10+0.1 | e.g., parameter adjustment |
| Simplification | -3 | 3 | 0.05 | 0.10 | 10+0.1 | Ensure no regression |

### Step 4: Create Test Script

Create a test script (e.g., `/workspace/run_sprt_test.sh`):

```bash
#!/bin/bash

# SPRT Test Script for SeaJay
# Customize the parameters below for your specific test

TEST_ID="SPRT-YYYY-XXX"  # Update with actual test ID
TEST_NAME="Test Binary"   # Descriptive name
BASE_NAME="Base Binary"   # Descriptive name

# SPRT Parameters (adjust based on expected improvement)
ELO0=0      # Lower bound (usually 0)
ELO1=50     # Upper bound (adjust based on expectation)
ALPHA=0.05  # Type I error probability
BETA=0.05   # Type II error probability

# Test configuration
TC="10+0.1"  # Time control
ROUNDS=1000  # Maximum rounds (SPRT may stop earlier)

# Paths
TEST_BIN="/workspace/bin/seajay_test"
BASE_BIN="/workspace/bin/seajay_base"
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo "SPRT Test: ${TEST_ID}"
echo "Testing: ${TEST_NAME} vs ${BASE_NAME}"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Starting at $(date)"
echo ""

# Run fast-chess with SPRT
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${TEST_BIN}" \
    -engine name="${BASE_NAME}" cmd="${BASE_BIN}" \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -openings file="${BOOK}" format=pgn \
    -pgnout "${OUTPUT_DIR}/games.pgn" \
    -log file="${OUTPUT_DIR}/fastchess.log" level=info \
    2>&1 | tee "${OUTPUT_DIR}/console_output.txt"

echo ""
echo "Test completed at $(date)"
echo "Results saved to: ${OUTPUT_DIR}"
```

### Step 5: Run the Test

Execute the test script:
```bash
chmod +x /workspace/run_sprt_test.sh
./run_sprt_test.sh
```

**Note:** SPRT tests can take significant time (15 minutes to several hours depending on the Elo difference and hardware). The test will automatically stop when statistical significance is reached.

### Step 6: Interpret Results

#### Understanding Fast-chess Output

Fast-chess will display results like:
```
Score of Stage7 vs Stage6: 147 - 23 - 30 [0.810]
Elo: 234.89 +/- 44.52 (95% CI)
SPRT: LLR = 2.95 [-2.94, 2.94] (elo0=0, elo1=200, alpha=0.05, beta=0.05)
SPRT: H1 accepted
```

#### Result Interpretation:
- **SPRT: H1 accepted** - Test passed, improvement confirmed
- **SPRT: H0 accepted** - Test failed, no improvement detected
- **SPRT: Inconclusive** - Max games reached without decision

### Step 7: Document Results

Create a summary in `/workspace/sprt_results/[TEST_ID]/test_summary.md`:

```markdown
# SPRT Test Results - [Feature Name]

**Test ID:** SPRT-YYYY-XXX
**Date:** YYYY-MM-DD
**Result:** PASS/FAIL/INCONCLUSIVE

## Engines Tested
- **Test Engine:** [Version and features]
- **Base Engine:** [Version and features]

## Test Parameters
- **Elo bounds:** [elo0, elo1]
- **Significance:** α = X.XX, β = X.XX
- **Time control:** X+Y seconds

## Results
- **Games played:** XXX
- **Score:** XXX/XXX (XX.X%)
- **Win/Draw/Loss:** XX/XX/XX
- **LLR:** X.XX [lower, upper]
- **Estimated Elo:** +XX ± XX

## Conclusion
[Brief interpretation of results]
```

### Step 8: Post-Test Actions

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

## Example: Stage 7 SPRT Test

### Current Situation
- **Implementation Complete:** 4-ply negamax search with iterative deepening
- **Base Version:** Stage 6 (material-only evaluation, 1-ply)
- **Test Version:** Stage 7 (negamax search, 4-ply)
- **Expected Improvement:** Major (200+ Elo)

### Prepared Resources

1. **Binaries Ready:**
   - `/workspace/bin/seajay_stage6` - Material evaluation only
   - `/workspace/bin/seajay_stage7` - With negamax search

2. **Test Script Created:** `/workspace/run_stage7_sprt.sh`
   ```bash
   #!/bin/bash
   TEST_ID="SPRT-2025-001"
   OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"
   
   /workspace/external/testers/fast-chess/fastchess \
       -engine name=Stage7 cmd=/workspace/bin/seajay_stage7 \
       -engine name=Stage6 cmd=/workspace/bin/seajay_stage6 \
       -each proto=uci tc=10+0.1 \
       -sprt elo0=0 elo1=200 alpha=0.05 beta=0.05 \
       -rounds 1000 \
       -openings file=/workspace/external/books/4moves_test.pgn format=pgn \
       -pgnout "${OUTPUT_DIR}/games.pgn" \
       -log file="${OUTPUT_DIR}/fastchess.log" level=info
   ```

3. **To Run the Test:**
   ```bash
   ./run_stage7_sprt.sh
   ```

### Expected Output Format
```
Score of Stage7 vs Stage6: 147 - 23 - 30 [0.810]
Elo: 234.89 +/- 44.52 (95% CI)
SPRT: LLR = 2.95 [-2.94, 2.94] (elo0=0, elo1=200)
SPRT: H1 accepted
```

### After Test Completion
Please either:
1. Copy the console output and share it for review
2. Save output to `/workspace/sprt_results/SPRT-2025-001/console_output.txt` for analysis

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