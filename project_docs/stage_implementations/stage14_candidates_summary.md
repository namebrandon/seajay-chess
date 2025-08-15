# Stage 14 Candidates Summary

## Overview
Stage 14 implemented quiescence search to resolve tactical positions. After discovering that quiescence was never compiled in (Candidates 2-6), we successfully fixed the issue and are now optimizing parameters.

## Candidate Performance Summary

### Golden Reference
**Candidate 1 (GOLDEN)** - ce52720
- Size: 411,336 bytes
- Performance: +300 ELO over Stage 13
- Status: Preserved as golden reference
- Note: Had ENABLE_QUIESCENCE manually added during testing

### Failed Attempts (Missing Quiescence)
**Candidates 2-6** - Various attempts
- Size: ~384 KB (27KB smaller than golden)
- Problem: ENABLE_QUIESCENCE flag was never defined
- Result: All fell back to static evaluation
- Performance: -191 to -300 ELO vs Golden

### Successful Fixes
**Candidate 7 (FIXED)** - 8ff5538
- Size: 411,384 bytes
- Changes: Added ENABLE_QUIESCENCE to CMakeLists.txt
- Performance: ~0 ELO vs Golden (51.88% after 80 games)
- Status: Validated the fix

**Candidate 8 (NO-IFDEFS)** - 8ff5538
- Size: 411,384 bytes
- Changes: Removed all dangerous ifdef patterns
- Performance: 0.00 ELO vs Golden (exactly 50% after 40 games)
- Status: Clean architecture established

### Optimization Attempts
**Candidate 9 (TUNED)** - 8fc25dd
- Size: 411,384 bytes
- Changes:
  - Delta margins: 900→200cp (endgame: 600→150cp)
  - Check depth limit: 8→3 plies
  - Added panic mode for time pressure (<100ms)
- Expected: +20-40 ELO improvement
- Status: Ready for SPRT testing

### Future Candidates
**Candidate 10 (Planned)**
- Proposed: Add quiet checks at depth 0
- Expected: +15-25 ELO
- Deferred from C9 to avoid too many changes at once

## Key Metrics

| Candidate | Binary Size | vs Golden ELO | vs Stage 13 ELO | Status |
|-----------|------------|---------------|-----------------|---------|
| C1 Golden | 411,336 B | Baseline | +300 | Reference |
| C2-C6 | ~384 KB | -191 to -300 | +87 | Failed (no QS) |
| C7 Fixed | 411,384 B | ~0 | +300 | Success |
| C8 No-ifdefs | 411,384 B | 0.00 | +300 | Success |
| C9 Tuned | 411,384 B | TBD | TBD | Testing |

## Lessons Learned

1. **Compile-time flags are dangerous** - Lost 4 hours to missing ENABLE_QUIESCENCE
2. **Binary size is a critical signal** - 27KB difference = entire feature missing
3. **Always preserve working binaries** - Golden C1 saved the project
4. **Build system caching can lie** - Always force clean rebuilds when debugging
5. **Small optimizations matter** - Delta margin tuning can yield 20-30 ELO

## Test Scripts

All SPRT test scripts are in `/workspace/tools/scripts/sprt_tests/`:
- `run_golden_c1_vs_candidate7_final.sh` - Validates the fix
- `run_golden_c1_vs_candidate8_no_ifdefs.sh` - Tests ifdef removal
- `run_golden_c1_vs_candidate9_improvements.sh` - Tests optimizations

## Next Steps

1. Complete SPRT testing of Candidate 9
2. If C9 shows improvement, consider implementing quiet checks (C10)
3. If C9 fails, investigate parameter tuning further
4. Document final Stage 14 completion when optimal candidate identified

---
*Last Updated: August 15, 2025*
*Current Best: Candidate 8 (matches Golden performance with clean architecture)*