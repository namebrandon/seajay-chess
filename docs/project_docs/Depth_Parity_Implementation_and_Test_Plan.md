Title: SeaJay Depth Parity – Implementation and Test Plan

IMPORTANT: Do not modify the plan text above the “Progress Log” section. All changes, notes, results, and decisions should be appended as dated entries to the Progress Log at the bottom of this document.

Objective
- Achieve comparable (or better) depth vs time to peer engines (Stockfish, Stash, Laser, Komodo) at the same time controls on the same hardware, without regressing tactical strength.

Scope
- Focused performance improvements that primarily reduce explored nodes and per-node overhead in search. No functional rule changes to chess logic. Build flags remain unchanged.

Non‑Goals
- No new evaluation features that increase compute cost.
- No multi-threading or SMP changes.
- No UCI protocol changes beyond diagnostic toggles.

Success Metrics (acceptance criteria)
- Depth parity: At 1s/move and 3s/move across a 10–20 FEN suite, SeaJay’s median depth within 0.5 of Stash and Laser; within 1 of Stockfish.
- Node efficiency: ≥20% fewer nodes vs current main on the same suite and time limit.
- Search quality: Tactical success rate unchanged or improved (≥ baseline 80–85%).
- OpenBench: Non-regression at 10+0.1 and 15+0.15; ideally +Elo from better pruning.

High‑Level Plan (Phased)
1) Transposition Table (TT) clustering + replacement policy
2) Staged MovePicker (TT → SEE winning caps → killers → history/countermove → rest)
3) Fast evaluation path for qsearch and static pruning decisions
4) Selective search micro-tuning (NMP/LMR gating, minimal behavior changes)
5) Lightweight attack caching in legality checks (isSquareAttacked/tryMakeMove)

Risk & Mitigations
- TT design risk: Wrong replacement increases pollution → add A/B toggles, collect hit/collision stats, and test TT correctness on unit tests.
- Move ordering risk: Over-aggressive SEE filters can miss tactics → keep SEE only for captures, measure first‑move cutoff rate, track re‑search rate.
- Fast eval risk: Bias in qsearch stand‑pat → keep material+PST+pawns only, gate via UCI option for quick rollback.
- Caching risk: Stale attack cache entries → key on (zobrist, square, color), invalidate on any make/unmake by embedding zobrist in cache key.

Implementation Roadmap

Phase 1: Transposition Table Clustering (4‑way) + Replacement
- Design
  - Cluster size: 4 entries per index (64B, cache-line sized); still 16B per entry.
  - Probe: index(key) → scan 4 entries; hit on key32 match.
  - Replacement policy, in order: empty → older generation → lower depth → non-EXACT bound → NO_MOVE entries → oldest.
  - Generation aging unchanged; keep hashfull and stats.
- Code hooks
  - Implement clustered storage behind current API (no caller changes).
  - Preserve `prefetch` to first line; optionally prefetch subsequent lines.
  - Keep TT_EVAL_NONE behavior; continue storing static evals.
- Instrumentation
  - Track: probes, hits, hashfull, per-cluster scan length, replacement causes.
- Validation
  - Unit: tests/unit/test_transposition_table.cpp (expand for clusters: hit, miss, replacement). Perft unaffected.
  - Functional: existing unit and perft tests, plus `uci debug tt` (already exists).
- A/B toggles
  - UCI option: UseClusteredTT (default: off → on via branch).
- Success
  - +10–25% TT hit rate; earlier cutoffs; node reduction ≥10% on depth suite.

Phase 2: Staged MovePicker
- Design
  - Yield moves in phases without sorting: [TT] → [winning captures SEE≥0] → [killers] → [history/countermove quiets] → [remaining captures] → [remaining quiets].
  - No full stable_sort; use small local buffers or single-pass selection per phase.
  - Lower gating: enable quiet heuristics from depth ≥2 (currently depth ≥6).
- Code hooks
  - New MovePicker class (search/move_picker.*) used by negamax and qsearch.
  - Reuse existing HistoryHeuristic, Killers, CounterMoves, SEE.
- Telemetry
  - First-move cutoff rate, cutoffsByPosition, PVS re‑search rate (already tracked), TTMove found/first.
- Validation
  - Unit: ensure MovePicker yields only legal (post tryMakeMove) and matches generator set.
  - Functional: tactical_test.py must remain ≥ baseline pass rate.
- Success
  - Increase first‑move cutoff to 85–90% at typical depths; node reduction 5–15%.

Phase 3: Fast Evaluation Path for qsearch / pruning decisions
- Design
  - fast_eval = material (phase‑interpolated) + PST (interpolated) + cached pawn structure; omit mobility/king safety/outposts.
  - Use fast_eval for: qsearch stand‑pat, static null‑move margin check, futility/razoring prechecks, delta pruning comparisons.
- Code hooks
  - evaluation/fast_evaluate.{h,cpp} with shared code paths to reduce duplication.
  - Gate with UCI: UseFastEvalForQsearch (on/off) for quick rollback.
- Telemetry
  - Time spent in eval vs search, qsearch node count, prunes taken.
- Validation
  - Unit: equivalence to full eval in pure material/PST cases; bounds monotonicity for pruning checks.
  - Functional: qsearch test scripts and node explosion diagnostics; ensure no tactical regressions.
- Success
  - 5–20% time reduction in qsearch-heavy positions; overall depth improves ~0.2–0.5 at 1s/move.

Phase 4: Selective Search Micro‑tuning (Low Risk)
- Changes
  - Reduce quiet-heuristics gating to depth ≥2 for history/countermoves.
  - NMP: Maintain current tuned margins; add one more reduction step at depth ≥10 when eval−beta is large (guarded by TT bound context).
  - LMR: Keep table; compute “improving” from stack evals already tracked; avoid extra work in hot path.
- Telemetry
  - Null‑move stats (already present), LMR success rate, re‑search rate.
- Validation
  - Tactical suite stable; node explosion report shows improved early cutoffs and fewer late cutoffs.

Phase 5: Lightweight Attack Cache
- Design
  - Tiny per-thread cache keyed by (zobrist, square, color) → attacked bool.
  - Direct-mapped or 4‑entry set; 1–2KB per thread.
  - Use only inside isSquareAttacked() and tryMakeMove() checks.
- Telemetry
  - Cache hit/miss; legality-check time in aggregate (if measurable).
- Validation
  - All unit tests and perft match; no functional changes.
- Success
  - 5–10% overhead reduction in legality-heavy positions.

Testing & Measurement Plan
- Local micro-benchmarks
  - Built-in benchmark: `echo "bench" | ./bin/seajay` (sanity NPS, unchanged semantics).
  - Perft correctness: `./bin/perft_tool` or build target for tests/perft/*.
  - Unit tests: run all tests in tests/unit/*.

- Depth vs Time (new tool)
  - Script: `./tools/depth_vs_time.py --time-ms 1000 --engine ./bin/seajay --engine ./external/engines/stash-bot/stash --fen startpos --fen "<FEN>" --out depth_vs_time.csv`
  - Metric: max depth per FEN and nodes at max depth; compare engines side-by-side.

- Tactical Suite
  - `./tools/tactical_test.py ./bin/seajay external/UHO_4060_v2.epd 2000` (or your curated suite), baseline ≥80–85%.

- Node Explosion Diagnostics
  - `./tools/node_explosion_diagnostic.sh` to compare node ratios vs external engines and track cutoff distributions.

- TT Diagnostics (UCI)
  - `uci` → `debug tt` (already available) to view probes/hits/hashfull; expect improved hit rate with clusters.

- OpenBench A/B
  - Branching per Git_Strategy; run SPRT at 10+0.1s T=1 Hash=128MB (ensure consistent Hash across engines).
  - Targets: PASS vs main with ≥0 Elo regression allowed (performance is the goal), prefer positive Elo.

Rollout & Branch Strategy
- Phase branches
  - feature/DEPTHP-tt-cluster
  - feature/DEPTHP-movepicker
  - feature/DEPTHP-fast-eval
  - feature/DEPTHP-selective-tuning
  - feature/DEPTHP-attack-cache
- Each phase merges only after:
  - Unit/perft tests pass
  - Depth vs time shows improvement on local suite
  - Node explosion diagnostics improved or neutral
  - OpenBench non-regression (or positive) at 10+0.1

Data & Telemetry to Capture (for each A/B)
- depth_vs_time.csv runs (archive per date)
- tactical_test_*.csv and failures csv
- node_explosion_report_*.txt
- TT stats snapshot (hit rate, collisions, hashfull)

Rollback Plan
- Each feature behind a UCI flag for fast compare.
- Keep previous binaries (bin/seajay.prev) for quick A/B.
- If TT clusters underperform in some TC, revert to single-entry via UCI and revisit replacement thresholds.

Ownership & Timeline (suggested)
- Week 1: TT clustering + tests + local A/B → OpenBench (short run)
- Week 2: MovePicker + tests + A/B → OpenBench
- Week 3: Fast eval + tests + A/B → OpenBench
- Week 4: Selective tuning + attack cache + consolidation run

Acceptance Review
- Sign-off when success metrics are met across 1s and 3s per move suites and OpenBench shows non-regression or gains.

How to Contribute to This Plan
- Do not edit the plan text above. Append your dated notes, measurements, and decisions under “Progress Log”.

Progress Log (append-only)
- 2025-09-07: Initialized plan. Added tools/depth_vs_time.py to repository for depth vs time measurement.
- 2025-09-07: Added scaffolds for review (not compiled):
  - MovePicker design: docs/project_docs/scaffolds/MovePicker_Design.md
  - TT Cluster design: docs/project_docs/scaffolds/TT_Cluster_Design.md
  - Staged MovePicker interface (scaffold): src/experimental/move_picker/MovePicker.hpp
  - Clustered TT interface (scaffold): src/experimental/tt/ClusteredTT.hpp
  These are thread-safe by construction (no globals), and designed for LazySMP workers with per-thread history/killers/countermoves; TT supports benign lock-free races with generation-based aging.
- 2025-09-07: Added integration checklists and edge-case notes for implementation readiness:
  - MovePicker Integration Checklist: docs/project_docs/scaffolds/MovePicker_Integration_Checklist.md
  - Clustered TT Backend Swap Checklist: docs/project_docs/scaffolds/TT_Backend_Swap_Checklist.md
  - Depth Parity Edge Cases & Gotchas: docs/project_docs/scaffolds/Depth_Parity_Edge_Cases.md
  These documents should be reviewed prior to coding and used as acceptance gates in A/B runs. Refer to them during Phase 2 (MovePicker) and Phase 1 (TT clustering) in this plan.
- 2025-09-07: Added UCI scaffold toggles (no behavior change yet) for upcoming A/B:
  - UseClusteredTT (prints/accepts setoption; does not switch backend yet)
  - UseStagedMovePicker (prints/accepts setoption; does not change ordering yet)
  Files: src/uci/uci.h, src/uci/uci.cpp
  Implementers: wire these toggles during Phase 1/2 integration; keep default false.
