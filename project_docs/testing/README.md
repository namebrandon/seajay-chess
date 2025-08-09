# SeaJay Testing Documentation

This directory contains comprehensive documentation for testing the SeaJay Chess Engine, with particular emphasis on SPRT (Sequential Probability Ratio Test) validation.

## Documents

### For Beginners
- **[SPRT_How_To_Guide.md](SPRT_How_To_Guide.md)** - Start here if you're new to SPRT testing
  - What is SPRT and why use it
  - Understanding parameters
  - Step-by-step instructions
  - Practical examples
  - Troubleshooting

### Process & Governance
- **[SPRT_Testing_Process.md](SPRT_Testing_Process.md)** - Formal testing process
  - When SPRT is required
  - Version numbering system
  - Quality standards
  - Compliance requirements
  - Best practices

### Tracking & History
- **[SPRT_Results_Log.md](SPRT_Results_Log.md)** - Historical test results
  - All SPRT tests conducted
  - Pass/fail statistics
  - Strength progression
  - Test parameters used

- **[Version_History.md](Version_History.md)** - Version progression
  - Release history
  - Feature additions
  - Performance milestones
  - Breaking changes

## Quick Reference

### Standard SPRT Parameters

| Phase | elo0 | elo1 | α | β | Time Control |
|-------|------|------|---|---|--------------|
| Phase 2 | 0 | 5 | 0.05 | 0.05 | 10+0.1 |
| Phase 3 | 0 | 3 | 0.05 | 0.05 | 10+0.1 |
| Phase 4 | -1 | 3 | 0.05 | 0.10 | 20+0.2 |

### Quick Commands

```bash
# Run SPRT test
python3 /workspace/tools/scripts/run_sprt.py \
    engine_new engine_base \
    --elo0 0 --elo1 5

# Check regression
bash /workspace/tools/scripts/run_regression_tests.sh

# Run perft validation
python3 /workspace/tools/scripts/run_perft_tests.py

# Track performance
python3 /workspace/tools/scripts/benchmark_baseline.py
```

### Test Scripts Location
All testing scripts are in `/workspace/tools/scripts/`:
- `run_sprt.py` - SPRT statistical testing
- `run_perft_tests.py` - Move generation validation
- `run_tournament.py` - Tournament management
- `run_regression_tests.sh` - Regression testing
- `benchmark_baseline.py` - Performance tracking

### Configuration Files
- `/workspace/tools/scripts/sprt_config.json` - SPRT parameters by phase
- `/workspace/external/books/4moves_test.pgn` - Quick test opening book

## Testing Workflow

1. **Development Phase**
   - Make changes
   - Run regression tests
   - Check performance baseline

2. **Validation Phase**
   - Build test version
   - Run SPRT test
   - Wait for statistical decision

3. **Integration Phase**
   - Document results in log
   - Update version history
   - Merge if passed

## Important Notes

- **Phase 1**: No SPRT required (foundation only)
- **Phase 2+**: All strength claims must be SPRT validated
- **Hardware**: Keep consistent within a phase
- **Documentation**: Update logs immediately after tests

## Support

For questions about testing:
1. Check the How-To Guide first
2. Review the Process document
3. Look at historical results for examples
4. Consult the development diary

## Future Enhancements

Planned improvements to testing infrastructure:
- Automated SPRT on git commits
- Web dashboard for results
- Cloud-based testing farm
- Real-time strength tracking
- Automated regression detection

---

*Testing is the foundation of chess engine development. Every improvement must be measured, validated, and documented.*