# Investigation Log – Illegal King Capture & Zobrist Mismatch

**Date:** 2025-09-16  
**Investigators:** Codex agent (GPT-5)  
**Branch:** `feature/20250913-phase3-fast-eval`

---

## 1. Executive Summary
- **Symptom A:** Debug builds aborted in `Board::validateStateIntegrity()` when running `go depth N` with `UseFastEvalForQsearch=true`, reporting `Invalid king count for BLACK: 0`.  
- **Symptom B:** After resolving king removal, sequential `go movetime 1000` searches (any toggle state) still trigger a `Zobrist key mismatch` at the start of the next search; the standalone `build/perft_tool` binary aborts for the same reason.
- **Root Cause (A):** Pseudo-legal capture generators included the opponent king in their target mask. Search was therefore permitted to “capture” the king; the subsequent board validation detected the missing monarch.
- **Status:** King-capture bug fixed; Zobrist mismatch remains under investigation.

---

## 2. Detailed Timeline
| Time (UTC) | Event |
|------------|-------|
| 23:05 | Reproduced crash by running `go depth 4` on the "KiwiPete" FEN with `UseFastEvalForQsearch=true`. Debug guard reported missing black king. |
| 23:07 | Observed same crash without fast-eval toggles and in Release `go depth 4`, indicating baseline move-gen issue. |
| 23:10 | Identified pseudo-legal move generators (pawn/knight/bishop/rook/queen/king captures) or'ed against `board.pieces(them)` without excluding the king. |
| 23:18 | Applied fix: mask off the king bit for each capture generator. |
| 23:25 | Rebuilt Debug/Release; reran depth-limited searches. Crash resolved for fixed-depth runs. |
| 23:27 | `go movetime 1000` still yielded Zobrist mismatch at start of subsequent search; observed same behaviour without fast-eval toggles. |
| 23:30 | `build/perft_tool` continued to abort (identical Zobrist mismatch). No further root-cause yet. |

---

## 3. Reproduction & Diagnostics
### 3.1 Original Failure (Resolved)
```bash
# Debug build
./build.sh Debug
printf "uci\nsetoption name UseFastEvalForQsearch value true\nposition fen r3k2r/pppq1ppp/2npbn2/3N4/2B1P3/2N5/PPPP1PPP/R1BQ1RK1 w kq - 0 1\ngo depth 4\nquit\n" | ./bin/seajay
```
Log excerpt before fix:
```
info string InCheckClassOrdering: checkers=1 ...
Zobrist key mismatch!
  Actual:        0x5b5964ac7a5bb4a9
  Reconstructed: 0xe50c9b3f99b04b98
Invalid board state before operation: makeMove
```
`BoardStateValidator::checkPieceCountLimits` confirmed the missing black king.

### 3.2 Current Outstanding Failure
```bash
# Either build
printf "uci\nsetoption name Hash value 32\ngo movetime 1000\ngo movetime 1000\nquit\n" | ./bin/seajay
```
Second search starts with:
```
Zobrist key mismatch! Actual ... Reconstructed ...
Invalid board state before operation: makeMove
```
Reproduced as well via `./build/perft_tool startpos 2` (process receives SIGSEGV / abort); core file unusable due to Rosetta sandbox but message is consistent with Zobrist divergence.

---

## 4. Root Cause Analysis (Symptom A)
- All pseudo-legal capture routines construct `theirPieces = board.pieces(them);`.  
- During search, `tryMakeMove` only ensures the moving side’s king is safe; it does not forbid capturing the *opposing* king.  
- Consequently, the first legal move generation that attacked the enemy king left the board in an illegal state (opponent king removed).  
- `Board::makeMove()` / `BoardStateValidator` caught the inconsistency on entry to the next move.

### Code Reference (Pre-fix)
```
Bitboard ourBishops = board.pieces(us, BISHOP);
Bitboard theirPieces = board.pieces(them);
Bitboard attacks = getBishopAttacks(...);
Bitboard captures = attacks & theirPieces; // includes king square
```

---

## 5. Implemented Fix
- **Change:** Mask the opponent king out of every capture target bitboard:  
  `theirPieces = board.pieces(them) & ~board.pieces(them, KING);`
- **Files:** `src/core/move_generation.cpp` (pawn, knight, bishop, rook, queen, king capture routines).  
- **Rationale:** Pseudo-legal move lists should never include king captures; the rules of chess forbid removing the opponent king, and search relies on that invariant.
- **Verification:**
  - Debug/Release depth searches on startpos, KiwiPete, and K+P vs K FEN now complete without assertions.
  - Fast-eval toggles no longer influence this behaviour.

---

## 6. Remaining Open Issue – Zobrist Mismatch After Timed Search
- **Symptom:** Consecutive `go movetime 1000` (or any timed search) triggers `BoardStateValidator::validateFullIntegrity` failure at the start of the *next* search.  
- **Scope:** Occurs in Debug & Release, with or without fast-eval toggles, implies incremental Zobrist update bug when the search terminates asynchronously (e.g., `SearchInfo::checkTime()` stop path).  
- **Impact:** Blocks perft tool (which relies on repeated searches) and undermines confidence in timed search correctness.
- **Initial Hypotheses:**
  1. `Board::unmakeMove` paths during forced stop fail to restore mailbox/bitboards in some late exit path.  
  2. Search termination via `negamax` returns while stack contains partially-made moves (e.g., due to panic stop), skipping some `unmakeMove`.  
  3. `SearchInfo::pushSearchPosition` / history tracking updates cause mismatched `m_zobristKey` vs `ZobristKeyManager::computeKey` when search aborts mid-node.
- **Artifacts:** No usable core (Rosetta sandbox). Need targeted instrumentation (e.g., logging first diverging node, verifying stack unwinding on stop, ensuring `Board::makeMove` guard snapshots remain balanced).

---

## 7. Next Actions
1. **Instrument Search Stop Path:** Add DEBUG-only logging around `negamax` exit when `data.stopped == true` to confirm every made move is unmade.  
2. **Add Zobrist Consistency Assertions:** Temporarily enable `Board::validateZobristDebug()` after each `unmakeMove` during stop conditions to isolate first mismatch.  
3. **Reproduce in Minimal Context:** Create a dedicated regression harness (maybe under `tests/`) that runs `go movetime 50` twice on a simple FEN, recording board hashes between runs.  
4. **Document Findings:** Append future results to this log, and once resolved, update `docs/project_docs/known_bugs.md` and this investigation record.

---

## 8. References
- `src/core/move_generation.cpp` – capture generator fix.  
- `src/core/board.cpp` & `board_safety.cpp` – state validation and Zobrist helpers.  
- `docs/project_docs/Phase3D_Pawn_Cache.md` – context for Phase 3D fast-eval work (unrelated but concurrent).

---

*End of log.*

## 9. Update – 2025-09-17
- **Null-move zobrist bug:** `Board::makeNullMove` was clearing the en passant component using the file index instead of the full square index, leaving the incremental key out of sync once a null move occurred with an en-passant square present. The function now XORs `s_zobristEnPassant` with the square identifier, matching `makeMoveInternal()` and `ZobristKeyManager::computeKey` semantics.
- **Perft crash during follow-up testing:** `perft_tool` invoked move generation before the magic-bitboard tables were initialised, so the first `isSquareAttacked` dereference hit a null attacks pointer. The tool now includes `magic_bitboards.h` and calls `seajay::magic_v2::ensureMagicsInitialized()` at startup.
- **Verification:**
  - Reproduced the original `go movetime` double-run scenario on the new binaries (Release & Debug); no assertions or mismatched keys observed.
  - `perft_tool startpos 1/2` and `--suite --max-depth 3` complete without crashes.
