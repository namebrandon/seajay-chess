# Move Picking / TT Coverage Prompt (2025-10-05)

Resume guide for `feature/20251002-move-picking` (commit `9c510f8 docs: log TT coverage after SPSA defaults`).

## Current State
- TT instrumentation buckets probes/stores by ply and kind (PV / NonPV / Quiescence). `debug tt` prints compact `Coverage` lines.
- Root logging (`setoption name LogRootTTStores value true`) dumps `info string RootTTProbe/RootTTStore` per iteration.
- Iterative deepening suppresses aspiration for one iteration whenever the root PV changes; this keeps non-PV coverage ≥40% through ply 9 on WAC.049.
- Current defaults after SPSA (2025-10-05): `AspirationWindow=9`, `AspirationMaxAttempts=6`, `StabilityThreshold=5`.

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
- Baseline ordered runs with new defaults still hit ≥40% non-PV coverage through ply 9 (see `docs/project_docs/telemetry/TTCoverage_WAC_PostSPSA.md`).
- OpenBench test #784 (`feature/20251002-move-picking` vs `main`, 10+0.1) showed −44.5 ± 14.9 nELO despite the coverage improvements.
- Disabling aspiration pushes the cliff to ply 10–11; unordered picker keeps coverage high (diagnostics only).

## Next Actions
1. Investigate why improved coverage still yields a −44 nELO SPRT result (OpenBench #784). Focus on root oscillation, aspiration growth mode, and TT saturation (hashfull ≈3/1000).
2. Use WAC.049/WAC.002 depth-10 harness (ordered + SEE-off) with `LogRootTTStores=true` after each tweak to confirm coverage depth.
3. Once diagnostics show neutral or positive behaviour, schedule another short SPRT vs `main` with the standard option pack (`UseRankedMovePicker=true`, `UseSearchNodeAPIRefactor=false`, `EnableExcludedMoveParam=false`, `ShowMovePickerStats=true`).

## Reference Logs
- Ordered baseline: `logs/tt_probe/wac049_base_ttcoverage_afterfix.log`, `logs/tt_probe/wac002_base_ttcoverage_afterfix.log`
- Controls: `*_noasp_ttcoverage.log`, `*_seeoff_ttcoverage.log`, `*_unordered_ttcoverage.log`

## Bench
- Last measured: `bench 2549006`

Always rebuild (`./build.sh Release`) and re-run the WAC depth-10 harness after any change to confirm TT availability, first-move fail-high rate, and logging remain consistent.
