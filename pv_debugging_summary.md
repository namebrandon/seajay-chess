# PV (Principal Variation) Display Issue - Debugging Summary

## Problem Statement
SeaJay is only displaying the first move of the principal variation (e.g., `pv b1c3`) instead of the full line like other engines such as Komodo (e.g., `pv e2e4 e7e6 g1f3 f8e7`). This makes it difficult to understand and debug the engine's evaluation and search decisions.

## Why This Matters
- **Debugging**: Cannot see what continuation the engine expects, making it hard to understand evaluation issues
- **Analysis**: Users/developers cannot see the full planned line of play
- **GUI Integration**: Chess GUIs expect to display the full PV for analysis
- **Development**: Hard to debug search and evaluation without seeing what the engine is "thinking"

## What Has Been Implemented

### Phase 1: Infrastructure (✅ Complete)
- Created `TriangularPV` class in `/workspace/src/search/principal_variation.h`
- Cache-aligned triangular array structure for efficient PV storage
- ~17KB memory per thread
- Added PV parameter to negamax function signature
- **Result**: No functional change, infrastructure ready

### Phase 2: Root PV Collection (✅ Complete)
- Pass `rootPV` to negamax at root position
- Update PV when new best move found at root (ply == 0)
- **Result**: Collecting PV at root, but only single move

### Phase 3: Full Tree PV Tracking (✅ Complete)
- Create child PV for all recursive calls in PV nodes
- Pass child PV through search tree
- Update parent PV with child PV when finding better move
- **Result**: Full infrastructure working, 1-2% performance overhead as expected

### Phase 4: UCI Display (✅ Complete with bug)
- Modified `sendIterationInfo()` to accept and display full PV
- Extract PV moves from TriangularPV array
- Validate moves before output
- **Result**: Display infrastructure complete, but only showing single move

## The Current Bug

### Symptom
Only the first move is displayed in the PV, not the full continuation.

### Root Cause Analysis
The issue appears to be in the PVS (Principal Variation Search) logic:

1. **PVS Search Pattern**:
   - First move: Full window search (should populate PV)
   - Other moves: Scout search with zero window (no PV)
   - If scout fails high: Re-search with full window (should populate PV)

2. **The Problem**:
   - `childPV` is created fresh for each move in the loop
   - Scout searches don't populate the childPV (they pass nullptr)
   - When re-search happens, it uses the same childPV variable
   - The childPV might not be properly preserved between different search phases

3. **Attempted Fix**:
   ```cpp
   // Created separate pointers for different search phases
   TriangularPV* firstMoveChildPV = (pv != nullptr && isPvNode) ? &childPV : nullptr;
   TriangularPV* reSearchChildPV = (pv != nullptr && isPvNode) ? &childPV : nullptr;
   ```
   - This ensures childPV is only passed when needed
   - But the PV is still not propagating correctly

## Why This Doesn't Affect Playing Strength

The PV is **only** used for:
1. **Display**: Showing moves to GUI/user
2. **Pondering**: Thinking on opponent's time (not implemented in SeaJay)
3. **Debugging**: Understanding engine's plan

The actual move selection uses:
- Alpha-beta search for move scoring
- Transposition table for move ordering
- Best move tracking independent of PV

**Proof**: All SPRT tests showed neutral results (no ELO change)

## Possible Solutions to Investigate

### 1. Check PV Clear/Initialize
- Ensure childPV is properly initialized before use
- Check if PV needs to be cleared at node entry

### 2. Debug PV Update Logic
- Add detailed logging to track when PV is updated
- Verify childPV has data before updating parent

### 3. Review PVS Re-search
- Ensure re-search properly populates childPV
- Check if childPV survives between scout and re-search

### 4. Alternative Approach: Explicit PV Tracking
- Instead of relying on childPV in move loop
- Explicitly track best PV separately
- Update only when finding new best move

## Code Locations

Key files to examine:
- `/workspace/src/search/negamax.cpp` - Lines 704-814 (move loop and PV update)
- `/workspace/src/search/principal_variation.h` - PV storage implementation
- `/workspace/src/search/negamax.cpp` - Lines 1668-1701 (PV display)

## Testing Approach

To verify when fixed:
```bash
echo -e "position startpos\ngo depth 8\nquit" | ./bin/seajay
```

Should see output like:
```
info depth 8 ... pv e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6
```

Instead of current:
```
info depth 8 ... pv e2e4
```

## Next Steps

1. **Add comprehensive debug logging** to understand exact PV propagation
2. **Test with simpler positions** (e.g., mate in 2) where PV is obvious
3. **Compare with reference implementation** (e.g., Stockfish's triangular PV)
4. **Consider alternative PV storage** if current approach proves problematic

## Impact on Development

Until fixed, debugging evaluation and search issues is harder because:
- Cannot see what continuation the engine expects
- Hard to verify if search is finding reasonable lines
- Cannot compare expected vs actual play in analysis

This is a high-priority issue for development productivity, even though it doesn't affect playing strength.