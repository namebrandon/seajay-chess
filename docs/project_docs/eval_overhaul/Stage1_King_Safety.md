# Evaluation Overhaul — Stage 1: King Safety

## 1. Context
- Singular extension work exposed large static-eval gaps; king danger terms are the first recurring root cause.
- Current implementation (Phase 1a) introduced infrastructure, but tuning gaps and performance overhead led to ~−30 Elo vs. `main`.
- Stage 1 isolates king-safety fixes before moving to other evaluation components (passed pawns, PST, pawn structure compensation).

## 2. Scope
**In scope**
- Rebalance shield accounting so uncastled kings do not receive free cover while castled kings retain accurate penalties.
- Introduce attacker weighting and aggregation (“attack units”) to scale danger by piece type and multi-attacker pressure.
- Cache reusable proximity data to reduce per-node overhead and enforce a performance guardrail (≤2% node-rate hit vs. current `main`).
- Harden evaluation regression harness (`tests/eval/problem_positions_test.cpp`) with enforced thresholds for king-safety canary FENs.
- Capture telemetry/baselines that quantify king-safety contributions pre- and post-change.

**Out of scope**
- Passed-pawn overhaul, pawn-structure compensation, and PST retuning (Stage 2+).
- Search-parameter adjustments (null move, futility margins) unless required as fallout from this stage.
- UCI option plumbing or SPSA auto-tuning (defer until post-Stage 1 measurement).

## 3. Goals & Success Criteria
- **Strength**: Reach parity with `main` within ±5 Elo at 10+0.1 (SPRT) while fixing known watch-list FEN mis-evaluations.
- **Accuracy**: All enforcing entries in `tests/eval/problem_position_expectations.json` within target windows; zero regressions flagged by harness.
- **Performance**: Maintain Release bench within 2% of baseline `main` (record bench counts in every commit message).
- **Operability**: Telemetry scripts (`tools/analyze_position.sh`, harness CSV diffs) produce before/after reports stored in `external/eval_baselines/`.

## 4. Workstreams & Milestones
| Milestone | Description | Exit Criteria | Target Branch | Artifacts |
|-----------|-------------|---------------|---------------|-----------|
| M1 — Fortress Normalization | Gate shield bonuses and penalties using a blended “fortress factor”; remove free bonuses for exposed kings. | Harness deltas shrink; micro-SPRT (`test` branch) shows ≤10 Elo deficit; profile shows <1% overhead. | `feature/202510xx-eval-ks-stage1` | Harness log, commit with `bench xxxx`, comparison CSV |
| M2 — Attack Unit Scaling | Implement attacker-weight aggregation and cached proximity data; integrate into king-ring penalties. | Watch-list FENs match reference ranges; performance regression ≤2%; targeted SPRT neutral or positive. | same | Updated expectations JSON, telemetry write-up |
| M3 — Enforcement & Guardrails | Flip harness entries to `enforce=true`, add perf guard in CI (bench delta check) and capture documentation. | Harness passes enforced checks; baseline CSV archived; documentation updated. | same | Updated harness config, new baseline CSVs |
| M4 — Integration & Cleanup | Re-run profiling, launch full SPRT, merge back to `integration/evaluation-overhaul`, update roadmap. | SPRT PASS, documentation updated, integration branch rebased. | `integration/evaluation-overhaul` | Final stage summary, merged commits |

## 5. Execution Checklist
1. Branch from `integration/evaluation-overhaul` after syncing latest `main` (post singular-extension merge).
2. Create feature branch `feature/202510xx-eval-ks-stage1` (date-stamped).
3. Implement M1 changes; run harness (`ctest -R eval --verbose`), record bench, commit with `bench <nodes>`.
4. Launch short self-play or node-limited regression for confidence; archive CSVs under `external/eval_baselines/`.
5. Iterate through M2 and M3, running micro-SPRTs (`test/` branches) when experimenting.
6. Once stable, flip harness enforcement, capture documentation updates here & in master plan.
7. Final SPRT + profiling, prepare merge notes, land on integration branch, then prep Stage 2 plan.

## 6. Telemetry & Reporting
- **Harness**: Promote critical king-safety FENs to `enforce=true`; track tolerances in expectations file.
- **Baselines**: Store before/after CSVs and `debug eval` logs at `external/eval_baselines/king_safety/<YYYY-MM-DD>_*.csv`.
- **Performance**: Record `bench` output per commit; target <2% variance.
- **SPRT**: Use OpenBench standard (10+0.1, SPRT [−5, +15] Elo boundaries) for final verification.
- **Status Updates**: Append findings to `docs/project_docs/Evaluation_Overhaul_Plan.md` under Stage 1 progress, plus short summaries in integration branch PR description.

## 7. Risks & Mitigations
- **Over-penalizing active kings**: Monitor harness deltas for FENs where dynamic attacks are correct; adjust fortress factor and attack weights.
- **Performance regression**: Profile `king_safety.cpp`; consider precomputing tables or memoizing per node when needed.
- **Regression reintroduction**: Harness enforcement plus baseline CSV diff ensures immediate detection.
- **Timeline creep**: Weekly checkpoint review—if M1 spills past two days, reassess scope or split into sub-iterations.

## 8. Dependencies
- Singular-extension SPRT must converge and merge to `main`.
- Integration branch `integration/evaluation-overhaul` must contain latest harness infrastructure.
- Updated build toolchain (Makefile/CMake with `-O3 -march=native -flto`) for consistent perf measurements.

## 9. Parking Lot (Future Stages)
- Stage 2: Passed-pawn sanity checks and blockade detection.
- Stage 3: Pawn-structure compensation & PST retuning (may involve SPSA).
- Stage 4: Search interaction adjustments (null move, futility, pruning margins).
- Potential SPSA/automated tuning once infrastructure stabilizes.

## 10. Deliverables
- Stage summary in this document (updated as milestones complete).
- Enforced harness expectations and updated telemetry artifacts.
- Integration branch merge notes, including bench figures and SPRT outcome.
- Retrospective entry capturing lessons learned before Stage 2 kickoff.
