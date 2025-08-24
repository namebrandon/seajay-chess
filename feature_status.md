# Feature: Rook on Open/Semi-Open Files

## Overview
Implementing evaluation bonuses for rooks on open and semi-open files. This is a fundamental positional evaluation feature that recognizes the increased value of rooks when they control files without pawn obstructions.

**Expected ELO gain:** +15-25 ELO  
**Feature branch:** feature/20250824-rook-open-files  
**Base commit:** fae5569 (fix: Reduce king safety values by ~70%)

## Definitions
- **Open file:** No pawns (neither white nor black) on the file
- **Semi-open file:** No friendly pawns on the file (but enemy pawns may exist)
- **Typical bonuses:** 
  - Open file: +20-30 centipawns
  - Semi-open file: +10-15 centipawns

## Implementation Plan

### Phase 1: Infrastructure (0 ELO expected)
- Add file detection methods to Board class
- Implement isOpenFile() and isSemiOpenFile() helpers
- No integration with evaluation yet
- Expected: 0 ELO (infrastructure only)

### Phase 2: Integration (+15-25 ELO expected)
- Integrate rook file bonuses into static evaluation
- Apply bonuses for rooks on open/semi-open files
- Expected: +15-25 ELO gain

## Phase Status

### Phase 1: Infrastructure
- **Status:** Not started
- **Branch:** feature/20250824-rook-open-files
- **Commit:** TBD
- **Bench:** TBD
- **OpenBench:** Not tested
- **Result:** N/A

### Phase 2: Integration
- **Status:** Not started
- **Branch:** TBD
- **Commit:** TBD
- **Bench:** TBD
- **OpenBench:** Not tested
- **Result:** N/A

## Testing Summary

| Phase | Description | Expected ELO | Actual ELO | Status |
|-------|-------------|--------------|------------|--------|
| 1 | Infrastructure | 0 | - | Not started |
| 2 | Integration | +15-25 | - | Not started |

## Reference Implementations
- 4ku engine: Uses simple open/semi-open file detection
- Stash-bot: Similar approach with tuned values

## Key Learnings
- TBD

## Implementation Notes
- TBD