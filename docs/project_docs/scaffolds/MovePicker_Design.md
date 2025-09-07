Title: Staged MovePicker – Design Scaffold (Thread‑Safe, LazySMP‑Ready)

Overview
- Replace full list sort/partition with a staged MovePicker that yields moves on demand in efficient phases:
  1) TT move (if legal)
  2) Winning captures (SEE ≥ 0)
  3) Killer moves (2 slots)
  4) History/countermove‑scored quiets
  5) Remaining captures (SEE < 0)
  6) Remaining quiets

Goals
- Reduce per-node overhead and improve first‑move cutoff rate.
- Maintain legality via tryMakeMove at call site; MovePicker does not make/unmake.
- Avoid allocations; cache-friendly iteration; no global state.

Thread‑Safety & LazySMP
- Stateless apart from per‑instance buffers; no globals; safe to instantiate per thread.
- Uses only read‑only references to search data (history, killers, countermoves) which must be thread‑local or have thread‑safe APIs. For LazySMP, each worker keeps its own history/killers/countermoves by convention.
- No synchronization inside MovePicker. Callers decide on per-thread objects.

Data Sources
- TT move (optional): provided by caller post‑TT probe.
- SEE for captures (board snapshot at node): read-only.
- KillerMoves/HistoryHeuristic/CounterMoves: thread‑local inputs.

API Sketch
- new files (scaffold only; not compiled yet):
  - src/experimental/move_picker/MovePicker.hpp

Key Implementation Notes
- Use small fixed-size arrays as phase buckets; push indices only.
- Avoid std::sort on the whole list. For captures, scan once to fill two buckets: winning (SEE ≥ 0) and losing (SEE < 0). For quiets, keep a simple top‑K selection by history/countermove bonus or iterate in natural order if K is small.
- TT move de-duplication: if TT move appears in captures/quiets, skip duplicates via a small bloom/bitset of seen indices.
- Provide a reset() to reuse the picker for re-search (PVS re-search) without rebuilding all buckets when possible.

Correctness
- No change in legal move set; only iteration order.
- Quiescence integration: a lightweight mode to emit only captures/promotions and optionally checks.

Metrics to Track (optional)
- First‑move cutoff rate; index of cutoff; fraction of TT move found/first.

Integration Plan
- Implement MovePicker.hpp and wire into negamax after TT probe and before loop. Keep a UCI option to fall back to legacy ordering during A/B.

