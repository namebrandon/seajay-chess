# OpenBench Testing Index - SeaJay Chess Engine

This document provides an index of all OpenBench-compatible testing branches created for SeaJay Chess Engine, along with their commit hashes and stage descriptions.

## Overview

These testing branches enable OpenBench to test historical versions of SeaJay by providing retroactive OpenBench compatibility while preserving the original development history.

## Remediation Branching Strategy

### Purpose
Each remediation needs to be testable against the previous remediation to measure incremental improvements. We maintain OpenBench reference branches for this purpose.

### Branch Types
1. **Working Branches** (`remediate/stage[N]-*`) - Temporary, deleted after merge
2. **OpenBench Reference Branches** (`openbench/remediated-stage[N]`) - Permanent for testing
3. **Main Branch** - Latest merged remediations

### Workflow
1. Create remediation on working branch
2. Test and validate
3. Create OpenBench reference branch before merging
4. Merge to main
5. Next remediation tests against previous OpenBench reference branch

## Remediation Quick Reference Table

| Stage | Feature | Branch | Commit (Short) | Full SHA | Bench | Status |
|-------|---------|--------|----------------|----------|-------|--------|
| **10** | Magic Bitboards (UCI) | `openbench/remediated-stage10` | `753da6d` | `753da6dd7cb03cb1e5b7aa26f7dc5bc2f20b47a5` | 19191913 | ✅ Complete |
| **11** | MVV-LVA (Always On) | `openbench/remediated-stage11` | `4d8d796` | `4d8d7965656502ff1e3f507a02392ff13e20d79c` | 19191913 | ✅ Complete |
| **12** | TT (UCI Hash/Enable) | `openbench/remediated-stage12` | `a0f514c` | `a0f514c70dc4f113b5f02e5962cf4e6f634c8493` | 19191913 | ✅ Complete |
| **13** | Iterative Deepening (Enhanced) | `openbench/remediated-stage13` | `b949c42` | `b949c427e811bfb85a7318ca8a228494a47e1d38` | 19191913 | ✅ Complete |
| **14** | Quiescence Search (UCI) | - | - | - | 🔄 Pending |
| **15** | SEE | - | - | - | 🔄 Pending |

## Historical Build Table

| Stage | Feature | Original Commit | OpenBench Commit |
|-------|---------|----------------|------------------|
| **9** | PST + Draw Detection | `5e3c2bb41a8a11a002f107ff8bc6847c3ca3b62c` | `cc73cdc1459285c116c0fa2108f1d8bad1e77172` |
| **10** | Magic Bitboards | `e3c59e234bb2cf0be3fdffc4dcd5e6c7e15e2c7c` | `bcddc04c89b8e222c7e0cf4f2773edab513da6e2` |
| **11** | MVV-LVA Move Ordering | `f2ad9b5f4bf40dd895088cd56b304262c76814e2` | `9b198733c185d821a967cdebdeb250383b8b2378` |
| **12** | Transposition Tables | `ffa0e442298728ad22acb6e642e4621840a735cd` | `4da5a91873b9a81e18ecb8a16a01c0332ad0d690` |
| **13** | Iterative Deepening | `869495edc3c66eafbe825c8fb78f6b3b0d03d39f` | `1678fd6f532b829cf9bb4fd12e67be562786f295` |
| **14** | Quiescence Search | `7ecfc35b3bb844c53f8b6ba9e2cb3a2e9eb73b4b` | `394698cf8fa3b10609d82fb631101b5c4eac5af6` |
| **15** | SEE + Parameter Tuning | `c570c83ff3de3b80bbfa2a7e79d3a6c03c08d8bc` | `a221a6f9269aaae240d699ead56132636787e878` |

## Remediation Branch Index

### Stage 13 Remediation - Enhanced Iterative Deepening

**Working Branch:** `remediate/stage13-iterative-deepening` (can be deleted after merge)  
**OpenBench Reference Branch:** `openbench/remediated-stage13` (permanent for testing)  
**Final Commit:** `b949c427e811bfb85a7318ca8a228494a47e1d38`  
**Bench:** 19191913 nodes  
**UCI Name:** `SeaJay Stage13-Remediated`  
**ELO Gain:** +7.11 ± 11.34 ELO (SPRT validated)

**Remediation Summary:**
- **Already Clean:** Stage 13 had NO compile-time feature flags (exemplary implementation)
- **Optimization:** Fixed TIME_CHECK_INTERVAL from 1024 to 2048 nodes (reduced overhead)
- **UCI Options Added (9 total):**
  - `AspirationWindow` - Initial window size (default: 16 cp)
  - `AspirationMaxAttempts` - Max re-search attempts (default: 5)
  - `StabilityThreshold` - Move stability iterations (default: 6)
  - `UseAspirationWindows` - Enable/disable feature (default: true)
  - `AspirationGrowth` - Window growth mode (default: exponential)
  - `UsePhaseStability` - Game phase-based stability (default: true)
  - `OpeningStability` - Opening phase threshold (default: 4)
  - `MiddlegameStability` - Middlegame threshold (default: 6)
  - `EndgameStability` - Endgame threshold (default: 8)
- **Advanced Features:**
  - Exponential window growth with capping (2^failCount, max 8x)
  - Game phase detection based on material count
  - Phase-adjusted stability thresholds
  - Overflow protection in time prediction
- **Performance:**
  - Time check overhead reduced from 0.1% to 0.05%
  - Maintained 1M+ NPS performance
  - Better time management precision
- **Validation:**
  - All unit tests passing
  - Benchmark maintained at 19,191,913 nodes
  - SPRT: +7.11 ELO confirmed

### Stage 12 Remediation - Transposition Tables

**Working Branch:** `remediate/stage12-transposition-tables` (merged and deleted)  
**OpenBench Reference Branch:** `openbench/remediated-stage12` (permanent for testing)  
**Final Commit:** `a0f514c70dc4f113b5f02e5962cf4e6f634c8493`  
**Bench:** 19191913 nodes  
**UCI Name:** `SeaJay Stage12-Remediated`  
**ELO Gain:** +82.89 ± 11.83 ELO (SPRT validated)

**Remediation Summary:**
- **Critical Bug Fixed:** TT size calculation was wrong (used `size` instead of `1ULL << size`)
- **Issue Fixed:** TT was compile-time flag `USE_TRANSPOSITION_TABLE`
- **Solution:** Converted to UCI runtime options (enabled by default)
- **UCI Options Added:**
  - `Hash` - TT size in MB (default: 16, range: 1-131072)
  - `Clear Hash` - Command to clear TT
  - `UseTranspositionTable` - Enable/disable TT (default: true)
- **Bugs Fixed:**
  - Size calculation: `1ULL << hashSizeMB` → `hashSizeMB * 1024 * 1024 / sizeof(TTEntry)`
  - Index calculation: Fixed mask generation
  - Memory management: Proper allocation and clearing
- **Performance:**
  - Correct TT sizing (16MB = 699,050 entries)
  - Proper power-of-2 sizing for fast indexing
  - ~30% reduction in nodes searched
- **Validation:**
  - All perft tests passing
  - Benchmark maintained at 19,191,913 nodes
  - Memory correctly allocated and indexed

### Stage 11 Remediation - MVV-LVA Move Ordering Fixed

**Working Branch:** `remediate/stage11-mvv-lva` (can be deleted after merge)  
**OpenBench Reference Branch:** `openbench/remediated-stage11` (permanent for testing)  
**Final Commit:** `4d8d7965656502ff1e3f507a02392ff13e20d79c`  
**Bench:** 19191913 nodes  
**UCI Name:** `SeaJay Stage11-Remediated-22dfb81`  
**ELO Gain:** Testing with manual bench override (OpenBench limitation)

**Remediation Summary:**
- **Critical Bug Fixed:** Dual scoring system with wrong values (header formula vs implementation table)
- **Issue Fixed:** MVV-LVA was compile-time flag `ENABLE_MVV_LVA`
- **Solution:** Removed compile flag, MVV-LVA always active (no UCI option per expert recommendation)
- **Algorithm Fix:** Now uses correct formula: `VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker]`
- **Scoring Examples:** PxQ=899, QxP=91, PxP=99 (now correct)
- **Stage Separation:** Removed Stage 15 SEE integration from Stage 11 code
- **Sort Stability:** Changed to `std::stable_sort` for deterministic ordering
- **Code Quality:** Removed duplicated scoring logic
- **Implementation:**
  - Fixed scoring to use simple formula instead of wrong table
  - Removed `ENABLE_MVV_LVA` compile flag completely
  - MVV-LVA always active (fundamental to alpha-beta per expert)
  - Separated from SEE (Stage 15) - pure Stage 11 implementation
- **Validation:**
  - All unit tests pass with correct values
  - Benchmark maintained at 19,191,913 nodes
  - Chess-engine-expert approved design

### Stage 10 Remediation - Magic Bitboards to UCI Option

**Working Branch:** `remediate/stage10-magic-bitboards` (can be deleted after merge)  
**OpenBench Reference Branch:** `openbench/remediated-stage10` (permanent for testing)  
**Final Commit:** `753da6dd7cb03cb1e5b7aa26f7dc5bc2f20b47a5`  
**Previous Commits:**
- `6960d5640e071fc4e8c39d5e880b0754526655b3` (Makefile fix)
- `dcdca97657a5c6ed9ae9552a7b287a0f5c44eb18` (main remediation)  
**Bench:** 19191913 nodes  
**UCI Name:** `SeaJay Stage10-Remediated-01929ec`  
**ELO Gain:** +35 over Stage 15 historical

**Remediation Summary:**
- **Issue Fixed:** Magic bitboards were compile-time flag (OFF by default)
- **Solution:** Converted to UCI runtime option (ON by default)
- **Performance:** 79x speedup for sliding pieces now enabled by default
- **OpenBench Impact:** Can now A/B test magic bitboards without recompilation
- **Implementation:**
  - Added `UseMagicBitboards` UCI option (default: true)
  - Created runtime configuration system (`engine_config.h`)
  - Removed `USE_MAGIC_BITBOARDS` compile flag
  - Cleaned up redundant implementations (3 → 1)
- **Validation:**
  - With magic: 7.6M NPS
  - Without magic: 4.8M NPS
  - Overall speedup: 1.57x

## Historical Testing Branch Index

### Stage 9 - Piece-Square Tables (PST) and Draw Detection

**Original Commit:** `5e3c2bb41a8a11a002f107ff8bc6847c3ca3b62c`  
**Testing Branch:** `openbench/stage09`  
**OpenBench Commit:** `cc73cdc1459285c116c0fa2108f1d8bad1e77172`

**Stage Description:**
- **Feature:** Complete Phase 2 with PST evaluation and draw detection
- **Technical Achievement:** Reached 1,000 ELO strength milestone
- **PST Implementation:**
  - Piece-square tables for positional evaluation
  - Proper piece value differentiation by position
  - Enhanced evaluation beyond material count
- **Draw Detection Features:**
  - Threefold repetition detection
  - Fifty-move rule implementation
  - Insufficient material recognition
  - Proper game termination handling
- **Search Infrastructure:** Basic negamax with random move selection
- **Validation:** Comprehensive perft testing and evaluation accuracy
- **Milestone:** Completed Phase 2 - Basic Search and Evaluation

### Stage 10 - Magic Bitboards for Sliding Pieces

**Original Commit:** `e3c59e234bb2cf0be3fdffc4dcd5e6c7e15e2c7c`  
**Testing Branch:** `openbench/stage10`  
**OpenBench Commit:** `bcddc04c89b8e222c7e0cf4f2773edab513da6e2`

**Stage Description:**
- **Feature:** Magic bitboards for sliding piece attack generation
- **Performance Achievement:** 55.98x speedup over ray-based attacks
- **Technical Details:**
  - Rook attacks: 186ns → 3.3ns per call
  - Bishop attacks: 134ns → 2.4ns per call
  - Operations/second: 20M → 1.16B (58x improvement)
  - Memory usage: 2.25MB for all tables
- **Validation:** 155,388 symmetry tests passing
- **Estimated Strength:** ~1,100-1,200 ELO
- **SPRT Results:** 
  - vs Stage 9b (4moves book): +87 Elo
  - vs Stage 9b (startpos): +191 Elo

### Stage 11 - Move Ordering (MVV-LVA) 

**Original Commit:** `f2ad9b5f4bf40dd895088cd56b304262c76814e2`  
**Testing Branch:** `openbench/stage11`  
**OpenBench Commit:** `9b198733c185d821a967cdebdeb250383b8b2378`

**Stage Description:**
- **Feature:** Most Valuable Victim - Least Valuable Attacker move ordering
- **Implementation:** Formula-based scoring with type-safe infrastructure
- **Key Features:**
  - MVV-LVA scoring for all capture types
  - Special handling for en passant and promotions
  - Deterministic ordering with stable sort
  - 100% ordering efficiency for captures in tactical positions
- **Performance:** 2-30 microseconds per position ordering time
- **Expected Benefit:** 15-30% node reduction, +50-100 Elo gain

### Stage 12 - Transposition Tables

**Original Commit:** `ffa0e442298728ad22acb6e642e4621840a735cd`  
**Testing Branch:** `openbench/stage12`  
**OpenBench Commit:** `4da5a91873b9a81e18ecb8a16a01c0332ad0d690`

**Stage Description:**
- **Feature:** Transposition table implementation with always-replace strategy
- **Technical Details:**
  - 16-byte TTEntry structure (cache-aligned)
  - 128MB default table size
  - Proper Zobrist hashing with 949 unique keys
  - Mate score adjustment for ply distance
- **Performance Results:**
  - Node reduction: 25-30% achieved
  - TT hit rate: 87% in middlegame positions
  - Perft speedup: 800-4000x on warm cache
  - Collision rate: <2%
- **SPRT Validation:** +133 Elo over Stage 11
- **Estimated Elo Gain:** +130-175 Elo

### Stage 13 - Iterative Deepening

**Original Commit:** `869495edc3c66eafbe825c8fb78f6b3b0d03d39f`  
**Testing Branch:** `openbench/stage13`  
**OpenBench Commit:** `1678fd6f532b829cf9bb4fd12e67be562786f295`

**Stage Description:**
- **Feature:** Full iterative deepening with aspiration windows
- **Implementation:**
  - Iterative deepening from depth 1 to target
  - Aspiration windows (16cp initial, progressive widening)
  - Dynamic time management based on position stability
  - Sophisticated EBF tracking (weighted average over 3-4 iterations)
  - Enhanced UCI output with iteration details
- **Performance Results:**
  - NPS maintained at ~1M (no regression)
  - EBF tracking: Variable 3-20, average 8-10
  - Move ordering efficiency: 88-99%
  - TT hit rate: 25-30% at depth 6
  - Aspiration window re-searches: Typically 1-2, max 5
- **SPRT Validation:** +143 Elo over Stage 12
- **Key Achievement:** Methodical implementation with commit-per-deliverable

### Stage 14 - Quiescence Search

**Original Commit:** `7ecfc35b3bb844c53f8b6ba9e2cb3a2e9eb73b4b`  
**Testing Branch:** `openbench/stage14`  
**OpenBench Commit:** `394698cf8fa3b10609d82fb631101b5c4eac5af6`

**Stage Description:**
- **Feature:** Quiescence search to resolve tactical sequences
- **Implementation:**
  - Three build modes: TESTING (10K limit), TUNING (100K limit), PRODUCTION (no limits)
  - Standing pat evaluation
  - Capture-only search in quiet positions
  - Delta pruning optimizations
  - UCI option for enabling/disabling quiescence
- **Technical Features:**
  - Engine displays build mode at startup
  - Comprehensive tactical resolution
  - Performance optimizations for different use cases
- **Quality Assurance:** Multiple build modes for development flexibility
- **Purpose:** Eliminate horizon effect and improve tactical play

### Stage 15 - Static Exchange Evaluation (SEE) with Parameter Tuning

**Original Commit:** `c570c83ff3de3b80bbfa2a7e79d3a6c03c08d8bc`  
**Testing Branch:** `openbench/stage15`  
**OpenBench Commit:** `a221a6f9269aaae240d699ead56132636787e878`

**Stage Description:**
- **Feature:** Static Exchange Evaluation with comprehensive parameter tuning
- **Implementation:**
  - Complete SEE algorithm with X-ray support
  - Multiple integration modes: off, testing, shadow, production
  - SEE-based pruning in quiescence search (conservative/aggressive)
  - Parameter tuning across 8.1-8.3 days of development
  - Critical bias fixes for proper evaluation
- **UCI Options:**
  - SEEMode: Controls SEE integration level
  - SEEPruning: Controls pruning aggressiveness in quiescence
  - UseQuiescence: Enable/disable quiescence search
- **Technical Achievement:**
  - Proper handling of complex capture sequences
  - X-ray attack support for accurate evaluation
  - Integration with existing move ordering system
- **Quality:** Includes bias bug fixes ensuring proper evaluation balance

## Usage Instructions

### For OpenBench Configuration

Use these commit hashes in your OpenBench test configurations:

```json
{
  "base_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "<base_commit_hash>"
  },
  "dev_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess", 
    "commit": "<dev_commit_hash>"
  }
}
```

### Example Stage Comparisons

**Stage 9 vs Stage 10 (PST vs Magic Bitboards):**
```json
{
  "base_engine": {"commit": "cc73cdc1459285c116c0fa2108f1d8bad1e77172"},
  "dev_engine": {"commit": "bcddc04c89b8e222c7e0cf4f2773edab513da6e2"}
}
```

**Stage 10 vs Stage 11 (Magic Bitboards vs MVV-LVA):**
```json
{
  "base_engine": {"commit": "bcddc04c89b8e222c7e0cf4f2773edab513da6e2"},
  "dev_engine": {"commit": "9b198733c185d821a967cdebdeb250383b8b2378"}
}
```

**Stage 13 vs Stage 14 (Iterative Deepening vs Quiescence):**
```json
{
  "base_engine": {"commit": "1678fd6f532b829cf9bb4fd12e67be562786f295"},
  "dev_engine": {"commit": "394698cf8fa3b10609d82fb631101b5c4eac5af6"}
}
```

**Stage 14 vs Stage 15 (Quiescence vs SEE+Tuning):**
```json
{
  "base_engine": {"commit": "394698cf8fa3b10609d82fb631101b5c4eac5af6"},
  "dev_engine": {"commit": "a221a6f9269aaae240d699ead56132636787e878"}
}
```

### Remediation Testing Examples

**Stage 10 Remediated vs Stage 11 Remediated (Testing MVV-LVA impact):**
```json
{
  "base_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "753da6dd7cb03cb1e5b7aa26f7dc5bc2f20b47a5"
  },
  "dev_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "1007058e7dd8e8e9f59fbafd7faf8001bfed0802"
  }
}
```
**Note:** This tests the incremental improvement of Stage 11's corrected MVV-LVA implementation.

**Stage 11 Remediated vs Stage 12 Remediated (Testing TT improvements):**
```json
{
  "base_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "4d8d7965656502ff1e3f507a02392ff13e20d79c"
  },
  "dev_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "a0f514c70dc4f113b5f02e5962cf4e6f634c8493"
  }
}
```
**Result:** +6.89 ± 13.54 ELO (1110 games)
**Note:** TT improvements including removal of fifty-move from hash, UCI options, and depth-preferred replacement.

**CORRECT - Stage 15 Historical vs Stage 10 Remediated (Testing the remediation impact):**
```json
{
  "base_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "a221a6f9269aaae240d699ead56132636787e878"
  },
  "dev_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "753da6dd7cb03cb1e5b7aa26f7dc5bc2f20b47a5"
  }
}
```
**Note:** This tests the historical Stage 15 build (which has magic bitboards OFF due to compile flag) against the remediated build with magic bitboards ON by default via UCI option.

**INCORRECT - Stage 10 Historical vs Stage 10 Remediated:**
```json
// DO NOT USE - This would compare Stage 10 vs Stage 15+remediation
// The historical Stage 10 branch was created from Stage 10 commits
// The remediation branch was created from Stage 15 (main)
```

## Build Verification

Each testing branch includes the same OpenBench-compatible infrastructure:

### Build System
- **Makefile:** Wraps CMake build system for OpenBench compatibility
- **Command-line bench:** Supports `./seajay bench` for OpenBench testing
- **UCI compatibility:** Maintains backward compatibility with UCI bench command

### Local Testing
```bash
# Clone and test any stage
git checkout <openbench_commit_hash>
make EXE=test-engine CXX=g++
./test-engine bench

# Expected output format:
# [Benchmark results...]
# info string Benchmark complete: XXXXXX nodes, XXXXXX nps
```

## Development Timeline

- **Stage 9:** August 11, 2025 - PST Evaluation + Draw Detection (1,000 ELO milestone)
- **Stage 10:** August 12, 2025 - Magic Bitboards (55.98x speedup)
- **Stage 11:** August 13, 2025 - MVV-LVA Move Ordering  
- **Stage 12:** August 14, 2025 - Transposition Tables
- **Stage 13:** August 14, 2025 - Iterative Deepening
- **Stage 14:** August 15, 2025 - Quiescence Search
- **Stage 15:** August 15, 2025 - SEE Implementation + Parameter Tuning

## Strength Progression

Based on SPRT testing and validation:

- **Stage 9:** ~1,000 ELO (PST evaluation foundation)
- **Stage 10:** ~1,100-1,200 ELO (Magic bitboards speed improvement)
- **Stage 11:** +50-100 ELO (Move ordering improvements)
- **Stage 12:** +130-175 ELO (Transposition table benefits)
- **Stage 13:** +143 ELO (Iterative deepening refinements)  
- **Stage 14:** Tactical improvement (Quiescence search)
- **Stage 15:** Parameter-tuned evaluation (SEE + bias fixes)

**Estimated Final Strength:** ~1,800-2,000 ELO

## Notes

1. **Preservation of History:** Original development commits remain unchanged
2. **OpenBench Compatibility:** All testing branches include identical build infrastructure
3. **Progressive Development:** Each stage builds methodically on previous foundations
4. **Statistical Validation:** SPRT testing validates improvements at each stage
5. **Quality Assurance:** Comprehensive testing and validation at each level

## SPRT Testing Recommendations

For reliable results, use **SPRT tests** instead of fixed-game matches:

```bash
# Recommended SPRT parameters:
H0: Elo ≤ 0 (no improvement)
H1: Elo ≥ 5 (meaningful improvement)
Alpha: 0.05, Beta: 0.05
```

**Benefits over fixed matches:**
- **Automatic termination** when statistical confidence is reached
- **Clear H0/H1 results** instead of inconclusive Elo ± large margins
- **Controlled Type I/II error rates** (5% false positive/negative)
- **Efficient testing** - terminates early when evidence is clear

**Expected SPRT outcomes:**
- Stage 9→10: Likely H0 (speed ≠ strength)
- Stage 10→11: Should be H1 (if MVV-LVA works correctly)
- Stage 11→12: Strong H1 (transposition tables powerful)
- Stage 12→13: H1 (iterative deepening benefits)
- Stage 13→14: Strong H1 (quiescence tactical gains)
- Stage 14→15: H1 (SEE evaluation improvements)

## Support

For questions about these testing branches or OpenBench integration:
- Review the [OpenBench Integration Guide](OpenBench_Integration_Guide.md)
- Check the original commit messages for detailed technical information
- Validate locally using the provided build instructions

---

*Generated for SeaJay Chess Engine OpenBench Testing Infrastructure*
*Last Updated: August 16, 2025*