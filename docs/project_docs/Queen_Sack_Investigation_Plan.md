# Queen-Sack Tactical Remediation Plan (2025-10-09)

## Purpose
- Arrest the recurring queen/slider capture-check misses highlighted in tactical telemetry while preserving overall strength and speed.
- Provide a staged roadmap with single-responsibility deliverables so search and evaluation adjustments can be validated independently before landing on OpenBench.

## Current Evidence
- Tactical audit shows queen contact sacrifices consistently replaced by quiet moves (e.g., `Qxh7+` → `b3b4`) across the 54-position failure set (`docs/project_docs/investigation_20250918_tactical_failures.md:8-34`).
- Focused telemetry on WAC.049 confirms `Qxh7+` is generated but remains ≈−500 cp until depth ≥9, so history/SEE pressure buries it after the first fail-low (`docs/issues/WAC.049_Tactical_Investigation.md:5-87`).
- Evaluation bias tracker still reports +150–300 cp optimism whenever the queen is boxed behind friendly pieces or sliders lack pressure penalties (`docs/issues/eval_bias_tracker.md:11-44`).
- Selectivity probes only add two Komodo move matches when pruning guards are relaxed, confirming that evaluation and move prioritisation—not legality—drive the failure (`docs/project_docs/telemetry/eval_bias/selectivity_probe_results.md:1-35`).
- 2025-10-06 update: forcing queen contact capture-checks now bypass LMR/LMP/move-count pruning; the 200 ms queen-sack sweep improved from 2/20 to 4/20 solved, indicating search coverage gains with evaluation work still pending (`docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-06_19-38-59.csv`).

## Assets

### Queen-Sack Tactical Suite
- Canonical 20-position EPD for contact-check sacrifices and queen-led forcing lines lives at `tests/positions/queen_sack_suite.epd`.
- Source FENs and current engine responses remain traceable through the shadow failure CSV (`docs/project_docs/telemetry/phase4/tactical_suite_phase4_2_shadow_failures.csv:4-61`).

| ID | FEN | Expected | Current Engine |
| --- | --- | --- | --- |
| QS.WAC014 | `r2rb1k1/pp1q1p1p/2n1p1p1/2bp4/5P2/PP1BPR1Q/1BPN2PP/R5K1 w - - 0 1` | Qxh7+ | b3b4 |
| QS.WAC049 | `2b3k1/4rrpp/p2p4/2pP2RQ/1pP1Pp1N/1P3P1P/1q6/6RK w - - 0 1` | Qxh7+ | h5h6 |
| QS.WAC055 | `r3r1k1/pp1q1pp1/4b1p1/3p2B1/3Q1R2/8/PPP3PP/4R1K1 w - - 0 1` | Qxg7+ | f4f1 |
| QS.WAC074 | `5r1k/pp4pp/2p5/2b1P3/4Pq2/1PB1p3/P3Q1PP/3N2K1 b - - 0 1` | Qf1+ | f8d8 |
| QS.WAC097 | `6k1/5p2/p5np/4B3/3P4/1PP1q3/P3r1QP/6RK w - - 0 1` | Qa8+ | e5f4 |
| QS.WAC141 | `4r1k1/p1qr1p2/2pb1Bp1/1p5p/3P1n1R/1B3P2/PP3PK1/2Q4R w - - 0 1` | Qxf4 | g2f1 |
| QS.WAC161 | `3r3k/3r1P1p/pp1Nn3/2pp4/7Q/6R1/Pq4PP/5RK1 w - - 0 1` | Qxd8+ | h4h6 |
| QS.WAC163 | `5rk1/2p4p/2p4r/3P4/4p1b1/1Q2NqPp/PP3P1K/R4R2 b - - 0 1` | Qg2+ | c6d5 |
| QS.WAC184 | `4kn2/r4p1r/p3bQ2/q1nNP1Np/1p5P/8/PPP3P1/2KR3R w - - 0 1` | Qe7+ | g5h7 |
| QS.WAC185 | `1r1rb1k1/2p3pp/p2q1p2/3PpP1Q/Pp1bP2N/1B5R/1P4PP/2B4K w - - 0 1` | Qxh7+ | h4g6 |
| QS.WAC193 | `5bk1/p4ppp/Qp6/4B3/1P6/Pq2P1P1/2rr1P1P/R4RK1 b - - 0 1` | Qxe3 | b3d5 |
| QS.WAC207 | `r1bq2kr/p1pp1ppp/1pn1p3/4P3/2Pb2Q1/BR6/P4PPP/3K1BNR w - - 0 1` | Qxg7+ | g1f3 |
| QS.WAC212 | `rn1qr2Q/pbppk1p1/1p2pb2/4N3/3P4/2N5/PPP3PP/R4RK1 w - - 0 1` | Qxg7+ | h8h5 |
| QS.WAC217 | `r3kb1r/1pp3p1/p3bp1p/5q2/3QN3/1P6/PBP3P1/3RR1K1 w kq - 0 1` | Qd7+ | e4g3 |
| QS.WAC220 | `3rr1k1/ppp2ppp/8/5Q2/4n3/1B5R/PPP1qPP1/5RK1 b - - 0 1` | Qxf1+ | e4f6 |
| QS.WAC225 | `4R3/4q1kp/6p1/1Q3b2/1P1b1P2/6KP/8/8 b - - 0 1` | Qh4+ | e7c7 |
| QS.WAC241 | `2rq1rk1/pp3ppp/2n2b2/4NR2/3P4/PB5Q/1P4PP/3R2K1 w - - 0 1` | Qxh7+ | f5f6 |
| QS.WAC244 | `r6r/pp3ppp/3k1b2/2pb4/B4Pq1/2P1Q3/P5PP/1RBR2K1 w - - 0 1` | Qxc5+ | d1d5 |
| QS.WAC245 | `4rrn1/ppq3bk/3pPnpp/2p5/2PB4/2NQ1RPB/PP5P/5R1K w - - 0 1` | Qxg6+ | c3d5 |
| QS.WAC263 | `rnbqr2k/pppp1Qpp/8/b2NN3/2B1n3/8/PPPP1PPP/R1B1K2R w KQ - 0 1` | Qg8+ | f7h5 |

- Latest telemetry (200 ms, 2025-10-06): `docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-06_19-38-59.csv` – 4/20 successes with the search gating in place.

### Evaluation Bias FENs
- Queen mobility and slider-pressure deltas remain in the tracker (`docs/issues/eval_bias_tracker.md:11-44`); re-run `tools/eval_compare.py` against these after each phase to ensure eval drift is shrinking.
- Selectivity logs for `r1b1k2r/p2n1p2/...` capture how `g3e4` depends on LMR/null/SEE guards and will be reused as regression harness (`docs/project_docs/telemetry/eval_bias/selectivity_bounds_g3e4.txt:194-451`).

## Code Touchpoints
- **Search selectivity & re-search**: Queen checks get reduced in the main negamax when early fail-lows occur (`src/search/negamax.cpp:351-470`).
- **LMR gating**: We currently skip reductions for captures/checks but the thresholds are tuned for general play, not motif-specific rescoring (`src/search/lmr.cpp:59-179`).
- **Move prioritisation**: Killer/history/countermove ordering can bury sacrificial checks once the first pass fails (`src/search/move_ordering.cpp:200-320`).
- **History updates**: Bonus/penalty scaling defines how quickly a fail-low tanks a queen check (`src/search/history_heuristic.cpp:13-57`).
- **Attack detection hot path**: Every deeper king-safety tweak increases `isSquareAttacked` pressure; the cache/bitboard optimisations remain outstanding (`src/core/move_generation.cpp:605-811`).
- **King safety scoring**: Current shield/air-square terms lack exposed-king penalties that would reward sacrificing material to attack (`src/evaluation/king_safety.cpp:23-176`).

## Phased Plan & Deliverables

### Phase QS0 – Suite & Telemetry Infrastructure
- Deliverables
  - Wire `tests/positions/queen_sack_suite.epd` into `tools/tactical_investigation.py` and add a `--suite queen-sack` preset for recurring runs (`tools/tactical_investigation.py:255`).
  - Extend `tools/eval_compare.py` driver to accept suite lists so the eval deltas can be batch-reported alongside search outcomes (`docs/issues/eval_bias_tracker.md:11`).
- Validation
  - Reproduce current failures and baseline `mate?`, node counts, and cp deltas for all 20 positions.
  - Archive telemetry in `docs/project_docs/telemetry/queen_sack/` before code work starts.

### Phase QS1 – Search Selectivity Adjustments
- Scope
  - Add a dedicated queen/slider check bucket that bypasses the first fail-low reduction and forces a PV re-search when the move returns >= beta/4 within reduced windows (`src/search/negamax.cpp:351-470`).
  - Tag queen contact checks so LMR uses a lower `minMoveNumber` threshold, preventing them from falling into the “late move” bucket (`src/search/lmr.cpp:59-179`).
- Status (2025-10-06): queen contact capture-checks temporarily skip LMR/LMP reductions and re-search as full PV nodes; move-count pruning is disabled for those motifs until evaluation reinforcements land.
- Validation
  - Queen-sack suite must pick the forcing move in ≥12/20 positions within 1 s.
  - No regression on `tools/mini_wac.sh` baseline run and no >2% slowdown via `bench`.

### Phase QS2 – Move Ordering & History Retuning
- Scope
  - Introduce positive history kickers for checking captures and ensure fail-low penalties decay instead of saturating for these motifs (`src/search/history_heuristic.cpp:13-57`).
  - Inject a temporary “contact-check killer” slot so the move reappears early on the second pass even without TT help (`src/search/move_ordering.cpp:200-320`).
- Status (2025-10-07): **Code landed in `fd569d9` (`bench 2501279`)** – checking-capture history kicker plus contact-check replay hash per ply (`src/search/history_heuristic.cpp`, `src/search/types.h`, `src/search/negamax.cpp`). Ordering reruns and history telemetry still pending. First follow-up run (`tactical_queen-sack_2025-10-07_23-37-00.csv`) shows 4/20 hits (unchanged from QS1), so the search changes alone are not enough yet.
- Validation
  - Track proportion of queen checks searched before quiet moves via `NodeExplosionStats`; require ≥70% ordering rate in telemetry rerun (`docs/project_docs/telemetry/eval_bias/selectivity_bounds_g3e4.txt:194-451`).
  - Confirm no regressions on the broader 54-position WAC subset.
- TODO before advancing:
  - [x] Rebuild Release (`./build.sh Release`) and re-run `tools/tactical_investigation.py --suite queen-sack --time 200` capturing CSV under `docs/project_docs/telemetry/queen_sack/2025-10-07/` (`tactical_queen-sack_2025-10-07_23-37-00.csv` → 4/20).
  - [x] Run `tools/eval_compare.py --suite queen_sack_suite.epd --depth 18` to see eval deltas with the new ordering; append Markdown diff to `docs/issues/eval_bias_tracker.md`. (Output captured in `docs/project_docs/telemetry/queen_sack/2025-10-07/eval_compare_queen_sack_depth18.{json,md}` – large negative deltas remain because the forcing moves still aren’t preferred.)
  - [ ] Collect `SearchData` stats (move ordering/histories). Retry with a bounded `go nodes 300000` run (prior 1M-node attempt stalled); stash the log under `docs/project_docs/telemetry/queen_sack/2025-10-07/`.
  - [ ] Update this plan + Evaluation Bias Index with telemetry outcomes and decision on QS2 readiness.

### Phase QS3 – Evaluation Reinforcement
- Scope
  - Expand king-danger scaling for open-file exposure and slider backbone pressure to reward sacrificing material for mating nets (`src/evaluation/king_safety.cpp:23-176`).
  - Log new EvalExtended breakdowns for queen mobility penalties and integrate into the bias tracker.
- Validation
  - Queen-sack suite must stay solved after enabling the eval tweaks.
  - Depth-18 eval deltas in `docs/issues/eval_bias_tracker.md:11-44` shrink by ≥50 cp without flipping sign.

### Phase QS4 – Performance & Regression Governance
- Scope
  - Implement cached king-zone attack bitboards to offset additional king-safety probes (`src/core/move_generation.cpp:605-811`).
  - Run `bench`, `tools/run_wac_test.sh`, and at least one 10k-node self-play mini-match per change set.
- Validation
  - `bench` delta ≤ ±1% and no new hotspots in `perf` output.
  - OpenBench gating SPRT vs. `main` before merging any phase that touches evaluation weights.

## Reporting & Checkpoints
- Track progress in `feature_status.md` whenever a queen-sack branch is active (`docs/project_docs/feature_guidelines.md:232-335`).
- Update the Evaluation Bias Index after each completed phase with new deltas and telemetry summaries (`docs/project_docs/Evaluation_Bias_Index.md:6-49`).
- Each phase concludes with a package containing: queen-sack suite results, eval-compare output, and perf summary.
- Current checkpoint (2025-10-07): Head commit `fd569d9` (`bench 2501279`). QS2 code merged; telemetry + validation runs outstanding (see TODO list above). Leave all fresh artifacts under `docs/project_docs/telemetry/queen_sack/2025-10-07/` for continuity.

## Open Questions
- Do we need an explicit TT retry trigger when a sacrificial check fails low purely on static eval?
- Should queen mobility penalties be tapered by material deficit to avoid over-rewarding speculative sacs?
- Are additional pruning heuristics (null-move desperation margin, SEE gating) interacting with these motifs beyond what the current probes reveal?
