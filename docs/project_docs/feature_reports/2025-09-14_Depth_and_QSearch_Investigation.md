Title: Depth Scaling, Node Growth, and QSearch Investigation (Phase 3/Integration)

Summary
- Goal: Understand why SeaJay sometimes fails to reach target depth in reasonable time and “explodes” in nodes versus peers; identify fixes and add levers to contain node growth without harming tactical strength.
- Focus: Iterative deepening behavior, LMP range, null-move verification overhead, and especially quiescence (qsearch) cost.
- Scope: Work performed on integration/20250912-depth-and-search-speed via feature/20250914-lmp-max-depth.

Branches & Commits
- Working branch: feature/20250914-lmp-max-depth (based on integration/20250912-depth-and-search-speed@a3467eb)
- Key commits (with bench lines):
  - feat(search): depth-only stability + diagnostics; add MoveCountMaxDepth; fix UB in king safety and time overflow — bench 3788008
  - feat(uci+qsearch): Add QSEEPruning and QSearchMaxCaptures; wire quiescence diagnostics and constraints — bench 3788008

What We Reproduced
- Time-based early termination in depth-only runs: Iterative wrapper could stop early at deep fixed-depth searches (e.g., “Early termination at depth 17/19”), giving the impression of “flat-lining”.
- Position 1 (8/4bk1p/1p3np1/p2r1p2/6n1/5N2/PP3PKP/2RRB3 b - - 1 29):
  - Earlier “flat-line” node counts were artifact of a 60s external timeout; engine hadn’t completed deeper iterations. After fixes, depth-only runs complete.
  - qsearch share ≈ 47–48% at depth 16–18; strong indication that deep tactical resolution in qsearch is a major cost.
- Position 2 (r1bq1rk1/pp3ppp/2nbpn2/2pp4/2PP4/P3PN2/1P1NBPPP/R1BQ1RK1 w - - 1 10):
  - Decisive capture–recapture line; all engines agree. SeaJay picks d4c5 at root, so root selection isn’t the issue.
  - At depth 21 baseline: total nodes ≈ 19.3M; qsearch ≈ 52.5% of total nodes; average EBF ≈ 0.94.

Findings
1) LMP depth cap was hardcoded at ≤ 8 plies
- Hypothesis: node growth at deep iterations is exacerbated because LMP stops at depth > 8.
- Action: added UCI MoveCountMaxDepth (default 8) to A/B. Position-dependent results:
  - Position 1: Slight node increase at depth 18 with LMP=12 (worse).
  - Position 2: ~6–8% fewer nodes at depth 18 with LMP=12 (better).
  Conclusion: extending LMP helps some positions; needs controlled rollout and SPRT.

2) Time management caused early termination in depth-only tests
- Fixed: robust depth-only detection and guards in both iterative test path and legacy search; no time-based stops when go depth N with no time controls.

3) UB / Stability issues
- Fixed signed overflow in hard-limit computation (infinite optimum case).
- Fixed 64-bit shift UB in king-safety; added defensive guard on kingSquare.

4) QSearch is a primary driver of node growth in decisive lines
- Position 2 depth 21 baseline: qsearch ≈ 52.5% of total nodes.
- Diagnostics lacked granular capture metrics; added instrumentation:
  - Stand-pat cutoffs recorded per qply.
  - Captures generated vs captures actually searched in qsearch (non-check nodes).

Changes Implemented (current branch)
- New UCI controls:
  - MoveCountMaxDepth: spin, default 8 — caps LMP range.
  - QSEEPruning: combo, default conservative — separate SEE prune mode for qsearch only.
  - QSearchMaxCaptures: spin, default 32 — per-node capture budget in qsearch (except when in check).
- Depth-only stability: completely disables time-based early termination in depth-only runs.
- Diagnostics: stand-pat and qsearch capture metrics recorded when NodeExplosionDiagnostics=true.

Representative Results (Release, diagnostics on)
- Position 1 (depth 18):
  - Baseline (LMP=8): main nodes ~6.48M; qsearch ~46.9% of total; tt hit% ~31.9; null cut% ~43.
  - LMP=12: slight node increase vs baseline (position-dependent downside).
- Position 2 (depth 21):
  - Baseline (LMP=8): main nodes ≈ 9.18M; total ≈ 19.3M; qsearch ≈ 52.5%; first-move cutoffs ≈ 80%.
  - LMP=12 (@depth 18 earlier): ~6–8% fewer nodes than LMP=8.
  - Next step: re-run depth 21 with LMP=12 and new qsearch toggles for a clean matrix.

Interpretation
- Root selection: correct on Position 2 (d4c5), so not the main culprit.
- Node growth is strongly tied to qsearch workload in decisive lines:
  - High capture traffic with modest stand-pat cutoffs at depth.
  - Conservative SEE pruning may be letting through inexpensive but non-improving captures; equal-exchange pruning only partial.
- LMP beyond depth 8 can help reduce main-tree width in some positions; however it’s not universally helpful.

Next Steps (Plan)
1) A/B Matrix on Position 2 (depth 21)
   - Baseline: LMP=8, QSEE=conservative, QMaxCaps=32.
   - LMP extended: LMP=12, QSEE=conservative, QMaxCaps=32.
   - QSEE aggressive: LMP=8, QSEE=aggressive, QMaxCaps=32.
   - Capture cap tighter: LMP=8, QSEE=aggressive, QMaxCaps=16.
   Collect: nodes, qsearch%, stand-pat%, capture gen/searched, SEE prune counts, fail-high rates; verify identical best line and no tactical regressions.

2) Extend matrix to Position 1 and 3
   - Re-check where deeper LMP worsened Position 1; see if qsearch controls still lower total nodes without hurting result quality.

3) QSearch pruning refinements
   - Consider adaptive qsearchMaxCaptures by qply (tighter deeper) and by material/phase.
   - Consider stronger SEE thresholds for non-critical captures (guarded by checks, near-mate, TT context).
   - Review delta-pruning margins by phase; possibly increase in middlegames for low-value captures.

4) Null-move verification telemetry
   - Track verification-trigger rate and cost by depth; if overly frequent at deep plies, raise verifyDepth or gate by NPM/phase.

5) LMP rank-aware gating
   - Ensure rank-aware LMP gates respect MoveCountMaxDepth everywhere; A/B small adjustments if unordered.

6) OpenBench A/B
   - Once local results are promising, run SPRT at 10+0.1 with: (a) LMP=8 vs 12; (b) QSEE conservative vs aggressive; (c) qsearch capture cap variants.
   - Ensure no tactical regressions; prefer non-regression at standard TC.

Risks & Mitigations
- Over-pruning in qsearch can hide tactics: keep guards for checks, near-mate margins, and avoid pruning when static eval is near alpha/beta boundaries.
- Position-dependent behavior of LMP: keep MoveCountMaxDepth gate default at 8 until A/B passes; treat LMP extension as a tunable.

Appendix: Implementation Summary
- Depth-only guard: both searchIterativeTest() and legacy search() now skip early termination when no time controls are present.
- QSearch controls: added QSEEPruning (separate mode for quiescence) and QSearchMaxCaptures; wired to SearchLimits and SearchData.
- Diagnostics: stand-pat increments and capture generation/searched counters wired under NodeExplosionDiagnostics.

