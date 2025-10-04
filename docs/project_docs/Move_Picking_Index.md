# Move Picking Documentation Index (2025-10-06)

Use this as the starting point when resuming TT coverage / move-ordering work.

## Core References
- `docs/project_docs/Move_Picking_Optimization_Plan.md` – Master plan and reboot checklist
- `docs/project_docs/Move_Picking_Prompt.md` – Quick-start prompt summarising current state, commands, and next actions
- `feature_status.md` – Branch status, recent findings, and outstanding tasks

## Telemetry & Diagnostics
- `docs/project_docs/telemetry/TTCoverage_WAC_PostSPSA.md` – Latest WAC depth-10 coverage logs (ordered picker)
- `logs/tt_probe/` – Raw coverage traces (`wac049_*`, `wac002_*`) for ordered/SEE-off/aspiration-off/unordered runs
- `docs/SPSA_INTEGER_BUG_AUDIT.md` – History of SPSA rounding fixes for integer options

## Configuration & Implementation
- `src/core/transposition_table.h/.cpp` – Coverage counters, store/probe instrumentation
- `src/search/negamax.cpp` – Aspiration guard, TT usage, root logging
- `src/uci/uci.cpp/.h` – UCI options (`AspirationWindow=9`, `AspirationMaxAttempts=6`, `StabilityThreshold=5`) and diagnostics toggles
- `src/search/types.h` – Search defaults (mirrors UCI defaults)

## Recent Experiments
- Commit `9c510f8` (2025-10-06) records TT coverage after applying SPSA defaults
- OpenBench test #784 (`feature/20251002-move-picking` vs `main`) – regression: −44.5 ± 14.9 nELO with aspiration guard + new defaults

## Next Steps Snapshot
1. Investigate TT coverage collapse despite low hash usage (hashfull ≈3/1000). Determine whether root oscillation, aspiration strategy, or TT sizing is responsible.
2. Revisit root move-order heuristics / aspiration growth before touching deeper picker code.
3. Plan follow-up SPRT only after addressing coverage + stability. Use `LogRootTTStores=true` during diagnostics.

Keep this index updated whenever new documents or key results are added.
