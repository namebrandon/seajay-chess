# Stage Comparison Binaries for White Bias Investigation

## Purpose
These binaries were built to identify when the white bias bug was introduced by testing each stage in self-play.

## Binaries

### Stage 7: c09a377
- **File:** `seajay-stage7-c09a377`
- **Commit:** c09a377 - "feat: Complete Stage 7 - Basic Material-Only Evaluation"
- **Date:** 2025-08-09
- **MD5:** 6efd1aefb4d0cd9e5e0504db2de3a866
- **Features:** Material-only evaluation, no positional scoring

### Stage 8: 66a6637
- **File:** `seajay-stage8-66a6637`
- **Commit:** 66a6637 - "feat: Complete Stage 8 - Material Management Infrastructure"
- **Date:** 2025-08-09
- **MD5:** dc95a7f1ba5e356dc79722b6a300ed1a
- **Features:** Material tracking with incremental updates

### Stage 9: fe33035
- **File:** `seajay-stage9-fe33035`
- **Commit:** fe33035 - "feat: Complete Stage 9 - Piece-Square Tables (PST) Implementation"
- **Date:** 2025-08-09
- **MD5:** 862504c9f2247f0e74fe7f9840b08887
- **Features:** PST evaluation added to material evaluation

### Stage 10: e3c59e2
- **File:** `seajay-stage10-e3c59e2`
- **Commit:** e3c59e2 - "Merge Stage 10: Magic Bitboards Implementation"
- **Date:** 2025-08-13
- **MD5:** 2d0b0a1f272cf5bd64d83cf0e469c389
- **Features:** Magic bitboards for sliding piece move generation

### Stage 11: f2ad9b5
- **File:** `seajay-stage11-f2ad9b5`
- **Commit:** f2ad9b5 - "docs: Complete Stage 11 checklist and documentation updates"
- **Date:** 2025-08-14
- **MD5:** ccf8c16addfb3334d24dbcaeb31038f5
- **Features:** MVV-LVA move ordering implementation

### Stage 12: ffa0e44
- **File:** `seajay-stage12-ffa0e44`
- **Commit:** ffa0e44 - "feat: Stage 12 Transposition Tables - FINAL COMPLETION ✅"
- **Date:** 2025-08-14
- **MD5:** 2dbf7b17902c0b979dd86f17a3971047
- **Features:** Transposition table for search result caching

### Stage 13: 869495e
- **File:** `seajay-stage13-869495e`
- **Commit:** 869495e - "chore: Stage 13 FINAL - Complete iterative deepening implementation"
- **Date:** 2025-08-14
- **MD5:** 45f79e54745e4940dc2b08dfc154dade
- **Features:** Iterative deepening with aspiration windows

## Testing Instructions

To test for white bias in self-play:

```bash
# Test Stage 7
./binaries/seajay-stage7-c09a377 <<EOF
position startpos
go depth 5
quit
EOF

# Run multiple games and track win rates
# If White wins >60% consistently, bias exists at this stage
```

## Expected Findings

Based on the investigation, we expect:
- Stage 7: Should be relatively balanced (material-only)
- Stage 8: Still material-only, should remain balanced
- Stage 9: PST introduced - likely where bias appears

The PST implementation in Stage 9 is the primary suspect because:
1. It introduces positional scoring for the first time
2. The sign handling for Black pieces has been problematic
3. Move ordering (a1-h8 via popLsb) combined with PST may create systematic bias