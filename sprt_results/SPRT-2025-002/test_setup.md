# SPRT Test Setup - Stage 10: Magic Bitboards

**Test ID:** SPRT-2025-002  
**Date:** 2025-08-12  
**Feature:** Magic Bitboards Implementation  
**Expected Improvement:** Significant (30-50 Elo from improved search efficiency)

## Test Configuration

### Engines
- **Test Engine:** Stage 10 - With Magic Bitboards (commit 70d6340)
  - Features: All Stage 9b features + magic bitboards for sliding piece attack generation
  - Performance: 55.98x faster attack generation
  
- **Base Engine:** Stage 9b - Without Magic Bitboards (commit 5e3c2bb)
  - Features: Full evaluation (material, PST, mobility, king safety), alpha-beta search, draw detection
  - Strength: ~1000 Elo (validated in previous SPRT)

### SPRT Parameters
Based on expected improvement from dramatically faster move generation:
- **elo0:** 0 (null hypothesis: no improvement)
- **elo1:** 30 (alternative hypothesis: 30+ Elo gain)
- **alpha:** 0.05 (Type I error probability)
- **beta:** 0.05 (Type II error probability)
- **Time Control:** 10+0.1 (10 seconds + 0.1 increment)
- **Max Rounds:** 2000

### Rationale
Magic bitboards provide:
1. **55.98x faster attack generation** (1.14B ops/sec vs 20.4M ops/sec)
2. **Deeper search** at same time control due to faster move generation
3. **More nodes evaluated** per second
4. **Expected 30-50 Elo gain** from search efficiency alone

## Binary Preparation Instructions

Due to git issues, you'll need to manually prepare the binaries:

### Option 1: Using Git Checkout (Recommended)
```bash
# Fix git issues first if needed
rm ../.gitignore.lock 2>/dev/null

# Save current work
git add -A
git commit -m "temp: save Stage 10 work for SPRT test"

# Build Stage 9b binary
git checkout 5e3c2bb
cd /workspace/build
cmake -DUSE_MAGIC_BITBOARDS=OFF .. && make clean && make -j8
cp seajay ../bin/seajay_stage9b_no_magic

# Build Stage 10 binary  
git checkout stage-10-magic-bitboards
cmake -DUSE_MAGIC_BITBOARDS=ON .. && make clean && make -j8
cp seajay ../bin/seajay_stage10_magic

# Verify both work
echo -e "position startpos\\ngo depth 1\\nquit" | ../bin/seajay_stage9b_no_magic
echo -e "position startpos\\ngo depth 1\\nquit" | ../bin/seajay_stage10_magic
```

### Option 2: Using Current Build
If you already have Stage 10 compiled with magic bitboards:
```bash
# Current binary should have magic bitboards
cp /workspace/build/seajay /workspace/bin/seajay_stage10_magic

# Need to get Stage 9b binary from commit 5e3c2bb
# This requires checking out that commit and building
```

## Test Execution Script

Created at: `/workspace/run_stage10_sprt.sh`

```bash
#!/bin/bash

# SPRT Test for Stage 10: Magic Bitboards
TEST_ID="SPRT-2025-002"
TEST_NAME="Stage10-Magic"
BASE_NAME="Stage9b-NoMagic"

# SPRT Parameters
ELO0=0
ELO1=30
ALPHA=0.05
BETA=0.05

# Test configuration
TC="10+0.1"
ROUNDS=2000

# Paths
TEST_BIN="/workspace/bin/seajay_stage10_magic"
BASE_BIN="/workspace/bin/seajay_stage9b_no_magic"
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo "=========================================="
echo "SPRT Test: ${TEST_ID}"
echo "Testing: ${TEST_NAME} vs ${BASE_NAME}"
echo "Feature: Magic Bitboards (55.98x speedup)"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Starting at $(date)"
echo "=========================================="
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
echo "=========================================="
echo "Test completed at $(date)"
echo "Results saved to: ${OUTPUT_DIR}"
echo "=========================================="
```

## Expected Results

Given the massive performance improvement in attack generation:
- **Expected Elo gain:** 30-50 points
- **Expected games needed:** 500-1500 (for 30 Elo difference)
- **Expected time:** 1-3 hours at 10+0.1 time control
- **Expected result:** PASS (H1 accepted)

## Success Criteria

The test will PASS if:
1. LLR ≥ 2.89 (strong evidence for improvement)
2. Estimated Elo gain ≥ 30
3. No crashes or illegal moves
4. Win rate significantly higher for Stage 10

## Post-Test Actions

### If PASSED ✅
1. Document results in test_summary.md
2. Update project_status.md with Stage 10 completion
3. Consider merging to main branch
4. Archive test results

### If FAILED ❌
1. Investigate why performance gains didn't translate to Elo
2. Check for bugs in magic bitboards implementation
3. Verify test setup was correct
4. Consider different SPRT parameters

## Notes

- Magic bitboards are a well-established technique with proven benefits
- The 55.98x speedup in attack generation should translate to significant Elo gain
- Even a conservative 30 Elo improvement would be excellent for this optimization
- The main risk is implementation bugs, not the technique itself