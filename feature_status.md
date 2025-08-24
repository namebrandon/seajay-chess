# Piece Mobility Feature Implementation

## Overview
**Feature:** Piece Mobility Evaluation
**Expected ELO Gain:** +30-50 ELO
**Reference:** 4ku uses mobility bonuses with pawn attack exclusion
**Branch:** feature/20250824-piece-mobility

## Timeline
- **Start Date:** 2025-08-24
- **Base Commit:** 33d786e (main)

## Implementation Plan

### Phase 1: Infrastructure (0 ELO expected)
- Add mobility counting functions for each piece type
- No integration with evaluation yet
- Expected: 0 ELO (infrastructure only)
- SPRT Bounds: [-5.00, 3.00]

### Phase 2: Basic Integration (+20-30 ELO expected)
- Integrate mobility bonus into evaluation
- Use conservative uniform values initially
- Expected: +20-30 ELO
- SPRT Bounds: [0.00, 8.00]

### Phase 3: Tuning (+10-20 ELO expected)
- Tune mobility values per piece type
- Optimize for different game phases
- Expected: +10-20 ELO additional
- SPRT Bounds: [0.00, 5.00]

## Phase Status

### Phase 1: Infrastructure
- **Status:** Complete
- **Commit:** 3be1c61
- **Bench:** 19191913
- **Test Results:** Awaiting OpenBench

### Phase 2: Basic Integration
- **Status:** Complete
- **Commit:** 16774bf
- **Bench:** 19191913
- **Test Results:** Awaiting OpenBench

### Phase 3: Tuning
- **Status:** Pending
- **Commit:** TBD
- **Bench:** TBD
- **Test Results:** Awaiting

## Testing Summary

| Phase | Expected ELO | Actual ELO | Status | Notes |
|-------|-------------|------------|--------|-------|
| Phase 1 | 0 | TBD | Pending | Infrastructure only |
| Phase 2 | +20-30 | TBD | Pending | Basic integration |
| Phase 3 | +10-20 | TBD | Pending | Per-piece tuning |

## Key Design Decisions
- Count pseudo-legal moves for each piece
- Exclude squares attacked by enemy pawns (safer mobility)
- Phase-scaled bonuses (mobility more important in middlegame)
- Different values per piece type (knights benefit more from mobility)

## Implementation Details
Based on 4ku approach:
- Knights: Count all pseudo-legal moves not attacked by enemy pawns
- Bishops: Count diagonal moves not attacked by enemy pawns  
- Rooks: Count horizontal/vertical moves not attacked by enemy pawns
- Queens: Combine bishop and rook mobility
- Kings: Limited mobility bonus (safety more important)