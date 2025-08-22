# PP3 Minimal Implementation Plan

Since PP2 gives us +58.50 ELO, and PP3 is losing all of it, we need to add features one at a time to identify the problem.

## Phase PP3a: Add ONLY protected passer bonus
- Start from PP2 code
- Add only: if pawn is protected by another pawn, +20% bonus
- Test vs main

## Phase PP3b: Add ONLY connected passer bonus  
- Start from PP3a
- Add only: if pawn has adjacent passed pawn, +30% bonus
- Test vs main

## Phase PP3c: Add ONLY blockader evaluation
- Start from PP3b
- Add only: penalty for pieces blocking passed pawns
- Test vs main

## Phase PP3d: Add ONLY rook behind passed pawn
- Start from PP3c
- Add only: +15 bonus for rook behind passer
- Test vs main

## Phase PP3e: Add ONLY king proximity in endgame
- Start from PP3d
- Add only: king distance bonuses in endgame only
- Test vs main

## Phase PP3f: Add unstoppable passer
- Start from PP3e
- Add only: unstoppable passer detection
- Test vs main

This incremental approach will identify which feature is causing the regression.