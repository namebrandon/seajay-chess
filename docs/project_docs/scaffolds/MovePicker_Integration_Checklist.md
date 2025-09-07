Title: Staged MovePicker – Integration Checklist (Review Before Coding)

Purpose
Ensure a safe, incremental integration of the staged MovePicker into negamax and quiescence without regressions, with clear rollback and A/B testing.

Preconditions
- Baseline tests (unit, perft, tactical suite) passing on current main.
- Depth vs time baseline collected with tools/depth_vs_time.py for a 10–20 FEN suite at 1s and 3s/move.

Design & API
- Confirm final interface in src/experimental/move_picker/MovePicker.hpp covers both search and quiescence modes.
- Decide inputs ownership: Board as const& for SEE, thread‑local History/Killers/CounterMoves.
- Confirm SEE availability and noexcept behavior for capture scoring.

Integration Steps (Search)
1) After TT probe in negamax, build MovePicker with:
   - board (const), ttMove, depth, ply, inCheck, isPvNode, history, killers, countermoves.
2) Replace full move generation + legacy order with staged iteration:
   - Generate pseudo-legal moves first (or reuse generator internally if desired later).
   - For each move from MovePicker.next():
     - tryMakeMove() for legality; continue if false.
     - Maintain existing pruning (futility, NMP guards, LMR checks) semantics.
3) Preserve PV logic: first legal child at PV parent is PV; re‑search on fail‑high unchanged.
4) Maintain diagnostics: moveCount, legalMoveCount, cutoff position stats, TT move effectiveness.

Integration Steps (Quiescence)
1) Use MovePicker in capture‑only mode (promos included) when not in check.
2) Preserve current qsearch ordering tweaks (queen promos front, discovered checks) or re‑express them via a pre‑phase bucket.

Gating & A/B
- UCI: UseStagedMovePicker (default off) to switch between legacy and staged ordering per run.
- For PV nodes, optionally a separate toggle UseStagedMovePickerPV to isolate impact.

Thread‑Safety
- Ensure MovePicker has no shared/global state. All tables (history/killers/countermoves) are per‑thread and passed by const pointer/reference.
- Avoid static locals in hot paths; prefer inline helpers or per‑instance state.

Performance Considerations
- Avoid std::stable_sort on full lists; single pass to partition captures SEE>=0 vs SEE<0.
- For quiets, simple scoring and linear scan suffice; consider top‑K for early phases to reduce memory traffic.
- Keep branch predictability by fixed phase order.

Failure Modes & Rollback
- If tactical pass rate < baseline, toggle off via UCI and capture offending positions.
- If depth vs time worsens, revert and inspect SEE thresholds and quiet gating (min depth ≥ 2 recommended).

Validation
- Unit: generator equivalence (set equality) between legacy and staged order sources.
- Functional: full tactical_test.py; node_explosion_diagnostic.sh; depth_vs_time.py A/B.
- Telemetry: first‑move cutoff % up; PVS re‑search rate same or lower; TT move first % up.

