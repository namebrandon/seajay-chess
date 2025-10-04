# Move Picking / TT Coverage Prompt (2025-10-05)

Resume guide for `feature/20251002-move-picking` (commit `83857cf docs: capture TT coverage state for reboot`).

## Current State
- TT instrumentation buckets probes/stores by ply and kind (PV / NonPV / Quiescence). `debug tt` prints compact `Coverage` lines.
- Root logging (`setoption name LogRootTTStores value true`) dumps `info string RootTTProbe/RootTTStore` per iteration.
- Iterative deepening suppresses aspiration for one iteration whenever the root PV changes; this keeps non-PV coverage ≥40% through ply 9 on WAC.049.

## Key Files
- TT stats & API: `src/core/transposition_table.h/.cpp`
- Search integration: `src/search/negamax.cpp`, `src/search/quiescence*.cpp`, `src/search/negamax_legacy.inc`
- UCI toggles & telemetry: `src/uci/uci.cpp/.h`
- Docs: `docs/project_docs/Move_Picking_Optimization_Plan.md`, `feature_status.md`

## Telemetry Commands
```
printf 'uci\nsetoption name LogRootTTStores value true\nposition fen <FEN>\ngo depth 10\ndebug tt\nquit\n' | ./bin/seajay > logs/tt_probe/<tag>.log
```
- SEE-off control: add `setoption name SEEPruning value off` / `QSEEPruning value off` before `go`.
- Root logging shows PV movement; `Coverage` lines reveal where non-PV reuse collapses.

## Recent Findings
- Baseline ordered runs: first <40% non-PV bucket at ply 9 (WAC.049) and 9 (WAC.002).
- Disabling aspiration pushes the cliff to ply 10–11; unordered picker keeps coverage high (used for diagnostics only).

## Next Actions
1. Run directional SPSA on numeric aspiration options:
   - `AspirationWindow,int,13,8,28,2.0,0.002`
   - `AspirationMaxAttempts,int,5,3,8,1.0,0.002`
   - `StabilityThreshold,int,6,3,12,1.5,0.002`
   Keep `LogRootTTStores=true` so every candidate shows a coverage curve before measuring Elo.
2. After tuning, capture WAC.049/WAC.002 depth-10 logs (ordered vs SEE-off) to confirm coverage depth.
3. If coverage still drops at ply 9, evaluate root move-order heuristics before altering non-root picker logic.

## Reference Logs
- Ordered baseline: `logs/tt_probe/wac049_base_ttcoverage_afterfix.log`, `logs/tt_probe/wac002_base_ttcoverage_afterfix.log`
- Controls: `*_noasp_ttcoverage.log`, `*_seeoff_ttcoverage.log`, `*_unordered_ttcoverage.log`

## Bench
- Last measured: `bench 2549006`

Always rebuild (`./build.sh Release`) and re-run the WAC depth-10 harness after any change to confirm TT availability, first-move fail-high rate, and logging remain consistent.
