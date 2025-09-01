# SeaJay Evaluation & Search Efficiency Remediation Plan

Author: Team SeaJay
Status: Draft for review (do not merge to main)
Scope: Evaluation correctness (priority) and search efficiency (instrument + investigate)

## Prime Directives

- NEVER merge this work to `main` automatically. All phases require human approval after OpenBench (SPRT) results. Explicitly: do not `git merge` to `main` in any phase of this plan without a separate, explicit merge authorization.
- Follow Feature Guidelines (docs/project_docs/feature_guidelines.md):
  - Every commit message MUST include a bench count line in exact format: `bench <number>`.
  - Phased, small, testable increments; STOP → TEST → PROCEED between phases.
  - Use meaningful branch names (Git Strategy doc) and keep feature docs on-branch.
- Build exactly as in docs/BUILD_SYSTEM.md:
  - Local development: `rm -rf build && ./build.sh Release` (CMake)
  - OpenBench/production: `make clean && make` (Makefile is authoritative)
  - Benchmark: `echo "bench" | ./bin/seajay` and extract the node count.

## Branching & Workflow

- Create a long-lived integration branch for this multi-surface effort:
  - `integration/eval-and-search-remediation` (no date prefix; will live weeks)
- Each phase below uses a short-lived feature branch off that integration branch:
  - Example: `feature/20250901-eval-E1-king-safety-sanity`
  - Merge feature branch back into the integration branch only after validation.
- OpenBench tests always compare the feature branch vs `main` (or the integration branch if specifically stated). Do not test vs a prior phase unless called out.

## Test Harness & Reference Positions

- Use tools/eval_compare.sh to build (or `--no-build`) and run UCI `eval` on reference FENs.
  - Shows true King Safety and phase info (A2) in the breakdown.
  - Defaults:
    - FEN1: Example Game 2 (middlegame)
    - FEN2: Example Game 1 (endgame choice)
    - FEN3: King Safety sanity (expect KS ~ +48 for White)
- Include these in local sanity checks:
  1) Example Game 2: `r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/1R2R1K1 w kq - 2 17`
  2) Example Game 1: `8/5p2/2R2Pk1/5r1p/5P1P/5KP1/8/8 b - - 26 82`
  3) KS sanity FEN: `rnbqkbnr/ppppp3/8/8/8/5N2/PPPPPPPP/RNBQ1BKR w KQ - 0 1`
- Store local outputs (eval breakdown, PVs, bestmoves) in per-phase notes.

## OpenBench Configuration Template

- Dev Branch: the phase branch (see each phase)
- Base Branch: `main`
- Time Control: start with `10+0.1`; use `60+0.6` for endgame-heavy phases
- Book: `UHO_4060_v2.epd` (general). For endgame-centric phases (e.g., A3), prefer `external/endgames.epd`.
- Bounds (nELO): per phase; see guidance bullets
- Description: short, clear phase description

IMPORTANT: OpenBench runs are MANUAL
- OpenBench tests are run by a human on a remote server that pulls branches from the remote/origin repository.
- This assistant cannot initiate OpenBench runs. After each push, the assistant will output a concise “OB Test Request” with:
  - Branch: exact branch name to test
  - Commit: full 40-char SHA (not short)
  - Bounds: suggested SPRT nELO bounds for the phase
  - Expected: brief expected outcome (pass/neutral) and approximate effect
  - Rationale: short explanation of the change and why the bounds/expectation are appropriate

OB Test Request Template (assistant will fill):
- Branch: <feature/…>
- Commit: <full-40-char-sha>
- Base: main
- TC/Book: 10+0.1 (or 60+0.6) / UHO_4060_v2.epd (or external/endgames.epd for endgame phases)
- Bounds: [L, U] (nELO)
- Expected: <e.g., neutral, small positive>
- Rationale: <1–2 sentences>

---

## Roadmap Overview

Priority A: Evaluation correctness in the positions highlighted (Example Game 2 focus)
Priority B: Search efficiency instrumentation and low-risk improvements

Phases: Each phase is small, testable, and gated by OpenBench. The default expectation is minor nELO changes; evaluation phases aim to reduce egregious eval errors first, not maximize Elo.

Legend for bounds:
- Infrastructure/no-expected-change: `[-5.00, 3.00]`
- Minor expected positive: `[0.00, 5.00]`
- Medium expected positive: `[2.00, 8.00]`

---

## A. Evaluation Priority Phases

### Phase A0 — Eval Visibility & Sanity Harness (Infrastructure)
- Goal: Make evaluation disagreements immediately visible and repeatable per FEN.
- Implementation:
  - Add a developer script `tools/eval_compare.sh` that:
    - Builds Release per docs/BUILD_SYSTEM.md.
    - Runs `uci` → `eval` on the two key FENs, capturing eval breakdown in cp.
    - Outputs a terse table: material, PST, kingSafety, mobility, etc., total.
  - No engine behavior change; visibility only.
- Commit message: `chore: Add eval compare harness (Phase A0)

bench <nodes>`
- OpenBench: Optional (infra only). Bounds: `[-5.00, 3.00]`
- STOP → Validate locally that breakdowns are captured and stable run-to-run.

Status: COMPLETED
- Script present (`tools/eval_compare.sh`) with KS sanity FEN (FEN3) and phase info in output.
- UCI eval breakdown shows King Safety (A0 support commit) and phase (A2).

### Phase A1 — King Safety Sanity (Weight Audit; No New Terms)
- Goal: Address the clear underweighting of king safety visible in Example Game 2 without architectural churn.
- Implementation (hot path awareness):
  - Adjust KingSafety::KingSafetyParams constants modestly (e.g., directShieldMg from 10 → 16, advancedShieldMg 8 → 12; keep EG values small or negative).
  - Do NOT add new loops or board scans in hot path; we reuse existing detection.
  - Ensure evaluation stays branch-predictable; keep arithmetic simple.
  - Verify that in Example Game 2, `h2h3` rises in ranking (local quick check).
- Commit message: `eval: modest king safety weight increase (Phase A1)

bench <nodes>`
- OpenBench: Bounds `[0.00, 5.00]` at 10+0.1; if noisy, re-run at 60+0.6.
- STOP → Proceed only if non-negative and subjectively reduces egregious choices.

Status: COMPLETED — Tentative OB PASS
- Branch: `feature/20250901-eval-king-safety-sanity`
- Commit (weights): 002f6e7cb6e7ab7d421e877e75ecfd360c58967a
- OB (human): LLR 1.41 @ 2500 games, ≈ +12 ELO (tentative pass).

### Phase A2 — PST Phase Consistency Audit (Infrastructure)
- Goal: Align PST tapering (continuous 0–256) with phase-dependent terms to reduce inconsistencies near phase boundaries.
- Implementation:
  - Add a debug UCI flag to dump phase (0–256) and coarse GamePhase per node root-only (no per-node spam) to verify that critical positions (Example Game 2) are in the intended phase.
  - No functional change; visibility only.
- Commit message: `chore: add phase visibility (Phase A2)

bench <nodes>`
- OpenBench: Optional. Bounds `[-5.00, 3.00]`.
- STOP → Confirm Example Game 2 is treated as middlegame by both systems.

Status: COMPLETED
- Added UCI option `ShowPhaseInfo` (default true) and eval prints phase (0–256) and coarse GamePhase.
- Commit: 9eaa3dc4c542bb471d811de3f49964a111a52aaf

### Phase A3 — Endgame Sensibility Guardrails (Minimal)
- Goal: Improve endgame decisions where simplifications to inferior rook endgames occur.
- Implementation (very conservative):
  - Slightly increase rook endgame PST values on 7th rank (already > MG) by a small delta (+2 to +4 cp eg-only) to better reflect activity in basic rook endgames.
  - Add a tiny bonus for king proximity to own rook in pure endgames only (reuse cached king squares; O(1) arithmetic).
  - Keep all logic branch-light and table-driven; no new scans.
- Commit message: `eval: small EG tweaks (rook 7th, K–R proximity) (Phase A3)

bench <nodes>`
- OpenBench: Bounds `[0.00, 5.00]` at 60+0.6 (endgame heavy).
- STOP → Proceed only if non-negative; verify Example Game 1 behavior improves locally.

Status: IMPLEMENTED — OB PENDING
- PST: Rook 7th-rank EG values +4 (20→24); tiny EG-only king–rook proximity bonus (≤6cp).
- Commit: 99fe2427bd60fac99b70a97711c6e68f42ffab1d
- Recommended OB: Book `external/endgames.epd`, TC 60+0.6, Bounds [0.00, 5.00].

### Phase A4 — Quiet-Prophylaxis Nudges (Micro)
- Goal: Make quiet king-safety-preserving moves (like h2h3) slightly more competitive without broader complexity.
- Implementation:
  - Add a tiny static bonus (+2 cp) for a side whose king has at least one “air” square created by a friendly pawn move (only in MG; O(1) using existing mask checks; no scans).
  - Keep under a strict cap and guard by simple conditions to avoid noise.
- Commit message: `eval: micro prophylaxis nudge (Phase A4)

bench <nodes>`
- OpenBench: Bounds `[0.00, 5.00]`.
- STOP → Validate no regressions; confirm Example Game 2 eval sign shifts toward consensus.

### Phase A5 — SPSA Prep (Endgame PST Only; No Behavior Change Yet)
- Goal: Prepare a minimal SPSA configuration for EG-only PST differentials (already supported by the PST infrastructure).
- Implementation:
  - Add/update UCI option mapping for a small EG-only subset (center squares for pawns, rook 7th multiplier), using rounding-safe parsing (audit integer parameter rounding; avoid truncations).
  - Do not change defaults; no gameplay change.
- Commit message: `chore: expose minimal EG PST UCI knobs (Phase A5)

bench <nodes>`
- OpenBench: None (infra only). Bounds `[-5.00, 3.00]`.
- STOP → Later phases may run SPSA weaves; separate plan.

---

## B. Search Efficiency Phases (Instrumentation-first)

### Phase B0 — Search Telemetry (Infrastructure; Low Overhead)
- Goal: Quantify the node gap drivers precisely.
- Implementation (hot path mindful, amortized sampling):
  - Counters (most exist; ensure toggles via UCI):
    - PVS: scout searches, re-searches, rate of first-legal-move being searched as PV vs scout.
    - TT: probes, hits, cutoffs, bound distribution, hashfull.
    - Cutoffs by move index (bucket 1..10, >10) — already present; expose summary on demand.
    - Pruning: null-move attempts/cutoffs; futility/move-count prunes (depth buckets).
    - Legality: count of illegal pseudo-legal moves before first legal move per node (root and non-root sampling).
  - Add a UCI `SearchStats=on|off` to print a one-shot summary at the end of a search; default off.
- Commit message: `chore: add low-overhead search telemetry (Phase B0)

bench <nodes>`
- OpenBench: Optional. Bounds `[-5.00, 3.00]`.
- STOP → Use the two FENs and a couple of startpos searches to profile.

### Phase B1 — PV First-Legal Move Correctness (Small Logic Fix)
- Goal: Ensure the first actual legal move at a node is treated as the PV move to maximize early cutoffs; avoid penalizing when the first pseudo-legal move is illegal.
- Implementation:
  - Adjust move loop so PV/full-window search applies to the first legal move encountered, not the first generated pseudo-legal move. Pseudocode:
    - Track `sawLegal = false`; when `tryMakeMove` fails, continue without incrementing the “PV slot”. The first `tryMakeMove==true` enters the full-window PV path; later legal moves use PVS with reductions.
  - Keep all other ordering unchanged.
- Commit message: `search: PV on first legal move, not pseudo-legal (Phase B1)

bench <nodes>`
- OpenBench: Expect node reduction at equal depth; bounds `[0.00, 5.00]`.
- STOP → Compare node counts vs baseline on same depth; proceed if equal or better Elo.

### Phase B2 — Quiescence SEE Pruning/Ordering Toggle Experiment (No Default Change)
- Goal: Quantify impact of enabling conservative SEE in qsearch and/or SEE-based capture ordering.
- Implementation:
  - Add UCI toggles (if not already exposed) to enable:
    - SEE pruning conservative/aggressive (existing enums) and a middle “conservative+0-only prune” mode.
    - SEE-based capture ordering in qsearch only.
  - Default stays OFF; this is a measurement phase.
- Commit message: `search: expose qsearch SEE toggles (Phase B2)

bench <nodes>`
- OpenBench: Two micro-tests:
  - Nodes-at-depth comparison on sample FENs (local), then `[0.00, 5.00]` bounds in OB.
- STOP → If positive or neutral with fewer nodes, plan a future activation phase.

### Phase B3 — Move Count Pruning Calibration (Parameter Only)
- Goal: Make late quiet pruning slightly more decisive without tactical blindness.
- Implementation:
  - Small parameter sweep (via UCI) for `moveCountLimit[3..6]` and `moveCountHistoryThreshold/Bonus/ImprovingRatio`; 1–2 commits, each with bench and OB.
  - No algorithm change.
- Commit message: `search: conservative MCP parameter tighten (Phase B3)

bench <nodes>`
- OpenBench: Bounds `[0.00, 5.00]`.
- STOP → Keep only if non-negative and measurable node reduction.

### Phase B4 — Aspiration Stability Audit (Telemetry-driven)
- Goal: Reduce costly re-search loops.
- Implementation:
  - Use Phase B0 stats to identify re-search rates; if high, test slightly wider initial window or growth mode tweaks (exponential → capped exponential) only at depths where fail-logs cluster.
  - Keep within a single commit with bench.
- Commit message: `search: reduce aspiration re-search churn (Phase B4)

bench <nodes>`
- OpenBench: Bounds `[0.00, 5.00]`.
- STOP → Retain if nodes decrease without Elo loss.

### Phase B5 — Null Move Pruning Checks (No Algorithmic Change)
- Goal: Ensure null-move is firing appropriately.
- Implementation:
  - Measure attempts/cutoffs/verification-research rates with B0 telemetry; test small `R` (reduction) or margin tweaks via UCI. Keep default unchanged unless OB positive.
- Commit message: `search: null-move telemetry + tiny R tweak experiment (Phase B5)

bench <nodes>`
- OpenBench: Bounds `[0.00, 5.00]`.
- STOP → Revert if any tactical regressions observed.

### Phase B6 — Lazy Legality vs Legal Generation (Experiment)
- Goal: Validate whether pseudo-legal generation materially hurts PVS efficiency at our current strength.
- Implementation:
  - Add a compile-time or UCI switch for “PV nodes use legal-generation only” (root/PV nodes only) to test PV-first correctness vs lazy legality, keeping most nodes lazy for speed.
  - One-off branch for measurement; do not enable by default.
- Commit message: `search: experiment PV legal-gen toggle (Phase B6)

bench <nodes>`
- OpenBench: Nodes-at-depth local test first; then OB `[0.00, 5.00]` if promising.
- STOP → Make no default change unless clear win.

---

## C. Execution Cadence (Per Phase)

1) Create feature branch per phase (see names above).
2) Implement minimal, focused change.
3) Build per BUILD_SYSTEM.md (Release build for bench; Makefile for OB).
4) Capture bench count; commit with exact `bench <number>` line.
5) Push the branch. A human will run OpenBench manually on the remote server.
   - The assistant will print an “OB Test Request” (branch, full SHA, bounds, expected, rationale) for convenience.
6) STOP. Await human review of OB results.
7) If PASS or neutral and meets qualitative goal (e.g., reduced egregious eval), merge into the integration branch only.

## D. Validation and Guardrails

- Always run the two reference FENs locally with `uci eval` and save the breakdowns in the phase’s notes.
- For node-gap tests, run fixed-depth searches and capture nodes, seldepth, and PV stats before/after.
- Use `make clean && make VERBOSE=1` if verifying flags; use `objdump` checks per BUILD_SYSTEM.md when needed.
- Keep debug-only outputs behind UCI flags; default quiet.

## E. Exit Criteria

- Example Game 2 class of positions: SeaJay’s best move choices and eval sign align more closely with consensus engines at shallow depths.
- Node gap at equivalent depth reduced measurably (ideally ≥20% fewer nodes on representative positions) without Elo loss.
- No illegal-move regressions seen in harness logs.

## F. Risk Notes

- Hot paths: negamax loop, move ordering, tryMakeMove, quiescence. All changes must be branch-light, table/constant-driven where possible, and behind UCI toggles for measurement.
- TT behavior: monitor for pathological re-use via stats; any adjustment to TT usage must be incremental and stats-justified.
- Build flags (`-O3 -march=native -flto`) can magnify UB. Keep sanity asserts in DEBUG, but avoid overhead in Release.

## G. Administration

- Documentation: Update this plan with per-phase outcomes. Keep any temporary `feature_status.md` on the branch only; never merge that file to `main`.
- Cleanups: After each phase is accepted into the integration branch, prune experimental/test branches using the provided git aliases (per Git Strategy doc).
