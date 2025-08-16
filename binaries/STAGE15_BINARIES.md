# Stage 15 Binary Inventory

## Production Binaries

### 1. Stage 15 Original (SPRT Candidate 1)
- **File**: `seajay-stage15-original`
- **MD5**: `fcf0edacd7cec502cbfd982e69d201be`
- **Size**: 474,984 bytes
- **Commit**: 4418fb5 (before PST "fixes")
- **Notes**: Original Stage 15 with SEE implementation, no PST bugs

### 2. Stage 15 Fixed (Current)
- **File**: `seajay-stage15-fixed`
- **MD5**: `e2e2ea6b41eab6c353cc116539f216ed`
- **Size**: 474,984 bytes
- **Date**: 2025-08-15 21:49
- **Notes**: Fixed version with PST double-negation bug resolved
- **Changes**: Removed negation from PST::value() to fix 290 cp evaluation error

### 3. Stage 14 Final (Baseline)
- **File**: `seajay-stage14-final`
- **MD5**: `1c65de0c2e95cb371e0d637368f4d60d`
- **Size**: 411,496 bytes
- **Notes**: Baseline for comparison, last known good version with quiescence search

## Quarantined Binaries

All other Stage 15 binaries have been moved to `/workspace/binaries/stage15_quarantine/` including:
- Various PST bugfix attempts
- Margin tuning variants
- UCI bugfix versions
- Tuning experiments

These should NOT be used for testing as they contain various bugs.

## Bug History

1. **aa269a9**: Fixed PST sign handling in board.cpp makeMove (legitimate fix)
2. **d75ee06**: Added negation to PST::value() (introduced 290 cp bug)
3. **Current fix**: Reverted d75ee06 negation while keeping aa269a9 fixes

## Validation Status

✅ **Stage 15 Fixed** validated to:
- Eliminate 290 cp evaluation error
- Provide reasonable evaluations
- Include SEE enhancements as intended

## SPRT Testing Plan

Test: Stage 15 Fixed vs Stage 14 Final
- Expected: +30-40 Elo improvement from SEE
- Time Control: 10+0.1
- Opening Book: 4moves_test.pgn