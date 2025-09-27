# Phase SE1 - Singular Extension Enablement

## Overview
- Objective: Enable production-ready singular extensions atop the Search Node API refactor without regressing depth parity or NPS.
- Guardrails: All new behaviour remains behind `UseSingularExtensions` (default false) with supporting toggles `UseSearchNodeAPIRefactor=true` and `EnableExcludedMoveParam=true` during development.
- Success Criteria: +10 nELO cumulative across SE stages, ≤10% cumulative NPS regression, and validated diagnostics for future singular tuning work.

## Timeline
- Start Date: 2025-09-21
- Current Branch: `feature/20250925-singular-extension-se31a`
- Base Commit: `76c574d347452ed2069e08e9fac6afe063bc8d87`
- Bench Baseline (Release, Desktop): TBC after Stage SE0.1 telemetry capture

## Stage Progress
| Phase | Status | Commit | Bench | Notes |
|-------|--------|--------|-------|-------|
| SingularExtension_Phase_SE0.1a – Thread-local telemetry scaffold | Completed | 0e924ae | 2350511 | `SingularStats` embedded per thread with cache alignment and zero-cost reset semantics. |
| SingularExtension_Phase_SE0.1b – Global aggregation | Completed | 62f260f | 2350511 | Atomic roll-up of per-thread stats with InfoBuilder reporting gated by telemetry flush cadence. |
| SingularExtension_Phase_SE0.2a – UCI toggle exposure | Completed | HEAD | 2350511 | `UseSingularExtensions` wired through UCI → SearchLimits → SearchData with telemetry/reporting gated behind the toggle. |
| SingularExtension_Phase_SE0.2b – Defensive assertions | Completed | 95229b3 | 2350511 | RAII guard + DEBUG asserts ensure excluded move lifecycle stays clean on all negamax exit paths. |
| SingularExtension_Phase_SE0.3 – Legacy cleanup | Completed | 0fab434 | 2350511 | Removed SearchInfo excluded-move plumbing; NodeContext now exclusively manages lifecycle. |
| SingularExtension_Phase_SE1.1a – Verification helper skeleton | Completed | 4a62f46 | 2350511 | `verify_exclusion` now guards on `UseSingularExtensions`/`EnableExcludedMoveParam` and logs bypass/invoked counts in DEBUG. |
| SingularExtension_Phase_SE1.1b – Depth reduction clamp | Completed | ec7f07c | 2350511 | Clamp verification depth to `depth - 1 - 3`; DEBUG `ineligible` counter tracks early exits. |
| SingularExtension_Phase_SE1.1c – Window narrowing | Completed | 91ab2ce | 2350511 | Compute singular verification window `[beta-1, beta]` with clamping; child context wiring deferred to SE1.1d. |
| SingularExtension_Phase_SE1.1d – Negamax recursion hookup | Completed | c912525 | 2350511 | Wire verification search into `negamax` using excluded child context and narrow window; still returns score to caller. |
| SingularExtension_Phase_SE1.2a – TT store policy | Completed | 83e92c5 | 2350511 | Verification mode uses `StorePolicyGuard`; TT entries marked `TT_EXCLUSION` reside only in empty/flagged slots and are first to be evicted by primary stores. |
| SingularExtension_Phase_SE1.2b – TT contamination guards | Completed | 1b185fa | 2350511 | DEBUG sentinel/asserts guard verification stores; TT stats track verification store/skip counters for telemetry. |
| SingularExtension_Phase_SE2.1a – TT probe gate | Completed | a586d04 | 2350511 | TT probe now validates depth/EXACT/move before counting singular candidates (telemetry via `singularStats`). |
| SingularExtension_Phase_SE2.1b – Score margin calculation | Completed | a157286 | 2350511 | Added constexpr `singular_margin` table and TT-driven windowing; `verify_exclusion` now clamps `singularBeta` via margin-based subtract. |
| SingularExtension_Phase_SE2.1c – Move qualification | Completed | 8cb8359 | 2350511 | TT move legality/quiet filters populate `singularStats` qualified/reject counters; NodeContext primed for exclusion until verification wiring lands. |
| SingularExtension_Phase_SE2.2a – Verification trigger | Completed | e29f0d9 | 2350511 | Launch verification search with margin-clamped window; record `verificationsStarted` ahead of SE2.2b comparisons. |
| SingularExtension_Phase_SE2.2b – Verification outcome tracking | Completed | 742af1c | 2350511 | Telemetry differentiates fail-low/high outcomes; root guard added to prevent context misuse ahead of SE3. |
| SingularExtension_Phase_SE3.1a – Extension tracking infrastructure | Completed | 4ac6ee0 | 2350511 | Implemented extension budget clamp and telemetry; verified neutral bench with toggles enabled. |
| SingularExtension_Phase_SE3.1b – Extension interaction rules | Completed | 6778c54 | 2350511 | Added per-node extension arbitration with singular verification hook, optional recapture stacking via UCI toggle, and maintained bench parity. |
| SingularExtension_Phase_SE3.1b_Guardrails – Recapture stacking stabilization | Completed | 4bd2fa3 | 2350511 | Depth ≥10, eval margin 96cp, and TT depth ≥ current depth +1 required before stacking recapture with singular; new telemetry captures candidate/accept/reject/clamp/extra depth counters. |
| SingularExtension_Phase_SE3.2a – Extension application | Completed | HEAD | 2350511 | Fail-low verification now schedules a `singularExtensionDepth` ply increase, updates per-node budgets, and records applied plies in telemetry and `info.singularExtensions`. |
| SingularExtension_Phase_SE3.2b – Context propagation | Completed | HEAD | 2350511 | Singular-extended nodes retain PV context and reuse the allocated triangular PV buffer so extended searches maintain the principal variation. |

## Telemetry Checklist
| Machine | Branch/Commit | Bench Nodes | Threads | Raw NPS | Normalized NPS (`NPS / bench`) | Depth @10s | TT Hit % | Notes |
|---------|---------------|-------------|---------|---------|-------------------------------|------------|----------|-------|
| Desktop | (pending) | (pending) | 1 / 2 / 4 / 8 | (pending) | (pending) | (pending) | (pending) | Idle load assumption |
| Laptop | (pending) | (pending) | 1 / 2 / 4 | (pending) | (pending) | (pending) | (pending) | Document battery/thermal state |

### Recent Findings (2025-09-27)
- `tools/stacked_telemetry.py --epd tests/positions/wacnew.epd --limit 200 --movetime 500` (stacked off/on) with margins ≥8→200/≥6→260 and verification reduction 4 produced `verified≈6.4k`, `fail_low≤3`, `extended≤3`, slack averages ~+30 cp. Extensions remain effectively inactive despite aggressive margins and shallower verification searches.
- Switching to `tests/positions/bratko_kopec.epd` under the same configuration shows identical behaviour: `verified≈700`, `fail_low=0`, slack ~+31 cp. The issue is not specific to WAC positions; our verification window still very rarely fails low.
- Stacked counters (candidates/applied/rejections) stay at zero because no singular verification produces a fail-low even when stacking is enabled. `info.singularExtensions` and `singularStats.extensionsApplied` remain 0 across all runs, explaining the -18 nELO OpenBench regression when the feature was enabled.
- Further stress testing (verification reduction = 8, `SINGULAR_DEPTH_MIN=2`, margin table up to 840 cp for shallow depths) still yields `fail_low=0`, `extended=0` over 200-position sweeps (`wacnew.epd` and `bratko_kopec.epd`). Average slack balloons to ~35 cp, confirming the current verification strategy always returns above β even under extreme settings.
- Current interpretation: the TT score gap between the singular candidate and the verification search remains too large (~30 cp on average). Additional gating or a different verification strategy will be required before proceeding to SPRT.

## Risk Notes
- Telemetry counters must stay cache-aligned; verify with `static_assert(alignof(SingularStats) == 64)` before enabling instrumentation.
- Ensure Release builds strip telemetry when toggle disabled to avoid exceeding ≤2% NPS budget for SE0 stages.
- Cross-machine baseline comparisons rely on normalized NPS; capture bench outputs alongside raw NPS for each data point.

## Next Actions
1. Revisit verification window strategy: experiment with even tighter margins, alternate reduction depth, or TT-bound adjustments so fail-low events occur with measurable frequency before scheduling SPRTs.
2. Implement SE3.1c check-extension coordination toggle and instrumentation once singular extensions can trigger reliably.
3. Stage SE4.1a: surface singular tuning parameters (`SingularDepthMin`, `SingularMarginBase`, etc.) via UCI to support future tuning and telemetry sweeps.
