# SPRT Test Results - Stage 9 FIXED: Piece-Square Tables

**Test ID:** SPRT-2025-007-FIXED
**Date:** 2025-08-10
**Status:** IN PROGRESS

## Critical Bug Fix
The original Stage 9 had PST tables defined but NEVER integrated:
- No PST tracking in Board class
- No PST in evaluation function  
- 100% loss rate in original test

This is the FIXED version with proper PST integration.

## Engines Tested
- **Test Engine:** Stage 9 FIXED with PST (material + positional evaluation)
- **Base Engine:** Stage 8 with alpha-beta (material-only evaluation)

## Test Parameters
- **Elo bounds:** [0, 50]
- **Significance:** α = 0.05, β = 0.05
- **Time control:** 10+0.1 seconds
- **Opening book:** 4moves_test.pgn

## Expected Outcome
PST typically provides +150-200 Elo improvement by adding:
- Piece centralization bonuses
- Pawn advancement rewards
- King safety positioning
- Rook on 7th rank bonus

## Results
(Will be updated when test completes)
