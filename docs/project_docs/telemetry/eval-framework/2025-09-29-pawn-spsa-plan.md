# Pawn Evaluation SPSA Preparation — 2025-09-29

## Context
- Telemetry harness run: `tools/eval_harness/run_eval_pack.sh` with `movetime=300` on `tests/packs/eval_pawn_focus.epd` (13 positions)
- Artifacts:
  - Raw evaluation log: `tmp/eval_logs/pawn_eval.log`
  - JSON report: `tmp/eval_reports/eval_20250929T205434Z.json`
  - Summary JSON: `tmp/eval_reports/eval_20250929T205434Z_summary.json`
  - Derived telemetry digest: `tmp/eval_reports/eval_20250929T205434Z.telemetry.txt`
- Reference engine: Komodo 14.1 (auto-selected, verified present)

## Key Findings from Telemetry
- **Passed pawn term swings:** Mean contribution −47.8 cp across the pack with extremes down to −367 cp (`position #13`), driving the largest Komodo deltas (−379 cp).
- **Isolated/doubled penalties overweighted:** Positions #2/#5/#7 show +32 – 45 cp swing in SeaJay’s favour despite losing attacks, suggesting isolation/doubling rebates need rebalancing against king danger.
- **King danger interaction:** In #2 and #7 SeaJay underestimates incoming attacks (−38 cp king safety vs Komodo’s higher pressure), implying pawn guard rebates, loose-pawn penalties, and candidate lever support should be tuned jointly.
- **Pawn span telemetry:** Push-ready counts average 3.1 (white) / 2.8 (black) with near-zero tension/infiltration, indicating current thresholds rarely trigger bonuses for the critical themes.
- **Cache coverage:** 13/13 pawn cache hits confirm instrumentation won’t mask SPSA adjustments.

## Target Parameter Bundles
1. **Defensive cover / semi-open files**
   - `EvalSemiOpenGuardRebate`
   - `EvalLoosePawnOwnHalfPenalty`
   - `EvalLoosePawnEnemyHalfPenalty`
   - `EvalLoosePawnPhalanxRebate`

2. **Passed-pawn phalanx scaling**
   - `EvalPasserPhalanxSupportBonus`
   - `EvalPasserPhalanxAdvanceBonus`
   - `EvalPasserPhalanxRookBonus`

3. **Candidate lever progression**
   - `EvalCandidateLeverBaseBonus`
   - `EvalCandidateLeverAdvanceBonus`
   - `EvalCandidateLeverSupportBonus`
   - `EvalCandidateLeverRankBonus`

4. **Span aggression knobs**
   - `EvalPawnInfiltrationBonus`
   - `EvalPawnTensionPenalty`
   - `EvalPawnPushThreatBonus`

## SPSA Configuration
- Parameter list + bounds captured in `docs/project_docs/telemetry/eval-framework/spsa_pawn_knobs_20250929.csv`
- Use default SPSA hyperparameters (`alpha=0.602`, `gamma=0.101`, `A-ratio=0.1`, `pairs-per=8`) per `docs/SPSA_UCI_EG_TUNING.md`
- Suggested experiment path:
  1. Run 1 k-iteration dry-run to confirm OpenBench updates all options (watch for bounds hits)
  2. Launch 60+0.6 SPSA with 50 k iterations targeting bundle 2 + 3 (passed/candidate) first
  3. Follow-up sweep for bundle 1 + 4 once passer scaling stabilises
- Monitoring checklist:
  - Track SeaJay vs Komodo deltas after every 5 k iterations using the same telemetry pack
  - Flag any parameter that pegs a bound for manual range widening before continuing
  - Record final tuned set alongside bench nodes in `feature_status.md` for the integration branch

## Open Questions
- Do we need to widen `EvalPawnInfiltrationBonus` bounds beyond ±32 to capture stronger infiltration bonuses? (Currently not activating on the pack.)
- Should king-safety scaling knobs join the second SPSA run if pawn guard adjustments do not correct the attack gaps in #2/#7?
- Are additional FENs required to stress symmetrical endings like #13 from both sides after tuning passes?


## 2025-09-30 Updates
- 1k/10k dry-run on bundles 2+3 (phalanx + candidate lever knobs) using `C_end` bumped to 4/3/2 showed active exploration (final temporary values: support=10, advance=13, rook=6, base=5, advance=7, support=5, rank=3).
- Long SPSA run launched at TC 30+0.3, pairs-per=8, 50k iterations, book `pohl.epd`, parameters limited to bundles 2+3 with `C_end` per updated CSV (`spsa_pawn_knobs_20250929.csv`).
- Mid-run telemetry captured (≈10% progress) at `tmp/eval_reports/eval_20250930T015657Z.{json,summary.json,telemetry.txt}` using the pawn focus pack; deltas remain close to baseline, confirming SPSA has only begun exploring.
- CSV reference updated (+1 to all `C_end`) to preserve larger perturbations for integer knobs ahead of future bundles (guards/aggression).
- Next checkpoints: rerun telemetry around 50% completion, monitor OpenBench logs for bound hits, and prepare bundle 1+4 sweep once passer/lever parameters stabilise.
- SPSA batch #1 final values (support=3, advance=16, rook=4, lever base=5, lever advance=9, lever support=5, lever rank=4) promoted to engine defaults for upcoming guard/aggression sweep.
