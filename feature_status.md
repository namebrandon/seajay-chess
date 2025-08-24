# King Safety / Pawn Shield Feature Implementation

## Overview
Implementing king safety evaluation with focus on pawn shield bonuses. Expected gain: +20-40 ELO total across all phases.

## Reference Implementation
Using 4ku chess engine as reference:
- S(33, -10) per shielding pawn directly in front
- S(25, -7) per pawn one rank forward  
- Only applies for kings on reasonable squares (castled positions)

## Timeline
- **Start Date:** 2025-08-24
- **Branch:** feature/20250824-king-safety  
- **Base Commit:** 28048ef (main)

## Phase Status

### Phase KS1: Infrastructure (0 ELO expected)
**Status:** Pending
**Description:** Add king safety data structures and helper functions
- Add KingSafety class/structure
- Add pawn shield detection functions
- Add king position classification
- No integration with evaluation yet

**Expected Changes:**
- New files: `src/eval/king_safety.h`, `src/eval/king_safety.cpp`
- Test coverage for shield detection

**SPRT Bounds:** [-5.00, 3.00] (infrastructure validation)

### Phase KS2: Integration Without Scoring (0 to -5 ELO expected)
**Status:** Not Started
**Description:** Hook into evaluation system but return 0 score
- Integrate KingSafety into Evaluation class
- Call king safety evaluation but multiply by 0
- Validates integration without affecting strength

**Expected Changes:**
- Modified: `src/eval/evaluation.cpp`
- Added calls to king safety evaluation

**SPRT Bounds:** [-5.00, 3.00] (overhead validation)

### Phase KS3: Activation (+20-30 ELO expected)
**Status:** Not Started  
**Description:** Enable king safety scoring with initial values
- Enable S(33, -10) for direct shield pawns
- Enable S(25, -7) for advanced shield pawns
- Only for reasonable king positions

**Expected Changes:**
- Enable scoring in evaluation
- UCI option for tuning (default enabled)

**SPRT Bounds:** [10.00, 25.00] (main gain phase)

### Phase KS4: Tuning (+5-10 ELO expected)
**Status:** Not Started
**Description:** SPSA tuning of king safety parameters
- Tune shield pawn values
- Tune king position thresholds
- Optimize for different game phases

**Expected Changes:**
- Updated constants based on SPSA results

**SPRT Bounds:** [2.00, 8.00] (tuning refinement)

## Testing Summary

| Phase | Commit | Bench | Expected ELO | Actual ELO | Status |
|-------|--------|-------|--------------|------------|--------|
| KS1   | -      | -     | 0            | -          | Pending |
| KS2   | -      | -     | 0 to -5      | -          | Not Started |
| KS3   | -      | -     | +20-30       | -          | Not Started |
| KS4   | -      | -     | +5-10        | -          | Not Started |

## Key Learnings
- (To be updated as implementation progresses)

## Implementation Details

### King Safety Zones
Following 4ku's approach:
- Reasonable king squares: 0xC3D7 bitmask (castled positions)
- Shield zones adjust based on king file position
- Two-tier shield evaluation (direct and advanced)

### Scoring Values
Initial values from 4ku:
- Direct shield pawns: S(33, -10) 
- Advanced shield pawns: S(25, -7)
- Values apply in midgame, taper in endgame

### Integration Points
- Called from main evaluation function
- Applied per side (white and black)
- Combined with existing positional evaluation

## Notes
- Feature aligns with SeaJay's phased development approach
- Each phase independently testable via OpenBench
- Infrastructure phases validate stability before main implementation