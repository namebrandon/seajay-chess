# Bug Report: "Ghost King Replay" Illegal Moves in K+P vs K Endgames

## Overview
During late Phase 3 / early Phase 4 optimizations (lazy legality in search), the engine began occasionally attempting illegal king moves in simplified endgames. The pattern is a stale king move being “replayed” from the king’s previous square to its current square. This bypasses proper legality checks and can surface as an illegal UCI move in self-play harnesses.

## Symptom (What Happens)
- Illegal move pattern: The engine attempts a king move from an empty from-square to the king’s current square (a stale replay), e.g.:
  - After Kg8, engine attempts g8g7 (king already on g7).
  - After Kf5, engine attempts e5f5 (king already on f5).
  - After Kf3–g3, engine attempts g3f3 (king already on f3).
  - After Kg1–h1, engine attempts h1g1 (king already on g1).
  - In the K+P vs K laddering sequence, “g5g6” appears after White has just moved Kg6 (g5 is empty).

All examples are consistent with a stale-from-square move object being accepted as “legal.”

## Minimal Repro Position (provided)
- FEN after Black plays 57...Kg8 (White to move, move 58):

  6k1/8/6KP/8/8/8/8/8 w - - 9 58

- Expected illegal attempt in faulty builds: White tries “g5g6” (stale prior king square → current king square), which is illegal because g5 is empty.

## Root Cause Analysis
- The lazy-legality path (Phase 3.2) uses `Board::tryMakeMove` as the single legality gate during search. It currently:
  1) Applies the move with `makeMoveInternal` unconditionally, then
  2) Rejects only if our king is in check afterward.

- Missing preflight checks in `tryMakeMove`:
  - Does not verify `from` contains a piece.
  - Does not verify piece color matches `sideToMove`.
  - Does not verify the move is even pseudo-legal for that piece type (e.g., king adjacency, etc.).

- Consequences in code:
  - `movingPiece = pieceAt(from)` can be `NO_PIECE` when from-square is empty.
  - With `NO_PIECE = 12`, `typeOf(NO_PIECE) = 12 % 6 = PAWN` and `colorOf(NO_PIECE) = 12 / 6 = 2` (invalid `Color`).
  - `makeMoveInternal` proceeds with inconsistent state assumptions (undefined behavior risk), often leaving the board effectively unchanged but still flipping `sideToMove` and passing the post-check test.
  - The search then treats the stale move as legal and may output it at root.

## Code References
- `src/core/board.cpp`
  - `bool Board::tryMakeMove(Move move, UndoInfo& undo)` (around lines 1186–1212): lacks from-square occupancy/ownership checks before calling `makeMoveInternal`.
  - `void Board::makeMoveInternal(Move move, UndoInfo& undo)`: assumes a valid `movingPiece` from `pieceAt(from)`.
- `src/core/types.h`
  - `typeOf(Piece)` and `colorOf(Piece)` are arithmetic; `NO_PIECE` flows into them if not guarded.
- `src/core/move_generation.cpp`
  - Legal move generation and check evasion are correct when used, but search’s lazy path bypasses pre-filtering.

## Why the Removed “Redundant” Check Hid This
Before Phase 3.2, we validated moves by generating legal moves (or at least checking pseudo-legal) before applying them in search. That filtered out stale moves. After switching to lazy legality, `tryMakeMove` became the sole guard but does not validate basic invariants, so stale/corrupted moves can slip through.

## Impact
- Functional correctness: Illegal moves in self-play logs and harnesses; potential UB due to invalid `Color` usage when `movingPiece == NO_PIECE`.
- Search integrity: Tree can include positions reached via invalid transitions.

## Proposed Fix Options
1) Minimal, fast preflight in `tryMakeMove` (recommended initial patch):
   - Read `movingPiece = pieceAt(from)`.
   - If `movingPiece == NO_PIECE`, return false (no state change).
   - If `colorOf(movingPiece) != m_sideToMove`, return false.
   - If `typeOf(movingPiece) == KING`:
     - Require `to` in `getKingAttacks(from)` and `board.pieces(m_sideToMove)` not containing `to`.
   - This removes the observed illegal “replay” cases with near-zero overhead, preserving Phase 3.2 NPS gains.

2) Broader pseudo-legal precheck inside `tryMakeMove` (optional, still cheap):
   - Per-piece quick checks (pawn push/capture/EP structure, knight table, slider attacks with occupancy, promotions/castling flags consistency).
   - Guarantees correctness even if other stale moves (non-king) appear later.

3) Debug-only assertions/logging:
   - In debug builds, assert that `movingPiece != NO_PIECE` and `colorOf(movingPiece) == m_sideToMove`.
   - Log from/ to / flags for failed preflight to catch regressions.

## Validation Plan
- Unit/position test using provided FEN:
  - `position fen 6k1/8/6KP/8/8/8/8/8 w - - 9 58` then `go depth 4`.
  - Confirm engine never outputs an illegal bestmove, and internal search does not accept a move with `from` empty.
- PGN regression: Re-run the PGN suite in `external/2025_08_31_illegal_moves.pgn` and verify no “makes an illegal move:” markers appear.
- Perft/bench: Ensure `bench` still returns 19191913 nodes, NPS within noise of current baseline.
- Optional: Add a targeted test harness to inject a stale move at root and confirm `tryMakeMove` rejects it.

## Risk and Performance Considerations
- The minimal preflight adds a handful of instructions per attempted move and does not reintroduce the previous double make/unmake overhead.
- King-specific adjacency and own-occupancy checks are O(1) with precomputed tables.
- Full pseudo-legal checks would add slightly more overhead but remain far cheaper than pre-Phase 3.2 logic.

## Next Steps
1) Decide scope: minimal preflight vs broader pseudo-legal checks.
2) Implement in `Board::tryMakeMove` with DEBUG asserts.
3) Validate with the FEN and PGN replay; run perft and bench.
4) If broader checks are deferred, schedule a follow-up to extend preflight to all piece types.

