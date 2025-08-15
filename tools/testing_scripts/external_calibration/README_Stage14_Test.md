# Stage 14 External Calibration Test

## Purpose

This test evaluates SeaJay Stage 14 (C10-CONSERVATIVE) against a weakened Stockfish to gauge our current strength against an external benchmark.

## Test Configuration

### SeaJay Stage 14 (C10-CONSERVATIVE)
- **Binary:** `seajay-stage14-sprt-candidate10-conservative`
- **Estimated Strength:** ~2250 ELO
- **Key Features:**
  - Basic quiescence search with captures and check evasions
  - Conservative delta pruning (900cp/600cp margins)
  - MVV-LVA move ordering in quiescence
  - Transposition table integration
  - +300 ELO improvement over Stage 13

### Stockfish Benchmark Configuration
- **Engine:** Stockfish 17.1
- **Strength Limitation:** Skill Level 5 (~1200 ELO)
- **CPU Cores:** 1 (CRITICAL for fairness with single-threaded SeaJay)
- **Hash:** 16 MB (minimal)

## Fairness Considerations

**Single CPU Core Requirement:**
SeaJay is currently single-threaded, so Stockfish must be limited to 1 CPU core for a fair comparison. This is enforced via the `option.Threads=1` setting in fastchess.

**Why This Matters:**
- Multi-threaded Stockfish would have an unfair advantage
- Time controls would be effectively different (parallel vs sequential search)
- Results would not reflect SeaJay's true strength

## Expected Results

Given the estimated strength difference (~1000+ ELO):
- **Expected Score:** 80-90% for SeaJay
- **SPRT Bounds:** [300, 500] ELO advantage
- **Test Duration:** Should terminate quickly with H1 acceptance

## Historical Context

This continues our external calibration series:
- Previous tests used Stage 12 binary (~1800 ELO)
- Stage 14 represents a massive improvement (+450 ELO from Stage 12)
- Should show dramatic improvement vs the same Stockfish benchmark

## Running the Test

```bash
cd /workspace
./tools/testing_scripts/external_calibration/run_seajay_stage14_vs_stockfish_1200_sprt.sh
```

## Key Improvements Tested

Stage 14's major enhancements being validated:
1. **Quiescence Search:** Resolves tactical sequences at leaf nodes
2. **Conservative Tuning:** Lessons learned from C9 catastrophic failure  
3. **Build System Fixes:** Resolution of ENABLE_QUIESCENCE flag issues
4. **Stable Implementation:** Validated through extensive C1-C10 candidate testing

## Success Metrics

- **Primary:** SPRT accepts H1 (SeaJay significantly stronger)
- **Secondary:** Score percentage confirms ~2250 ELO estimate
- **Tertiary:** No time losses or technical issues during extended play

This test validates Stage 14's substantial improvement and confirms our estimated strength before proceeding to Stage 15 (SEE).